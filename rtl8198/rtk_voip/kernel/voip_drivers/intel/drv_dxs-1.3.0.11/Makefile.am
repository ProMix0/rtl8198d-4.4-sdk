#
#
#                              Copyright (c) 2014
#                            Lantiq Deutschland GmbH
#                             http://www.lantiq.com
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.
#
#


AUTOMAKE_OPTIONS = foreign 1.7 nostdinc
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

# set distcheck options - see configure.in for list of modifications
DISTCHECK_CONFIGURE_FLAGS=@CONFIGURE_OPTIONS@
CONFIG_CLEAN_FILES=drv_config_user.h src/modules.order

drv_dxs_docdir = ${prefix}/doc/drv_dxs

drv_dxs_doc_DATA = \
	README\
	LICENSE

EXTRA_DIST = $(drv_dxs_doc_DATA) \
	ChangeLog\
	doc/doxyconfig\
	doc/footer.html\
	doc/header.html\
	doc/logo.jpg\
	doc/stylesheet.css\
	doc/html/logo.jpg

# Make subdirectories inside tar.gz writable for everyone
# (required for 2.6 kbuild)
dist-hook:
	find $(distdir) -type d -exec chmod 777 {} \; -print

# Create self extracting linux distribution
distcheck-hook:
	makeself.sh --gzip --notemp $(distdir) drv_dxs-${PACKAGE_VERSION}.sh \
	"${PACKAGE_NAME}"
	cp $(distdir)/src/drv_config_user.template.h $(distdir)/_build/drv_config_user.h

ghs:
	cd src && make ghs

lint:
	@target=lint; \
    list='$(SUBDIRS)'; \
    for subdir in $$list; do \
       echo "Making $$target in $$subdir"; \
       if test "$$subdir" = "."; then \
          dot_seen=yes; \
          local_target="$$target-am"; \
       else \
          local_target="$$target"; \
       fi; \
       (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
       || eval $$failcom; \
     done;

errno:
	@top_srcdir@/script/updateErrno.pl @top_srcdir@/include/drv_dxs_errno.h @top_srcdir@/include/drv_dxs_strerrno.h @top_srcdir@/src
