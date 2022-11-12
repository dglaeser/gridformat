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
#include <cstdint>
#include <utility>
#include <vector>
#include <cassert>
#include <array>

#include <lzma.h>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/format.hpp>
#include <gridformat/common/traits.hpp>

#include <gridformat/compression/block_sizes.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

struct LZMAOptions {
    std::size_t block_size = 32768;
    std::uint32_t compression_level = 1;
};

class LZMA {
    using LZMAByte = std::uint8_t;

 public:
    using Options = LZMAOptions;

    explicit constexpr LZMA(Options opts = {})
    : _opts(std::move(opts))
    {}

    template<std::integral HeaderType = std::size_t>
    CompressedBlockSizes<HeaderType> compress(Serialization& serialization) const {
        static_assert(sizeof(typename Serialization::Byte) == sizeof(LZMAByte));
        const LZMAByte* data_in = reinterpret_cast<const LZMAByte*>(serialization.data());
        Serialization out(lzma_stream_buffer_bound(serialization.size()));
        LZMAByte* data_out = reinterpret_cast<LZMAByte*>(out.data());

        if (std::numeric_limits<HeaderType>::max() < serialization.size())
            throw TypeError("Chosen HeaderType is too small for given number of bytes");
        if (std::numeric_limits<HeaderType>::max() < _opts.block_size)
            throw TypeError("Chosen HeaderType is too small for given number of bytes");

        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        HeaderType size_in_bytes = static_cast<HeaderType>(serialization.size());
        BlockSizes<HeaderType> block_sizes{size_in_bytes, block_size};
        std::vector<LZMAByte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(lzma_stream_buffer_bound(_opts.block_size));
        compressed_block_sizes.reserve(block_sizes.num_blocks());

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;std::cout << "STAR"<<std::endl; std::cout << "SIB = " << size_in_bytes << std::endl;
        while (cur_in < size_in_bytes) {
            using std::min;
            const HeaderType cur_block_size = min(block_size, size_in_bytes - cur_in);
            std::cout << "BLOCKS = " << cur_block_size << std::endl;

            std::size_t out_pos = 0;
            assert(cur_in + cur_block_size <= size_in_bytes);
            const auto lzma_ret = lzma_easy_buffer_encode(
                _opts.compression_level, LZMA_CHECK_CRC32, nullptr,
                data_in + cur_in, cur_block_size,
                block_buffer.data(), &out_pos, block_buffer.capacity()
            );
            if (lzma_ret != LZMA_OK)
                throw InvalidState("Error upon compression with LZMA");

            std::cout << "OOUT = " << out_pos << std::endl;
            std::cout << "OS = " << out.size() << std::endl;
            std::cout << "OID = " << cur_out + out_pos << std::endl;
            if (cur_out + out_pos >= out.size())
                throw InvalidState("Hoo");
            std::copy_n(block_buffer.data(),
                        out_pos,
                        data_out + cur_out);
            cur_in += cur_block_size;
            cur_out += out_pos;
            compressed_block_sizes.push_back(static_cast<HeaderType>(out_pos));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(Format::as_error("(LZMACompressor) unexpected number of bytes processed"));

        // serialization.resize(cur_out);
        serialization = std::move(out);
        return {std::move(block_sizes), std::move(compressed_block_sizes)};
    }

 private:
    Options _opts;
};

#ifndef DOXYGEN_SKIP_DETAILS
namespace Detail {

struct LZMAAdapter {
    auto operator()(LZMAOptions opts = {}) const {
        return LZMA{std::move(opts)};
    }
};

}  // end namespace Detail
#endif  // DOXYGEN_SKIP_DETAILS

inline constexpr Detail::LZMAAdapter lzma_with;
inline constexpr LZMA lzma = LZMA{};

//! @} end Compression group

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_HAVE_LZMA
#endif  // GRIDFORMAT_COMPRESSION_LZMA_HPP_