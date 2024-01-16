// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup PredefinedTraits
 * \brief Traits specializations for <a href="https://gitlab.dune-project.org/core/dune-grid">dune grid views</a>
 */
#ifndef GRIDFORMAT_TRAITS_DUNE_HPP_
#define GRIDFORMAT_TRAITS_DUNE_HPP_

#include <ranges>
#include <cassert>
#include <algorithm>
#include <tuple>
// dune seems to not explicitly include this but uses std::int64_t.
// With gcc-13 this leads to an error, maybe before gcc-13 the header
// was included by some other standard library header...
#include <cstdint>

#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS
#include <dune/common/fvector.hh>
#include <dune/geometry/type.hh>
#include <dune/grid/common/gridview.hh>
#include <dune/grid/common/gridenums.hh>
#include <dune/grid/common/gridfactory.hh>
#include <dune/grid/yaspgrid.hh>
#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic pop
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat::Traits {

#ifndef DOXYGEN
namespace DuneDetail {

    inline int map_corner_index(const Dune::GeometryType& gt, int i) {
        if (gt.isQuadrilateral()) {
            assert(i < 4);
            static constexpr int map[4] = {0, 1, 3, 2};
            return map[i];
        }
        if (gt.isHexahedron()) {
            assert(i < 8);
            static constexpr int map[8] = {0, 1, 3, 2, 4, 5, 7, 6};
            return map[i];
        }
        return i;
    }

    inline constexpr GridFormat::CellType cell_type(const Dune::GeometryType& gt) {
        if (gt.isVertex()) return GridFormat::CellType::vertex;
        if (gt.isLine()) return GridFormat::CellType::segment;
        if (gt.isTriangle()) return GridFormat::CellType::triangle;
        if (gt.isQuadrilateral()) return GridFormat::CellType::quadrilateral;
        if (gt.isTetrahedron()) return GridFormat::CellType::tetrahedron;
        if (gt.isHexahedron()) return GridFormat::CellType::hexahedron;
        throw NotImplemented("Unknown Dune::GeometryType");
    }

    template<typename GridView>
    using Element = typename GridView::template Codim<0>::Entity;

    template<typename GridView>
    using Vertex = typename GridView::template Codim<GridView::dimension>::Entity;

}  // namespace DuneDetail
#endif  // DOXYGEN


template<typename Traits>
struct Points<Dune::GridView<Traits>> {
    static decltype(auto) get(const Dune::GridView<Traits>& grid_view) {
        using GV = Dune::GridView<Traits>;
        static constexpr int vertex_codim = GV::dimension;
        return std::ranges::subrange(
            grid_view.template begin<vertex_codim, Dune::InteriorBorder_Partition>(),
            grid_view.template end<vertex_codim, Dune::InteriorBorder_Partition>()
        );
    }
};

template<typename Traits>
struct Cells<Dune::GridView<Traits>> {
    static decltype(auto) get(const Dune::GridView<Traits>& grid_view) {
        return std::ranges::subrange(
            grid_view.template begin<0, Dune::Interior_Partition>(),
            grid_view.template end<0, Dune::Interior_Partition>()
        );
    }
};

template<typename Traits>
struct NumberOfPoints<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        static constexpr int point_codim = Dune::GridView<Traits>::dimension;
        if (grid_view.comm().size() == 1)
            return static_cast<std::size_t>(grid_view.size(point_codim));
        return static_cast<std::size_t>(
            Ranges::size(Points<Dune::GridView<Traits>>::get(grid_view))
        );
    }
};

template<typename Traits>
struct NumberOfCells<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        if (grid_view.comm().size() == 1)
            return static_cast<std::size_t>(grid_view.size(0));
        return static_cast<std::size_t>(
            Ranges::size(Cells<Dune::GridView<Traits>>::get(grid_view))
        );
    }
};

template<typename Traits>
struct NumberOfCellPoints<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static auto get(const Dune::GridView<Traits>&,
                    const DuneDetail::Element<Dune::GridView<Traits>>& cell) {
        return cell.subEntities(Dune::GridView<Traits>::dimension);
    }
};

template<typename Traits>
struct CellPoints<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Element<Dune::GridView<Traits>>& element) {
        static constexpr int dim = Dune::GridView<Traits>::dimension;
        return std::views::iota(unsigned{0}, element.subEntities(dim)) | std::views::transform([&] (int i) {
            return element.template subEntity<dim>(DuneDetail::map_corner_index(element.type(), i));
        });
    }
};

template<typename Traits>
struct CellType<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Element<Dune::GridView<Traits>>& element) {
        return DuneDetail::cell_type(element.type());
    }
};

template<typename Traits>
struct PointCoordinates<Dune::GridView<Traits>, DuneDetail::Vertex<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Vertex<Dune::GridView<Traits>>& vertex) {
        return vertex.geometry().center();
    }
};

template<typename Traits>
struct PointId<Dune::GridView<Traits>, DuneDetail::Vertex<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>& grid_view,
                              const DuneDetail::Vertex<Dune::GridView<Traits>>& vertex) {
        return grid_view.indexSet().index(vertex);
    }
};


#ifndef DOXYGEN
namespace DuneDetail {

    template<typename T>
    struct IsDuneYaspGrid : public std::false_type {};

    template<int dim, typename Coords>
    struct IsDuneYaspGrid<Dune::YaspGrid<dim, Coords>> : public std::true_type {
        static constexpr bool uses_tp_coords = std::same_as<
            Coords,
            Dune::TensorProductCoordinates<typename Coords::ctype, dim>
        >;
    };

    template<typename GridView>
    inline constexpr bool is_yasp_grid_view = IsDuneYaspGrid<typename GridView::Grid>::value;

    template<typename GridView> requires(is_yasp_grid_view<GridView>)
    inline constexpr bool uses_tensor_product_coords = IsDuneYaspGrid<typename GridView::Grid>::uses_tp_coords;

    template<typename ctype, int dim>
    auto spacing_in(int direction, const Dune::EquidistantCoordinates<ctype, dim>& coords) {
        return coords.meshsize(direction, 0);
    }
    template<typename ctype, int dim>
    auto spacing_in(int direction, const Dune::EquidistantOffsetCoordinates<ctype, dim>& coords) {
        return coords.meshsize(direction, 0);
    }

}  // namespace DuneDetail
#endif  // DOXYGEN


// Register YaspGrid as structured grid
template<typename Traits> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Extents<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        const auto& grid_level = *grid_view.grid().begin(level);
        const auto& interior_grid = grid_level.interior[0];
        const auto& gc = *interior_grid.dataBegin();

        static constexpr int dim = Traits::Grid::dimension;
        std::array<std::size_t, dim> result;
        for (int i = 0; i < Traits::Grid::dimension; ++i)
            result[i] = gc.max(i) - gc.min(i) + 1;
        return result;
    }
};

template<typename Traits, typename Entity> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Location<Dune::GridView<Traits>, Entity> {
    static auto get(const Dune::GridView<Traits>& grid_view, const Entity& entity) {
        auto const& grid_level = *grid_view.grid().begin(entity.level());
        auto const& interior = grid_level.interior[0];
        auto const& extent_bounds = *interior.dataBegin();

        auto result = entity.impl().transformingsubiterator().coord();
        for (int i = 0; i < Dune::GridView<Traits>::dimension; ++i)
            result[i] -= extent_bounds.min(i);
        return result;
    }
};

template<typename Traits> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Origin<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        auto const& grid_level = *grid_view.grid().begin(level);
        auto const& interior = grid_level.interior[0];
        auto const& extent_bounds = *interior.dataBegin();

        std::array<typename Traits::Grid::ctype, Traits::Grid::dimension> result;
        for (int i = 0; i < Traits::Grid::dimension; ++i)
            result[i] = grid_level.coords.coordinate(i, extent_bounds.min(i));
        return result;
    }
};

template<typename Traits>
    requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>> and
             !DuneDetail::uses_tensor_product_coords<Dune::GridView<Traits>>) // spacing not uniquely defined for tpcoords
struct Spacing<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        auto const& grid_level = *grid_view.grid().begin(level);

        std::array<typename Traits::Grid::ctype, Traits::Grid::dimension> result;
        for (int i = 0; i < Traits::Grid::dimension; ++i)
            result[i] = DuneDetail::spacing_in(i, grid_level.coords);
        return result;
    }
};

template<typename Traits> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Ordinates<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view, int direction) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        auto const& grid_level = *grid_view.grid().begin(level);
        auto const& interior = grid_level.interior[0];
        auto const& extent_bounds = *interior.dataBegin();

        const auto num_point_ordinates = extent_bounds.max(direction) - extent_bounds.min(direction) + 2;
        std::vector<typename Traits::Grid::ctype> ordinates(num_point_ordinates);
        for (int i = 0; i < num_point_ordinates; ++i)
            ordinates[i] = grid_level.coords.coordinate(direction, extent_bounds.min(direction) + i);
        return ordinates;
    }
};

}  // namespace GridFormat::Traits


// Higher-order output is only available with dune-localfunctions
#if GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS

#include <algorithm>
#include <iterator>
#include <utility>
#include <ranges>
#include <cassert>
#include <type_traits>
#include <unordered_map>
#include <map>

#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS
#include <dune/geometry/referenceelements.hh>
#include <dune/grid/common/mcmgmapper.hh>
#include <dune/localfunctions/lagrange/equidistantpoints.hh>
#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic pop
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS

#include <gridformat/common/reserved_vector.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/grid.hpp>


namespace GridFormat {

// create this alias s.t. we can explicitly
// address the original Dune namespace
namespace DUNE = Dune;

namespace Dune {

#ifndef DOXYGEN
namespace LagrangeDetail {

    inline int dune_to_gfmt_sub_entity(const DUNE::GeometryType& gt, int i, int codim) {
        if (gt.isTriangle()) {
            if (codim == 1) {
                assert(i < 3);
                static constexpr int map[3] = {0, 2, 1};
                return map[i];
            }
        }
        if (gt.isQuadrilateral()) {
            if (codim == 2) {
                assert(i < 4);
                static constexpr int map[4] = {0, 1, 3, 2};
                return map[i];
            }
            if (codim == 1) {
                assert(i < 4);
                static constexpr int map[4] = {3, 1, 0, 2};
                return map[i];
            }
        }
        if (gt.isTetrahedron()) {
            if (codim == 2) {
                assert(i < 6);
                static constexpr int map[6] = {0, 2, 1, 3, 4, 5};
                return map[i];
            }
            if (codim == 1) {
                assert(i < 4);
                static constexpr int map[4] = {3, 0, 2, 1};
                return map[i];
            }
        }
        if (gt.isHexahedron()) {
            if (codim == 3) {
                assert(i < 8);
                static constexpr int map[8] = {0, 1, 3, 2, 4, 5, 7, 6};
                return map[i];
            }
            if (codim == 2) {
                assert(i < 12);
                static constexpr int map[12] = {8, 9, 11, 10, 3, 1, 0, 2, 7, 5, 4, 6};
                return map[i];
            }
        }
        return i;
    }

    inline constexpr GridFormat::CellType cell_type(const DUNE::GeometryType& gt) {
        if (gt.isLine()) return GridFormat::CellType::lagrange_segment;
        if (gt.isTriangle()) return GridFormat::CellType::lagrange_triangle;
        if (gt.isQuadrilateral()) return GridFormat::CellType::lagrange_quadrilateral;
        if (gt.isTetrahedron()) return GridFormat::CellType::lagrange_tetrahedron;
        if (gt.isHexahedron()) return GridFormat::CellType::lagrange_hexahedron;
        throw NotImplemented("Unsupported Dune::GeometryType");
    }

    // Exposes the lagrange points of a geometry in gridformat ordering
    template<typename GridView>
    class LocalPoints {
        // reserve space for third-order hexahedra
        static constexpr std::size_t reserved_size = 64;

     public:
        using PointSet = DUNE::EquidistantPointSet<typename GridView::ctype, GridView::dimension>;
        using Point = typename PointSet::LagrangePoint;

        explicit LocalPoints(unsigned int order)
        : _points(order)
        {}

        void build(const DUNE::GeometryType& geo_type) {
            if (geo_type.dim() != GridView::dimension)
                throw ValueError("Dimension of given geometry does not match the grid");
            _points.build(geo_type);
            _setup_sorted_indices(geo_type);
        }

        std::size_t size() const {
            return _points.size();
        }

        const Point& at(std::size_t i) const {
            return _points[_sorted_indices.at(i)];
        }

        std::ranges::range auto get() const {
            return _sorted_indices | std::views::transform([&] (unsigned int i) {
                return _points[i];
            });
        }

     private:
        void _setup_sorted_indices(const DUNE::GeometryType& geo_type) {
            std::ranges::copy(
                std::views::iota(unsigned{0}, static_cast<unsigned int>(_points.size())),
                std::back_inserter(_sorted_indices)
            );
            std::ranges::sort(_sorted_indices, [&] (unsigned int i1, unsigned int i2) {
                const auto& key1 = _points[i1].localKey();
                const auto& key2 = _points[i2].localKey();
                using LagrangeDetail::dune_to_gfmt_sub_entity;
                if (key1.codim() == key2.codim()) {
                    const auto mapped1 = dune_to_gfmt_sub_entity(geo_type, key1.subEntity(), key1.codim());
                    const auto mapped2 = dune_to_gfmt_sub_entity(geo_type, key2.subEntity(), key2.codim());
                    return mapped1 == mapped2 ? key1.index() < key2.index() : mapped1 < mapped2;
                }
                return _points[i1].localKey().codim() > _points[i2].localKey().codim();
            });
        }

        PointSet _points;
        ReservedVector<unsigned int, reserved_size> _sorted_indices;
    };

    class PointMapper {
     public:
        template<typename GridView>
        explicit PointMapper(const GridView& grid_view) {
            _codim_to_global_indices.resize(GridView::dimension + 1);
            for (int codim = 0; codim < GridView::dimension + 1; ++codim)
                _codim_to_global_indices[codim].resize(grid_view.size(codim));
        }

        struct Key {
            unsigned int codim;
            std::size_t global_index;
            unsigned int sub_index;
        };

        bool contains(const Key& key) const {
            const auto& entity_dofs = _codim_to_global_indices[key.codim][key.global_index];
            if (entity_dofs.size() <= key.sub_index)
                return false;
            return entity_dofs[key.sub_index] != -1;
        }

        void insert(const Key& key, std::size_t index) {
            auto& entity_dofs = _codim_to_global_indices[key.codim][key.global_index];
            if (entity_dofs.size() < key.sub_index + 1)
                entity_dofs.resize(key.sub_index + 1, -1);
            entity_dofs[key.sub_index] = static_cast<std::int64_t>(index);
        }

        std::size_t get(const Key& key) const {
            assert(_codim_to_global_indices[key.codim][key.global_index].size() > key.sub_index);
            auto index = _codim_to_global_indices[key.codim][key.global_index][key.sub_index];
            assert(index != -1);
            return static_cast<std::size_t>(index);
        }

        void clear() {
            _codim_to_global_indices.clear();
        }

     private:
        std::vector<  // codim
            std::vector<  // entity index
                ReservedVector<std::int64_t, 20>  // sub-entity index to global indices
            >
        > _codim_to_global_indices;
    };

}  // namespace LagrangeDetail
#endif  // DOXYGEN


/*!
 * \ingroup PredefinedTraits
 * \brief Exposes a `Dune::GridView` as a grid composed of lagrange cells with the given order.
 *        Can be used to conveniently write `Dune::Functions` into grid files.
 * \note This is only available if dune-localfunctions is found on the system.
 */
template<typename GV>
class LagrangePolynomialGrid {
    using Element = typename GV::Grid::template Codim<0>::Entity;
    using ElementGeometry = typename Element::Geometry;
    using GlobalCoordinate = typename ElementGeometry::GlobalCoordinate;
    using LocalCoordinate = typename ElementGeometry::LocalCoordinate;

    using LocalPoints = LagrangeDetail::LocalPoints<GV>;
    using Mapper = DUNE::MultipleCodimMultipleGeomTypeMapper<GV>;
    static constexpr int dim = GV::dimension;

    template<typename Coord>
    struct P {
        std::size_t index;
        Coord coordinates;
    };

    using _GlobalPoint = P<GlobalCoordinate>;
    using _LocalPoint = P<LocalCoordinate>;

 public:
    using GridView = GV;
    using Position = GlobalCoordinate;
    using Cell = Element;
    using Point = _GlobalPoint;
    using LocalPoint = _LocalPoint;

    explicit LagrangePolynomialGrid(const GridView& grid_view, unsigned int order = 1)
    : _grid_view{grid_view}
    , _order{order} {
        if (order < 1)
            throw InvalidState("Order must be >= 1");
        update(grid_view);
    }

    void update(const GridView& grid_view) {
        clear();
        _grid_view = grid_view;
        _make_codim_mappers();
        _update_local_points();
        _update_mesh();
    }

    void clear() {
        _codim_to_mapper.clear();
        _local_points.clear();
        _points.clear();
        _cells.clear();
    }

    std::size_t number_of_cells() const {
        return _cells.empty() ? 0 : Traits::NumberOfCells<GridView>::get(_grid_view);
    }

    std::size_t number_of_points() const {
        return _points.size();
    }

    std::size_t number_of_points(const Element& element) const {
        return _local_points.at(element.type()).size();
    }

    std::ranges::range auto cells() const {
        return Traits::Cells<GridView>::get(_grid_view);
    }

    std::ranges::range auto points() const {
        return std::views::iota(std::size_t{0}, _points.size())
            | std::views::transform([&] (std::size_t idx) {
                return Point{idx, _points[idx]};
            });
    }

    std::ranges::range auto points(const Element& e) const {
        const auto& corners = _cells[_codim_to_mapper[0].index(e)];
        return std::views::iota(std::size_t{0}, corners.size())
            | std::views::transform([&] (std::size_t i) {
                return Point{
                    .index = corners[i],
                    .coordinates = _points[corners[i]]
                };
            });
    }

    const GridView& grid_view() const {
        return _grid_view;
    }

 private:
    void _update_local_points() {
        for (const auto& gt : _grid_view.indexSet().types(0)) {
            _local_points.emplace(gt, _order);
            _local_points.at(gt).build(gt);
        }
    }

    void _make_codim_mappers() {
        _codim_to_mapper.reserve(dim + 1);
        _codim_to_mapper.emplace_back(Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<0>{})});
        if constexpr (int(GridView::dimension) >= 1)
            _codim_to_mapper.emplace_back(Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<1>{})});
        if constexpr (int(GridView::dimension) >= 2)
            _codim_to_mapper.emplace_back(Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<2>{})});
        if constexpr (int(GridView::dimension) == 3)
            _codim_to_mapper.emplace_back(Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<3>{})});
    }

    void _update_mesh() {
        LagrangeDetail::PointMapper point_mapper{_grid_view};
        std::size_t dof_index = 0;
        _cells.resize(_grid_view.size(0));
        for (const auto& element : Traits::Cells<GridView>::get(_grid_view)) {
            const auto& local_points = _local_points.at(element.type()).get();
            const auto& element_geometry = element.geometry();
            const auto element_index = _codim_to_mapper[0].index(element);
            _cells[element_index].reserve(local_points.size());
            for (const auto& local_point : local_points) {
                const auto& localKey = local_point.localKey();
                const typename LagrangeDetail::PointMapper::Key key{
                    .codim = localKey.codim(),
                    .global_index = _codim_to_mapper[localKey.codim()].subIndex(
                        element, localKey.subEntity(), localKey.codim()
                    ),
                    .sub_index = localKey.index()
                };
                if (!point_mapper.contains(key)) {
                    point_mapper.insert(key, dof_index);
                    _cells[element_index].push_back(dof_index);
                    _points.push_back(element_geometry.global(local_point.point()));
                    dof_index++;
                } else {
                    _cells[element_index].push_back(point_mapper.get(key));
                }
            }
        }
    }

    GridView _grid_view;
    unsigned int _order;
    std::vector<Mapper> _codim_to_mapper;
    std::map<DUNE::GeometryType, LocalPoints> _local_points;
    std::vector<GlobalCoordinate> _points;
    std::vector<std::vector<std::size_t>> _cells;
};

namespace Traits {

template<typename GV>
struct GridView {
    using type = GV;
    static const auto& get(const GV& gv) {
        return gv;
    }
};

template<typename GV>
struct GridView<LagrangePolynomialGrid<GV>> {
    using type = GV;
    static const auto& get(const LagrangePolynomialGrid<GV>& grid) {
        return grid.grid_view();
    }
};

}  // namespace Traits
}  // namespace Dune


namespace Traits {

template<typename GridView>
struct Points<Dune::LagrangePolynomialGrid<GridView>> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh) {
        return mesh.points();
    }
};

template<typename GridView>
struct Cells<Dune::LagrangePolynomialGrid<GridView>> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh) {
        return mesh.cells();
    }
};

template<typename GridView>
struct NumberOfPoints<Dune::LagrangePolynomialGrid<GridView>> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh) {
        return mesh.number_of_points();
    }
};

template<typename GridView>
struct NumberOfCells<Dune::LagrangePolynomialGrid<GridView>> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh) {
        return mesh.number_of_cells();
    }
};

template<typename GridView>
struct NumberOfCellPoints<Dune::LagrangePolynomialGrid<GridView>,
                          typename Dune::LagrangePolynomialGrid<GridView>::Cell> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh,
                    const typename Dune::LagrangePolynomialGrid<GridView>::Cell& cell) {
        return mesh.number_of_points(cell);
    }
};

template<typename GridView>
struct CellPoints<Dune::LagrangePolynomialGrid<GridView>,
                  typename Dune::LagrangePolynomialGrid<GridView>::Cell> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh,
                    const typename Dune::LagrangePolynomialGrid<GridView>::Cell& cell) {
        return mesh.points(cell);
    }
};

template<typename GridView>
struct CellType<Dune::LagrangePolynomialGrid<GridView>,
                typename Dune::LagrangePolynomialGrid<GridView>::Cell> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>&,
                    const typename Dune::LagrangePolynomialGrid<GridView>::Cell& cell) {
        return Dune::LagrangeDetail::cell_type(cell.type());
    }
};

template<typename GridView>
struct PointCoordinates<Dune::LagrangePolynomialGrid<GridView>,
                        typename Dune::LagrangePolynomialGrid<GridView>::Point> {
    static const auto& get(const Dune::LagrangePolynomialGrid<GridView>&,
                           const typename Dune::LagrangePolynomialGrid<GridView>::Point& point) {
        return point.coordinates;
    }
};

template<typename GridView>
struct PointId<Dune::LagrangePolynomialGrid<GridView>,
               typename Dune::LagrangePolynomialGrid<GridView>::Point> {
    static auto get(const Dune::LagrangePolynomialGrid<GridView>&,
                    const typename Dune::LagrangePolynomialGrid<GridView>::Point& point) {
        return point.index;
    }
};

}  // namespace Traits


namespace Dune {
namespace Concepts {

template<typename T, typename GridView>
concept Function = requires(const T& f, const typename GridView::template Codim<0>::Entity& element) {
    { localFunction(f) };
    { localFunction(f).bind(element) };
    { localFunction(f)(element.geometry().center()) };
};

}  // namespace Concepts

#ifndef DOXYGEN
namespace FunctionDetail {

    template<typename Function, typename GridView>
    using RangeType = std::invoke_result_t<
        typename std::remove_cvref_t<Function>::LocalFunction,
        typename GridView::template Codim<0>::Entity::Geometry::LocalCoordinate
    >;

    template<typename Function, typename GridView>
    using RangeScalar = FieldScalar<RangeType<Function, GridView>>;

}  // FunctionDetail
#endif  // DOXYGEN

/*!
 * \ingroup PredefinedTraits
 * \brief Implements the field interface for a Dune::Function defined on a (wrapped) Dune::GridView.
 * \note Takes ownership of the given function if constructed with an rvalue reference, otherwise it
 *       stores a reference.
 */
template<typename _Function, typename Grid, GridFormat::Concepts::Scalar T>
    requires(Concepts::Function<_Function, typename Dune::Traits::GridView<Grid>::type>)
class FunctionField : public GridFormat::Field {
    using Function = std::remove_cvref_t<_Function>;
    using GridView = typename Dune::Traits::GridView<Grid>::type;
    using Element = typename GridView::template Codim<0>::Entity;
    using ElementGeometry = typename Element::Geometry;

    static constexpr bool is_higher_order = std::is_same_v<Grid, LagrangePolynomialGrid<GridView>>;

 public:
    template<typename F>
        requires(std::is_same_v<std::remove_cvref_t<F>, Function>)
    explicit FunctionField(F&& function,
                           const Grid& grid,
                           const Precision<T>& = {},
                           bool cellwise_constant = false)
    : _function{std::forward<F>(function)}
    , _grid{grid}
    , _cellwise_constant{cellwise_constant} {
        if constexpr (requires { function.basis(); }) {
            static_assert(std::is_same_v<typename Function::Basis::GridView, GridView>);
            if (&function.basis().gridView().grid() != &Dune::Traits::GridView<Grid>::get(_grid).grid())
                throw ValueError("Function and mesh do not use the same underlying grid");
        }
    }

 private:
    MDLayout _layout() const override {
        return get_md_layout<FunctionDetail::RangeType<Function, GridView>>(
            _cellwise_constant ? GridFormat::Traits::NumberOfCells<Grid>::get(_grid)
                               : GridFormat::Traits::NumberOfPoints<Grid>::get(_grid)
        );
    }

    DynamicPrecision _precision() const override {
        return {Precision<T>{}};
    }

    Serialization _serialized() const override {
        const auto layout = _layout();
        const auto num_entries = layout.number_of_entries();
        const auto num_entries_per_value = layout.dimension() == 1 ? 1 : layout.number_of_entries(1);

        Serialization result(num_entries*sizeof(T));
        auto out_data = result.template as_span_of<T>();

        using GridFormat::Traits::Cells;
        if (_cellwise_constant) {
            std::size_t count = 0;
            auto local_function = localFunction(_function);
            for (const auto& element : Cells<Grid>::get(_grid)) {
                local_function.bind(element);
                const auto& elem_geo = element.geometry();
                const auto& local_pos = elem_geo.local(elem_geo.center());
                std::size_t offset = (count++)*num_entries_per_value;
                _copy_values(local_function(local_pos), out_data, offset);
            }
        } else {
            _fill_point_values(out_data, num_entries_per_value);
        }

        return result;
    }

    void _fill_point_values(std::span<T> out_data, std::size_t num_entries_per_value) const
        requires(!is_higher_order) {
        using GridFormat::Traits::Cells;
        using GridFormat::Traits::CellPoints;
        using GridFormat::Traits::PointCoordinates;
        using GridFormat::Traits::PointId;
        using GridFormat::Traits::NumberOfPoints;

        auto local_function = localFunction(_function);
        auto point_id_to_running_idx = make_point_id_map(_grid);
        std::vector<bool> handled(NumberOfPoints<Grid>::get(_grid), false);

        std::ranges::for_each(Cells<Grid>::get(_grid), [&] <typename C> (const C& element) {
            const auto& element_geometry = element.geometry();
            local_function.bind(element);
            std::ranges::for_each(CellPoints<Grid, Element>::get(_grid, element), [&] <typename P> (const P& point) {
                const auto point_id = PointId<Grid, P>::get(_grid, point);
                const auto running_idx = point_id_to_running_idx.at(point_id);
                if (!handled[running_idx]) {
                    const auto local_pos = element_geometry.local(PointCoordinates<Grid, P>::get(_grid, point));
                    std::size_t offset = running_idx*num_entries_per_value;
                    _copy_values(local_function(local_pos), out_data, offset);
                }
                handled[running_idx] = true;
            });
        });
    }

    void _fill_point_values(std::span<T> out_data, std::size_t num_entries_per_value) const
        requires(is_higher_order) {
        using GridFormat::Traits::Cells;
        using GridFormat::Traits::CellPoints;
        auto local_function = localFunction(_function);
        std::vector<bool> handled(_grid.number_of_points(), false);
        std::ranges::for_each(Cells<Grid>::get(_grid), [&] <typename C> (const C& element) {
            const auto& element_geometry = element.geometry();
            local_function.bind(element);
            std::ranges::for_each(_grid.points(element), [&] <typename P> (const P& point) {
                if (!handled[point.index]) {
                    const auto local_pos = element_geometry.local(point.coordinates);
                    std::size_t offset = point.index*num_entries_per_value;
                    _copy_values(local_function(local_pos), out_data, offset);
                }
                handled[point.index] = true;
            });
        });
    }

    template<std::ranges::range R>
    void _copy_values(R&& range, std::span<T> out, std::size_t& offset) const {
        std::ranges::for_each(range, [&] (const auto& entry) {
            _copy_values(entry, out, offset);
        });
    }

    template<GridFormat::Concepts::Scalar S>
    void _copy_values(const S value, std::span<T> out, std::size_t& offset) const {
        out[offset++] = static_cast<T>(value);
    }

    _Function _function;
    const Grid& _grid;
    bool _cellwise_constant;
};


template<typename F, typename G, typename T = FunctionDetail::RangeScalar<F, typename Traits::GridView<G>::type>>
    requires(std::is_lvalue_reference_v<F>)
FunctionField(F&&, const G&, const Precision<T>& = {}, bool = false) -> FunctionField<
    std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<F>>>, G, T
>;

template<typename F, typename G, typename T = FunctionDetail::RangeScalar<F, typename Traits::GridView<G>::type>>
    requires(!std::is_lvalue_reference_v<F>)
FunctionField(F&&, const G&, const Precision<T>& = {}, bool = false) -> FunctionField<std::remove_cvref_t<F>, G, T>;


#ifndef DOXYGEN
template<typename T> struct IsLagrangeGrid : public std::false_type {};
template<typename GV> struct IsLagrangeGrid<LagrangePolynomialGrid<GV>> : public std::true_type {};

namespace FunctionDetail {
    template<typename F, typename W, typename T>
    void set_function(F&& f, W& w, const std::string& name, const Precision<T>& prec, bool is_cellwise) {
        if (is_cellwise)
            w.set_cell_field(name, FunctionField{std::forward<F>(f), w.grid(), prec, true});
        else
            w.set_point_field(name, FunctionField{std::forward<F>(f), w.grid(), prec, false});
    }

    template<typename F, typename W>
    void set_function(F&& f, W& w, const std::string& name, bool is_cellwise) {
        using GV = Dune::Traits::GridView<typename W::Grid>::type;
        using T = RangeScalar<F, GV>;
        set_function(std::forward<F>(f), w, name, Precision<T>{}, is_cellwise);
    }
}  // namespace FunctionDetail
#endif  //DOXYGEN

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given Dune::Function to the writer as point field.
 * \note This requires the Writer to have been constructed with a LagrangePolynomialGrid.
 */
template<typename Function, typename Writer>
void set_point_function(Function&& f, Writer& writer, const std::string& name) {
    FunctionDetail::set_function(std::forward<Function>(f), writer, name, false);
}

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given Dune::Function to the writer as point field with the given precision.
 * \note This requires the Writer to have been constructed with a LagrangePolynomialGrid.
 */
template<typename Function, typename Writer, GridFormat::Concepts::Scalar T>
void set_point_function(Function&& f, Writer& writer, const std::string& name, const Precision<T>& prec) {
    FunctionDetail::set_function(std::forward<Function>(f), writer, name, prec, false);
}

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given Dune::Function to the writer as cell field.
 * \note This requires the Writer to have been constructed with a LagrangePolynomialGrid.
 */
template<typename Writer, typename Function>
void set_cell_function(Function&& f, Writer& writer, const std::string& name) {
    FunctionDetail::set_function(std::forward<Function>(f), writer, name, true);
}

/*!
 * \ingroup PredefinedTraits
 * \brief Insert the given Dune::Function to the writer as cell field with the given precision.
 * \note This requires the Writer to have been constructed with a LagrangePolynomialGrid.
 */
template<typename Writer, typename Function, GridFormat::Concepts::Scalar T>
void set_cell_function(Function&& f, Writer& writer, const std::string& name, const Precision<T>& prec) {
    FunctionDetail::set_function(std::forward<Function>(f), writer, name, prec, true);
}

}  // namespace Dune
}  // namespace GridFormat

#else  // GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS

#ifndef DOXYGEN
namespace GridFormat::Dune {

template<typename... T>
class LagrangePolynomialGrid {
    template<typename... Ts> struct False { static constexpr bool value = false; };
 public:
    template<typename... Args>
    LagrangePolynomialGrid(Args&&... args) {
        static_assert(False<Args...>::value, "Dune-localfunctions required for higher-order output");
    }
};

}  // namespace GridFormat::Dune
#endif  // DOXYGEN

#endif  // GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS

namespace GridFormat::Dune {

inline constexpr ::Dune::GeometryType to_dune_geometry_type(const CellType& ct) {
    namespace DGT = ::Dune::GeometryTypes;
    switch (ct) {
        case CellType::vertex: return DGT::vertex;
        case CellType::segment: return DGT::line;
        case CellType::triangle: return DGT::triangle;
        case CellType::pixel: return DGT::quadrilateral;
        case CellType::quadrilateral: return DGT::quadrilateral;
        case CellType::tetrahedron: return DGT::tetrahedron;
        case CellType::hexahedron: return DGT::hexahedron;
        case CellType::voxel: return DGT::hexahedron;
        case CellType::polygon: throw NotImplemented("No conversion from polygon to Dune::GeometryType");
        case CellType::lagrange_segment: throw NotImplemented("Cannot map higher-order cells to Dune::GeometryType");
        case CellType::lagrange_triangle: throw NotImplemented("Cannot map higher-order cells to Dune::GeometryType");
        case CellType::lagrange_quadrilateral: throw NotImplemented("Cannot map higher-order cells to Dune::GeometryType");
        case CellType::lagrange_tetrahedron: throw NotImplemented("Cannot map higher-order cells to Dune::GeometryType");
        case CellType::lagrange_hexahedron: throw NotImplemented("Cannot map higher-order cells to Dune::GeometryType");
        default: throw NotImplemented("Unknown cell type.");
    }
}

template<std::integral TargetType, std::integral T>
auto to_dune(const CellType& ct, const std::vector<T>& corners) {
    auto gt = to_dune_geometry_type(ct);
    std::vector<TargetType> reordered(corners.size());

    // voxels/pixels map to hexes/quads, but reordering has to be skipped
    if (ct != CellType::pixel && ct != CellType::voxel) {
        std::ranges::copy(
            std::views::iota(std::size_t{0}, corners.size())
            | std::views::transform([&] (std::size_t i) {
                return corners[GridFormat::Traits::DuneDetail::map_corner_index(gt, i)];
            }),
            reordered.begin()
        );
    } else {
        std::ranges::copy(corners, reordered.begin());
    }

    return std::make_tuple(std::move(gt), std::move(reordered));
}

/*!
 * \ingroup PredefinedTraits
 * \brief Adapter around a Dune::GridFactory to be compatible with GridFormat::Concepts::GridFactory.
 *        Can be used to export a grid from a reader directly into a Dune::GridFactory. For instance:
 *        \code{.cpp}
 *            GridFormat::Reader reader; reader.open(filename);
 *            Dune::GridFactory<DuneGrid> factory;
 *            {
 *                GridFormat::Dune::GridFactoryAdapter adapter{factory};
 *                reader.export_grid(adapter);
 *            }
 *            // ... use dune grid factory
 *        \endcode
 */
template<typename Grid>
class GridFactoryAdapter {
 public:
    static constexpr int space_dim = Grid::dimensionworld;
    using ctype = typename Grid::ctype;
    using DuneFactory = ::Dune::GridFactory<Grid>;

    GridFactoryAdapter(DuneFactory& factory)
    : _factory{factory}
    {}

    template<std::size_t _space_dim>
    void insert_point(const std::array<ctype, _space_dim>& point) {
        ::Dune::FieldVector<ctype, space_dim> p;
        std::copy_n(point.begin(), space_dim, p.begin());
        _factory.insertVertex(p);
    }

    void insert_cell(const CellType& ct, const std::vector<std::size_t>& corners) {
        const auto [dune_gt, dune_corners] = to_dune<unsigned int>(ct, corners);
        _factory.insertElement(dune_gt, dune_corners);
    }

 private:
    DuneFactory& _factory;
};

}  // namespace GridFormat::Dune

#endif  // GRIDFORMAT_TRAITS_DUNE_HPP_
