alphadir = $(pkgdatadir)/world
alpha_DATA = alpha.bin

CLEANFILES = $(alpha_DATA)

SUFFIXES = .bin
.world.bin:
	ultra-sdk-world $^ $@
