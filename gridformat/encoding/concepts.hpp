// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Concepts related to data encoding.
 */
#ifndef GRIDFORMAT_ENCODING_CONCEPTS_HPP_
#define GRIDFORMAT_ENCODING_CONCEPTS_HPP_

#include <cstddef>
#include <istream>
#include <span>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/serialization.hpp>

namespace GridFormat::Concepts {

/*!
 * \ingroup Encoding
 * \brief Encoders allow wrapping of an output stream, yielding
 *        a stream that allows writing spans of data to it.
 */
template<typename T, typename S>
concept Encoder = requires(const T& encoder, S& stream) {
    { encoder(stream) } -> WriterFor<std::span<const std::byte>>;
};

/*!
 * \ingroup Encoding
 * \brief Decoders allow decoding spans of characters or directly from an input stream.
 */
template<typename T>
concept Decoder = requires(const T& decoder,
                           std::istream& input_stream,
                           std::span<char> characters) {
    { decoder.decode(characters) } -> std::convertible_to<std::size_t>;
    { decoder.decode_from(input_stream, std::size_t{}) } -> std::same_as<Serialization>;
};

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_ENCODING_CONCEPTS_HPP_
