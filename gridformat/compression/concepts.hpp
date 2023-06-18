// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Compression
 * \brief Concepts related to data compression
 */
#ifndef GRIDFORMAT_COMPRESSION_CONCEPTS_HPP_
#define GRIDFORMAT_COMPRESSION_CONCEPTS_HPP_

#include <ranges>
#include <type_traits>

#include <gridformat/common/serialization.hpp>
#include <gridformat/compression/common.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Compression
//! \{

#ifndef DOXYGEN
namespace CompressionDetail {

    template<typename T>
    struct IsCompressedBlocks : public std::false_type {};
    template<typename Header>
    struct IsCompressedBlocks<Compression::CompressedBlocks<Header>> : public std::true_type {};

    template<typename T>
    concept ValidCompressionResult = IsCompressedBlocks<T>::value;

}  // namespace CompressionDetail
#endif  // DOXYGEN

//! Concept that compressors must fulfill
template<typename T>
concept Compressor = requires(const T& t, Serialization& s) {
    { t.compress(s) } -> CompressionDetail::ValidCompressionResult;
};

//! \} group Compression

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_COMPRESSION_CONCEPTS_HPP_
