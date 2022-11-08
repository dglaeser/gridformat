// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \ingroup Compression
 * \brief TODO: Doc
 */
#ifndef GRIDFORMAT_COMPRESSION_BLOCK_SIZES_HPP_
#define GRIDFORMAT_COMPRESSION_BLOCK_SIZES_HPP_

#include <vector>
#include <cassert>
#include <utility>
#include <algorithm>

namespace GridFormat::Compression {

/*!
 * \ingroup Common
 * \ingroup Compression
 * \brief TODO: Doc
 */
template<std::integral HeaderType = std::size_t>
class BlockSizes {
 public:
    BlockSizes(HeaderType size_in_bytes, HeaderType block_size)
    : _block_size(block_size)
    , _last_raw_block_size(size_in_bytes%block_size)
    , _num_blocks(_last_raw_block_size ? size_in_bytes/block_size + 1 : size_in_bytes/block_size)
    {}

    HeaderType num_blocks() const { return _num_blocks; }
    HeaderType block_size() const { return _block_size; }
    HeaderType residual_block_size() const { return _last_raw_block_size; }

 private:
    HeaderType _block_size;
    HeaderType _last_raw_block_size;
    HeaderType _num_blocks;
};

/*!
 * \ingroup Common
 * \ingroup Compression
 * \brief TODO: Doc
 */
template<std::integral HeaderType = std::size_t>
class CompressedBlockSizes {
 public:
    CompressedBlockSizes(BlockSizes<HeaderType>&& raw_sizes,
                         std::vector<HeaderType>&& compressed_block_sizes)
    : _raw_sizes(std::move(raw_sizes))
    , _compressed_block_sizes(std::move(compressed_block_sizes)) {
        assert(_compressed_block_sizes.size() == _raw_sizes.num_blocks());
        assert(
            _raw_sizes.residual_block_size() == 0
            || _compressed_block_sizes.back() <= _raw_sizes.residual_block_size()
        );
        assert(std::ranges::all_of(_compressed_block_sizes, [&] (HeaderType size) {
            return size <= _raw_sizes.block_size();
        }));
    }

    HeaderType num_blocks() const { return _raw_sizes.num_blocks(); }
    HeaderType block_size() const { return _raw_sizes.block_size(); }
    HeaderType residual_block_size() const { return _raw_sizes.residual_block_size(); }
    const auto& compressed_block_sizes() const { return _compressed_block_sizes; }

 private:
    BlockSizes<HeaderType> _raw_sizes;
    std::vector<HeaderType> _compressed_block_sizes;
};

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_COMPRESSION_BLOCK_SIZES_HPP_