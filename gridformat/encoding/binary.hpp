// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_BINARY_HPP_
#define GRIDFORMAT_COMMON_ENCODING_BINARY_HPP_

#include <ostream>

#include <gridformat/common/concepts.hpp>

namespace GridFormat {

template<typename Stream = std::ostream>
class RawBinaryStream : public StreamWrapperBase<Stream> {
    using Byte = std::byte;
 public:
    explicit constexpr RawBinaryStream(Stream& s)
    : StreamWrapperBase<Stream>(s)
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) const {
        this->_write_raw(data);
    }
};

namespace Encoding {

struct RawBinary {
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return RawBinaryStream{s};
    }
};

inline constexpr RawBinary raw_binary;

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_BINARY_HPP_