AM_CXXFLAGS = \
	$(ULTRA_CFLAGS) \
	-I$(top_srcdir)/src

pkglib_LTLIBRARIES = \
	libentity.la \
	libalpha_platform.la \
	libskeleton.la \
	libvictor.la

libentity_la_SOURCES = \
	entity.cc \
	single_sprite_entity.cc
libentity_la_LIBADD = $(ULTRA_LIBS)

libalpha_platform_la_SOURCES = alpha_platform.cc
libalpha_platform_la_LIBADD = \
	$(ULTRA_LIBS) \
	libentity.la
libalpha_platform_la_LDFLAGS = -module -avoid-version -export-dynamic

libskeleton_la_SOURCES = skeleton.cc
libskeleton_la_LIBADD = \
	$(ULTRA_LIBS) \
	libentity.la
libskeleton_la_LDFLAGS = -module -avoid-version -export-dynamic

libvictor_la_SOURCES = victor.cc
libvictor_la_LIBADD = \
	$(ULTRA_LIBS) \
	libentity.la
libvictor_la_LDFLAGS = -module -avoid-version -export-dynamic

install-exec-hook:
	cd $(DESTDIR)$(pkglibdir) && \
		$(RM) $(pkglib_LTLIBRARIES) $(pkglib_LTLIBRARIES:.la=.a)
