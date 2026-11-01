//==============================
//	fgdParser.cpp
//==============================

#include "map/fgdParser.h"
#include "map/entityDefs.h"
#include "map/mapReport.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace
{

enum TokenType
{
	TOK_END,
	TOK_WORD,		// names, types, numbers
	TOK_STRING,		// "quoted"
	TOK_AT, TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
	TOK_EQUALS, TOK_COLON, TOK_COMMA, TOK_PLUS,
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

std::string Lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s;
}

/*
==================
Tokenizer

// comments, "strings" (on one line), the punctuation as tokens of its own
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

	static const char punct[] = "@()[]=:,+";
	static const TokenType punctTypes[] = { TOK_AT, TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
		TOK_EQUALS, TOK_COLON, TOK_COMMA, TOK_PLUS };

	if (const char *p = strchr(punct, c))
	{
		t.type = punctTypes[p - punct];
		t.text.assign(1, c);
		pos++;
		return t;
	}

	if (c == '"')
	{
		size_t end = pos + 1;
		while (end < text.size() && text[end] != '"' && text[end] != '\n')
			end++;
		if (end >= text.size() || text[end] != '"')
			throw FgdParseError{ t.line, t.column, "string not closed before the end of the line" };
		t.type = TOK_STRING;
		t.text = text.substr(pos + 1, end - pos - 1);
		pos = end + 1;
		return t;
	}

	size_t end = pos;
	while (end < text.size() && !isspace((unsigned char)text[end]) && !strchr(punct, text[end]) && text[end] != '"')
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
	Parser(const std::string &text, EntityDefs &d, MapReport &r) : tok(text), defs(d), report(r) {}

	void	ParseFile();

private:
	[[noreturn]] void Fail(const Token &t, const std::string &msg)
	{
		throw FgdParseError{ t.line, t.column, msg };
	}

	Token		Expect(TokenType type, const char *what);
	std::string	Description();				// "a" + "b"
	double		Number(const Token &t, const char *what);
	void		SkipBalanced(TokenType open, TokenType close);

	void	ParseClass(const Token &at, EntityClass::Type type);
	void	ParseHelper(EntityClass &cls, const Token &name);
	void	ParseProperty(EntityClass &cls, const Token &name);
	void	ParseChoices(EntityProperty &prop);

	Tokenizer	tok;
	EntityDefs	&defs;
	MapReport	&report;
};

Token Parser::Expect(TokenType type, const char *what)
{
	Token t = tok.Next();
	if (t.type != type)
		Fail(t, std::string("expected ") + what + ", found " + Describe(t));
	return t;
}

std::string Parser::Description()
{
	std::string s = Expect(TOK_STRING, "a quoted description").text;
	while (tok.Peek().type == TOK_PLUS)
	{
		tok.Next();
		s += Expect(TOK_STRING, "a quoted string after '+'").text;
	}
	return s;
}

double Parser::Number(const Token &t, const char *what)
{
	char *end;
	double v = strtod(t.text.c_str(), &end);
	if (t.type != TOK_WORD || end == t.text.c_str() || *end)
		Fail(t, std::string("expected ") + what + ", found " + Describe(t));
	return v;
}

void Parser::SkipBalanced(TokenType open, TokenType close)
{
	Token start = Expect(open, "an opening bracket");
	int depth = 1;
	while (depth > 0)
	{
		Token t = tok.Next();
		if (t.type == open)
			depth++;
		else if (t.type == close)
			depth--;
		else if (t.type == TOK_END)
			Fail(start, "this bracket is never closed");
	}
}

/*
==================
Parser::ParseFile
==================
*/
void Parser::ParseFile()
{
	for (;;)
	{
		Token at = tok.Next();
		if (at.type == TOK_END)
			return;
		if (at.type != TOK_AT)
			Fail(at, "expected '@' starting a class, found " + Describe(at));

		Token kind = Expect(TOK_WORD, "a class type after '@' (BaseClass, PointClass, SolidClass)");
		std::string k = Lower(kind.text);

		if (k == "baseclass")
			ParseClass(at, EntityClass::BASE);
		else if (k == "pointclass")
			ParseClass(at, EntityClass::POINT);
		else if (k == "solidclass")
			ParseClass(at, EntityClass::SOLID);
		else
		{
			// @include "x.fgd", @mapsize(a, b), @MaterialExclusion [ ... ]...
			if (tok.Peek().type == TOK_STRING)
				tok.Next();
			if (tok.Peek().type == TOK_LPAREN)
				SkipBalanced(TOK_LPAREN, TOK_RPAREN);
			if (tok.Peek().type == TOK_LBRACKET)
				SkipBalanced(TOK_LBRACKET, TOK_RBRACKET);
			report.Warning("unsupported directive", at.line, "@%s isn't supported, skipped", kind.text.c_str());
		}
	}
}

/*
==================
Parser::ParseClass

helpers up to '=', then name [: "description"] [ properties ]
==================
*/
void Parser::ParseClass(const Token &at, EntityClass::Type type)
{
	EntityClass cls;
	cls.type = type;
	cls.line = at.line;

	while (tok.Peek().type == TOK_WORD)
		ParseHelper(cls, tok.Next());
	Expect(TOK_EQUALS, "a helper like base(...) or '=' before the class name");

	cls.name = Expect(TOK_WORD, "the class name").text;
	if (tok.Peek().type == TOK_COLON)
	{
		tok.Next();
		cls.description = Description();
	}

	Expect(TOK_LBRACKET, "'[' starting the class properties");
	for (;;)
	{
		Token t = tok.Next();
		if (t.type == TOK_RBRACKET)
			break;
		if (t.type != TOK_WORD)
			Fail(t, "expected a property name or ']' closing " + cls.name + ", found " + Describe(t));
		ParseProperty(cls, t);
	}

	defs.classes.push_back(std::move(cls));
}

/*
==================
Parser::ParseHelper

base(A, B), color(r g b), size(x y z, x y z) or size(x y z); the rest skipped
==================
*/
void Parser::ParseHelper(EntityClass &cls, const Token &name)
{
	std::string h = Lower(name.text);
	std::vector<Token> args;

	Expect(TOK_LPAREN, ("'(' after " + name.text).c_str());
	for (;;)
	{
		Token t = tok.Next();
		if (t.type == TOK_RPAREN)
			break;
		if (t.type == TOK_END)
			Fail(name, name.text + "( is never closed");
		args.push_back(t);
	}

	if (h == "base")
	{
		for (const Token &a : args)
		{
			if (a.type == TOK_WORD)
				cls.bases.push_back(a.text);
			else if (a.type != TOK_COMMA)
				Fail(a, "expected a base class name, found " + Describe(a));
		}
	}
	else if (h == "color")
	{
		if (args.size() != 3)
			Fail(name, "color() takes three numbers, 0-255");
		for (int i = 0; i < 3; i++)
			cls.color[i] = (float)Number(args[i], "a color component");
		cls.hasColor = true;
	}
	else if (h == "size")
	{
		std::vector<double> n;
		for (const Token &a : args)
			if (a.type != TOK_COMMA)
				n.push_back(Number(a, "a size coordinate"));

		if (n.size() == 6)
		{
			cls.mins = dvec3(n[0], n[1], n[2]);
			cls.maxs = dvec3(n[3], n[4], n[5]);
		}
		else if (n.size() == 3)		// just the dimensions, centered on the origin
		{
			cls.mins = dvec3(-n[0] / 2, -n[1] / 2, -n[2] / 2);
			cls.maxs = dvec3(n[0] / 2, n[1] / 2, n[2] / 2);
		}
		else
			Fail(name, "size() takes x y z or x y z, x y z");
		cls.hasSize = true;
	}
	// studio, iconsprite, sprite, model, decal, halfgridsnap...: not used yet
}

/*
==================
Parser::ParseProperty

key(type) [: "description" [: default [: "long description"]]] [= [ choices ]]
any of the fields may be left empty: key(string) : : "default"
==================
*/
void Parser::ParseProperty(EntityClass &cls, const Token &name)
{
	EntityProperty prop;
	prop.name = name.text;
	prop.line = name.line;

	Expect(TOK_LPAREN, ("'(' with the type of " + name.text).c_str());
	prop.type = Lower(Expect(TOK_WORD, "the property type").text);
	Expect(TOK_RPAREN, "')' after the property type");

	// hammer's readonly / report (any other word is already the next property)
	while (tok.Peek().type == TOK_WORD && (Lower(tok.Peek().text) == "readonly" || Lower(tok.Peek().text) == "report"))
		tok.Next();

	for (int field = 0; field < 3 && tok.Peek().type == TOK_COLON; field++)
	{
		tok.Next();
		const Token &p = tok.Peek();
		if (p.type == TOK_COLON || p.type == TOK_EQUALS || p.type == TOK_RBRACKET)
			continue;		// empty field

		if (field == 0)
			prop.description = Description();
		else if (field == 1)
		{
			Token v = tok.Next();
			if (v.type != TOK_WORD && v.type != TOK_STRING)
				Fail(v, "expected the default value of " + prop.name + ", found " + Describe(v));
			prop.defaultValue = v.text;
			prop.hasDefault = true;
		}
		else
			Description();	// the long description, not kept
	}

	if (tok.Peek().type == TOK_EQUALS)
	{
		tok.Next();
		ParseChoices(prop);
	}

	for (const EntityProperty &p : cls.properties)
		if (p.name == prop.name)
			report.Warning("duplicate property", prop.line, "%s: %s is defined twice, the last one is used", cls.name.c_str(), prop.name.c_str());

	cls.properties.push_back(std::move(prop));
}

/*
==================
Parser::ParseChoices

[ value : "description" [: flag default] ... ], values a number or a quoted string
==================
*/
void Parser::ParseChoices(EntityProperty &prop)
{
	Expect(TOK_LBRACKET, "'[' starting the choices");
	for (;;)
	{
		Token v = tok.Next();
		if (v.type == TOK_RBRACKET)
			break;
		if (v.type != TOK_WORD && v.type != TOK_STRING)
			Fail(v, "expected a value or ']' closing the choices of " + prop.name + ", found " + Describe(v));

		PropertyChoice c;
		c.value = v.text;
		Expect(TOK_COLON, "':' after the value");
		c.description = Description();
		if (tok.Peek().type == TOK_COLON)
		{
			tok.Next();
			c.flagOn = Number(tok.Next(), "the flag's default (0 or 1)") != 0;
		}
		prop.choices.push_back(std::move(c));
	}
}

}	// namespace

void FgdParser::Parse(const std::string &text, EntityDefs &defs, MapReport &report)
{
	Parser parser(text, defs, report);
	parser.ParseFile();
}
