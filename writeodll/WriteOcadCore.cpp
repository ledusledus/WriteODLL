// WriteOcad.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include "..\libocad\libocad.h"
#include "WriteOcadCore.h"
using namespace std;
#define min(a,b) ((a)>(b)?(b):(a))

bool convertPascalString(const char *str, char *out, unsigned out_len)
{
	int len = strlen(str);
	if (out_len == 0) return false;
	len = min(out_len - 1, len);
	if (len > 255) return false;
	out[0] = (unsigned char)len;
	memcpy(out + 1, str, len);
	return true;
}

struct point
{
	point(int _x, int _y) :x(_x), y(_y)
	{}
	int x, y;
};

u16 exportCoordinates( const vector<point> &points, OCADPoint** buffer )
{
	s16 num_points = 0;
	bool curve_start = false;
	bool hole_point = false;
	bool curve_continue = false;
	for (size_t i = 0, end = points.size(); i < end; ++i)
	{
		OCADPoint p;
		p.x = (points[i].x*10)<<8;
		p.y = (points[i].y*10)<<8; 

		**buffer = p;
		++(*buffer);
		++num_points;
	}
	return num_points;
}
void exportCommonSymbolFields(int number, const char * name, OCADSymbol* ocad_symbol, int size)
{
	if (!number) number += 1;
	ocad_symbol->size = (s16)size;
	convertPascalString(name, ocad_symbol->name, 32);
	ocad_symbol->number = number;

	u8 bitmask = 1;
	u8* bitpos = ocad_symbol->colors;

	// Icon: 22x22 with 4 bit color code, origin at bottom left, some padding
	const int icon_size = 22;
	u8* ocad_icon = (u8*)ocad_symbol->icon;
	for (int y = icon_size - 1; y >= 0; --y)
	{
		for (int x = 0; x < icon_size; x += 2)
		{
			int first = 0xa;
			int second = 0xa;
			*(ocad_icon++) = (first << 4) + (second);
		}
		ocad_icon++;
	}
}

s16 exportAreaSymbol(const char *name, int number, OCADFile * file, int color)
{
	int data_size = (sizeof(OCADAreaSymbol) - sizeof(OCADPoint));
	OCADAreaSymbol* ocad_symbol = (OCADAreaSymbol*)ocad_symbol_new(file, data_size);
	exportCommonSymbolFields(number,name, (OCADSymbol*)ocad_symbol, data_size);

	// Basic settings
	ocad_symbol->type = OCAD_AREA_SYMBOL;
	ocad_symbol->extent = 0;
	ocad_symbol->color = color;
	ocad_symbol->fill = 1;
	// Hatch
	ocad_symbol->hmode = 0;
	return ocad_symbol->number;
}

class Singleton
{
public:
	Singleton()
	{
		ocad_init();
	}
	~Singleton()
	{
		ocad_shutdown();
	}
};
// ocad structure init and then shutdown on destory.
Singleton singleton;

class OcadWriter:public IOcadWriter
{
private:
	// private constructor to allow only factory creation
	OcadWriter(double _offsetx, double _offsety, double _scale) : 
		offsetx(_offsetx), 
		offsety(_offsety), 
		scale(_scale),
		colorcount(0)
	{
		file = nullptr;
	}
	int Init();
	OCADFile *file;
	double offsetx, offsety, scale;
	int colorcount;
public:
	// adds a color to the file with given name, returns current color value
	virtual int addcolor(const char *name);
	// adds area symbol with name and number encoded as 4100 == 410.0 in ocad
	virtual int addareasymbol(const char *name, int number, int color);
	virtual int exportArea(const vector<point>&area, int symbol);
	virtual int writeFile(const char * name);
	static OcadWriter* Factory(double _offsetx, double _offsety, double _scale);
	virtual ~OcadWriter();
};
OcadWriter::~OcadWriter()
{
	ocad_file_close(file);
}

OcadWriter* OcadWriter::Factory(double _offsetx, double _offsety, double _scale)
{
	OcadWriter *writer = new OcadWriter(_offsetx, _offsety, _scale);
	if (writer->Init())
	{
		delete writer;
		return nullptr;
	}
	return writer;
}

IOcadWriter* OcadWriterFactory(double _offsetx, double _offsety, double _scale)
{
    return OcadWriter::Factory(_offsetx, _offsety, _scale );
}

#define ChkErr( expr ) { if (expr != 0) { return -1; } }
int OcadWriter::Init()
{
	ChkErr( ocad_file_new(&file) );
	OCADSetup* setup = file->setup;
	setup->zoom = 10;
	setup->scale = scale;
	setup->offsetx = offsetx;
	setup->offsety = offsety;
	setup->angle = 0;
	setup->realcoord = 1; 

	// Fill header struct
	OCADFileHeader* header = file->header;
	*(((u8*)&header->magic) + 0) = 0xAD;
	*(((u8*)&header->magic) + 1) = 0x0C;
	header->ftype = 2;
	header->major = 8;
	header->minor = 0;
}
int OcadWriter::addcolor(const char *name)
{
	++file->header->ncolors;
	OCADColor *ocad_color = ocad_color_at(file, colorcount);
	ocad_color->number = colorcount;

	ocad_color->cyan = 100;
	ocad_color->magenta = 100;
	ocad_color->yellow = 100;
	ocad_color->black = 100;
	convertPascalString("only color", ocad_color->name, 32);
	colorcount++;
	return colorcount - 1;
}
int OcadWriter::addareasymbol(const char *name, int number, int color)
{
	int retnumb = exportAreaSymbol(name, number, file,color);
	if (retnumb != number) return -1;
	return 0;
}
int OcadWriter::exportArea(const vector<point>&area, int symbol)
{
	OCADObject* ocad_object = ocad_object_alloc(NULL);
	memset(ocad_object, 0, sizeof(OCADObject) - sizeof(OCADPoint) + 8 * (ocad_object->npts + ocad_object->ntext));

	// Fill some common entries	of object struct
	OCADPoint* coord_buffer = ocad_object->pts;
	ocad_object->npts = exportCoordinates(area, &coord_buffer);
	ocad_object->angle = 0;

	ocad_object->symbol = symbol;
	ocad_object->type = 3;	// Area
	OCADObjectEntry* entry;
	ocad_object_add(file, ocad_object, &entry);
	entry->npts = ocad_object->npts + ocad_object->ntext;
	return 0;
}

int OcadWriter::writeFile(const char * name)
{
	ofstream fout(name, ios::out | ios::binary);
	fout.write((const char*)file->buffer, file->size);
	return 0;
}

