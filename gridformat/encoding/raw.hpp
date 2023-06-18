// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream for raw binary output
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_RAW_HPP_
#define GRIDFORMAT_COMMON_ENCODING_RAW_HPP_

#include <gridformat/common/output_stream.hpp>

namespace GridFormat {

//! \addtogroup Encoding
//! \{

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

namespace Encoding {

//! Raw binary encoder
struct RawBinary {
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return RawBinaryStream{s};
    }
};

inline constexpr RawBinary raw;  //!< Instance of the raw binary encoder

}  // namespace Encoding

//! \} group Encoding

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_RAW_HPP_
