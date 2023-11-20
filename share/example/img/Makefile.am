imgdir = $(pkgdatadir)/img
img_DATA = alpha.bmp victor.bmp skeleton.bmp

CLEANFILES = $(img_DATA)

victor.bmp: TILESET = $(srcdir)/../tileset/victor.tsx
skeleton.bmp: TILESET = $(srcdir)/../tileset/skeleton.tsx

SUFFIXES = .png .bmp
.png.bmp:
	ultra-sdk-img $^ $(TILESET) $@
