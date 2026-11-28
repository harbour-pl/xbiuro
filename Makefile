#
# Program...: xBiuro
# Plik......: Makefile
#
#   make            buduje prog-bibl/libbiuro.dll, xbiuro.exe i moduły w prog-modu/ (i biblioteki XWGUI i xRDD) oraz
#                   kopiuje libxwgui.dll, libxrdd.dll i libharu (libhpdf.dll z zlib1.dll i libpng16-16.dll) do prog-bibl/
#   make run        buduje i uruchamia xbiuro.exe
#   make clean      usuwa xbiuro.exe, moduły, libbiuro.dll z biblioteką importu i kopie DLL z prog-bibl/
#
# Bez plików obiektów: każdy plik wynikowy powstaje jednym poleceniem prosto ze źródeł
#

CC       = clang
XWGUI   ?= ../xwgui

SRCDIR   = src/biuro
BIBL     = prog-bibl
MODU     = prog-modu

EXE      = xbiuro.exe
EXESRCS  = $(SRCDIR)/biuro.c
HDRS     = $(wildcard $(SRCDIR)/*.h $(XWGUI)/include/*.h)

# Wspólny kod xBiura dla programu i modułów (zasada 7): wszystko z src/biuro/ poza biuro.c
BIURO    = $(BIBL)/libbiuro.dll
BIUROIMP = lib/libbiuro.dll.a
BIUROSRCS = $(filter-out $(EXESRCS),$(wildcard $(SRCDIR)/*.c))

# Moduły (zasada 6): DLL w prog-modu/, ładowane przez xbiuro.exe przy wejściu w moduł
FAKTURA  = $(MODU)/faktura.dll
FAKTURASRCS = $(wildcard src/faktura/*.c)
FAKTURAHDRS = $(wildcard src/faktura/*.h)

# XWGUI i xRDD (tabele DBF xDev 7) jako DLL: program linkuje ich biblioteki importu, a DLL leżą we wspólnych
# bibliotekach programu; obie buduje Makefile XWGUI
XWGUIDLL = $(XWGUI)/lib/libxwgui.dll
XRDDDLL  = $(XWGUI)/lib/libxrdd.dll
XSIECDLL = $(XWGUI)/lib/libxsiec.dll

# libharu (PDF) z MSYS2 clang64, katalogu kompilatora, razem z bibliotekami, których potrzebuje
SYSBIN   = $(dir $(shell command -v $(CC)))
HPDFDLL  = libhpdf.dll zlib1.dll libpng16-16.dll

# libcurl i OpenSSL dla xSiec (KSeF) z MSYS2 clang64, razem z bibliotekami, których potrzebują
NETDLL   = libcurl-4.dll libcrypto-3-x64.dll libssl-3-x64.dll libssh2-1.dll libidn2-0.dll libbrotlidec.dll libbrotlicommon.dll libzstd.dll libnghttp2-14.dll libngtcp2_crypto_ossl-0.dll libngtcp2-16.dll libnghttp3-9.dll libpsl-5.dll libiconv-2.dll libunistring-5.dll libintl-8.dll

# Manifest jak w programach xDev: Common Controls 6, strona kodowa UTF-8 i wspólne biblioteki z prog-bibl/;
# wbudowany w xbiuro.exe
MANIFEST = $(SRCDIR)/xbiuro.manifest

CFLAGS  ?= -O2
CFLAGS  += -std=c11 -Wall -Wextra
CPPFLAGS += -I$(XWGUI)/include -I$(SRCDIR) -DUNICODE -D_UNICODE
LDFLAGS += -mwindows -Wl,--Xlink=/manifest:embed -Wl,--Xlink=/manifestinput:$(MANIFEST)
LIBS     = -L$(XWGUI)/lib -lxwgui -lxrdd -lxsiec -lhpdf

.PHONY: all run clean FORCE

all: $(EXE) $(FAKTURA) $(BIBL)/libxwgui.dll $(BIBL)/libxrdd.dll $(BIBL)/libxsiec.dll $(addprefix $(BIBL)/,$(HPDFDLL) $(NETDLL))

$(BIURO): $(BIUROSRCS) $(HDRS) $(XWGUIDLL) $(XRDDDLL) FORCE
	@mkdir -p $(@D) $(dir $(BIUROIMP))
	@tmp=$$(mktemp -d); for f in $(BIUROSRCS); do echo "$(CC) $(CPPFLAGS) $(CFLAGS) -c $$f"; $(CC) $(CPPFLAGS) $(CFLAGS) -c $$f -o $$tmp/$$(echo $$f | tr / _).o || { rm -rf $$tmp; exit 1; }; done; echo "$(CC) -shared -o $@ -Wl,--export-all-symbols -Wl,--out-implib,$(BIUROIMP) $(LIBS)"; $(CC) -shared $$tmp/*.o -o $@ -Wl,--export-all-symbols -Wl,--out-implib,$(BIUROIMP) $(LIBS); status=$$?; rm -rf $$tmp; exit $$status

$(EXE): $(EXESRCS) $(HDRS) $(BIURO) $(MANIFEST) FORCE
	@tmp=$$(mktemp -d); for f in $(EXESRCS); do echo "$(CC) $(CPPFLAGS) $(CFLAGS) -c $$f"; $(CC) $(CPPFLAGS) $(CFLAGS) -c $$f -o $$tmp/$$(echo $$f | tr / _).o || { rm -rf $$tmp; exit 1; }; done; echo "$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -L$(dir $(BIUROIMP)) -lbiuro $(LIBS)"; $(CC) $(CFLAGS) $(LDFLAGS) $$tmp/*.o -o $@ -L$(dir $(BIUROIMP)) -lbiuro $(LIBS); status=$$?; rm -rf $$tmp; exit $$status

$(FAKTURA): $(FAKTURASRCS) $(FAKTURAHDRS) $(HDRS) $(BIURO) FORCE
	@mkdir -p $(@D)
	@tmp=$$(mktemp -d); for f in $(FAKTURASRCS); do echo "$(CC) $(CPPFLAGS) $(CFLAGS) -c $$f"; $(CC) $(CPPFLAGS) $(CFLAGS) -c $$f -o $$tmp/$$(echo $$f | tr / _).o || { rm -rf $$tmp; exit 1; }; done; echo "$(CC) -shared -o $@ -Wl,--export-all-symbols -L$(dir $(BIUROIMP)) -lbiuro $(LIBS)"; $(CC) -shared $$tmp/*.o -o $@ -Wl,--export-all-symbols -L$(dir $(BIUROIMP)) -lbiuro $(LIBS); status=$$?; rm -rf $$tmp; exit $$status

# Biblioteki buduje Makefile XWGUI
$(XWGUIDLL): FORCE
	@$(MAKE) --no-print-directory -C $(XWGUI)

$(XRDDDLL): $(XWGUIDLL)

$(XSIECDLL): $(XWGUIDLL)

$(BIBL)/libxwgui.dll: $(XWGUIDLL)
	cp $< $@

$(BIBL)/libxrdd.dll: $(XRDDDLL)
	cp $< $@

$(BIBL)/libxsiec.dll: $(XSIECDLL)
	cp $< $@

$(addprefix $(BIBL)/,$(HPDFDLL) $(NETDLL)): $(BIBL)/%: $(SYSBIN)%
	cp $< $@

run: all
	./$(EXE)

clean:
	rm -f $(EXE) $(FAKTURA) $(BIURO) $(BIUROIMP) $(BIBL)/libxwgui.dll $(BIBL)/libxrdd.dll $(BIBL)/libxsiec.dll $(addprefix $(BIBL)/,$(HPDFDLL) $(NETDLL))
