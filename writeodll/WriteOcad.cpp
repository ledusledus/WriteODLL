// WriteOcad.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "WriteOcadCore.h"
#include <vector>
using namespace std;
#define min(a,b) ((a)>(b)?(b):(a))

int doExport()
{
	IOcadWriter *writer = OcadWriterFactory(10000,10000,10000);

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
bool OcadExp()
{
	return 0==doExport();
}

int _tmain(int argc, _TCHAR* argv[])
{
	OcadExp();
	return 0;
}

