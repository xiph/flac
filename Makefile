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
	(cd src/$@ ; $(MAKE) $(CONFIG))

flac: libFLAC
	(cd src/$@ ; $(MAKE) $(CONFIG))

plugin_xmms: libFLAC
	(cd src/$@ ; $(MAKE) $(CONFIG))

test_streams: libFLAC
	(cd src/$@ ; $(MAKE) $(CONFIG))

test_unit: libFLAC
	(cd src/$@ ; $(MAKE) $(CONFIG))

test: debug
	(cd test ; $(MAKE))

clean:
	-(cd src/libFLAC ; $(MAKE) clean)
	-(cd src/flac ; $(MAKE) clean)
	-(cd src/plugin_xmms ; $(MAKE) clean)
	-(cd src/test_streams ; $(MAKE) clean)
	-(cd src/test_unit ; $(MAKE) clean)
	-(cd test ; $(MAKE) clean)
