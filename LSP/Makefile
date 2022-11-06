!include <win32.mak>

.cpp.obj:
	$(cc) $(cdebug) -c -W3 -DWIN32 -LDd -D_WIN32 -DFD_SETSIZE=1024 $(cvarsdll) $<

DLL_FILES= spi.cpp        \
           extension.cpp  \
           overlap.cpp    \
           provider.cpp   \
           sockinfo.cpp   \
           asyncselect.cpp

OBJS= $(DLL_FILES:.cpp=.obj)

INST_FILES= instlsp.cpp \
            provider.cpp

INST_OBJS= $(INST_FILES:.cpp=.obj)

all: lsp.dll instlsp.exe

lsp.dll: $(OBJS)
	$(link) $(linkdebug) $(dlllflags) -map -def:lsp.def \
	-out:$*.dll $(OBJS) -pdb:$*.pdb $(guilibsdll) ws2_32.lib
        copy $*.dll %SYSTEMROOT%\System32

instlsp.exe: $(INST_OBJS)
    $(link) $(linkdebug) $(conlflags) -out:$*.exe $(INST_OBJS) \
        $(conlibs) sporder.lib ws2_32.lib  rpcrt4.lib

clean:
    -del *.obj
	-del *.dll
	-del *.pch
	-del *.exp
    -del *.pdb
	-del lsp.lib
    -del *.exe
    -del *.map
