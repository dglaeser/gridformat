// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream using base64
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_
#define GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_

#include <cassert>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/output_stream.hpp>

namespace GridFormat {

//! \addtogroup Encoding
//! \{

//! Wrapper around a given stream to write output encoded with base64
template<typename OStream>
class Base64Stream : public OutputStreamWrapperBase<OStream> {
    using Byte = char;
    static constexpr int buffer_size = 3;
    static_assert(sizeof(std::byte) == sizeof(Byte));

    static constexpr unsigned char base64_alphabet[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    // Top 6 bits of byte 0
    inline Byte _encodeSextet0(const Byte* buffer) const {
        return base64_alphabet[((buffer[0] & 0b1111'1100) >> 2)];
    }
    // Bottom 2 bits of byte 0, Top 4 bits of byte 1
    inline Byte _encodeSextet1(const Byte* buffer) const {
        return base64_alphabet[((buffer[0] & 0b0000'0011) << 4)
               | ((buffer[1] & 0b1111'0000) >> 4)];
    }
    // Bottom 4 bits of byte 1, Top 2 bits of byte 2
    inline Byte _encodeSextet2(const Byte* buffer) const {
        return base64_alphabet[((buffer[1] & 0b0000'1111) << 2)
               | ((buffer[2] & 0b1100'0000) >> 6)];
    }
    // Bottom 6 bits of byte 2
    inline Byte _encodeSextet3(const Byte* buffer) const {
        return base64_alphabet[(buffer[2] & 0b0011'1111)];
    }

 public:
    explicit Base64Stream(OStream& s)
    : OutputStreamWrapperBase<OStream>(s)
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        auto byte_span = std::as_bytes(data);
        const Byte* bytes = reinterpret_cast<const Byte*>(byte_span.data());
        _write(bytes, byte_span.size());
    }

 private:
    void _write(const Byte* data, std::size_t size) {
        const auto num_full_buffers = size/buffer_size;
        for (const auto i : std::views::iota(std::size_t{0}, num_full_buffers))
            _flush_full_buffer(data + i*buffer_size);

        const auto written_bytes = num_full_buffers*buffer_size;
        if (size > written_bytes)
            _flush_buffer(data + written_bytes, size - written_bytes);
    }

    void _flush_full_buffer(const Byte* data) {
        const Byte out[4] = {
            _encodeSextet0(data),
            _encodeSextet1(data),
            _encodeSextet2(data),
            _encodeSextet3(data)
        };
        this->_stream.write(std::span{out});
    }

    void _flush_buffer(const Byte* data, std::size_t num_bytes) {
        if (num_bytes == 0)
            return;
        if (num_bytes > buffer_size)
            throw InvalidState("Residual bytes larger than buffer size");

        const Byte out[4] = {
            _encodeSextet0(data),
            _encodeSextet1(data),
            num_bytes > 1 ? _encodeSextet2(data) : '=',
            num_bytes > 2 ? _encodeSextet3(data) : '='
        };
        this->_stream.write(std::span{out});
    }
};

namespace Encoding {

//! Base64 encoder
struct Base64 {
    //! Return a base64 stream
    template<typename Stream>
    constexpr auto operator()(Stream& s) const noexcept {
        return Base64Stream{s};
    }
};

inline constexpr Base64 base64;  //!< Instance of the base64 encoder

}  // namespace Encoding

//! \} group Encoding

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_
