add_executable(test_libFLAC
    bitreader.c
    bitwriter.c
    crc.c
    decoders.c
    encoders.c
    endswap.c
    format.c
    main.c
    metadata.c
    metadata_manip.c
    metadata_object.c
    md5.c)

target_compile_definitions(test_libFLAC PRIVATE
    $<$<BOOL:${ENABLE_64_BIT_WORDS}>:ENABLE_64_BIT_WORDS>)
target_include_directories(test_libFLAC PRIVATE "$<TARGET_PROPERTY:FLAC-static,SOURCE_DIR>/include")
target_link_libraries(test_libFLAC FLAC-static grabbag test_libs_common)

add_test(NAME FLAC COMMAND test_libFLAC)
