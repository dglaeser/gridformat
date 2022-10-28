// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_LOGGING_HPP_
#define GRIDFORMAT_COMMON_LOGGING_HPP_

#include <string_view>
#include <iostream>

#include <gridformat/common/format.hpp>

namespace GridFormat::Logging {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
void as_warning(std::string_view msg, std::ostream& s = std::cout) {
    s << "[GFMT] " << Format::as_warning("Warning") << ": " << msg << "\n";
}

}  // namespace GridFormat::Logging

#endif  // GRIDFORMAT_COMMON_LOGGING_HPP_