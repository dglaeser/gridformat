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

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/entity_fields.hpp>

namespace GridFormat {

//! \addtogroup Grid
//! \{

template<typename F, typename E> requires(Concepts::Detail::EntityFunction<F, E>)
using EntityFunctionValueType = GridDetail::EntityFunctionValueType<F, E>;

template<typename F, typename E>
using EntityFunctionScalar = FieldScalar<EntityFunctionValueType<F, E>>;

//! Base class for grid data writers
template<typename Grid>
class GridWriterBase {
    using FieldStorage = GridFormat::FieldStorage<>;

 public:
    using Field = typename FieldStorage::Field;

    explicit GridWriterBase(const Grid& grid)
    : _grid(grid)
    {}

    template<Concepts::PointFunction<Grid> F, Concepts::Scalar T = EntityFunctionScalar<F, Point<Grid>>>
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

    template<Concepts::CellFunction<Grid> F, Concepts::Scalar T = EntityFunctionScalar<F, Cell<Grid>>>
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

    void clear() {
        _point_fields.clear();
        _cell_fields.clear();
    }

    const Grid& grid() const {
        return _grid;
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

 private:
    const Grid& _grid;
    FieldStorage _point_fields;
    FieldStorage _cell_fields;
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
