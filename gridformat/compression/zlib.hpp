// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \ingroup Compression
 * \brief Compressor using the ZLIB library.
 */
#ifndef GRIDFORMAT_COMPRESSION_ZLIB_HPP_
#define GRIDFORMAT_COMPRESSION_ZLIB_HPP_
#if GRIDFORMAT_HAVE_ZLIB

#include <concepts>
#include <utility>
#include <vector>
#include <cassert>
#include <algorithm>
#include <tuple>

#include <zlib.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/compression/common.hpp>
#include <gridformat/compression/decompress.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

//! Options for the zlib compressor
struct ZLIBOptions {
    std::size_t block_size = default_block_size;
    int compression_level = Z_DEFAULT_COMPRESSION;
};

//! Compressor using the zlib library
class ZLIB {
    using ZLIBByte = unsigned char;
    static_assert(sizeof(typename Serialization::Byte) == sizeof(ZLIBByte));

    struct BlockDecompressor {
        using ByteType = ZLIBByte;

        void operator()(std::span<const ByteType> in, std::span<ByteType> out) const {
            uLongf out_len = out.size();
            uLong in_len = in.size();
            if (uncompress(out.data(), &out_len, in.data(), in_len) != Z_OK)
                throw IOError("(ZLIBCompressor) Error upon decompression");
            if (out_len != out.size())
                throw IOError("(ZLIBCompressor) Unexpected decompressed size");
        }
    };

 public:
    using Options = ZLIBOptions;

    explicit constexpr ZLIB(Options opts = {})
    : _opts(std::move(opts))
    {}

    template<std::integral HeaderType = std::size_t>
    CompressedBlocks<HeaderType> compress(Serialization& in) const {
        if (std::numeric_limits<HeaderType>::max() < in.size())
            throw TypeError("Chosen HeaderType is too small for given number of bytes");
        if (std::numeric_limits<HeaderType>::max() < _opts.block_size)
            throw TypeError("Chosen HeaderType is too small for given block size");

        auto [blocks, out] = _compress<HeaderType>(in.template as_span_of<const ZLIBByte>());
        in = std::move(out);
        in.resize(blocks.compressed_size());
        return blocks;
    }

    template<std::integral HeaderType>
    static void decompress(Serialization& in, const CompressedBlocks<HeaderType>& blocks) {
        Compression::decompress(in, blocks, BlockDecompressor{});
    }

    static ZLIB with(Options opts) {
        return ZLIB{std::move(opts)};
    }

 private:
    template<std::integral HeaderType>
    auto _compress(std::span<const ZLIBByte> in) const {
        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(in.size());
        Blocks<HeaderType> blocks{size_in_bytes, block_size};

        Serialization compressed;
        std::vector<ZLIBByte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(compressBound(_opts.block_size));
        compressed_block_sizes.reserve(blocks.number_of_blocks);
        compressed.resize(block_buffer.capacity()*blocks.number_of_blocks);

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
        auto out = compressed.template as_span_of<ZLIBByte>();
        while (cur_in < size_in_bytes) {
            using std::min;
            const HeaderType cur_block_size = min(block_size, size_in_bytes - cur_in);
            assert(cur_in + cur_block_size <= size_in_bytes);

            uLongf out_len = block_buffer.capacity();
            uLong in_len = cur_block_size;
            if (compress2(block_buffer.data(), &out_len,
                          in.data() + cur_in, in_len,
                          _opts.compression_level) != Z_OK)
                throw InvalidState(as_error("Error upon compression with ZLib"));

            assert(cur_out + out_len <= out.size());
            std::copy_n(block_buffer.data(),
                        out_len,
                        out.data() + cur_out);
            cur_in += cur_block_size;
            cur_out += out_len;
            compressed_block_sizes.push_back(static_cast<HeaderType>(out_len));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(as_error("(ZLIBCompressor) unexpected number of bytes processed"));

        return std::make_tuple(
            CompressedBlocks<HeaderType>{blocks, std::move(compressed_block_sizes)},
            compressed
        );
    }

    Options _opts;
};

inline constexpr ZLIB zlib;  //!< Instance of the zlib compressor

#ifndef DOXYGEN
namespace Detail { inline constexpr bool _have_zlib = true; }
#endif  // DOXYGEN

//! @} group Compression

}  // end namespace GridFormat::Compression

#else  // GRIDFORMAT_HAVE_ZLIB

namespace GridFormat::Compression {
namespace Detail { inline constexpr bool _have_zlib = false; }
class ZLIB {
 public:
    template<bool b = false, typename... Args>
    explicit ZLIB(Args&&...) { static_assert(b, "ZLIB compressor requires the ZLIB library."); }
};

}  // namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_ZLIB
#endif  // GRIDFORMAT_COMPRESSION_ZLIB_HPP_
