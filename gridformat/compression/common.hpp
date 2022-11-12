// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Compression
 * \brief TODO: Doc
 */
#ifndef GRIDFORMAT_COMPRESSION_COMMON_HPP_
#define GRIDFORMAT_COMPRESSION_COMMON_HPP_

#include <vector>
#include <utility>
#include <cassert>

#include <gridformat/common/serialization.hpp>

namespace GridFormat::Compression {

template<std::integral HeaderType = std::size_t>
struct Blocks {
    const HeaderType block_size;
    const HeaderType residual_block_size;
    const HeaderType number_of_blocks;

    Blocks(HeaderType size_in_bytes, HeaderType block_size)
    : block_size(block_size)
    , residual_block_size(size_in_bytes%block_size)
    , number_of_blocks(residual_block_size ? size_in_bytes/block_size + 1 : size_in_bytes/block_size)
    {}
};

template<std::integral HeaderType = std::size_t>
struct CompressedBlocks {
    const HeaderType block_size;
    const HeaderType residual_block_size;
    const HeaderType number_of_blocks;
    const std::vector<HeaderType> compressed_block_sizes;

    CompressedBlocks(const Blocks<HeaderType>& blocks,
                     std::vector<HeaderType>&& comp_block_sizes)
    : block_size{blocks.block_size}
    , residual_block_size{blocks.residual_block_size}
    , number_of_blocks{blocks.number_of_blocks}
    , compressed_block_sizes{std::move(comp_block_sizes)} {
        assert(compressed_block_sizes.size() == number_of_blocks);
    }
};

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_COMPRESSION_COMMON_HPP_