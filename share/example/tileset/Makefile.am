tilesetdir = $(pkgdatadir)/tileset
tileset_DATA = victor.bin

CLEANFILES = $(tileset_DATA)

SUFFIXES = .tsx .bin
.tsx.bin:
	ultra-sdk-tileset $^ $@
