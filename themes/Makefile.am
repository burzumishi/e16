
THEME = winter
THEME_FILES = winter.etheme

EXTRA_DIST = $(THEME_FILES) 

THEME_DIR = $(DESTDIR)$(pkgdatadir)/themes/$(THEME)

install-data-local:
	$(mkinstalldirs) $(THEME_DIR)
	for tf in $(THEME_FILES); do \
		gzip -d -c < $(srcdir)/$$tf | (cd $(THEME_DIR); tar -xmf -); \
	done
	-if [ "$$EUID" = "0" ]; then chown -R 0:0 $(THEME_DIR); fi

uninstall-local:
	-for tf in $(THEME_FILES); do \
		gzip -d -c < $(srcdir)/$$tf | (cd $(THEME_DIR); \
			rm -f `tar -tf -` 2>/dev/null); \
	done
