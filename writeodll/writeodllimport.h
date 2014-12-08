#pragma once
#include "writeodll.h"
extern "C"
{
	__declspec(dllimport) ExportHandle __cdecl CreateOcadWriter(double _offsetx, double _offsety, double _scale);
    __declspec(dllimport) void __cdecl CleanWriter(ExportHandle ohandle);
	__declspec(dllimport) int __cdecl AddColor(ExportHandle ohandle, const char *name);
	__declspec(dllimport) int __cdecl AddAreaSymbol(ExportHandle ohandle, const char *name, int number, int color);
	__declspec(dllimport) int __cdecl ExportArea(ExportHandle ohandle, const point * poPoints, unsigned coPoints, int symbol);
	__declspec(dllimport) int __cdecl WriteOcadFile(ExportHandle ohandle, const char * name);
}
