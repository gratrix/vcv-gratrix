FLAGS += -I../../src/core

SOURCES = $(wildcard src/*.cpp)


include ../../plugin.mk


dist: all
	mkdir -p dist/Gratrix
	cp LICENSE* dist/Gratrix/
	cp $(TARGET) dist/Gratrix/
	cp -R res dist/Gratrix/
