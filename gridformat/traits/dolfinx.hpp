// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup PredefinedTraits
 * \brief Traits specializations for <a href="https://docs.fenicsproject.org/dolfinx/v0.11.0/cpp/">dolfinx meshes</a>
 * \note These traits are written for and tested with dolfinx 0.11.
 */
#ifndef GRIDFORMAT_TRAITS_DOLFINX_HPP_
#define GRIDFORMAT_TRAITS_DOLFINX_HPP_

#include <array>
#include <ranges>
#include <cstdint>
#include <utility>
#include <memory>
#include <algorithm>
#include <concepts>
#include <span>
#include <vector>

#include <dolfinx/io/cells.h>
#include <dolfinx/io/vtk_utils.h>
#include <dolfinx/mesh/cell_types.h>
#include <dolfinx/mesh/Mesh.h>
#include <dolfinx/fem/Function.h>
#include <dolfinx/fem/FunctionSpace.h>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/writer.hpp>


namespace GridFormat {

namespace DolfinX {

struct Cell { std::int32_t index; };
struct Point { std::int32_t index; };

#ifndef DOXYGEN
namespace Detail {
    CellType cell_type(dolfinx::mesh::CellType ct) {
        // since dolfinx supports higher-order cells, let's simply return the lagrange variants always
        switch (ct) {
            case dolfinx::mesh::CellType::point: return GridFormat::CellType::vertex;
            case dolfinx::mesh::CellType::interval: return GridFormat::CellType::lagrange_segment;
            case dolfinx::mesh::CellType::triangle: return GridFormat::CellType::lagrange_triangle;
            case dolfinx::mesh::CellType::quadrilateral: return GridFormat::CellType::lagrange_quadrilateral;
            case dolfinx::mesh::CellType::tetrahedron: return GridFormat::CellType::lagrange_tetrahedron;
            case dolfinx::mesh::CellType::pyramid: break;
            case dolfinx::mesh::CellType::prism: break;
            case dolfinx::mesh::CellType::hexahedron: return GridFormat::CellType::lagrange_hexahedron;
        }
        throw NotImplemented("Support for dolfinx cell type '" + dolfinx::mesh::to_string(ct) + "'");
    }

    template<std::floating_point T>
    bool is_cellwise_constant(const dolfinx::fem::FunctionSpace<T>& space) {
        return space.dofmap()->element_dof_layout().num_dofs() == 1;
    }

    template<typename T, std::floating_point U>
    bool is_cellwise_constant(const dolfinx::fem::Function<T, U>& f) {
        assert(f.function_space());
        return is_cellwise_constant(*f.function_space());
    }

    //! Return the geometry nodes of the given cell (for meshes with a single cell type)
    template<std::floating_point T>
    std::span<const std::int32_t> cell_nodes(const dolfinx::mesh::Mesh<T>& mesh, std::int32_t cell) {
        const auto dofmap = mesh.geometry().dofmaps().front();
        const auto num_nodes = dofmap.extent(1);
        return {dofmap.data_handle() + static_cast<std::size_t>(cell)*num_nodes, num_nodes};
    }
}  // namespace Detail
#endif  // DOXYGEN

}  // namespace DolfinX

namespace Traits {

template<std::floating_point T>
struct Cells<dolfinx::mesh::Mesh<T>> {
    static std::ranges::range auto get(const dolfinx::mesh::Mesh<T>& mesh) {
        auto map = mesh.topology()->index_map(mesh.topology()->dim());
        if (!map) throw GridFormat::ValueError("Cell index map not available");
        return std::views::iota(0, map->size_local()) | std::views::transform([] (std::int32_t i) {
            return DolfinX::Cell{i};
        });
    }
};

template<std::floating_point T>
struct CellType<dolfinx::mesh::Mesh<T>, DolfinX::Cell> {
    static GridFormat::CellType get(const dolfinx::mesh::Mesh<T>& mesh, const DolfinX::Cell&) {
        return DolfinX::Detail::cell_type(mesh.topology()->cell_type());
    }
};

template<std::floating_point T>
struct CellPoints<dolfinx::mesh::Mesh<T>, DolfinX::Cell> {
    static std::ranges::range auto get(const dolfinx::mesh::Mesh<T>& mesh, const DolfinX::Cell& cell) {
        const auto nodes = DolfinX::Detail::cell_nodes(mesh, cell.index);
        auto permutation = dolfinx::io::cells::transpose(
            dolfinx::io::cells::perm_vtk(mesh.topology()->cell_type(), nodes.size())
        );
        return std::views::iota(std::size_t{0}, nodes.size())
            | std::views::transform([perm = std::move(permutation), nodes=nodes] (std::size_t i) {
                return DolfinX::Point{nodes[perm[i]]};
            }
        );
    }
};

template<std::floating_point T>
struct Points<dolfinx::mesh::Mesh<T>> {
    static std::ranges::range auto get(const dolfinx::mesh::Mesh<T>& mesh) {
        const auto num_points = mesh.geometry().x().size()/3;
        return std::views::iota(std::size_t{0}, num_points) | std::views::transform([] (std::size_t i) {
            return DolfinX::Point{static_cast<std::int32_t>(i)};
        });
    }
};

template<std::floating_point T>
struct PointCoordinates<dolfinx::mesh::Mesh<T>, DolfinX::Point> {
    static std::ranges::range auto get(const dolfinx::mesh::Mesh<T>& mesh, const DolfinX::Point& point) {
        const auto x = mesh.geometry().x();
        return std::array{x[point.index*3], x[point.index*3 + 1], x[point.index*3 + 2]};
    }
};

template<std::floating_point T>
struct PointId<dolfinx::mesh::Mesh<T>, DolfinX::Point> {
    static std::integral auto get(const dolfinx::mesh::Mesh<T>&, const DolfinX::Point& point) {
        return point.index;
    }
};

template<std::floating_point T>
struct NumberOfPoints<dolfinx::mesh::Mesh<T>> {
    static std::integral auto get(const dolfinx::mesh::Mesh<T>& mesh) {
        return mesh.geometry().x().size()/3;
    }
};

template<std::floating_point T>
struct NumberOfCells<dolfinx::mesh::Mesh<T>> {
    static std::integral auto get(const dolfinx::mesh::Mesh<T>& mesh) {
        auto map = mesh.topology()->index_map(mesh.topology()->dim());
        if (!map) throw GridFormat::ValueError("Cell index map not available");
        return map->size_local();
    }
};

template<std::floating_point T>
struct NumberOfCellPoints<dolfinx::mesh::Mesh<T>, DolfinX::Cell> {
    static std::integral auto get(const dolfinx::mesh::Mesh<T>& mesh, const DolfinX::Cell&) {
        return mesh.geometry().dofmaps().front().extent(1);
    }
};

}  // namespace Traits

namespace DolfinX {

/*!
 * \ingroup PredefinedTraits
 * \brief Wrapper around a nodal dolfinx::FunctionSpace, exposing it as a mesh
 *        composed of lagrange elements with the order of the given function space.
 * \tparam T The geometry (coordinate) type of the underlying dolfinx mesh.
 */
template<std::floating_point T>
class LagrangePolynomialGrid {
 public:
    LagrangePolynomialGrid() = default;
    LagrangePolynomialGrid(const dolfinx::fem::FunctionSpace<T>& space) {
        if (!space.mesh() || !space.element())
            throw ValueError("Cannot construct mesh from space without mesh or element");
        // dolfinx does not check this precondition of vtk_mesh_from_space and fails with undefined behaviour
        if (Detail::is_cellwise_constant(space))
            throw ValueError("Cannot construct mesh from a space with cell-wise constant (P0) functions");

        _cell_type = space.mesh()->topology()->cell_type();
        _mesh = space.mesh();
        _element = space.element();

        auto [x, xshape, xids, _ghosts, cells, cells_shape] = dolfinx::io::vtk_mesh_from_space(space);
        _node_coords = std::move(x);
        _node_coords_shape = std::move(xshape);
        _node_ids = std::move(xids);
        _cells = std::move(cells);
        _cells_shape = std::move(cells_shape);
        _set = true;
    }

    void update(const dolfinx::fem::FunctionSpace<T>& space) {
        *this = LagrangePolynomialGrid{space};
    }

    void clear() {
        _mesh = nullptr;
        _element = nullptr;
        _node_coords.clear();
        _node_ids.clear();
        _cells.clear();
        _set = false;
    }

    static LagrangePolynomialGrid from(const dolfinx::fem::FunctionSpace<T>& space) {
        return {space};
    }

    std::integral auto number_of_points() const { return _node_coords_shape[0]; }
    std::integral auto number_of_cells() const { return _cells_shape[0]; }
    std::integral auto number_of_cell_points() const { return _cells_shape[1]; }

    std::int64_t id(const Point& p) const {
        return _node_ids[p.index];
    }

    auto position(const Point& p) const {
        assert(_node_coords_shape[1] == 3);
        return std::array{
            _node_coords[p.index*3],
            _node_coords[p.index*3 + 1],
            _node_coords[p.index*3 + 2]
        };
    }

    std::ranges::range auto points() const {
        _check_built();
        return std::views::iota(std::size_t{0}, _node_coords_shape[0])
            | std::views::transform([] (std::size_t i) {
                return Point{static_cast<std::int32_t>(i)};
            });
    }

    std::ranges::range auto points(const Cell& cell) const {
        return std::views::iota(std::size_t{0}, _cells_shape[1])
            | std::views::transform([&, i=cell.index, ncorners=_cells_shape[1]] (std::size_t local_point_index) {
                return Point{static_cast<std::int32_t>(_cells[i*ncorners + local_point_index])};
            });
    }

    std::ranges::range auto cells() const {
        _check_built();
        return std::views::iota(std::size_t{0}, _cells_shape[0])
            | std::views::transform([] (std::size_t i) {
                return Cell{static_cast<std::int32_t>(i)};
            });
    }

    template<int rank = 0, int dim = 3, Concepts::Scalar V>
    auto evaluate(const dolfinx::fem::Function<V, T>& f, const Cell& c) const {
        assert(is_compatible(f));
        return _evaluate<rank, dim>(f, c.index);
    }

    template<int rank = 0, int dim = 3, Concepts::Scalar V>
    auto evaluate(const dolfinx::fem::Function<V, T>& f, const Point& p) const {
        assert(is_compatible(f));
        return _evaluate<rank, dim>(f, p.index);
    }

    auto cell_type() const {
        return _cell_type;
    }

    template<Concepts::Scalar V>
    bool is_compatible(const dolfinx::fem::Function<V, T>& f) const {
        if (!_set) return false;
        if (!f.function_space()->mesh()) return false;
        if (f.function_space()->mesh() != _mesh) return false;
        if (!Detail::is_cellwise_constant(f)) {
            if (!f.function_space()->element()) return false;
            if (*f.function_space()->element() != *_element) return false;
        }
        return true;
    }

 private:
    void _check_built() const {
        if (!_set)
            throw InvalidState("Mesh has not been built");
    }

    template<int rank = 0, int dim = 3, Concepts::Scalar V>
    auto _evaluate(const dolfinx::fem::Function<V, T>& f, const std::integral auto i) const {
        const auto f_components = f.function_space()->element()->block_size();
        const auto& values = f.x()->array();
        assert(f.function_space()->element()->value_shape().size() == rank);
        assert(values.size() >= static_cast<std::size_t>(i*f_components + f_components));

        if constexpr (rank == 0)
            return values[i*f_components];
        else if constexpr (rank == 1) {
            auto result = Ranges::filled_array<dim>(V{0});
            std::copy_n(
                values.data() + i*f_components,
                std::min(dim, f_components),
                result.begin()
            );
            return result;
        } else {
            throw NotImplemented("Tensor evaluation");
            return std::array<std::array<V, dim>, dim>{};  // for return type deduction
        }
    }

    dolfinx::mesh::CellType _cell_type;
    std::shared_ptr<const dolfinx::mesh::Mesh<T>> _mesh{nullptr};
    std::shared_ptr<const dolfinx::fem::FiniteElement<T>> _element{nullptr};

    std::vector<T> _node_coords;
    std::array<std::size_t, 2> _node_coords_shape;
    std::vector<std::int64_t> _node_ids;
    std::vector<std::int64_t> _cells;
    std::array<std::size_t, 2> _cells_shape;
    bool _set = false;
};

template<std::floating_point T>
LagrangePolynomialGrid(const dolfinx::fem::FunctionSpace<T>&) -> LagrangePolynomialGrid<T>;

/*!
 * \ingroup PredefinedTraits
 * \brief Create a LagrangePolynomialGrid from the given function space.
 */
template<std::floating_point T>
LagrangePolynomialGrid<T> make_lagrange_grid(const dolfinx::fem::FunctionSpace<T>& space) {
    return LagrangePolynomialGrid<T>{space};
}

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given function into the writer as point field.
 * \param f The function to be inserted
 * \param writer The writer in which to insert it
 * \param name The name of the field (defaults to `f.name`)
 * \param prec The precision with which to write the field (defaults to the function's scalar type)
 */
template<typename Writer, Concepts::Scalar T, std::floating_point U, Concepts::Scalar P = T>
void set_point_function(const dolfinx::fem::Function<T, U>& f,
                        Writer& writer,
                        std::string name = "",
                        const Precision<P>& prec = {}) {
    if (!writer.grid().is_compatible(f))
        throw ValueError("Grid passed to writer is incompatible with the given function");
    if (Detail::is_cellwise_constant(f))
        throw ValueError("Given function is not node-based");
    if (name.empty())
        name = f.name;
    const auto block_size = f.function_space()->element()->block_size();
    const auto dim = f.function_space()->mesh()->geometry().dim();
    if (block_size == 1)
        writer.set_point_field(name, [&] (const auto p) { return writer.grid().template evaluate<0>(f, p); }, prec);
    else if (dim >= block_size)
        writer.set_point_field(name, [&] (const auto p) { return writer.grid().template evaluate<1>(f, p); }, prec);
    else
        writer.set_point_field(name, [&] (const auto p) { return writer.grid().template evaluate<2>(f, p); }, prec);
}

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given function into the writer as cell field.
 * \param f The function to be inserted
 * \param writer The writer in which to insert it
 * \param name The name of the field (defaults to `f.name`)
 * \param prec The precision with which to write the field (defaults to the function's scalar type)
 */
template<typename Writer, Concepts::Scalar T, std::floating_point U, Concepts::Scalar P = T>
void set_cell_function(const dolfinx::fem::Function<T, U>& f,
                       Writer& writer,
                       std::string name = "",
                       const Precision<P>& prec = {}) {
    if (!writer.grid().is_compatible(f))
        throw ValueError("Grid passed to writer is incompatible with the given function");
    if (!Detail::is_cellwise_constant(f))
        throw ValueError("Given function is not constant per grid cell");
    if (name.empty())
        name = f.name;
    const auto block_size = f.function_space()->element()->block_size();
    const auto dim = f.function_space()->mesh()->geometry().dim();
    if (block_size == 1)
        writer.set_cell_field(name, [&] (const auto p) { return writer.grid().template evaluate<0>(f, p); }, prec);
    else if (dim >= block_size)
        writer.set_cell_field(name, [&] (const auto p) { return writer.grid().template evaluate<1>(f, p); }, prec);
    else
        writer.set_cell_field(name, [&] (const auto p) { return writer.grid().template evaluate<2>(f, p); }, prec);
}

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given function into the writer as field.
 * \param f The function to be inserted
 * \param writer The writer in which to insert it
 * \param name The name of the field (defaults to `f.name`)
 * \param prec The precision with which to write the field (defaults to the function's scalar type)
 */
template<typename Writer, Concepts::Scalar T, std::floating_point U, Concepts::Scalar P = T>
void set_function(const dolfinx::fem::Function<T, U>& f,
                  Writer& writer,
                  const std::string& name = "",
                  const Precision<P>& prec = {}) {
    if (Detail::is_cellwise_constant(f))
        set_cell_function(f, writer, name, prec);
    else
        set_point_function(f, writer, name, prec);
}

}  // namespace DolfinX

namespace Traits {

template<std::floating_point T>
struct Cells<DolfinX::LagrangePolynomialGrid<T>> {
    static std::ranges::range auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh) {
        return mesh.cells();
    }
};

template<std::floating_point T>
struct CellType<DolfinX::LagrangePolynomialGrid<T>, DolfinX::Cell> {
    static GridFormat::CellType get(const DolfinX::LagrangePolynomialGrid<T>& mesh, const DolfinX::Cell&) {
        return DolfinX::Detail::cell_type(mesh.cell_type());
    }
};

template<std::floating_point T>
struct CellPoints<DolfinX::LagrangePolynomialGrid<T>, DolfinX::Cell> {
    static std::ranges::range auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh, const DolfinX::Cell& cell) {
        return mesh.points(cell);
    }
};

template<std::floating_point T>
struct Points<DolfinX::LagrangePolynomialGrid<T>> {
    static std::ranges::range auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh) {
        return mesh.points();
    }
};

template<std::floating_point T>
struct PointCoordinates<DolfinX::LagrangePolynomialGrid<T>, DolfinX::Point> {
    static std::ranges::range auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh, const DolfinX::Point& point) {
        return mesh.position(point);
    }
};

template<std::floating_point T>
struct PointId<DolfinX::LagrangePolynomialGrid<T>, DolfinX::Point> {
    static std::integral auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh, const DolfinX::Point& point) {
        return mesh.id(point);
    }
};

template<std::floating_point T>
struct NumberOfPoints<DolfinX::LagrangePolynomialGrid<T>> {
    static std::integral auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh) {
        return mesh.number_of_points();
    }
};

template<std::floating_point T>
struct NumberOfCells<DolfinX::LagrangePolynomialGrid<T>> {
    static std::integral auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh) {
        return mesh.number_of_cells();
    }
};

template<std::floating_point T>
struct NumberOfCellPoints<DolfinX::LagrangePolynomialGrid<T>, DolfinX::Cell> {
    static std::integral auto get(const DolfinX::LagrangePolynomialGrid<T>& mesh, const DolfinX::Cell&) {
        return mesh.number_of_cell_points();
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_TRAITS_DOLFINX_HPP_
