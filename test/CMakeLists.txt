include(FetchContent)
FetchContent_Declare(
  ut
  GIT_REPOSITORY https://github.com/boost-ext/ut
  GIT_TAG v1.1.9
)
FetchContent_MakeAvailable(ut)

add_custom_target(build_tests)
function(gridformat_add_test NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE ${GFMT_NAMESPACE}::${PROJECT_NAME} Boost::ut)
    target_compile_options(${NAME} PRIVATE -fconcepts-diagnostics-depth=10)
    add_test(NAME ${NAME} COMMAND ./${NAME})
    add_dependencies(build_tests ${NAME})
endfunction()

function(gridformat_add_test_if CONDITION NAME SOURCE)
    if (${CONDITION})
        gridformat_add_test(${NAME} ${SOURCE})
    else ()
        set(DUMMY_MAIN "${CMAKE_CURRENT_BINARY_DIR}/test_dummy_${NAME}.cpp")
        file(WRITE ${DUMMY_MAIN}
            "#include <iostream>\n\n"
            "int main()\n"
            "{\n"
            "    std::cout << \"Test is skipped due to unmet condition '${CONDITION}'\" << std::endl;\n"
            "    return 255;\n"
            "}"
        )
        gridformat_add_test(${NAME} ${DUMMY_MAIN})
        set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE 255)
    endif ()
endfunction()

add_subdirectory(common)
add_subdirectory(compression)
add_subdirectory(encoding)
add_subdirectory(grid)
add_subdirectory(vtk)
add_subdirectory(xml)
