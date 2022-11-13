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

template<Concepts::Stream Stream = std::ostream>
class RawBinaryStream {
    using Byte = std::byte;
 public:
    explicit RawBinaryStream(Stream& s)
    : _stream(s)
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) const {
        _stream.write(std::as_bytes(data));
    }

 private:
    Stream& _stream;
};

namespace Encoding {

struct RawBinary {
    template<Concepts::Stream S>
    Concepts::Stream auto operator()(S& s) const {
        return RawBinaryStream{s};
    }
};

inline constexpr RawBinary raw_binary;

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_BINARY_HPP_