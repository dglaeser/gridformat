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

    template<std::integral HeaderType = std::size_t, typename T>
    auto compress(const T* data, std::size_t size) const {
        [[maybe_unused]] const LZMAByte* binary_data = reinterpret_cast<const LZMAByte*>(data);
        const std::size_t size_in_bytes = sizeof(T)*size;
        if (std::numeric_limits<HeaderType>::max() < size_in_bytes)
            throw TypeError("Chosen HeaderType is too small for given number of bytes");
        if (std::numeric_limits<HeaderType>::max() < _opts.block_size)
            throw TypeError("Chosen HeaderType is too small for given number of bytes");

        HeaderType block_size = static_cast<HeaderType>(_opts.block_size);
        BlockSizes<HeaderType> block_sizes{static_cast<HeaderType>(size_in_bytes), block_size};
        std::vector<LZMAByte> block_buffer;
        std::vector<HeaderType> compressed_block_sizes;
        block_buffer.reserve(lzma_stream_buffer_bound(_opts.block_size));
        compressed_block_sizes.reserve(block_sizes.num_blocks());

        HeaderType cur_in = 0;
        HeaderType cur_out = 0;
        std::vector<std::byte> compressed(lzma_stream_buffer_bound(size_in_bytes));
        LZMAByte* compressed_data = reinterpret_cast<LZMAByte*>(compressed.data());
        while (cur_in < size_in_bytes) {
            using std::min;
            const HeaderType cur_block_size = min(block_size, size_in_bytes - cur_in);

            std::size_t out_pos = 0;
            assert(cur_in + cur_block_size <= size_in_bytes);
            const auto lzma_ret = lzma_easy_buffer_encode(
                _opts.compression_level, LZMA_CHECK_CRC32, nullptr,
                binary_data + cur_in, cur_block_size,
                block_buffer.data(), &out_pos, block_buffer.capacity()
            );
            if (lzma_ret != LZMA_OK)
                throw InvalidState("Error upon compression with LZMA");

            std::copy_n(block_buffer.data(),
                        out_pos,
                        compressed_data + cur_out);
            cur_in += cur_block_size;
            cur_out += out_pos;
            compressed_block_sizes.push_back(static_cast<HeaderType>(out_pos));
        }

        if (cur_in != size_in_bytes)
            throw InvalidState(Format::as_error("(LZMACompressor) unexpected number of bytes processed"));

        compressed.resize(cur_out);
        return std::make_pair(
            std::move(compressed),
            CompressedBlockSizes<HeaderType>{std::move(block_sizes), std::move(compressed_block_sizes)}
        );
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