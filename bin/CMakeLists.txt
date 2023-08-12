# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

function(gridformat_add_app NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE gridformat::gridformat)
    target_compile_definitions(${NAME} PRIVATE GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS)
    target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wpedantic)
endfunction()

gridformat_add_app(gridformat-info info.cpp)
gridformat_add_app(gridformat-convert convert.cpp)
