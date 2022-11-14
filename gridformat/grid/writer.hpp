// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
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

#include <gridformat/grid/grid.hpp>

namespace GridFormat {
namespace Concepts {

template<typename T, typename Entity>
concept EntityFunction = std::invocable<T, const std::decay_t<Entity>&>;

template<typename T, typename Grid>
concept PointFunction = EntityFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept CellFunction = EntityFunction<T, Cell<Grid>>;

template<typename T, typename Grid>
concept GridEntityFunction = PointFunction<T, Grid> or CellFunction<T, Grid>;

template<typename T, typename Entity>
concept ScalarFunction = Scalar<std::invoke_result_t<T, const std::decay_t<Entity>&>>;

template<typename T, typename Entity>
concept VectorFunction = Vector<std::invoke_result_t<T, const std::decay_t<Entity>&>>;

template<typename T, typename Entity>
concept TensorFunction = Tensor<std::invoke_result_t<T, const std::decay_t<Entity>&>>;

template<typename T, typename Grid>
concept ScalarPointFunction = PointFunction<T, Grid> and ScalarFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept ScalarCellFunction = CellFunction<T, Grid> and ScalarFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept VectorPointFunction = PointFunction<T, Grid> and VectorFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept VectorCellFunction = CellFunction<T, Grid> and VectorFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept TensorPointFunction = PointFunction<T, Grid> and TensorFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept TensorCellFunction = CellFunction<T, Grid> and TensorFunction<T, Point<Grid>>;

}  // namespace Concepts

template<typename F, typename E> requires(Concepts::EntityFunction<F, E>)
using EntityFunctionResult = std::decay_t<std::invoke_result_t<F, const E&>>;

template<typename F, typename E>
using EntityFunctionScalar = FieldScalar<EntityFunctionResult<F, E>>;

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
template<typename Grid>
class GridWriter {
    using FieldStorage = GridFormat::FieldStorage<>;

 public:
    using Field = typename FieldStorage::Field;

    explicit GridWriter(const Grid& grid)
    : _grid(grid)
    {}

    template<Concepts::PointFunction<Grid> F, Concepts::Scalar T = EntityFunctionScalar<F, Point<Grid>>>
    requires(!std::is_lvalue_reference_v<F>)
    void set_point_field(const std::string& name, F&& point_function, const Precision<T>& prec = {}) {
        set_point_field(name, _make_entity_field(std::move(point_function), points(_grid), prec));
    }

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set_point_field(const std::string& name, F&& field) {
        _point_fields.set(name, std::forward<F>(field));
    }

    template<Concepts::CellFunction<Grid> F, Concepts::Scalar T = EntityFunctionScalar<F, Cell<Grid>>>
    requires(!std::is_lvalue_reference_v<F>)
    void set_cell_field(const std::string& name, F&& cell_function, const Precision<T>& prec = {}) {
        set_cell_field(name, _make_entity_field(std::move(cell_function), cells(_grid), prec));
    }

    template<std::derived_from<Field> F> requires(!std::is_lvalue_reference_v<F>)
    void set_cell_field(const std::string& name, F&& field) {
        _cell_fields.set(name, std::forward<F>(field));
    }

 protected:
    template<Concepts::GridEntityFunction<Grid> F,
             std::ranges::range EntityRange,
             Concepts::Scalar T>
    auto _make_entity_field(F&& f, EntityRange&& entities, const Precision<T>& prec) const {
        return _make_entity_field(_make_entity_function_range(std::move(f), std::move(entities)), prec);
    }

    template<std::ranges::forward_range R, Concepts::Scalar T>
    auto _make_entity_field(R&& r, const Precision<T>& prec) const {
        return RangeField{std::move(r), prec};
    }

    template<Concepts::GridEntityFunction<Grid> F, std::ranges::range EntityRange>
    auto _make_entity_function_range(F&& f, EntityRange&& entities) const {
        using Entity = std::ranges::range_value_t<EntityRange>;
        return std::move(entities)
            | std::views::transform([f = std::move(f)] (const Entity& e) {
                return f(e);
            });
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

    const Field& _get_cell_field(const std::string& name) const {
        return _cell_fields.get(name);
    }

    const Grid& _get_grid() const {
        return _grid;
    }

 private:
    const Grid& _grid;
    FieldStorage _point_fields;
    FieldStorage _cell_fields;
};

template<typename Grid>
class GridWriterBase : public GridWriter<Grid> {
    using ParentType = GridWriter<Grid>;

 public:
    using ParentType::ParentType;

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

template<typename Grid, Concepts::Scalar Time = double>
class TimeSeriesGridWriterBase : public GridWriter<Grid> {
    using ParentType = GridWriter<Grid>;

 public:
    using ParentType::ParentType;

    void write(const Time& t) const {
        _write(t);
    }

 private:
    virtual void _write(const Time& t) const = 0;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_WRITER_HPP_