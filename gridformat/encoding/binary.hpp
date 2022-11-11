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

    template<typename T>
    void write(const T* data, std::streamsize size) const {
        const std::byte* binary_data = reinterpret_cast<const std::byte*>(data);
        const std::size_t binary_size = size*sizeof(T);
        _stream.write(binary_data, binary_size);
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