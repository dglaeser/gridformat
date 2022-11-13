// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
#define GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_

#include <algorithm>

#include <gridformat/common/stream.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Encoding::Detail {

template<typename T>
struct AsciiPrintType : std::type_identity<T> {};
template<std::signed_integral T>
struct AsciiPrintType<T> : std::type_identity<std::intmax_t> {};
template<std::unsigned_integral T>
struct AsciiPrintType<T> : std::type_identity<std::uintmax_t> {};

}  // namespace Encoding::Detail
#endif  // DOXYGEN

struct AsciiFormatOptions {
    std::string delimiter = " ";
    std::string line_prefix = "";
    std::size_t entries_per_line = 10;
};

template<typename Stream>
class AsciiStream : public StreamWrapperBase<Stream> {
 public:
    AsciiStream(Stream& s, AsciiFormatOptions opts = {})
    : StreamWrapperBase<Stream>(s)
    , _opts(std::move(opts))
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        std::size_t count = 0;
        while (count + _opts.entries_per_line < data.size()) {
            _write(data.data() + count, _opts.entries_per_line);
            count += _opts.entries_per_line;
            this->_write_formatted("\n");
        }
        _write(data.data() + count, data.size() - count);
    }

 private:
    template<typename T>
    void _write(const T* data, std::size_t num_entries) {
        using PrintType = typename Encoding::Detail::AsciiPrintType<T>::type;
        std::for_each_n(data, num_entries, [&] (const T& value) {
            this->_write_formatted(static_cast<PrintType>(value));
            this->_write_formatted(_opts.delimiter);
        });
    }

    AsciiFormatOptions _opts;
};

namespace Encoding {

struct Ascii {
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return AsciiStream{s};
    }

    template<typename S>
    constexpr auto operator()(S& s, AsciiFormatOptions opts) const noexcept {
        return AsciiStream{s, std::move(opts)};
    }
};

inline constexpr Ascii ascii;

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_