// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \brief Base classes for grid data writers
 */
#ifndef GRIDFORMAT_GRID_WRITER_HPP_
#define GRIDFORMAT_GRID_WRITER_HPP_

#include <string>
#include <utility>
#include <ranges>
#include <fstream>
#include <ostream>
#include <concepts>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/range_field.hpp>
#include <gridformat/common/scalar_field.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/entity_fields.hpp>

namespace GridFormat {

//! \addtogroup Grid
//! \{

//! Base class for grid data writers, exposing the interfaces to add fields
template<typename Grid>
class GridWriterBase {
 public:
    using Field = typename FieldStorage::Field;

    explicit GridWriterBase(const Grid& grid)
    : _grid(grid)
    {}

    template<std::ranges::range R>
    void set_meta_data(const std::string& name, R&& range) {
        _meta_data.set(name, RangeField{std::forward<R>(range)});
    }

    template<Concepts::Scalar T>
    void set_meta_data(const std::string& name, T value) {
        _meta_data.set(name, ScalarField{value});
    }

    template<std::derived_from<Field> F>
    void set_meta_data(const std::string& name, F&& field) {
        static_assert(!std::is_lvalue_reference_v<F>, "Cannot take metadata fields by reference, please move");
        set_meta_data(name, std::make_shared<const F>(std::move(field)));
    }

    void set_meta_data(const std::string& name, std::shared_ptr<const Field> ptr) {
        _meta_data.set(name, ptr);
    }

    auto remove_meta_data(const std::string& name) {
        return _meta_data.pop(name);
    }

    template<Concepts::PointFunction<Grid> F, Concepts::Scalar T = GridDetail::PointFunctionScalarType<Grid, F>>
    void set_point_field(const std::string& name, F&& point_function, const Precision<T>& prec = {}) {
        static_assert(!std::is_lvalue_reference_v<F>, "Cannot take functions by reference, please move");
        set_point_field(name, _make_point_field(std::move(point_function), prec));
    }

    template<std::derived_from<Field> F>
    void set_point_field(const std::string& name, F&& field) {
        static_assert(!std::is_lvalue_reference_v<F>, "Cannot take fields by reference, please move or use shared_ptr");
        set_point_field(name, std::make_shared<const F>(std::forward<F>(field)));
    }

    void set_point_field(const std::string& name, std::shared_ptr<const Field> field_ptr) {
        _point_fields.set(name, field_ptr);
    }

    auto remove_point_field(const std::string& name) {
        return _point_fields.pop(name);
    }

    template<Concepts::CellFunction<Grid> F, Concepts::Scalar T = GridDetail::CellFunctionScalarType<Grid, F>>
    void set_cell_field(const std::string& name, F&& cell_function, const Precision<T>& prec = {}) {
        static_assert(!std::is_lvalue_reference_v<F>, "Cannot take functions by reference, please move");
        set_cell_field(name, _make_cell_field(std::move(cell_function), prec));
    }

    template<std::derived_from<Field> F>
    void set_cell_field(const std::string& name, F&& field) {
        static_assert(!std::is_lvalue_reference_v<F>, "Cannot take fields by reference, please move or use shared_ptr");
        set_cell_field(name, std::make_shared<const F>(std::forward<F>(field)));
    }

    void set_cell_field(const std::string& name, std::shared_ptr<const Field> field_ptr) {
        _cell_fields.set(name, field_ptr);
    }

    auto remove_cell_field(const std::string& name) {
        return _cell_fields.pop(name);
    }

    void clear() {
        _meta_data.clear();
        _point_fields.clear();
        _cell_fields.clear();
    }

    const Grid& grid() const {
        return _grid;
    }

    template<typename Writer>
    void copy_fields(Writer& w) const {
        for (const auto& [name, field_ptr] : meta_data_fields(*this))
            w.set_meta_data(name, field_ptr);
        for (const auto& [name, field_ptr] : point_fields(*this))
            w.set_point_field(name, field_ptr);
        for (const auto& [name, field_ptr] : cell_fields(*this))
            w.set_cell_field(name, field_ptr);
    }

    friend Concepts::RangeOf<std::pair<std::string, FieldPtr>> auto point_fields(const GridWriterBase& writer) {
        return writer._point_field_names() | std::views::transform([&] (std::string n) {
            auto field_ptr = writer._get_shared_point_field(n);
            return std::make_pair(std::move(n), std::move(field_ptr));
        });
    }

    friend Concepts::RangeOf<std::pair<std::string, FieldPtr>> auto cell_fields(const GridWriterBase& writer) {
        return writer._cell_field_names() | std::views::transform([&] (std::string n) {
            auto field_ptr = writer._get_shared_cell_field(n);
            return std::make_pair(std::move(n), std::move(field_ptr));
        });
    }

    friend Concepts::RangeOf<std::pair<std::string, FieldPtr>> auto meta_data_fields(const GridWriterBase& writer) {
        return writer._meta_data_field_names() | std::views::transform([&] (std::string n) {
            auto field_ptr = writer._get_shared_meta_data_field(n);
            return std::make_pair(std::move(n), std::move(field_ptr));
        });
    }

 protected:
    template<typename EntityFunction, Concepts::Scalar T>
    auto _make_point_field(EntityFunction&& f, const Precision<T>& prec) const {
        return PointField{_grid, std::move(f), prec};
    }

    template<typename EntityFunction, Concepts::Scalar T>
    auto _make_cell_field(EntityFunction&& f, const Precision<T>& prec) const {
        return CellField{_grid, std::move(f), prec};
    }

    std::ranges::range auto _point_field_names() const {
        return _point_fields.field_names();
    }

    std::ranges::range auto _cell_field_names() const {
        return _cell_fields.field_names();
    }

    const Field& _get_point_field(const std::string& name) const {
        return _point_fields.get(name);
    }

    std::shared_ptr<const Field> _get_shared_point_field(const std::string& name) const {
        return _point_fields.get_shared(name);
    }

    const Field& _get_cell_field(const std::string& name) const {
        return _cell_fields.get(name);
    }

    std::shared_ptr<const Field> _get_shared_cell_field(const std::string& name) const {
        return _cell_fields.get_shared(name);
    }

    std::ranges::range auto _meta_data_field_names() const {
        return _meta_data.field_names();
    }

    const Field& _get_meta_data_field(const std::string& name) const {
        return _meta_data.get(name);
    }

    std::shared_ptr<const Field> _get_shared_meta_data_field(const std::string& name) const {
        return _meta_data.get_shared(name);
    }

 private:
    const Grid& _grid;
    FieldStorage _point_fields;
    FieldStorage _cell_fields;
    FieldStorage _meta_data;
};

//! Abstract base class for writers of grid files
template<typename Grid>
class GridWriter : public GridWriterBase<Grid> {
 public:
    explicit GridWriter(const Grid& grid, std::string extension)
    : GridWriterBase<Grid>(grid)
    , _extension(std::move(extension))
    {}

    std::string write(const std::string& filename) const {
        std::string filename_with_ext = filename + _extension;
        _write(filename_with_ext);
        return filename_with_ext;
    }

    void write(std::ostream& s) const {
        _write(s);
    }

 private:
    std::string _extension;

    virtual void _write(const std::string& filename_with_ext) const {
        std::ofstream result_file(filename_with_ext, std::ios::out);
        _write(result_file);
    }

    virtual void _write(std::ostream&) const = 0;
};

//! Abstract base class for writers of time series
template<typename Grid>
class TimeSeriesGridWriter : public GridWriterBase<Grid> {
 public:
    explicit TimeSeriesGridWriter(const Grid& grid)
    : GridWriterBase<Grid>(grid)
    {}

    std::string write(double t) {
        return _write(t);
    }

 private:
    virtual std::string _write(double) = 0;
};

//! \} group Grid

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_WRITER_HPP_
