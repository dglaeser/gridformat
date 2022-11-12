// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
#define GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_

#include <ostream>
#include <iterator>
#include <algorithm>

#include <gridformat/common/concepts.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

template<typename T>
struct Print : std::type_identity<T> {};
template<std::signed_integral T>
struct Print<T> : std::type_identity<std::intmax_t> {};
template<std::unsigned_integral T>
struct Print<T> : std::type_identity<std::uintmax_t> {};

}  // namespace Detail
#endif  // DOXYGEN

struct AsciiFormatOptions {
    std::string delimiter = " ";
    std::string line_prefix = "";
    std::size_t num_entries_per_line = 10;
};

class AsciiStream {
 public:
    AsciiStream(std::ostream& s, AsciiFormatOptions opts = {})
    : _stream(s)
    , _opts(std::move(opts))
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) const {
        std::size_t count = 0;
        while (count + _opts.num_entries_per_line < size) {
            _write(data.data() + count, _opts.num_entries_per_line);
            count += _opts.num_entries_per_line;
            _stream << "\n";
        }
        _write(data.data() + count, size - count);
    }

 private:
    template<typename T>
    void _write(const T* data, std::size_t num_entries) const {
        using PrintType = typename Detail::Print<T>::type;
        std::copy_n(
            data,
            num_entries,
            std::ostream_iterator<PrintType>(_stream, _opts.delimiter.c_str())
        );
    }

    std::ostream& _stream;
    AsciiFormatOptions _opts;
};

namespace Encoding {

struct Ascii {
    template<Concepts::Stream S>
    Concepts::Stream auto operator()(S& s) const {
        return AsciiStream{s};
    }
};

inline constexpr Ascii ascii;

}  // namespace Encoding
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_