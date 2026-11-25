SUBDIRS = icons po
bin_PROGRAMS = tg-expert
EXTRA_PROGRAMS = tg-expert-dbg \
		 tg-expert-prf \
		 tg-expert-vlg
MOSTLYCLEANFILES = $(EXTRA_PROGRAMS)

tg_expert_SOURCES = src/algo.c \
		   src/algorithm_settings.c \
		   src/audio.c \
		   src/audio_settings.c \
		   src/computer.c \
		   src/config.c \
		   src/correlations.c \
		   src/interface.c \
		   src/output_panel.c \
		   src/serializer.c \
		   src/session.c \
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
	      -DDOCDIR=\"$(docdir)\" -DLOCALEDIR=\"$(localedir)\"
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

if HAVE_MSGFMT
tg_expert_dbg_DEPENDENCIES = po/stamp-locale
po/stamp-locale: $(srcdir)/po/es.po $(srcdir)/po/de.po
	$(MAKE) -C po
endif

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

# The user manual in English, Spanish and German, built with Typst when
# configure finds it. The program looks for tg-expert-manual-<lang>.pdf in
# $(docdir), then for the English tg-expert-manual.pdf.
if HAVE_TYPST
MANUALS = tg-expert-manual.pdf tg-expert-manual-es.pdf tg-expert-manual-de.pdf
doc_DATA = $(MANUALS)
CLEANFILES = $(MANUALS)
MANUAL_DIR = $(srcdir)/docs/manual
MANUAL_EN = $(srcdir)/version $(MANUAL_DIR)/main.typ $(MANUAL_DIR)/lib.typ \
	    $(MANUAL_DIR)/chapters/introduction.typ $(MANUAL_DIR)/chapters/installation.typ \
	    $(MANUAL_DIR)/chapters/first-start.typ $(MANUAL_DIR)/chapters/microphone.typ \
	    $(MANUAL_DIR)/chapters/main-window.typ $(MANUAL_DIR)/chapters/measuring.typ \
	    $(MANUAL_DIR)/chapters/algorithm.typ \
	    $(MANUAL_DIR)/chapters/trend.typ $(MANUAL_DIR)/chapters/calibration.typ \
	    $(MANUAL_DIR)/chapters/files.typ $(MANUAL_DIR)/chapters/swiss_lever.typ \
	    $(MANUAL_DIR)/chapters/troubleshooting.typ \
	    $(MANUAL_DIR)/chapters/appendix.typ
MANUAL_ES = $(srcdir)/version $(MANUAL_DIR)/main-es.typ $(MANUAL_DIR)/lib.typ \
	    $(MANUAL_DIR)/es/introduction.typ $(MANUAL_DIR)/es/installation.typ \
	    $(MANUAL_DIR)/es/first-start.typ $(MANUAL_DIR)/es/microphone.typ \
	    $(MANUAL_DIR)/es/main-window.typ $(MANUAL_DIR)/es/measuring.typ \
	    $(MANUAL_DIR)/es/algorithm.typ \
	    $(MANUAL_DIR)/es/trend.typ $(MANUAL_DIR)/es/calibration.typ \
	    $(MANUAL_DIR)/es/files.typ $(MANUAL_DIR)/es/swiss_lever.typ \
	    $(MANUAL_DIR)/es/troubleshooting.typ \
	    $(MANUAL_DIR)/es/appendix.typ
MANUAL_DE = $(srcdir)/version $(MANUAL_DIR)/main-de.typ $(MANUAL_DIR)/lib.typ \
	    $(MANUAL_DIR)/de/introduction.typ $(MANUAL_DIR)/de/installation.typ \
	    $(MANUAL_DIR)/de/first-start.typ $(MANUAL_DIR)/de/microphone.typ \
	    $(MANUAL_DIR)/de/main-window.typ $(MANUAL_DIR)/de/measuring.typ \
	    $(MANUAL_DIR)/de/algorithm.typ \
	    $(MANUAL_DIR)/de/trend.typ $(MANUAL_DIR)/de/calibration.typ \
	    $(MANUAL_DIR)/de/files.typ $(MANUAL_DIR)/de/swiss_lever.typ \
	    $(MANUAL_DIR)/de/troubleshooting.typ \
	    $(MANUAL_DIR)/de/appendix.typ
TYPST_FLAGS = --root "$(srcdir)"
tg-expert-manual.pdf: $(MANUAL_EN)
	$(TYPST) compile $(TYPST_FLAGS) $(MANUAL_DIR)/main.typ $@
tg-expert-manual-es.pdf: $(MANUAL_ES)
	$(TYPST) compile $(TYPST_FLAGS) $(MANUAL_DIR)/main-es.typ $@
tg-expert-manual-de.pdf: $(MANUAL_DE)
	$(TYPST) compile $(TYPST_FLAGS) $(MANUAL_DIR)/main-de.typ $@
manual: $(MANUALS)
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
	     version \
	     LICENSE \
	     README.md \
	     BUILDING.md \
	     CHANGELOG.md

.rc.o:
	$(WINDRES) $< -O coff -o $@

test: tg-expert-dbg$(EXEEXT)
	./tg-expert-dbg$(EXEEXT) test
.PHONY: test

valgrind: tg-expert-vlg
	valgrind --leak-check=full -v --num-callers=99 --suppressions=.valgrind.supp ./$^
.PHONY: valgrind
