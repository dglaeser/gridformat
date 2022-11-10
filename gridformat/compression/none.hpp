// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \ingroup Compression
 * \brief Compressor that does no compression.
 */
#ifndef GRIDFORMAT_COMPRESSION_NONE_HPP_
#define GRIDFORMAT_COMPRESSION_NONE_HPP_

#include <concepts>
#include <cstdint>
#include <vector>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/traits.hpp>

#include <gridformat/compression/block_sizes.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

struct None {
    template<std::integral HeaderType = std::size_t, Concepts::Serialization Bytes>
    CompressedBlockSizes<HeaderType> compress(Bytes& bytes) const {
        using T = ByteType<Bytes>;
        const HeaderType size_in_bytes = sizeof(T)*bytes.size();
        return {
            BlockSizes<HeaderType>{size_in_bytes, size_in_bytes},
            std::vector<HeaderType>{size_in_bytes}
        };
    }
};

inline constexpr None none;

//! @} end Compression group

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_COMPRESSION_NONE_HPP_