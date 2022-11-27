// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Compression
 * \brief Concepts related to data compression
 */
#ifndef GRIDFORMAT_COMPRESSION_CONCEPTS_HPP_
#define GRIDFORMAT_COMPRESSION_CONCEPTS_HPP_

#include <ranges>

#include <gridformat/common/serialization.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Compression
//! \{

template<typename T>
concept CompressedBlocks = requires(const T& t) {
    { t.block_size } -> std::convertible_to<std::size_t>;
    { t.residual_block_size } -> std::convertible_to<std::size_t>;
    { t.number_of_blocks } -> std::convertible_to<std::size_t>;
    { t.compressed_block_sizes } -> std::ranges::range;
    { *std::ranges::begin(t.compressed_block_sizes) } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept Compressor = requires(const T& t, Serialization& s) {
    { t.compress(s) } -> CompressedBlocks;
};

//! \} group Compression

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_COMPRESSION_CONCEPTS_HPP_
