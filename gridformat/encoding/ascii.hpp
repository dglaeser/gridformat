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

template<Concepts::Stream Stream = std::ostream>
class AsciiStream {
    using Byte = char;
    static constexpr int chunk_size = 3;
    static constexpr unsigned char base64Table[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    // Top 6 bits of byte 0
    inline Byte _encodeSextet0(const Byte* data) const {
        return base64Table[((data[0] & 0b1111'1100) >> 2)];
    }
    // Bottom 2 bits of byte 0, Top 4 bits of byte 1
    inline Byte _encodeSextet1(const Byte* data) const {
        return base64Table[((data[0] & 0b0000'0011) << 4)
               | ((data[1] & 0b1111'0000) >> 4)];
    }
    // Bottom 4 bits of byte 1, Top 2 bits of byte 2
    inline Byte _encodeSextet2(const Byte* data) const {
        return base64Table[((data[1] & 0b0000'1111) << 2)
               | ((data[2] & 0b1100'0000) >> 6)];
    }
    // Bottom 6 bits of byte 2
    inline Byte _encodeSextet3(const Byte* data) const {
        return base64Table[(data[2] & 0b0011'1111)];
    }

    // flush data into an output stream
    void _flush_triplet(const Byte* data, int real_chunk_size) const {
        assert(real_chunk_size > 0);
        Byte out[4];
        out[0] = real_chunk_size > 0 ? _encodeSextet0(data) : '=';
        out[1] = real_chunk_size > 0 ? _encodeSextet1(data) : '=';
        out[2] = real_chunk_size > 1 ? _encodeSextet2(data) : '=';
        out[3] = real_chunk_size > 2 ? _encodeSextet3(data) : '=';
        s.write(out, 4);
    }

 public:
    explicit Base64Stream(Stream& s)
    : _stream(s)
    {}

    void write(const std::byte* data, std::streamsize size) const {
        std::size_t num_full_chunks = size/chunk_size;
        std::ranges::for_each(
            std::views::iota(std::size_t{0}, num_full_chunks),
            [&] (std::size_t chunk_idx) {
                _flush_triplet(
                    data + chunk_idx*chunk_size,
                    chunk_size,
                );
        });
        int last_chunk_size = size%chunk_size;
        if (last_chunk_size > 0)
            _flush_triplet(
                data + chunk_size*num_full_chunks,
                last_chunk_size
            );
    }

 private:
    Stream& _stream;
};

namespace Encoding {

struct Base64 {
    template<Concepts::Stream S>
    Base64Stream<S> operator()(S& s) const {
        return Base64Stream{s};
    }
};

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_