// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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

struct RawBinary {
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return RawBinaryStream{s};
    }
};

inline constexpr RawBinary raw;

}  // namespace Encoding

//! \} group Encoding

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_RAW_HPP_