SUBDIRS = icons
bin_PROGRAMS = tg-expert
EXTRA_PROGRAMS = tg-expert-dbg \
		 tg-expert-prf \
		 tg-expert-vlg
MOSTLYCLEANFILES = $(EXTRA_PROGRAMS)

tg_expert_SOURCES = src/algo.c \
		   src/audio.c \
		   src/audio_settings.c \
		   src/computer.c \
		   src/config.c \
		   src/interface.c \
		   src/output_panel.c \
		   src/serializer.c \
		   src/trend.c \
		   src/tg.h

tg_expert_dbg_SOURCES = $(tg_expert_SOURCES)
tg_expert_prf_SOURCES = $(tg_expert_SOURCES)
tg_expert_vlg_SOURCES = $(tg_expert_SOURCES)

LIBS = $(GTK_LIBS) \
       $(GTHREAD_LIBS) \
       $(PORTAUDIO_LIBS) \
       $(FFTW_LIBS) \
       -lpthread \
       -lm

AM_CPPFLAGS = -DPROGRAM_NAME=\"Tg\ Expert\" -DVERSION=\"$(PACKAGE_VERSION)\" \
	      -DDOCDIR=\"$(docdir)\"
tg_expert_dbg_CPPFLAGS = $(AM_CPPFLAGS) -DDEBUG

AM_CFLAGS = $(GTK_CFLAGS) \
	    $(GTHREAD_CFLAGS) \
	    $(PORTAUDIO_CFLAGS) \
	    $(FFTW_CFLAGS)
tg_expert_dbg_CFLAGS = $(AM_CFLAGS) -ggdb
tg_expert_prf_CFLAGS = $(AM_CFLAGS) -pg
tg_expert_vlg_CFLAGS = $(AM_CFLAGS) -g

tg_expert_LDFLAGS = $(AM_LDFLAGS)
tg_expert_dbg_LDFLAGS = $(AM_LDFLAGS)
tg_expert_prf_LDFLAGS = $(AM_LDFLAGS)
tg_expert_vlg_LDFLAGS = $(AM_LDFLAGS)

if BE_WINDOWS
if HAVE_WINDRES
tg_expert_SOURCES += icons/tg-timer.rc
endif
tg_expert_LDFLAGS += -mwindows
tg_expert_dbg_LDFLAGS += -mconsole
tg_expert_prf_LDFLAGS += -mconsole
tg_expert_vlg_LDFLAGS += -mconsole
endif

desktopdir = $(datadir)/applications
dist_desktop_DATA = icons/tg-timer.desktop

mimedir = $(datadir)/mime/packages
dist_mime_DATA = icons/tg-timer.xml

dist_man_MANS = docs/tg-expert.1

# The user manual, built with Typst when configure finds it.
# The program looks for it in $(docdir).
if HAVE_TYPST
doc_DATA = tg-expert-manual.pdf
CLEANFILES = tg-expert-manual.pdf
MANUAL_FILES = $(srcdir)/docs/manual/main.typ \
		 $(srcdir)/docs/manual/lib.typ \
		 $(srcdir)/docs/manual/chapters/introduction.typ \
		 $(srcdir)/docs/manual/chapters/installation.typ \
		 $(srcdir)/docs/manual/chapters/first-start.typ \
		 $(srcdir)/docs/manual/chapters/microphone.typ \
		 $(srcdir)/docs/manual/chapters/main-window.typ \
		 $(srcdir)/docs/manual/chapters/measuring.typ \
		 $(srcdir)/docs/manual/chapters/trend.typ \
		 $(srcdir)/docs/manual/chapters/calibration.typ \
		 $(srcdir)/docs/manual/chapters/files.typ \
		 $(srcdir)/docs/manual/chapters/troubleshooting.typ \
		 $(srcdir)/docs/manual/chapters/appendix.typ
tg-expert-manual.pdf: $(MANUAL_FILES)
	$(TYPST) compile $(srcdir)/docs/manual/main.typ $@
manual: tg-expert-manual.pdf
else
manual:
	@echo "Typst was not found when configure ran. Install it (https://typst.app)" >&2
	@echo "and run ./configure again to build the user manual." >&2
	@exit 1
endif
.PHONY: manual

EXTRA_DIST = autogen.sh \
	     icons \
	     packaging \
	     docs/manual \
	     LICENSE \
	     README.md \
	     BUILDING.md \
	     CHANGELOG.md

.rc.o:
	$(WINDRES) $< -O coff -o $@

test: tg-expert-dbg
	./tg-expert-dbg test
.PHONY: test

valgrind: tg-expert-vlg
	valgrind --leak-check=full -v --num-callers=99 --suppressions=.valgrind.supp ./$^
.PHONY: valgrind
