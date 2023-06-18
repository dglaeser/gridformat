# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(DOXYGEN_IN ${DOXYGEN_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message(STATUS "Adding Doxygen documentation target")
    add_custom_target(doc_doxygen
                      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating API documentation with Doxygen"
                      VERBATIM)
elseif (DOXYGEN_FOUND)
    message(WARNING "Doxygen needs to be installed to generate the doxygen documentation")
endif ()
