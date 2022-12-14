// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \ingroup Compression
 * \brief Compressor using the LZ4 library.
 */
#ifndef GRIDFORMAT_COMPRESSION_LZ4_HPP_
#define GRIDFORMAT_COMPRESSION_LZ4_HPP_
#if GRIDFORMAT_HAVE_LZ4

#include <concepts>
#include <utility>
#include <vector>
#include <cassert>
#include <algorithm>

#include <lz4.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/compression/common.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

struct LZ4Options {
    std::size_t block_size = default_block_size;
    int acceleration_factor = 1;  // LZ4_ACCELERATION_DEFAULT
};

class LZ4 {
    using LZ4Byte = char;

 public:
    using Options = LZ4Options;

    explicit constexpr LZ4(Options opts = {})
    : _opts(std::move(opts))
    {}

    template<std::integral HeaderType = std::size_t>
    CompressedBlocks<HeaderType> compress(Serialization& in) const {
        static_assert(sizeof(typename Serialization::Byte) == sizeof(LZ4Byte));
        if (std::numeric_limits<HeaderType>::max() < in.size())
            throw TypeError("Chosen HeaderType is too small for given number of bytes");
        if (std::numeric_limits<HeaderType>::max() < _opts.block_size)
            throw TypeError("Chosen HeaderType is too small for given block size");

        Serialization out(LZ4_COMPRESSBOUND(in.size()));
        auto blocks = _compress<HeaderType>(
            in.template as_span_of<const LZ4Byte>(),
            out.template as_span_of<LZ4Byte>()
        );
        in = std::move(out);
        in.resize(blocks.compressed_size());
        return blocks;
    }

 private:
    template<std::integral HeaderType>
    CompressedBlocks<HeaderType> _compress(std::span<const LZ4Byte> in,
                                           std::span<LZ4Byte> out) const {
        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(in.size());
        Blocks<HeaderType> blocks{size_in_bytes, block_size};

        std::vector<LZ4Byte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(LZ4_COMPRESSBOUND(_opts.block_size));
        compressed_block_sizes.reserve(blocks.number_of_blocks);

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
        while (cur_in < size_in_bytes) {
            using std::min;
            const HeaderType cur_block_size = min(block_size, size_in_bytes - cur_in);
            assert(cur_in + cur_block_size <= size_in_bytes);

            const auto compressed_length = LZ4_compress_fast(
                in.data() + cur_in,        // const char* src
                block_buffer.data(),       // char* dst
                cur_block_size,            // src_size
                block_buffer.capacity(),   // dst_capacity
                _opts.acceleration_factor  // lz4 acc factor
            );
            if (compressed_length == 0)
                throw InvalidState(as_error("Error upon compression with LZ4"));

            assert(cur_out + compressed_length < out.size());
            std::copy_n(block_buffer.data(),
                        compressed_length,
                        out.data() + cur_out);
            cur_in += cur_block_size;
            cur_out += compressed_length;
            compressed_block_sizes.push_back(static_cast<HeaderType>(compressed_length));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(as_error("(LZ4Compressor) unexpected number of bytes processed"));

        return {blocks, std::move(compressed_block_sizes)};
    }

    Options _opts;
};

#ifndef DOXYGEN_SKIP_DETAILS
namespace Detail {

    struct LZ4Adapter {
        constexpr auto operator()(LZ4Options opts = {}) const {
            return LZ4{std::move(opts)};
        }
    };

}  // end namespace Detail
#endif  // DOXYGEN_SKIP_DETAILS

inline constexpr Detail::LZ4Adapter lz4_with;
inline constexpr LZ4 lz4 = lz4_with();

//! @} group Compression

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_LZ4
#endif  // GRIDFORMAT_COMPRESSION_LZ4_HPP_
