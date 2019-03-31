cmake_minimum_required(VERSION 3.12)

check_include_file("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_file("termios.h" HAVE_TERMIOS_H)

add_compile_definitions(
    $<$<BOOL:${HAVE_SYS_IOCTL_H}>:HAVE_SYS_IOCTL_H>
    $<$<BOOL:${HAVE_TERMIOS_H}>:HAVE_TERMIOS_H>)

add_executable(flac
    analyze.c
    decode.c
    encode.c
    foreign_metadata.c
    main.c
    local_string_utils.c
    utils.c
    vorbiscomment.c)
target_link_libraries(flac
    FLAC-static
    getopt
    grabbag
    replaygain_synthesis
    utf8)

install(TARGETS flac EXPORT targets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
