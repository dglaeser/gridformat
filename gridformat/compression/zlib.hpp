// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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

#include <zlib.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/compression/common.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

struct ZLIBOptions {
    std::size_t block_size = default_block_size;
    int compression_level = Z_DEFAULT_COMPRESSION;
};

class ZLIB {
    using ZLIBByte = unsigned char;

 public:
    using Options = ZLIBOptions;

    explicit constexpr ZLIB(Options opts = {})
    : _opts(std::move(opts))
    {}

    template<std::integral HeaderType = std::size_t>
    CompressedBlocks<HeaderType> compress(Serialization& in) const {
        static_assert(sizeof(typename Serialization::Byte) == sizeof(ZLIBByte));
        if (std::numeric_limits<HeaderType>::max() < in.size())
            throw TypeError("Chosen HeaderType is too small for given number of bytes");
        if (std::numeric_limits<HeaderType>::max() < _opts.block_size)
            throw TypeError("Chosen HeaderType is too small for given block size");

        Serialization out(compressBound(in.size()));
        auto blocks = _compress<HeaderType>(
            in.template as_span_of<const ZLIBByte>(),
            out.template as_span_of<ZLIBByte>()
        );
        in = std::move(out);
        in.resize(blocks.compressed_size());
        return blocks;
    }

 private:
    template<std::integral HeaderType>
    CompressedBlocks<HeaderType> _compress(std::span<const ZLIBByte> in,
                                           std::span<ZLIBByte> out) const {
        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(in.size());
        Blocks<HeaderType> blocks{size_in_bytes, block_size};

        std::vector<ZLIBByte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(compressBound(_opts.block_size));
        compressed_block_sizes.reserve(blocks.number_of_blocks);

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
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

            assert(cur_out + out_len < out.size());
            std::copy_n(block_buffer.data(),
                        out_len,
                        out.data() + cur_out);
            cur_in += cur_block_size;
            cur_out += out_len;
            compressed_block_sizes.push_back(static_cast<HeaderType>(out_len));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(as_error("(ZLIBCompressor) unexpected number of bytes processed"));

        return {blocks, std::move(compressed_block_sizes)};
    }

    Options _opts;
};

#ifndef DOXYGEN_SKIP_DETAILS
namespace Detail {

    struct ZLIBAdapter {
        constexpr auto operator()(ZLIBOptions opts = {}) const {
            return ZLIB{std::move(opts)};
        }
    };

}  // end namespace Detail
#endif  // DOXYGEN_SKIP_DETAILS

inline constexpr Detail::ZLIBAdapter zlib_with;
inline constexpr ZLIB zlib = zlib_with();

//! @} group Compression

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_ZLIB
#endif  // GRIDFORMAT_COMPRESSION_ZLIB_HPP_
