// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_STREAMS_HPP_
#define GRIDFORMAT_COMMON_STREAMS_HPP_

#include <ostream>

namespace GridFormat {

class OutputStream {
 public:
    explicit OutputStream(std::ostream& s)
    : _stream(s)
    {}

    void operator<<()

 private:
    std::ostream& _stream;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAMS_HPP_