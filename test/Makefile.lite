#
# GNU makefile
#
all: clean
	./test_unit.sh
	./test_streams.sh

debug: all

release: all

clean:
	rm -f *.raw *.flac *.cmp *.log
