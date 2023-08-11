// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <tuple>

#include <lz4.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/compression/common.hpp>
#include <gridformat/compression/decompress.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

//! Options for the lz4 compressor
struct LZ4Options {
    std::size_t block_size = default_block_size;
    int acceleration_factor = 1;  // LZ4_ACCELERATION_DEFAULT
};

//! Compressor using the lz4 compression library
class LZ4 {
    using LZ4Byte = char;
    static_assert(sizeof(typename Serialization::Byte) == sizeof(LZ4Byte));

    struct BlockDecompressor {
        using ByteType = LZ4Byte;

        void operator()(std::span<const ByteType> in, std::span<ByteType> out) const {
            int decompressed_length = LZ4_decompress_safe(
                in.data(),
                out.data(),
                static_cast<int>(in.size()),
                static_cast<int>(out.size())
            );
            if (decompressed_length != static_cast<int>(out.size()))
                throw IOError("(LZ4Compressor) Error upon block decompression");
        }
    };

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

        auto [blocks, out] = _compress<HeaderType>(in.template as_span_of<const LZ4Byte>());
        in = std::move(out);
        in.resize(blocks.compressed_size());
        return blocks;
    }

    template<typename HeaderType>
    static void decompress(Serialization& in, const CompressedBlocks<HeaderType>& blocks) {
        Compression::decompress(in, blocks, BlockDecompressor{});
    }

    static LZ4 with(Options opts) {
        return LZ4{std::move(opts)};
    }

 private:
    template<std::integral HeaderType>
    auto _compress(std::span<const LZ4Byte> in) const {
        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(in.size());
        Blocks<HeaderType> blocks{size_in_bytes, block_size};

        Serialization compressed;
        std::vector<LZ4Byte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(LZ4_COMPRESSBOUND(_opts.block_size));
        compressed_block_sizes.reserve(blocks.number_of_blocks);
        compressed.resize(block_buffer.capacity()*blocks.number_of_blocks);

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
        auto out = compressed.template as_span_of<LZ4Byte>();
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

            assert(cur_out + compressed_length <= out.size());
            std::copy_n(block_buffer.data(),
                        compressed_length,
                        out.data() + cur_out);
            cur_in += cur_block_size;
            cur_out += compressed_length;
            compressed_block_sizes.push_back(static_cast<HeaderType>(compressed_length));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(as_error("(LZ4Compressor) unexpected number of bytes processed"));

        return std::make_tuple(
            CompressedBlocks<HeaderType>{blocks, std::move(compressed_block_sizes)},
            compressed
        );
    }

    Options _opts;
};

inline constexpr LZ4 lz4;  //!< Instance of the lz4 compressor

#ifndef DOXYGEN
namespace Detail { inline constexpr bool _have_lz4 = true; }
#endif  // DOXYGEN

//! @} group Compression

}  // end namespace GridFormat::Compression

#else  // GRIDFORMAT_HAVE_LZ4

namespace GridFormat::Compression {

namespace Detail { inline constexpr bool _have_lz4 = false; }

class LZ4 {
 public:
    template<bool b = false, typename... Args>
    explicit LZ4(Args&&...) { static_assert(b, "LZ4 compressor requires the LZ4 library."); }
};

}  // namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_LZ4
#endif  // GRIDFORMAT_COMPRESSION_LZ4_HPP_
