# SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
# SPDX-License-Identifier: MIT

# Include all public headers in two translation units linked into the same executable,
# which fails at link time if any header defines a non-inline function or variable.
# use the headers of the installed package if the test suite is run against it
if (gridformat_INCLUDE_DIRS)
    set(GRIDFORMAT_HEADER_ROOT ${gridformat_INCLUDE_DIRS})
else ()
    set(GRIDFORMAT_HEADER_ROOT ${gridformat_SOURCE_DIR})
endif ()
file(GLOB_RECURSE GRIDFORMAT_PUBLIC_HEADERS
    RELATIVE ${GRIDFORMAT_HEADER_ROOT}
    CONFIGURE_DEPENDS ${GRIDFORMAT_HEADER_ROOT}/gridformat/*.hpp
)
list(FILTER GRIDFORMAT_PUBLIC_HEADERS EXCLUDE REGEX "^gridformat/traits/")
if (NOT GRIDFORMAT_PUBLIC_HEADERS)
    message(FATAL_ERROR "No GridFormat headers found in '${GRIDFORMAT_HEADER_ROOT}'")
endif ()
list(SORT GRIDFORMAT_PUBLIC_HEADERS)
set(INCLUDES "")
foreach (header ${GRIDFORMAT_PUBLIC_HEADERS})
    string(APPEND INCLUDES "#include <${header}>\n")
endforeach ()
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/all_headers.hpp CONTENT "${INCLUDES}")

gridformat_add_test(test_multiple_translation_units test_multiple_translation_units.cpp)
target_sources(test_multiple_translation_units PRIVATE other_translation_unit.cpp)
target_include_directories(test_multiple_translation_units PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
