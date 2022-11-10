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