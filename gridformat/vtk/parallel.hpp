// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Helper function for writing parallel VTK files
 */
#ifndef GRIDFORMAT_VTK_PARALLEL_HPP_
#define GRIDFORMAT_VTK_PARALLEL_HPP_

#include <string>

#include <gridformat/common/concepts.hpp>

namespace GridFormat::PVTK {

//! Return the piece filename (w/o extension) for the given rank
 std::string piece_basefilename(const std::string& par_filename, int rank) {
    const std::string base_name = par_filename.substr(0, par_filename.find_last_of("."));
    return base_name + "-" + std::to_string(rank);
}

}  // namespace GridFormat::PVTK

#endif  // GRIDFORMAT_VTK_PARALLEL_HPP_
