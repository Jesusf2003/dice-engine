//==============================
//	mapParser.cpp
//==============================

#include "map/mapParser.h"
#include "map/map.h"
#include "map/mapReport.h"
#include "core/common/log.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace
{

enum TokenType
{
	TOK_END,
	TOK_WORD,			// numbers and unquoted texture names
	TOK_STRING,			// "quoted"
	TOK_LPAREN, TOK_RPAREN,
	TOK_LBRACKET, TOK_RBRACKET,
	TOK_LBRACE, TOK_RBRACE,
};

struct Token
{
	TokenType	type = TOK_END;
	std::string	text;
	int			line = 0, column = 0;
};

std::string Describe(const Token &t)
{
	switch (t.type)
	{
	case TOK_END:		return "the end of the file";
	case TOK_STRING:	return "\"" + t.text + "\"";
	default:			return "'" + t.text + "'";
	}
}

/*
==================
Tokenizer

whitespace separated, // comments to the end of the line. ( ) [ ] are always
tokens of their own; { } only when standing alone, since texture names can
start with one ({fence, half-life's transparent textures)
==================
*/
class Tokenizer
{
public:
	explicit Tokenizer(const std::string &t) : text(t) {}

	const Token &Peek()
	{
		if (!peeked)
		{
			next = Read();
			peeked = true;
		}
		return next;
	}

	Token Next()
	{
		Peek();
		peeked = false;
		return next;
	}

private:
	Token Read();

	const std::string	&text;
	size_t	pos = 0, lineStart = 0;
	int		line = 1;
	bool	peeked = false;
	Token	next;
};

Token Tokenizer::Read()
{
	for (;;)
	{
		if (pos >= text.size())
		{
			Token t;
			t.line = line;
			t.column = (int)(pos - lineStart) + 1;
			return t;
		}

		char c = text[pos];
		if (c == '\n')
		{
			pos++;
			line++;
			lineStart = pos;
		}
		else if (isspace((unsigned char)c))
			pos++;
		else if (c == '/' && pos + 1 < text.size() && text[pos + 1] == '/')
		{
			while (pos < text.size() && text[pos] != '\n')
				pos++;
		}
		else
			break;
	}

	Token t;
	t.line = line;
	t.column = (int)(pos - lineStart) + 1;
	char c = text[pos];

	auto single = [&](TokenType type)
	{
		t.type = type;
		t.text.assign(1, c);
		pos++;
		return t;
	};

	switch (c)
	{
	case '(':	return single(TOK_LPAREN);
	case ')':	return single(TOK_RPAREN);
	case '[':	return single(TOK_LBRACKET);
	case ']':	return single(TOK_RBRACKET);
	case '{':
	case '}':
		if (pos + 1 >= text.size() || isspace((unsigned char)text[pos + 1]))
			return single(c == '{' ? TOK_LBRACE : TOK_RBRACE);
		break;		// part of a word
	case '"':
	{
		size_t end = pos + 1;
		while (end < text.size() && text[end] != '"' && text[end] != '\n')
			end++;
		if (end >= text.size() || text[end] != '"')
			throw MapParseError{ t.line, t.column, "string not closed before the end of the line" };
		t.type = TOK_STRING;
		t.text = text.substr(pos + 1, end - pos - 1);
		pos = end + 1;
		return t;
	}
	}

	size_t end = pos;
	while (end < text.size() && !isspace((unsigned char)text[end]) && !strchr("()[]\"", text[end]))
		end++;
	t.type = TOK_WORD;
	t.text = text.substr(pos, end - pos);
	pos = end;
	return t;
}

/*
==================
Parser
==================
*/
class Parser
{
public:
	Parser(const std::string &text, Map &m, MapReport &r) : tok(text), map(m), report(r) {}

	void	ParseMap();

private:
	[[noreturn]] void Fail(const Token &t, const std::string &msg)
	{
		throw MapParseError{ t.line, t.column, msg };
	}

	Token	Expect(TokenType type, const char *what);
	double	Number(const char *what);
	dvec3	Point();

	void	ParseEntity(const Token &open);
	void	ParseBrush(Entity &e, const Token &open);
	void	ParseFace(Brush &b);
	void	SkipBlock(const Token &open);

	Tokenizer	tok;
	Map			&map;
	MapReport	&report;
	int			standardFaces = 0, valveFaces = 0;
	bool		reportedSurfaceFlags = false;
};

Token Parser::Expect(TokenType type, const char *what)
{
	Token t = tok.Next();
	if (t.type != type)
		Fail(t, std::string("expected ") + what + ", found " + Describe(t));
	return t;
}

double Parser::Number(const char *what)
{
	Token t = tok.Next();
	if (t.type != TOK_WORD)
		Fail(t, std::string("expected ") + what + ", found " + Describe(t));

	char *end;
	double v = strtod(t.text.c_str(), &end);
	if (end == t.text.c_str() || *end || !std::isfinite(v))
		Fail(t, std::string("expected ") + what + ", found " + Describe(t));
	return v;
}

dvec3 Parser::Point()
{
	dvec3 p;
	Expect(TOK_LPAREN, "'(' starting a plane point");
	for (int k = 0; k < 3; k++)
		p[k] = Number("a plane point coordinate");
	Expect(TOK_RPAREN, "')' closing a plane point");
	return p;
}

/*
==================
Parser::ParseMap

{ entity } { entity } ...
==================
*/
void Parser::ParseMap()
{
	for (;;)
	{
		Token t = tok.Next();
		if (t.type == TOK_END)
			break;
		if (t.type != TOK_LBRACE)
			Fail(t, "expected '{' starting an entity, found " + Describe(t));
		ParseEntity(t);
	}

	if (valveFaces && standardFaces)
		map.format = MAPFORMAT_MIXED;
	else if (valveFaces)
		map.format = MAPFORMAT_VALVE220;
	else if (standardFaces)
		map.format = MAPFORMAT_STANDARD;
	else
		map.format = MAPFORMAT_UNKNOWN;
}

/*
==================
Parser::ParseEntity

"key" "value" pairs and { brushes } in any order, up to the closing }
==================
*/
void Parser::ParseEntity(const Token &open)
{
	Entity e;
	e.line = open.line;
	int index = (int)map.entities.size();

	for (;;)
	{
		Token t = tok.Next();
		switch (t.type)
		{
		case TOK_STRING:
		{
			Token v = tok.Next();
			if (v.type != TOK_STRING)
				Fail(v, "expected the value of \"" + t.text + "\" as a quoted string, found " + Describe(v));
			if (t.text.empty())
				report.Warning("empty key", t.line, "entity %d has a key with no name (value \"%s\")", index, v.text.c_str());
			if (e.HasKey(t.text))
				report.Warning("duplicate key", t.line, "entity %d: \"%s\" is set again, the last value is kept", index, t.text.c_str());
			e.SetKey(t.text, v.text);
			break;
		}
		case TOK_LBRACE:
			ParseBrush(e, t);
			break;
		case TOK_RBRACE:
			map.entities.push_back(std::move(e));
			return;
		default:
			Fail(t, "expected a \"key\", a brush or '}' closing the entity from line " + std::to_string(open.line) + ", found " + Describe(t));
		}
	}
}

/*
==================
Parser::ParseBrush

the opening { is already read. quake 3's "patchDef2 { ... }" and
"brushDef { ... }" start with a word instead of a face: skipped whole
==================
*/
void Parser::ParseBrush(Entity &e, const Token &open)
{
	if (tok.Peek().type == TOK_WORD)
	{
		std::string kind = tok.Peek().text;
		SkipBlock(open);
		report.Warning("unsupported primitive", open.line, "%s isn't a Quake 1 brush (Quake 3 / Doom 3 format), skipped", kind.c_str());
		return;
	}

	Brush b;
	b.line = open.line;
	while (tok.Peek().type == TOK_LPAREN)
		ParseFace(b);
	Expect(TOK_RBRACE, "'(' starting a face or '}' closing the brush");
	e.brushes.push_back(std::move(b));
}

void Parser::SkipBlock(const Token &open)
{
	int depth = 1;
	while (depth > 0)
	{
		Token t = tok.Next();
		if (t.type == TOK_LBRACE)
			depth++;
		else if (t.type == TOK_RBRACE)
			depth--;
		else if (t.type == TOK_END)
			Fail(open, "this block is never closed");
	}
}

/*
==================
Parser::ParseFace

( x y z ) ( x y z ) ( x y z ) TEXTURE then either
	xoff yoff rot xscale yscale
	[ ux uy uz uoff ] [ vx vy vz voff ] rot xscale yscale
==================
*/
void Parser::ParseFace(Brush &b)
{
	BrushFace f;
	f.line = tok.Peek().line;

	for (int i = 0; i < 3; i++)
		f.points[i] = Point();

	Token tex = tok.Next();
	if (tex.type != TOK_WORD && tex.type != TOK_STRING)
		Fail(tex, "expected a texture name, found " + Describe(tex));
	f.tex.name = tex.text;

	if (tok.Peek().type == TOK_LBRACKET)
	{
		f.tex.valve = true;
		for (int a = 0; a < 2; a++)
		{
			Expect(TOK_LBRACKET, "'[' starting a texture axis");
			for (int k = 0; k < 3; k++)
				f.tex.axis[a][k] = Number("a texture axis component");
			f.tex.shift[a] = Number("a texture offset");
			Expect(TOK_RBRACKET, "']' closing a texture axis");
		}
		valveFaces++;
	}
	else
	{
		f.tex.shift[0] = Number("the texture x offset");
		f.tex.shift[1] = Number("the texture y offset");
		standardFaces++;
	}
	f.tex.rotate = Number("the texture rotation");
	f.tex.scale[0] = Number("the texture x scale");
	f.tex.scale[1] = Number("the texture y scale");

	// quake 2: contents, flags and value at the end of the line
	int extras = 0;
	while (extras < 3 && tok.Peek().type == TOK_WORD && tok.Peek().line == f.line)
	{
		Number("a surface flag");
		extras++;
	}
	if (extras && !reportedSurfaceFlags)
	{
		report.Warning("surface flags", f.line, "Quake 2 surface flags after the texture scale, ignored (Quake 1 has none)");
		reportedSurfaceFlags = true;
	}

	b.faces.push_back(std::move(f));
}

}	// namespace

void MapParser::Parse(const std::string &text, Map &map, MapReport &report)
{
	Parser parser(text, map, report);
	parser.ParseMap();
}
