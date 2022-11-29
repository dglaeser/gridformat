cmake_minimum_required(VERSION 3.25)
project(gridformat_examples)

include(FetchContent)
FetchContent_Declare(
  gridformat
  GIT_REPOSITORY https://github.com/dglaeser/gridformat
  GIT_TAG main
)
FetchContent_MakeAvailable(gridformat)

enable_testing()
function(gridformat_add_example NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PUBLIC gridformat::gridformat)
    add_test(NAME ${NAME} COMMAND ./${NAME})
endfunction()

gridformat_add_example(example_unstructured example_unstructured.cpp)
