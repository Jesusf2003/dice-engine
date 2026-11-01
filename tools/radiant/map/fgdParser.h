//==============================
//	fgdParser.h
//
//	reads an FGD (worldcraft / hammer / J.A.C.K. entity definitions):
//
//	@PointClass base(A, B) color(r g b) size(x y z, x y z) = name : "description"
//	[
//		key(type) : "description" : default : "long description"
//		key(choices) : "description" : 0 = [ 0 : "No"  1 : "Yes" ]
//		spawnflags(flags) = [ 1 : "Start open" : 0 ]
//	]
//
//	@BaseClass, @SolidClass the same. helpers it doesn't use (studio, iconsprite...)
//	and other @ directives (@mapsize, @include...) are skipped with a warning.
//	from trenchbroom's FgdParser, without the source engine io
//==============================

#ifndef __FGD_PARSER_H__
#define __FGD_PARSER_H__

#include <string>

class EntityDefs;
class MapReport;

struct FgdParseError
{
	int			line, column;
	std::string	message;
};

namespace FgdParser
{
	// appends to defs.classes, throws FgdParseError
	void	Parse(const std::string &text, EntityDefs &defs, MapReport &report);
}

#endif
