# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

include(FetchContent)
FetchContent_Declare(
  ut
  GIT_REPOSITORY https://github.com/boost-ext/ut
  GIT_TAG v1.1.9
)
FetchContent_MakeAvailable(ut)

option(GRIDFORMAT_DO_COVERAGE OFF "Turn on to add coverage-specific compiler flags & targets")
set(GRIDFORMAT_SKIP_REGRESSION ${GRIDFORMAT_DO_COVERAGE})

# make sure ut headers are not installed alongside ours
if (IS_DIRECTORY "${ut_SOURCE_DIR}")
    set_property(DIRECTORY ${ut_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
endif ()

find_package(Python COMPONENTS Interpreter)
if (Python_FOUND)
    message(STATUS "Preparing test vtk files")
    execute_process(
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/make_test_files.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ECHO STDOUT
    )
endif ()

add_custom_target(build_tests)
function(gridformat_set_test_props NAME)
    add_dependencies(build_tests ${NAME})
    set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE 42)
    set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
endfunction()


add_custom_target(memcheck)
find_program(VALGRIND_PATH valgrind)
function(gridformat_add_test_exe NAME SOURCE)
    cmake_parse_arguments(EXTRA_ARGS "SKIP_MEMCHECK" "" "" ${ARGN})
    add_executable(${NAME} ${SOURCE})
    # we omit the PRIVATE scope keyword to be compatible with dealii_setup_target()
    # which we use for testing the predefined deal.ii adapters
    target_link_libraries(${NAME} gridformat::gridformat Boost::ut)
    target_compile_definitions(${NAME} PRIVATE GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS)
    target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wpedantic)

    if (VALGRIND_PATH AND NOT EXTRA_ARGS_SKIP_MEMCHECK)
        set(MEMCHECK_TARGET ${NAME}_memcheck)
        add_custom_target(
            ${MEMCHECK_TARGET}
            COMMAND ${VALGRIND_PATH} --leak-check=yes --error-exitcode=1 $<TARGET_FILE:${NAME}>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_dependencies(memcheck ${MEMCHECK_TARGET})
    elseif (VALGRIND_PATH AND EXTRA_ARGS_SKIP_MEMCHECK)
        message(STATUS "Skipping memcheck target for '${NAME}'")
    endif ()
endfunction()


function(gridformat_add_test NAME SOURCE)
    gridformat_add_test_exe(${NAME} ${SOURCE} ${ARGN})
    add_test(NAME ${NAME} COMMAND "./${NAME}")
    gridformat_set_test_props(${NAME})
endfunction()


function(gridformat_add_test_if CONDITION NAME SOURCE)
    if (${CONDITION})
        gridformat_add_test(${NAME} ${SOURCE} ${ARGN})
    else ()
        message(STATUS "Skipping '${NAME}' because '${CONDITION}' evaluated to false")
        set(DUMMY_MAIN "${CMAKE_CURRENT_BINARY_DIR}/test_dummy_${NAME}.cpp")
        file(WRITE ${DUMMY_MAIN}
            "#include <iostream>\n\n"
            "int main()\n"
            "{\n"
            "    std::cout << \"Test is skipped due to unmet condition '${CONDITION}'\" << std::endl;\n"
            "    return 42;\n"
            "}"
        )
        gridformat_add_test(${NAME} ${DUMMY_MAIN} ${ARGN} SKIP_MEMCHECK)
    endif ()
endfunction()


function(gridformat_add_regression_test NAME SOURCE REGEX)
    cmake_parse_arguments(EXTRA_ARGS "" "SCRIPT_ARGS" "" "${ARGN}")
    if (NOT Python_Interpreter_FOUND OR GRIDFORMAT_SKIP_REGRESSION)
        message(STATUS "Skipping regression testing for '${NAME}'")
        gridformat_add_test(${NAME} ${SOURCE} ${ARGN})
    else ()
        gridformat_add_test_exe(${NAME} ${SOURCE} ${ARGN})
        add_test(
            NAME ${NAME}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py  -c "./${NAME}" -r ${REGEX} ${EXTRA_ARGS_SCRIPT_ARGS}
        )
        gridformat_set_test_props(${NAME})
    endif ()
endfunction()


find_package(MPI COMPONENTS CXX)
function(gridformat_add_parallel_test NAME SOURCE NPROCS)
    if (MPI_FOUND)
        # we skip memcheck because issues from mpi always arise
        gridformat_add_test_exe(${NAME} ${SOURCE} SKIP_MEMCHECK)
        add_test(NAME ${NAME} COMMAND ${MPIEXEC} -n ${NPROCS} --oversubscribe ${NAME})
        gridformat_set_test_props(${NAME})
    else ()
        gridformat_add_test_if(MPI_FOUND ${NAME} ${SOURCE} ${ARGN})
    endif ()
endfunction()


function(gridformat_add_parallel_test_if CONDITION NAME SOURCE NPROCS)
    if (MPI_FOUND AND ${CONDITION})
        gridformat_add_parallel_test(${NAME} ${SOURCE} ${NPROCS} ${ARGN})
    else ()
        gridformat_add_test_if(false ${NAME} ${SOURCE} ${ARGN})
    endif ()
endfunction()


function(gridformat_add_parallel_regression_test NAME SOURCE NPROC REGEX)
    cmake_parse_arguments(EXTRA_ARGS "" "SCRIPT_ARGS" "" "${ARGN}")
    if (NOT MPI_FOUND)
        message(STATUS "Skipping parallel test '${NAME}' because MPI was not found")
        gridformat_add_test_if(MPI_FOUND ${NAME} ${SOURCE} ${ARGN})
    elseif (GRIDFORMAT_SKIP_REGRESSION)
        message(STATUS "Skipping regression-testing '${NAME}'")
        gridformat_add_parallel_test(${NAME} ${SOURCE} ${NPROC})
    else ()
        gridformat_add_test_exe(${NAME} ${SOURCE} ${ARGN} SKIP_MEMCHECK)
        add_test(
            NAME ${NAME}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py -c "${MPIEXEC} -n ${NPROC} --oversubscribe ${NAME}" -r ${REGEX} ${EXTRA_ARGS_SCRIPT_ARGS}
        )
        gridformat_set_test_props(${NAME})
    endif ()
endfunction()


function(gridformat_add_regression_test_if CONDITION NAME SOURCE REGEX)
    if (${CONDITION})
        gridformat_add_regression_test(${NAME} ${SOURCE} ${REGEX} ${ARGN})
    else ()
        gridformat_add_test_if(${CONDITION} ${NAME} ${SOURCE} ${ARGN})
    endif ()
endfunction()


function(gridformat_add_parallel_regression_test_if CONDITION NAME SOURCE NPROC REGEX)
    cmake_parse_arguments(EXTRA_ARGS "" "SCRIPT_ARGS;SKIP_MEMCHECK" "" "${ARGN}")
    if (${CONDITION})
        gridformat_add_parallel_regression_test(${NAME} ${SOURCE} ${NPROC} ${REGEX} ${ARGN})
    else ()
        gridformat_add_test_if(${CONDITION} ${NAME} ${SOURCE} ${ARGN})
    endif ()
endfunction()

# handle coverage request
if (GRIDFORMAT_DO_COVERAGE)
    include(GridFormatCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_html(
        NAME test_coverage
        EXECUTABLE ctest
        DEPENDENCIES build_tests
        EXCLUDE test/ deps/ examples/ build/
    )
endif ()

add_subdirectory(common)
add_subdirectory(parallel)
add_subdirectory(compression)
add_subdirectory(encoding)
add_subdirectory(grid)
add_subdirectory(vtk)
add_subdirectory(xml)
add_subdirectory(api)
add_subdirectory(adapters)
