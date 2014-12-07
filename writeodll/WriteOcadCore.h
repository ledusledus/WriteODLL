#pragma once
#include <vector>
using namespace std; // polluting the namespace..
struct point
{
	point(int _x, int _y) :x(_x), y(_y)
	{}
	int x, y;
};

class IOcadWriter
{
public:
    // adds a color to the file with given name, returns current color value
	virtual int addcolor(const char *name) = 0;
	// adds area symbol with name and number encoded as 4100 == 410.0 in ocad
	virtual int addareasymbol(const char *name, int number, int color) = 0;
	virtual int exportArea(const vector<point>&area, int symbol) = 0;
	virtual int writeFile(const char * name) = 0;
};
IOcadWriter* OcadWriterFactory(double _offsetx, double _offsety, double _scale);
#define ChkErr( expr ) { if (expr != 0) { return -1; } }
