// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper classes & wrappers around output streams
 */
#ifndef GRIDFORMAT_COMMON_STREAM_HPP_
#define GRIDFORMAT_COMMON_STREAM_HPP_

#include <span>
#include <limits>
#include <ostream>
#include <concepts>
#include <type_traits>

#include <gridformat/common/concepts.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Wrapper around an output stream to expose our required interface
 *        We require being able to write spans into output streams, and use
 *        our desired precision for formatted output of floating point values.
 */
class OutputStream {
 public:
    explicit OutputStream(std::ostream& s)
    : _stream{s}
    {}

    template<Concepts::StreamableWith<std::ostream> T>
        requires(not Concepts::Scalar<T>)
    OutputStream& operator<<(const T& t) {
        _stream << t;
        return *this;
    }

    template<Concepts::Scalar T>
    OutputStream& operator<<(const T& t) {
        auto orig_precision = _stream.precision();
        _stream.precision(std::numeric_limits<T>::digits10);
        _stream << t;
        _stream.precision(orig_precision);
        return *this;
    }

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        _write_chars(std::as_bytes(data));
    }

    template<std::size_t size>
    void write(std::span<const char, size> data) {
        _write_chars(data.data(), data.size());
    }

 private:
    template<typename Byte, std::size_t size>
    void _write_chars(std::span<const Byte, size> data) {
        static_assert(sizeof(Byte) == sizeof(char));
        const char* chars = reinterpret_cast<const char*>(data.data());
        _stream.write(chars, data.size());
    }

    void _write_chars(const char* chars, std::size_t size) {
        _stream.write(chars, size);
    }

    std::ostream& _stream;
};

#ifndef DOXYGEN
namespace Detail {

    template<typename T> struct OutputStreamStorage : std::type_identity<T&> {};
    template<std::derived_from<std::ostream> T> struct OutputStreamStorage<T> : std::type_identity<OutputStream> {};

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Base class for wrappers around output streams.
 *        Makes it possible to wrap both instances of OutputStream or std::ostream.
 */
template<typename OStream>
class OutputStreamWrapperBase {
    using Storage = typename Detail::OutputStreamStorage<OStream>::type;
    using Stream = std::remove_cvref_t<Storage>;

 public:
    template<typename S>
        requires(std::constructible_from<Storage, S>)
    explicit OutputStreamWrapperBase(S&& s)
    : _stream{std::forward<S>(s)}
    {}

 protected:
    template<Concepts::StreamableWith<Stream> T>
    void _write_formatted(const T& t) {
        _stream << t;
    }

    template<typename T, std::size_t size>
    void _write_raw(std::span<T, size> data) {
        _stream.write(data);
    }

    Storage _stream;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STREAM_HPP_
