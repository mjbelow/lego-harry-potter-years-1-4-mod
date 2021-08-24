CC = i686-w64-mingw32-g++
ASM = llvm-ml
INC = -Iinclude

ODIR = obj
SDIR = src

LIBDIR = lib
BINDIR = bin
OUTPUT = /output

MINGWBIN = /opt/llvm-mingw/i686-w64-mingw32/bin

.PHONY: install clean

_OBJS = imgui.o imgui_demo.o imgui_draw.o imgui_impl_dx9.o imgui_impl_win32.o imgui_widgets.o kiero.o main.o assembly_llvm.obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_BINS = InjectedDll.dll MinHook.x86.dll
BINS = $(patsubst %,$(BINDIR)/%,$(_BINS))

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) $(INC) -c -o $@ $<

$(ODIR)/%.obj: $(SDIR)/%.asm
	$(ASM) /Fo $@ $<

install: $(BINS)
	cp $^ $(MINGWBIN)/libunwind.dll $(MINGWBIN)/libc++.dll $(OUTPUT)

clean:
	@rm -f $(ODIR)/*.o $(ODIR)/*.obj
	@rm -f $(BINDIR)/InjectedDll.dll

$(BINDIR)/InjectedDll.dll: $(OBJS) $(LIBDIR)/MinHook.x86.lib $(LIBDIR)/xinput.lib
	$(CC) $^ -lws2_32 -o $@ -shared
