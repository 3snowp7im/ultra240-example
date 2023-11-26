AC_INIT([ultra240-example], [0.0.1])
PKG_CHECK_MODULES([GLM], [glm])
PKG_CHECK_MODULES([SDL2], [sdl2])
PKG_CHECK_MODULES([WAYLAND], [wayland-egl])
PKG_CHECK_MODULES([ULTRA], [ultra240])
PKG_CHECK_MODULES([ULTRA_SDK], [ultra240-sdk])
AC_ARG_ENABLE([debug], [
  AS_HELP_STRING([--enable-debug],
    [Whether to include debug symbols (default is no)]
  )
], [enable_debug=$enableval], [enable_debug=no]
)
AM_INIT_AUTOMAKE([subdir-objects])
AM_CONDITIONAL([LINUX], [test $(uname -s) = Linux])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = xyes])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([
  Makefile
  src/example/Makefile
  src/example/entity/Makefile
  share/example/Makefile
  share/example/img/Makefile
  share/example/tileset/Makefile
  share/example/world/Makefile
  share/example/world/alpha/Makefile
])
AC_PROG_CC
AC_PROG_CXX
LT_INIT
AC_OUTPUT
