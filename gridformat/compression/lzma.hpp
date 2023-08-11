// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <tuple>
#include <cstdint>

#include <lzma.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/compression/common.hpp>
#include <gridformat/compression/decompress.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

//! Options for the lzma compressor
struct LZMAOptions {
    std::size_t block_size = default_block_size;
    std::uint32_t compression_level = LZMA_PRESET_DEFAULT;
};

//! Compressor using the lzma library
class LZMA {
    using LZMAByte = std::uint8_t;
    static_assert(sizeof(typename Serialization::Byte) == sizeof(LZMAByte));

    struct BlockDecompressor {
        using ByteType = LZMAByte;

        void operator()(std::span<const ByteType> in, std::span<ByteType> out) const {
            size_t in_pos = 0;
            size_t out_pos = 0;
            uint64_t memlim = UINT64_MAX;
            if (lzma_stream_buffer_decode(
                &memlim,      // No memory limit
                uint32_t{0},  // Don't use any decoder flags
                nullptr, // Use default allocators (malloc/free)
                in.data(), &in_pos, in.size(),
                out.data(), &out_pos, out.size()) != LZMA_OK)
                throw IOError("(LZMACompressor) Error upon decompression");
        }
    };

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

        auto [blocks, out] = _compress<HeaderType>(in.template as_span_of<const LZMAByte>());
        in = std::move(out);
        in.resize(blocks.compressed_size());
        return blocks;
    }

    template<std::integral HeaderType>
    static void decompress(Serialization& in, const CompressedBlocks<HeaderType>& blocks) {
        Compression::decompress(in, blocks, BlockDecompressor{});
    }

    static LZMA with(Options opts) {
        return LZMA{std::move(opts)};
    }

 private:
    template<std::integral HeaderType>
    auto _compress(std::span<const LZMAByte> in) const {
        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(in.size());
        Blocks<HeaderType> blocks{size_in_bytes, block_size};

        Serialization compressed;
        std::vector<LZMAByte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(lzma_stream_buffer_bound(_opts.block_size));
        compressed_block_sizes.reserve(blocks.number_of_blocks);
        compressed.resize(block_buffer.capacity()*blocks.number_of_blocks);

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
        auto out = compressed.template as_span_of<LZMAByte>();
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

            assert(cur_out + out_pos <= out.size());
            std::copy_n(block_buffer.data(),
                        out_pos,
                        out.data() + cur_out);
            cur_in += cur_block_size;
            cur_out += out_pos;
            compressed_block_sizes.push_back(static_cast<HeaderType>(out_pos));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(as_error("(LZMACompressor) unexpected number of bytes processed"));

        return std::make_tuple(
            CompressedBlocks<HeaderType>{blocks, std::move(compressed_block_sizes)},
            compressed
        );
    }

    Options _opts;
};

inline constexpr LZMA lzma;  //!< Instance of the lzma compressor

#ifndef DOXYGEN
namespace Detail { inline constexpr bool _have_lzma = true; }
#endif  // DOXYGEN

//! @} group Compression

}  // end namespace GridFormat::Compression

#else  // GRIDFORMAT_HAVE_LZMA

namespace GridFormat::Compression {

namespace Detail { inline constexpr bool _have_lzma = false; }
class LZMA {
 public:
    template<bool b = false, typename... Args>
    explicit LZMA(Args&&...) { static_assert(b, "LZMA compressor requires the LZMA library."); }
};

}  // namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_LZMA
#endif  // GRIDFORMAT_COMPRESSION_LZMA_HPP_
