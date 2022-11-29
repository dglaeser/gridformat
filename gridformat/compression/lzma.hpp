// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \ingroup Compression
 * \brief Compressor using the LZMA library.
 */
#ifndef GRIDFORMAT_COMPRESSION_LZMA_HPP_
#define GRIDFORMAT_COMPRESSION_LZMA_HPP_
#if GRIDFORMAT_HAVE_LZMA

#include <concepts>
#include <utility>
#include <vector>
#include <cassert>
#include <algorithm>

#include <lzma.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/compression/common.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

struct LZMAOptions {
    std::size_t block_size = default_block_size;
    std::uint32_t compression_level = LZMA_PRESET_DEFAULT;
};

class LZMA {
    using LZMAByte = std::uint8_t;

 public:
    using Options = LZMAOptions;

    explicit constexpr LZMA(Options opts = {})
    : _opts(std::move(opts))
    {}

    template<std::integral HeaderType = std::size_t>
    CompressedBlocks<HeaderType> compress(Serialization& in) const {
        static_assert(sizeof(typename Serialization::Byte) == sizeof(LZMAByte));
        if (std::numeric_limits<HeaderType>::max() < in.size())
            throw TypeError("Chosen HeaderType is too small for given number of bytes");
        if (std::numeric_limits<HeaderType>::max() < _opts.block_size)
            throw TypeError("Chosen HeaderType is too small for given block size");

        Serialization out(lzma_stream_buffer_bound(in.size()));
        auto blocks = _compress<HeaderType>(
            in.template as_span_of<const LZMAByte>(),
            out.template as_span_of<LZMAByte>()
        );
        in = std::move(out);
        in.resize(blocks.compressed_size());
        return blocks;
    }

 private:
    template<std::integral HeaderType>
    CompressedBlocks<HeaderType> _compress(std::span<const LZMAByte> in,
                                           std::span<LZMAByte> out) const {
        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(in.size());
        Blocks<HeaderType> blocks{size_in_bytes, block_size};

        std::vector<LZMAByte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(lzma_stream_buffer_bound(_opts.block_size));
        compressed_block_sizes.reserve(blocks.number_of_blocks);

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
        while (cur_in < size_in_bytes) {
            using std::min;
            const HeaderType cur_block_size = min(block_size, size_in_bytes - cur_in);
            assert(cur_in + cur_block_size <= size_in_bytes);

            std::size_t out_pos = 0;
            const auto lzma_ret = lzma_easy_buffer_encode(
                _opts.compression_level, LZMA_CHECK_CRC32, nullptr,
                in.data() + cur_in, cur_block_size,
                block_buffer.data(), &out_pos, block_buffer.capacity()
            );
            if (lzma_ret != LZMA_OK)
                throw InvalidState(as_error("(LZMACompressor) Error upon compression"));

            assert(cur_out + out_pos < out.size());
            std::copy_n(block_buffer.data(),
                        out_pos,
                        out.data() + cur_out);
            cur_in += cur_block_size;
            cur_out += out_pos;
            compressed_block_sizes.push_back(static_cast<HeaderType>(out_pos));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(as_error("(LZMACompressor) unexpected number of bytes processed"));

        return {blocks, std::move(compressed_block_sizes)};
    }

    Options _opts;
};

#ifndef DOXYGEN_SKIP_DETAILS
namespace Detail {

    struct LZMAAdapter {
        constexpr auto operator()(LZMAOptions opts = {}) const {
            return LZMA{std::move(opts)};
        }
    };

}  // end namespace Detail
#endif  // DOXYGEN_SKIP_DETAILS

inline constexpr Detail::LZMAAdapter lzma_with;
inline constexpr LZMA lzma = lzma_with();

//! @} group Compression

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_LZMA
#endif  // GRIDFORMAT_COMPRESSION_LZMA_HPP_
