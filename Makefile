
SOURCES = $(wildcard src/*.cpp)

include ../../plugin.mk


dist: all
	mkdir -p dist/Gratrix
	cp LICENSE* dist/Gratrix/
	cp plugin.* dist/Gratrix/
	cp -R res dist/Gratrix/
