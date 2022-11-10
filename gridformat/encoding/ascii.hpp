// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
#define GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_

#include <ostream>

#include <gridformat/common/concepts.hpp>

namespace GridFormat {

class AsciiStream {
 public:
    explicit AsciiStream(std::ostream& s)
    : _stream(s)
    {}

    template<typename T>
    void write(const T* data, std::streamsize size) const {
        std::copy_n(data, size, std::ostream_iterator<T>(_stream));
    }

 private:
    std::ostream& _stream;
};

namespace Encoding {

struct Ascii {
    template<Concepts::Stream S>
    Concepts::Stream auto operator()(S& s) const {
        return AsciiStream{s};
    }
};

inline constexpr Ascii ascii;

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_