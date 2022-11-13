// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_
#define GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_

#include <ostream>
#include <cassert>
#include <algorithm>

#include <gridformat/common/stream.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

template<typename Stream = std::ostream>
class Base64Stream : public StreamWrapperBase<Stream> {
    using Byte = char;
    using Buffer = std::array<Byte, 3>;

    static constexpr unsigned char base64Table[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    // Top 6 bits of byte 0
    inline Byte _encodeSextet0() const {
        return base64Table[((_buffer[0] & 0b1111'1100) >> 2)];
    }
    // Bottom 2 bits of byte 0, Top 4 bits of byte 1
    inline Byte _encodeSextet1() const {
        return base64Table[((_buffer[0] & 0b0000'0011) << 4)
               | ((_buffer[1] & 0b1111'0000) >> 4)];
    }
    // Bottom 4 bits of byte 1, Top 2 bits of byte 2
    inline Byte _encodeSextet2() const {
        return base64Table[((_buffer[1] & 0b0000'1111) << 2)
               | ((_buffer[2] & 0b1100'0000) >> 6)];
    }
    // Bottom 6 bits of byte 2
    inline Byte _encodeSextet3() const {
        return base64Table[(_buffer[2] & 0b0011'1111)];
    }

 public:
    explicit Base64Stream(Stream& s)
    : StreamWrapperBase<Stream>(s)
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        std::ranges::for_each(std::as_bytes(data), [&] (const std::byte& byte) {
            _insert(static_cast<Byte>(byte));
        });
        if (_num_buffered_bytes() > 0)
            _flush();
    }

 private:
    std::size_t _num_buffered_bytes() const {
        const auto const_it = static_cast<typename Buffer::const_iterator>(_it);
        return std::distance(_buffer.begin(), const_it);
    }

    void _insert(const Byte byte) {
        *_it = byte;
        if (++_it; _it == _buffer.end())
            _flush();
    }

    void _flush() {
        const auto num_bytes = std::distance(_buffer.begin(), _it);
        const Byte out[4] = {
            num_bytes > 0 ? _encodeSextet0() : '=',
            num_bytes > 0 ? _encodeSextet1() : '=',
            num_bytes > 1 ? _encodeSextet2() : '=',
            num_bytes > 2 ? _encodeSextet3() : '='
        };
        this->_stream.write(std::span{out});
        _reset_buffer();
    }

    void _reset_buffer() {
        std::ranges::fill(_buffer, 0);
        _it = _buffer.begin();
    }

    Buffer _buffer;
    typename Buffer::iterator _it = _buffer.begin();
};

namespace Encoding {

struct Base64 {
    template<typename Stream>
    constexpr auto operator()(Stream& s) const noexcept {
        return Base64Stream{s};
    }
};

inline constexpr Base64 base64;

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_