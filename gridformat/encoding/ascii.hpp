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

#include <gridformat/common/output_stream.hpp>

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
    std::string delimiter = "";
    std::string line_prefix = "";
    std::size_t entries_per_line = std::numeric_limits<std::size_t>::max();
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

class AsciiWithOptions {
 public:
    explicit AsciiWithOptions(AsciiFormatOptions opts)
    : _opts(std::move(opts))
    {}

    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return AsciiOutputStream{s, _opts};
    }

 public:
    AsciiFormatOptions _opts;
};

struct Ascii {
    template<typename S>
    constexpr auto operator()(S& s) const noexcept {
        return AsciiOutputStream{s};
    }

    auto with(AsciiFormatOptions opts) const {
        return AsciiWithOptions{std::move(opts)};
    }
};

inline constexpr Ascii ascii;

}  // namespace Encoding

//! \} group Encoding

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENCODING_ASCII_HPP_
