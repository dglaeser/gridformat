# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

function(gridformat_have_dependency DEPENDENCY)
    get_target_property(_GFMT_IF_COMPILE_DEFS gridformat::gridformat INTERFACE_COMPILE_DEFINITIONS)
    foreach (_GFMT_IF_COMPILE_DEF IN LISTS _GFMT_IF_COMPILE_DEFS)
        if (_GFMT_IF_COMPILE_DEF MATCHES "GRIDFORMAT_HAVE_${DEPENDENCY}")
            set(GRIDFORMAT_HAVE_${DEPENDENCY} true PARENT_SCOPE)
        endif ()
    endforeach ()
endfunction()
