import sys
from ctypes import *
platform="x86"
if sys.maxsize > 2**32:
    platform = "amd64"
print platform 

lib=windll.LoadLibrary(platform+"\\writeodll.dll")

class POINT(Structure):
    _fields_ = ("x", c_int), ("y", c_int)

CreateOcadWriter=getattr(lib, "CreateOcadWriter") 
CreateOcadWriter.restype=c_void_p
CleanWriter=getattr(lib, "CleanWriter")
CleanWriter.argtypes=[c_void_p]

AddAreaSymbol=getattr(lib, "AddAreaSymbol") 
AddAreaSymbol.argtypes=[c_void_p,c_char_p, c_int, c_int]
AddColor=getattr(lib, "AddColor")
AddColor.argtypes=[c_void_p,c_char_p]
ExportArea=getattr(lib, "ExportArea") 
WriteOcadFile=getattr(lib, "WriteOcadFile") 

h_writer=CreateOcadWriter(c_double(5555000),c_double(4444000), c_double(10000))
col=AddColor(h_writer, c_char_p("some color"))
sym1=AddAreaSymbol(h_writer,c_char_p("symone"), 4100, col)
sym2=AddAreaSymbol(h_writer,c_char_p("symtwo"), 4010, col)

TYPE=(POINT*3)
t=TYPE()
array=((10,100),(100,10),(100,100))
for i in range(len(array)):
    t[i].x=array[i][0]
    t[i].y=array[i][1]

re = ExportArea(h_writer, t, 3, 4010)

WriteOcadFile(h_writer, c_char_p("c:\\projekti\\WriteODLL\\a.ocd"))
CleanWriter(h_writer)

print "Success?"