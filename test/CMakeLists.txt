# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

include(FetchContent)
FetchContent_Declare(
  ut
  GIT_REPOSITORY https://github.com/boost-ext/ut
  GIT_TAG v1.1.9
)
FetchContent_MakeAvailable(ut)

# make sure ut headers are not installed alongside ours
if (IS_DIRECTORY "${ut_SOURCE_DIR}")
    set_property(DIRECTORY ${ut_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
endif ()

add_custom_target(build_tests)
function(gridformat_add_test_exe NAME SOURCE CMD)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE GridFormat::gridformat Boost::ut)
    add_test(NAME ${NAME} COMMAND ${CMD})
    add_dependencies(build_tests ${NAME})
    set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE 255)
    set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
endfunction()

function(gridformat_add_test NAME SOURCE)
    gridformat_add_test_exe(${NAME} ${SOURCE} "./${NAME}")
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
        gridformat_add_test_exe(${NAME} ${DUMMY_MAIN} "./${NAME}")
    endif ()
endfunction()

find_package(Python COMPONENTS Interpreter)
function(gridformat_add_regression_test NAME SOURCE REGEX)
    if (NOT ${Python_Interpreter_FOUND})
        message(STATUS "Skipping regression testing for '${NAME}' because Python was not found")
        gridformat_add_test(${NAME} ${SOURCE})
    else ()
        gridformat_add_test(
            ${NAME}
            ${SOURCE}
            "${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py -c \"./${NAME}\" -r ${REGEX}"
        )
    endif ()
endfunction()

find_package(MPI COMPONENTS CXX)
function(gridformat_add_parallel_regression_test NAME SOURCE NPROC REGEX)
    if (NOT ${MPI_FOUND})
        message(STATUS "Skipping parallel test '${NAME}' because MPI was not found")
        gridformat_add_test_if(MPI_FOUND ${NAME} ${SOURCE})
    else ()
        gridformat_add_test(
            ${NAME}
            ${SOURCE}
            "${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py -c \"${MPIEXEC} -n ${NPROC} --oversubscribe ${NAME}\" -r ${REGEX}"
        )
    endif ()
endfunction()

add_subdirectory(common)
add_subdirectory(compression)
add_subdirectory(encoding)
add_subdirectory(grid)
add_subdirectory(vtk)
add_subdirectory(xml)
