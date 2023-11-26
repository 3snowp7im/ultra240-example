SUBDIRS = entity

if DEBUG
BGFX_BUILD_TYPE = debug64
BGFX_BIN_TYPE = Debug
DEBUG_FLAGS = -DBX_CONFIG_DEBUG
else
BGFX_BUILD_TYPE = release64
BGFX_BIN_TYPE = Release
endif

if LINUX
BGFX_BUILD_TARGET = linux-$(BGFX_BUILD_TYPE)
BGFX_BIN_DIR = $(top_srcdir)/src/bgfx/.build/linux64_gcc/bin
BGFX_LIB = bgfx-shared-lib$(BGFX_BIN_TYPE)
BGFX_SHADERC = $(BGFX_BIN_DIR)/shaderc$(BGFX_BIN_TYPE)
BGFX_SHADERC_PLATFORM = linux
else
endif

SHADER_SRCS = \
	shader/vs_pass.c \
	shader/fs_quad.c \
	shader/vs_box.c \
	shader/fs_line.c

BUILT_SOURCES = $(SHADER_SRCS)

CLEANFILES = $(SHADER_SRCS)

bin_PROGRAMS = example
example_SOURCES = example.cc
example_CXXFLAGS = \
	$(GLM_CFLAGS) \
	$(ULTRA_CFLAGS) \
	$(SDL2_CFLAGS) \
	$(WAYLAND_CFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/bgfx/include \
	-I$(top_srcdir)/src/bx/include \
	$(DEBUG_FLAGS)

example_LDADD = \
	$(GLM_LIBS) \
	$(ULTRA_LIBS) \
	$(SDL2_LIBS) \
	$(WAYLAND_LIBS)
example_LDFLAGS = -L$(BGFX_BIN_DIR) -l$(BGFX_LIB)
example_DEPENDENCIES = \
	$(BGFX_BIN_DIR)/lib$(BGFX_LIB).so \
	$(SHADER_SRCS)

# Installs bgfx shared library.
bgfx_libdir = $(libdir)
dist_bgfx_lib_SCRIPTS = $(BGFX_BIN_DIR)/lib$(BGFX_LIB).so

$(BGFX_BIN_DIR)/lib$(BGFX_LIB).so $(BGFX_SHADERC):
	cd $(top_srcdir)/src/bgfx && make $(BGFX_BUILD_TARGET)

$(SHADER_SRCS): $(BGFX_SHADERC)

SUFFIXES = .fsc .vsc
.vsc.c:
	mkdir -p shader
	$(BGFX_SHADERC) \
		-i $(top_srcdir)/src/bgfx/src \
		--platform $(BGFX_SHADERC_PLATFORM) \
		--profile 150 \
		--type vertex \
		-f $^ \
		--define shader/varying.def.sc \
		--bin2c \
		-o $@
.fsc.c:
	mkdir -p shader
	$(BGFX_SHADERC) \
		-i $(top_srcdir)/src/bgfx/src \
		--platform $(BGFX_SHADERC_PLATFORM) \
		--profile 150 \
		--type fragment \
		-f $^ \
		--define shader/varying.def.sc \
		--bin2c \
		-o $@
