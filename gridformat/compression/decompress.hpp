// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Compression
 * \brief Decompress compressed data.
 */
#ifndef GRIDFORMAT_COMPRESSION_DECOMPRESS_HPP_
#define GRIDFORMAT_COMPRESSION_DECOMPRESS_HPP_

#include <string>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>

#include <gridformat/compression/concepts.hpp>
#include <gridformat/compression/common.hpp>

namespace GridFormat::Compression {

/*!
 * \ingroup Compression
 * \brief Decompress compressed data.
 */
template<std::integral HeaderType, Concepts::BlockDecompressor Decompressor>
void decompress(Serialization& in,
                const CompressedBlocks<HeaderType>& blocks,
                const Decompressor& block_decompressor) {
    using Byte = typename Decompressor::ByteType;

    const auto last_block_size = blocks.residual_block_size > 0 ? blocks.residual_block_size : blocks.block_size;
    const auto out_size = blocks.block_size*(blocks.number_of_blocks-1) + last_block_size;

    std::size_t in_offset = 0;
    std::size_t out_offset = 0;
    Serialization out{out_size};

    const auto* in_data = in.template as_span_of<const Byte>().data();
    auto* out_data = out.template as_span_of<Byte>().data();
    std::ranges::for_each(std::views::iota(HeaderType{0}, blocks.number_of_blocks), [&] (HeaderType i) {
        const auto out_block_size = (i == blocks.number_of_blocks - 1) ? last_block_size : blocks.block_size;

        block_decompressor(
            std::span{in_data + in_offset, blocks.compressed_block_sizes[i]},
            std::span{out_data + out_offset, out_block_size}
        );

        in_offset += blocks.compressed_block_sizes[i];
        out_offset += out_block_size;
    });
    if (out_offset != out.size())
        throw SizeError(
            "Unexpected number of bytes written: " + std::to_string(out_offset) + " vs. " + std::to_string(out.size())
        );
    in = out;
}

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_COMPRESSION_DECOMPRESS_HPP_
