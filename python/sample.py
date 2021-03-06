from dllwrapper import *

class TestPointContainer(unittest.TestCase):
    def setUp(self):
        pass
    def testDllAPIs(self):
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

if __name__ == '__main__':
    unittest.main()
