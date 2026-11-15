# Translations of the interface (gettext).
#
# make update-po   extract the strings from the sources (POTFILES.in) into
#                  tg-expert.pot, and merge them into each language's .po
# make             compile the .po files to .gmo; the debug build finds
#                  them in po/locale
# make install     install them in $(localedir)/<lang>/LC_MESSAGES
#
# To add a language: add it to LINGUAS, POFILES and GMOFILES, create its .po
# with msginit -i tg-expert.pot -l <lang>, and run make update-po.

DOMAIN = $(PACKAGE)
LINGUAS = es de
POFILES = es.po de.po
GMOFILES = es.gmo de.gmo

EXTRA_DIST = POTFILES.in $(DOMAIN).pot $(POFILES)

SUFFIXES = .po .gmo

if HAVE_MSGFMT
all-local: stamp-locale

.po.gmo:
	$(MSGFMT) -c -o $@ $<

# Copies in po/locale/<lang>/LC_MESSAGES, where the debug build looks
stamp-locale: $(GMOFILES)
	for l in $(LINGUAS); do \
	  $(MKDIR_P) locale/$$l/LC_MESSAGES && \
	  cp $$l.gmo locale/$$l/LC_MESSAGES/$(DOMAIN).mo || exit 1; \
	done
	touch $@

install-data-local: $(GMOFILES)
	for l in $(LINGUAS); do \
	  $(MKDIR_P) "$(DESTDIR)$(localedir)/$$l/LC_MESSAGES" && \
	  $(INSTALL_DATA) $$l.gmo "$(DESTDIR)$(localedir)/$$l/LC_MESSAGES/$(DOMAIN).mo" || exit 1; \
	done

uninstall-local:
	for l in $(LINGUAS); do \
	  rm -f "$(DESTDIR)$(localedir)/$$l/LC_MESSAGES/$(DOMAIN).mo"; \
	done

clean-local:
	rm -rf locale

CLEANFILES = $(GMOFILES) stamp-locale
endif

update-po:
	$(XGETTEXT) --force-po --from-code=UTF-8 --keyword=_ --keyword=N_ \
	  --add-comments=TRANSLATORS: \
	  --package-name=$(PACKAGE) --package-version=$(VERSION) \
	  --msgid-bugs-address="https://github.com/jefebus/tg-expert/issues" \
	  --directory=$(top_srcdir) --files-from=$(srcdir)/POTFILES.in \
	  -o $(srcdir)/$(DOMAIN).pot
	for l in $(LINGUAS); do \
	  $(MSGMERGE) --update --backup=none $(srcdir)/$$l.po $(srcdir)/$(DOMAIN).pot || exit 1; \
	done
.PHONY: update-po
