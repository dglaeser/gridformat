// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Compression
 * \brief Common classes used in the context of data compression
 */
#ifndef GRIDFORMAT_COMPRESSION_COMMON_HPP_
#define GRIDFORMAT_COMPRESSION_COMMON_HPP_

#include <vector>
#include <utility>
#include <numeric>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! \{

inline constexpr std::size_t default_block_size = (1 << 15);  //!< as in VTK (https://gitlab.kitware.com/vtk/vtk/-/blob/65fc526a83ac829628a9462f61fa57f1801e2c7e/IO/XML/vtkXMLWriterBase.cxx#L44)

//! Stores the block sizes used for compressing the given amount of bytes
template<std::integral HeaderType = std::size_t>
struct Blocks {
    const HeaderType block_size;
    const HeaderType residual_block_size;
    const HeaderType number_of_blocks;

    Blocks(HeaderType size_in_bytes, HeaderType block_size)
    : block_size{block_size}
    , residual_block_size{size_in_bytes%block_size}
    , number_of_blocks{residual_block_size ? size_in_bytes/block_size + 1 : size_in_bytes/block_size}
    {}
};

//! Stores the uncompressed/compressed block sizes after completion of a compression
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
        if (compressed_block_sizes.size() != number_of_blocks)
            throw SizeError("Mismatch between blocks and number of compressed blocks");
    }

    std::size_t compressed_size() const {
        return std::accumulate(
            compressed_block_sizes.begin(),
            compressed_block_sizes.end(),
            1
        );
    }
};

//! \} group Compression

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_COMPRESSION_COMMON_HPP_
