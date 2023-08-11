// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream for raw binary output
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_RAW_HPP_
#define GRIDFORMAT_COMMON_ENCODING_RAW_HPP_

#include <span>
#include <istream>

#include <gridformat/common/serialization.hpp>
#include <gridformat/common/output_stream.hpp>

namespace GridFormat {

//! \addtogroup Encoding
//! \{

//! For compatibility with Base64
struct RawDecoder {
    Serialization decode_from(std::istream& stream, std::size_t num_decoded_bytes) const {
        Serialization result{num_decoded_bytes};
        auto chars = result.template as_span_of<char>();
        stream.read(chars.data(), chars.size());
        if (stream.gcount() != static_cast<std::istream::pos_type>(chars.size()))
            throw IOError("Could not read the requested number of bytes from input stream");
        return result;
    }

    template<std::size_t s>
    std::size_t decode(std::span<char, s> chars) const {
        return chars.size();
    }
};

//! Wrapper around a given stream to write raw binary data
template<typename OStream>
class RawBinaryStream : public OutputStreamWrapperBase<OStream> {
    using Byte = std::byte;
 public:
    explicit constexpr RawBinaryStream(OStream& s)
    : OutputStreamWrapperBase<OStream>(s)
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        this->_write_raw(data);
    }
};

//! \} group Encoding

}  // namespace GridFormat

namespace GridFormat::Encoding {

//! \addtogroup Encoding
//! \{

//! Raw binary encoder
struct RawBinary {
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return RawBinaryStream{s};
    }
};

//! Instance of the raw binary encoder
inline constexpr RawBinary raw;

//! \} group Encoding

}  // namespace GridFormat::Encoding

#endif  // GRIDFORMAT_COMMON_ENCODING_RAW_HPP_
