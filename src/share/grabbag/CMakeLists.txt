add_library(grabbag STATIC
    alloc.c
    cuesheet.c
    file.c
    picture.c
    replaygain.c
    seektable.c
    snprintf.c)
target_link_libraries(grabbag
    FLAC-static
    replaygain_analysis
    $<TARGET_NAME_IF_EXISTS:win_utf8_io>)
