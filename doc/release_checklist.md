1. Change version number in
   - /configure.ac
   - /CMakeLists.txt
   - /doc/Doxyfile.in
   - /man/flac.md
   - /man/metaflac.md
   - /test/metaflac-test-files/case07-expect.meta
1. Change version date in
   - /src/libFLAC/format.c
   - /test/metaflac-test-files/case07-expect.meta
1. Update changelog
1. Check copyright year and update if applicable
1. Check libFLAC and libFLAC++ for interface changes and update 
    version numbers in include/FLAC/export.h, include/FLAC++/export.h,
    src/libFLAC/Makefile.am, src/libFLAC++/Makefile.am,
    src/libFLAC/CMakeLists.txt and src/libFLAC++/CMakeLists.txt
1. Prepare and check release tarball by running 
    `git clean -ffxd && ./autogen.sh && ./configure && make distcheck`
1. Check whether release tarball contains api documentation and
    generated man pages
1. Prepare Windows release. Instructions are for building with MinGW-w64
   - Take last release as template
   - Update readme's if necessary
   - Copy changelog and tool documentation
   - Unpack tarball and create empty directories build64 and build32
   - Unpack most recent libogg: change
      `add_library(ogg ${OGG_HEADERS} ${OGG_SOURCES})` to
      `add_library(ogg STATIC ${OGG_HEADERS} ${OGG_SOURCES})`
   - Add `-static-libgcc` to FLAC's CFLAGS
   - Add `-static-libgcc  -static-libstdc++ -Wl,-Bstatic,--whole-archive
     -lwinpthread -Wl,-Bdynamic,--no-whole-archive` to FLAC's CXXFLAGS
   - Run `CMake -DBUILD_SHARED_LIBS=ON .. && ninja` in both build64 and
      build32 in the corresponding build environments
   - Check dependencies of flac.exe, metaflac.exe, libFLAC.dll and
      libFLAC++.dll, e.g. with objdump -x *.* | grep DLL.
      Dependencies should only include KERNEL32.DLL, ADVAPI32.DLL,
      msvcrt.dll and libFLAC.dll
   - Copy flac.exe, metaflac.exe, libFLAC.dll and libFLAC++.dll of both
      builds to proper directories
   - Zip directory
