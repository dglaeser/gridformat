// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Encoding
 * \brief Concepts related to data encoding
 */
#ifndef GRIDFORMAT_ENCODING_CONCEPTS_HPP_
#define GRIDFORMAT_ENCODING_CONCEPTS_HPP_

#include <cstddef>
#include <span>

#include <gridformat/common/concepts.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Encoding
//! \{

//! Encoders allow wrapping of an output stream, yielding
//! a stream that allows writing spans of data to it
template<typename T, typename S>
concept Encoder = requires(const T& encoder, S& stream) {
    { encoder(stream) } -> WriterFor<std::span<const std::byte>>;
};

//! \} group Encoding

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_ENCODING_CONCEPTS_HPP_
