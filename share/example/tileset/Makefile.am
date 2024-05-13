tilesetdir = $(pkgdatadir)/tileset
tileset_DATA = victor.bin

CLEANFILES = $(tileset_DATA)

SUFFIXES = .bin
.tsx.bin:
	ultra-sdk-tileset $^ $@
