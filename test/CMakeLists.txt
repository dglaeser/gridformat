# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

include(FetchContent)
FetchContent_Declare(
  ut
  GIT_REPOSITORY https://github.com/boost-ext/ut
  GIT_TAG v1.1.9
)
FetchContent_MakeAvailable(ut)

add_custom_target(build_tests)
set(GRIDFORMAT_SKIPPED_TEST_RETURN_CODE 255)
function(gridformat_add_test NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE ${GFMT_NAMESPACE}::${PROJECT_NAME} Boost::ut)
    target_compile_options(${NAME} PRIVATE -fconcepts-diagnostics-depth=10)
    add_test(NAME ${NAME} COMMAND ./${NAME})
    add_dependencies(build_tests ${NAME})
    set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE ${GRIDFORMAT_SKIPPED_TEST_RETURN_CODE})
    set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
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
            "    return ${GRIDFORMAT_SKIPPED_TEST_RETURN_CODE};\n"
            "}"
        )
        gridformat_add_test(${NAME} ${DUMMY_MAIN})
        set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE ${GRIDFORMAT_SKIPPED_TEST_RETURN_CODE})
        set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif ()
endfunction()

find_package(Python COMPONENTS Interpreter)
function(gridformat_add_regression_test NAME SOURCE REGEX)
    if (NOT ${Python_Interpreter_FOUND})
        message(STATUS "Skipping regression testing for '${NAME}' because Python was not found")
        gridformat_add_test(${NAME} ${SOURCE})
    else ()
        add_executable(${NAME} ${SOURCE})
        target_link_libraries(${NAME} PRIVATE ${GFMT_NAMESPACE}::${PROJECT_NAME} Boost::ut)
        target_compile_options(${NAME} PRIVATE -fconcepts-diagnostics-depth=10)
        add_dependencies(build_tests ${NAME})
        add_test(
            NAME ${NAME}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py -c "./${NAME}" -r ${REGEX}
        )
        set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE ${GRIDFORMAT_SKIPPED_TEST_RETURN_CODE})
        set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif ()
endfunction()

function(gridformat_add_parallel_regression_test NAME SOURCE NPROC REGEX)
    if (NOT ${MPI_FOUND})
        message(STATUS "Skipping parallel test '${NAME}' because MPI was not found")
        gridformat_add_test_if(MPI_FOUND ${NAME} ${SOURCE})
    else ()
        add_executable(${NAME} ${SOURCE})
        target_link_libraries(${NAME} PRIVATE ${GFMT_NAMESPACE}::${PROJECT_NAME} Boost::ut MPI::MPI_CXX)
        target_compile_options(${NAME} PRIVATE -fconcepts-diagnostics-depth=10)
        target_compile_definitions(${NAME} PRIVATE GRIDFORMAT_HAVE_MPI=1)
        add_dependencies(build_tests ${NAME})
        add_test(
            NAME ${NAME}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_regression_test.py -c "${MPIEXEC} -n ${NPROC} --oversubscribe ${NAME}" -r ${REGEX}
        )
        set_tests_properties(${NAME} PROPERTIES SKIP_RETURN_CODE ${GRIDFORMAT_SKIPPED_TEST_RETURN_CODE})
        set_target_properties(${NAME} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif ()
endfunction()

add_subdirectory(common)
add_subdirectory(compression)
add_subdirectory(encoding)
add_subdirectory(grid)
add_subdirectory(vtk)
add_subdirectory(xml)
