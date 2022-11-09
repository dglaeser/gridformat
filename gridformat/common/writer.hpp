// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_WRITER_HPP_
#define GRIDFORMAT_COMMON_WRITER_HPP_

#include <string>
#include <utility>
#include <ranges>
#include <fstream>
#include <ostream>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/range_field.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
class Writer {
    using FieldStorage = GridFormat::FieldStorage<>;

 public:
    using Field = typename FieldStorage::Field;

    template<Concepts::FieldValuesRange R, typename T = MDRangeScalar<std::decay_t<R>>>
    void set_point_field(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        set_point_field(name, RangeField{std::forward<R>(range), prec});
    }

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set_point_field(const std::string& name, F&& field) {
        _point_fields.set(name, std::forward<F>(field));
    }

    template<Concepts::FieldValuesRange R, typename T = MDRangeScalar<std::decay_t<R>>>
    void set_cell_field(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        set_cell_field(name, RangeField{std::forward<R>(range), prec});
    }

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set_cell_field(const std::string& name, F&& field) {
        _cell_fields.set(name, std::forward<F>(field));
    }

 protected:
    std::ranges::range auto _point_field_names() const {
        return _point_fields.field_names();
    }

    std::ranges::range auto _cell_field_names() const {
        return _cell_fields.field_names();
    }

    const Field& _get_point_field(const std::string& name) const {
        return _point_fields.get(name);
    }

    const Field& _get_cell_field(const std::string& name) const {
        return _cell_fields.get(name);
    }

 private:
    FieldStorage _point_fields;
    FieldStorage _cell_fields;
};

class WriterBase : public Writer {
 public:
    void write(const std::string& filename) const {
        std::ofstream result_file(filename, std::ios::out);
        write(result_file);
    }

    void write(std::ostream& s) const {
        _write(s);
    }

 private:
    virtual void _write(std::ostream& s) const = 0;
};

template<Concepts::Scalar Time = double>
class TimeSeriesWriterBase : public Writer {
 public:
    void write(const Time& t) const {
        _write(t);
    }

 private:
    virtual void _write(const Time& t) const = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_WRITER_HPP_