import sys
from ctypes import *
import unittest
import os

platform="x86"
if sys.maxsize > 2**32:
    platform = "amd64"
# print platform 
dirname=os.path.dirname(os.path.realpath(__file__))
lib=windll.LoadLibrary(os.path.join(dirname,platform,"writeodll.dll"))

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
