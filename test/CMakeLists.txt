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
    target_compile_options(${NAME} PRIVATE "-fconcepts-diagnostics-depth=10")
    add_test(NAME ${NAME} COMMAND ./${NAME})
    add_dependencies(build_tests ${NAME})
endfunction()

add_subdirectory(common)
add_subdirectory(grid)
add_subdirectory(vtk)
add_subdirectory(xml)