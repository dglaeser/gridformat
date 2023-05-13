# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

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


add_custom_target(build_tests)
function(gridformat_set_test_props NAME)
    add_dependencies(build_tests ${NAME})
    set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE 255)
    set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
endfunction()


add_custom_target(memcheck)
find_program(VALGRIND_PATH valgrind)
function(gridformat_add_test_exe NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE gridformat::gridformat Boost::ut)
    target_compile_definitions(${NAME} PRIVATE GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS)

    # With ARGV2 callers can control if memcheck should be skipped
    if (VALGRIND_PATH AND ARGC LESS_EQUAL 2)
        set(MEMCHECK_TARGET ${NAME}_memcheck)
        add_custom_target(
            ${MEMCHECK_TARGET}
            COMMAND ${VALGRIND_PATH} --leak-check=yes --error-exitcode=1 $<TARGET_FILE:${NAME}>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_dependencies(memcheck ${MEMCHECK_TARGET})
    endif ()
endfunction()


function(gridformat_add_test NAME SOURCE)
    if (ARGC GREATER 2)
        gridformat_add_test_exe(${NAME} ${SOURCE} SKIP_MEMCHECK)
    else ()
        gridformat_add_test_exe(${NAME} ${SOURCE})
    endif ()
    add_test(NAME ${NAME} COMMAND "./${NAME}")
    gridformat_set_test_props(${NAME})
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
        gridformat_add_test(${NAME} ${DUMMY_MAIN} SKIP_MEMCHECK)
    endif ()
endfunction()


find_package(Python COMPONENTS Interpreter)
function(gridformat_add_regression_test NAME SOURCE REGEX)
    if (ARGC GREATER "3")
        set(SCRIPT_ARGS ${ARGV3})
    endif ()
    if (NOT Python_Interpreter_FOUND OR GRIDFORMAT_SKIP_REGRESSION)
        message(STATUS "Skipping regression testing for '${NAME}'")
        gridformat_add_test(${NAME} ${SOURCE})
    else ()
        gridformat_add_test_exe(${NAME} ${SOURCE})
        add_test(
            NAME ${NAME}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py  -c "./${NAME}" -r ${REGEX} ${SCRIPT_ARGS}
        )
        gridformat_set_test_props(${NAME})
    endif ()
endfunction()


find_package(MPI COMPONENTS CXX)
function(gridformat_add_parallel_regression_test NAME SOURCE NPROC REGEX)
    if (ARGC GREATER "4")
        set(SCRIPT_ARGS ${ARGV4})
    endif ()
    if (NOT MPI_FOUND)
        message(STATUS "Skipping parallel test '${NAME}' because MPI was not found")
        gridformat_add_test_if(MPI_FOUND ${NAME} ${SOURCE})
    elseif (GRIDFORMAT_SKIP_REGRESSION)
        message(STATUS "Skipping regression-testing '${NAME}'")
        gridformat_add_test_exe(${NAME} ${SOURCE} SKIP_MEMCHECK)
        add_test(NAME ${NAME} COMMAND ${MPIEXEC} -n ${NPROC} --oversubscribe ${NAME})
        gridformat_set_test_props(${NAME})
    else ()
        gridformat_add_test_exe(${NAME} ${SOURCE} SKIP_MEMCHECK)
        add_test(
            NAME ${NAME}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py -c "${MPIEXEC} -n ${NPROC} --oversubscribe ${NAME}" -r ${REGEX} ${SCRIPT_ARGS}
        )
        gridformat_set_test_props(${NAME})
    endif ()
endfunction()


function(gridformat_add_regression_test_if CONDITION NAME SOURCE REGEX)
    if (ARGC GREATER "4")
        set(SCRIPT_ARGS ${ARGV4})
    endif ()
    if (${CONDITION})
        gridformat_add_regression_test(${NAME} ${SOURCE} ${REGEX} ${SCRIPT_ARGS})
    else ()
        gridformat_add_test_if(${CONDITION} ${NAME} ${SOURCE})
    endif ()
endfunction()


function(gridformat_add_parallel_regression_test_if CONDITION NAME SOURCE NPROC REGEX)
    if (ARGC GREATER "5")
        set(SCRIPT_ARGS ${ARGV5})
    endif ()
    if (${CONDITION})
        gridformat_add_parallel_regression_test(${NAME} ${SOURCE} ${NPROC} ${REGEX} ${SCRIPT_ARGS})
    else ()
        gridformat_add_test_if(${CONDITION} ${NAME} ${SOURCE})
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
add_subdirectory(compression)
add_subdirectory(encoding)
add_subdirectory(grid)
add_subdirectory(vtk)
add_subdirectory(xml)
# add_subdirectory(api)
add_subdirectory(adapters)
