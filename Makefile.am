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
		   src/notes_panel.c \
		   src/output_panel.c \
		   src/paperstrip_render.c \
		   src/render.c \
		   src/report.c \
		   src/report_dialog.c \
		   src/serializer.c \
		   src/session.c \
		   src/trend.c \
		   src/trend_render.c \
		   src/waveform_render.c \
		   src/tg.h

tg_expert_dbg_SOURCES = $(tg_expert_SOURCES)
tg_expert_prf_SOURCES = $(tg_expert_SOURCES)
tg_expert_vlg_SOURCES = $(tg_expert_SOURCES)

LIBS = $(GTK_LIBS) \
       $(CAIRO_PDF_LIBS) \
       $(GTHREAD_LIBS) \
       $(PORTAUDIO_LIBS) \
       $(FFTW_LIBS) \
       -lpthread \
       -lm

AM_CPPFLAGS = -DPROGRAM_NAME=\"tg-expert\" -DVERSION=\"$(PACKAGE_VERSION)\" \
	      -DDOCDIR=\"$(docdir)\" -DLOCALEDIR=\"$(localedir)\"
tg_expert_dbg_CPPFLAGS = $(AM_CPPFLAGS) -DDEBUG

AM_CFLAGS = $(GTK_CFLAGS) \
	    $(CAIRO_PDF_CFLAGS) \
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

# Test programs in tests/, each with its own main(). "make test" builds them
# and the debug build; "make test-run" also runs them all, then the built-in
# tests of the debug build ("tg-expert-dbg test"), and fails if any fails.
# Some include the source file they test (#include "../src/x.c"), so that
# file is not linked again; tests/stubs.c and tests/stubs_output.c stand in
# for the parts of interface.c and output_panel.c they do not link.
EXTRA_PROGRAMS += tests/algorithm_settings \
		  tests/algorithm_trim \
		  tests/calibration_fallback \
		  tests/correlations \
		  tests/detection_status \
		  tests/rate_only \
		  tests/render_preview \
		  tests/serializer_metadata \
		  tests/session_file \
		  tests/trend_be_ranges

tests_algorithm_settings_SOURCES = tests/algorithm_settings.c
tests_algorithm_trim_SOURCES = tests/algorithm_trim.c
tests_calibration_fallback_SOURCES = tests/calibration_fallback.c tests/stubs.c \
		  src/algo.c src/algorithm_settings.c src/audio.c
tests_correlations_SOURCES = tests/correlations.c tests/stubs.c tests/stubs_output.c \
		  src/algorithm_settings.c src/render.c src/trend.c
tests_detection_status_SOURCES = tests/detection_status.c tests/stubs.c \
		  src/algo.c src/algorithm_settings.c src/audio.c src/computer.c \
		  src/correlations.c src/paperstrip_render.c src/render.c src/trend.c \
		  src/trend_render.c src/waveform_render.c
tests_rate_only_SOURCES = tests/rate_only.c tests/stubs.c \
		  src/algo.c src/algorithm_settings.c src/audio.c src/serializer.c src/trend.c
tests_render_preview_SOURCES = tests/render_preview.c \
		  src/correlations.c src/paperstrip_render.c src/render.c src/report.c \
		  src/serializer.c src/session.c src/trend.c src/trend_render.c \
		  src/waveform_render.c
tests_serializer_metadata_SOURCES = tests/serializer_metadata.c tests/stubs.c \
		  src/algo.c src/algorithm_settings.c src/audio.c src/serializer.c src/trend.c
tests_session_file_SOURCES = tests/session_file.c
tests_trend_be_ranges_SOURCES = tests/trend_be_ranges.c tests/stubs.c \
		  src/algorithm_settings.c src/trend.c

# Run without arguments; render_preview is run apart, it writes two PDFs
TEST_RUN = tests/algorithm_settings$(EXEEXT) \
	   tests/algorithm_trim$(EXEEXT) \
	   tests/calibration_fallback$(EXEEXT) \
	   tests/correlations$(EXEEXT) \
	   tests/detection_status$(EXEEXT) \
	   tests/rate_only$(EXEEXT) \
	   tests/serializer_metadata$(EXEEXT) \
	   tests/session_file$(EXEEXT) \
	   tests/trend_be_ranges$(EXEEXT)
# Not "..._PROGRAMS": automake would take it for programs to install
TEST_BUILD = $(TEST_RUN) tests/render_preview$(EXEEXT)
MOSTLYCLEANFILES += render-preview.pdf report-preview.pdf

test: $(TEST_BUILD) tg-expert-dbg$(EXEEXT)

test-run: test
	@failed=""; \
	for t in $(TEST_RUN); do \
		echo "== $$t"; \
		./$$t || failed="$$failed $$t"; \
	done; \
	echo "== tests/render_preview$(EXEEXT) (writes render-preview.pdf and report-preview.pdf)"; \
	./tests/render_preview$(EXEEXT) render-preview.pdf || failed="$$failed tests/render_preview$(EXEEXT)"; \
	echo "== tg-expert-dbg test"; \
	./tg-expert-dbg$(EXEEXT) test || failed="$$failed tg-expert-dbg"; \
	if test -n "$$failed"; then \
		echo "FAILED:$$failed"; exit 1; \
	fi; \
	echo "All tests passed"
.PHONY: test test-run

valgrind: tg-expert-vlg
	valgrind --leak-check=full -v --num-callers=99 --suppressions=.valgrind.supp ./$^
.PHONY: valgrind
