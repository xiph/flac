#
# GNU Makefile
#
# Useful targets
#
# all     : build all libraries and programs in the default configuration (currently 'release')
# debug   : build all libraries and programs in debug mode
# release : build all libraries and programs in release mode
# test    : run the unit and stream tests
# clean   : remove all non-distro files
#

all: libFLAC flac test_streams test_unit

DEFAULT_CONFIG = release

CONFIG = $(DEFAULT_CONFIG)

debug   : CONFIG = debug
release : CONFIG = release

debug   : all
release : all

libFLAC:
	(cd src/$@ ; make $(CONFIG))

flac: libFLAC
	(cd src/$@ ; make $(CONFIG))

plugin_xmms: libFLAC
	(cd src/$@ ; make $(CONFIG))

test_streams: libFLAC
	(cd src/$@ ; make $(CONFIG))

test_unit: libFLAC
	(cd src/$@ ; make $(CONFIG))

test: debug
	(cd test ; make)

clean:
	-(cd src/libFLAC ; make clean)
	-(cd src/flac ; make clean)
	-(cd src/plugin_xmms ; make clean)
	-(cd src/test_streams ; make clean)
	-(cd src/test_unit ; make clean)
	-(cd test ; make clean)
