AM_LDFLAGS = -module -avoid-version -export-dynamic
AM_CXXFLAGS = $(ULTRA_CFLAGS)

pkglib_LTLIBRARIES = libalpha_platform.la libskeleton.la libvictor.la
libalpha_platform_la_SOURCES = alpha_platform.cc
libskeleton_la_SOURCES = skeleton.cc
libvictor_la_SOURCES = victor.cc

install-exec-hook:
	cd $(DESTDIR)$(pkglibdir) && \
		$(RM) $(pkglib_LTLIBRARIES) $(pkglib_LTLIBRARIES:.la=.a)
