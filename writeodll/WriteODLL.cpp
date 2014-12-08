// WriteODLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "WriteOcadCore.h"
#include "writeodll.h"
extern "C"
{
	__declspec(dllexport) ExportHandle __cdecl CreateOcadWriter(double _offsetx, double _offsety, double _scale)
	{
		return (ExportHandle)OcadWriterFactory(_offsetx, _offsety, _scale);
	}
	__declspec(dllexport) void __cdecl CleanWriter(ExportHandle ohandle)
	{
		IOcadWriter* p = (IOcadWriter*)ohandle;
		delete p;
	}
	__declspec(dllexport) int __cdecl AddColor(ExportHandle ohandle, const char *name)
	{
		return ((IOcadWriter*)ohandle)->addcolor(name);
	}
	__declspec(dllexport) int __cdecl AddAreaSymbol(ExportHandle ohandle, const char *name, int number, int color)
	{
		return ((IOcadWriter*)ohandle)->addareasymbol(name, number, color);
	}
	__declspec(dllexport) int __cdecl ExportArea(ExportHandle ohandle, const point * poPoints, unsigned coPoints, int symbol)
	{
		vector<point> vect(poPoints, poPoints + coPoints);
		return ((IOcadWriter*)ohandle)->exportArea(vect, symbol);
	}
	__declspec(dllexport) int __cdecl WriteOcadFile(ExportHandle ohandle, const char * name)
	{
		return ((IOcadWriter*)ohandle)->writeFile(name);
	}

}
#if 0
int doExport()
{
	IOcadWriter *writer = OcadWriterFactory(10000, 10000, 10000);

	int color = writer->addcolor("one");
	ChkErr(writer->addareasymbol("areasymbol", 4100, color));
	ChkErr(writer->addareasymbol("areasymbol", 4010, color));
	vector<point> cords_to_export;
	cords_to_export.push_back(point(1000, 1000));
	cords_to_export.push_back(point(2000, 1000));
	cords_to_export.push_back(point(1000, 2000)); // coordinate is 100
	ChkErr(writer->exportArea(cords_to_export, 4100));
	vector<point> cords_to_export2;
	cords_to_export2.push_back(point(500, 1000));
	cords_to_export2.push_back(point(1500, 1000));
	cords_to_export2.push_back(point(500, 1500)); // coordinate is 100
	ChkErr(writer->exportArea(cords_to_export2, 4010));

	ChkErr(writer->writeFile("c:\\projekti\\write_ocad\\second.ocd"));
	delete writer;
	return 0;
}
#endif
