# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

function(gridformat_have_feature FEATURE OUTPUT_VARIABLE)
    get_target_property(_GFMT_IF_COMPILE_DEFS gridformat::gridformat INTERFACE_COMPILE_DEFINITIONS)
    foreach (_GFMT_IF_COMPILE_DEF IN LISTS _GFMT_IF_COMPILE_DEFS)
        if (_GFMT_IF_COMPILE_DEF MATCHES "GRIDFORMAT_HAVE_${FEATURE}")
            set(${OUTPUT_VARIABLE} true PARENT_SCOPE)
        endif ()
    endforeach ()
endfunction()
