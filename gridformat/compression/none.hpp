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

#include <gridformat/common/serialization.hpp>
#include <gridformat/compression/common.hpp>

namespace GridFormat::Compression {

//! \addtogroup Compression
//! @{

struct None {
    template<std::integral HeaderType = std::size_t>
    CompressedBlocks<HeaderType> compress(Serialization& in) const {
        return {
            Blocks<HeaderType>{in.size(), in.size()},
            std::vector<HeaderType>{}
        };
    }
};

inline constexpr None none;

//! @} end Compression group

}  // end namespace GridFormat::Compression

#endif  // GRIDFORMAT_COMPRESSION_NONE_HPP_