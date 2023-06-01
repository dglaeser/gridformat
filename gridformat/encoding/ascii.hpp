// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream using ascii
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
#define GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_

#include <cmath>
#include <limits>
#include <algorithm>
#include <optional>
#include <cstdint>

#include <gridformat/common/output_stream.hpp>
#include <gridformat/common/reserved_string.hpp>

namespace GridFormat {

//! \addtogroup Encoding
//! \{

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
    ReservedString<30> delimiter{""};
    ReservedString<30> line_prefix{""};
    std::size_t entries_per_line = std::numeric_limits<std::size_t>::max();

    friend bool operator==(const AsciiFormatOptions& a, const AsciiFormatOptions& b) {
        return a.delimiter == b.delimiter
            && a.line_prefix == b.line_prefix
            && a.entries_per_line == b.entries_per_line;
    }
};

template<typename OStream>
class AsciiOutputStream : public OutputStreamWrapperBase<OStream> {
 public:
    AsciiOutputStream(OStream& s, AsciiFormatOptions opts = {})
    : OutputStreamWrapperBase<OStream>(s)
    , _opts{std::move(opts)}
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        using std::min;
        std::size_t count = 0;
        while (count < data.size()) {
            const auto num_entries = min(_opts.entries_per_line, data.size() - count);
            this->_write_formatted(count > 0 ? "\n" : "");
            this->_write_formatted(_opts.line_prefix);
            _write(data.data() + count, num_entries);
            count += num_entries;
        }
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
    constexpr Ascii() = default;
    constexpr explicit Ascii(AsciiFormatOptions opts)
    : _opts{std::move(opts)}
    {}

    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return AsciiOutputStream{s, options()};
    }

    static constexpr auto with(AsciiFormatOptions opts) {
        return Ascii{std::move(opts)};
    }

    constexpr AsciiFormatOptions options() const {
        return _opts.value_or(AsciiFormatOptions{});
    }

 private:
    // we use optional here in order to be able to define
    // an inline constexpr instance of this class below
    std::optional<AsciiFormatOptions> _opts = {};
};

inline constexpr Ascii ascii;

}  // namespace Encoding

//! \} group Encoding

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
