find_package(ZLIB)
if (ZLIB_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE ZLIB::ZLIB)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_ZLIB)
endif ()

find_package(LZ4)
if (LZ4_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE LZ4::LZ4)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_LZ4)
endif ()

find_package(LZMA)
if (LZMA_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE LZMA::LZMA)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_LZMA)
endif ()

