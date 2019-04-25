add_executable(metaflac
    main.c
    operations.c
    operations_shorthand_cuesheet.c
    operations_shorthand_picture.c
    operations_shorthand_seektable.c
    operations_shorthand_streaminfo.c
    operations_shorthand_vorbiscomment.c
    options.c
    usage.c
    utils.c)
target_link_libraries(metaflac FLAC-static getopt utf8)

install(TARGETS metaflac EXPORT targets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
