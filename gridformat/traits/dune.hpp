// SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
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
#include <dune/geometry/type.hh>
#include <dune/grid/common/gridview.hh>
#include <dune/grid/common/gridenums.hh>
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

        const auto& at(std::size_t i) const {
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

        DUNE::EquidistantPointSet<typename GridView::ctype, GridView::dimension> _points;
        ReservedVector<unsigned int, reserved_size> _sorted_indices;
    };

    class PointIndicesHelper {
     public:
        struct Key {
            unsigned int codim;
            std::size_t global_index;
            unsigned int sub_index;
        };

        bool contains(const Key& key) const {
            if (!_codim_to_global_indices.contains(key.codim))
                return false;
            if (!_codim_to_global_indices.at(key.codim).contains(key.global_index))
                return false;
            return _codim_to_global_indices.at(key.codim).at(key.global_index).contains(key.sub_index);
        }

        void insert(const Key& key, std::size_t index) {
            _codim_to_global_indices[key.codim][key.global_index][key.sub_index] = index;
        }

        std::size_t get(const Key& key) {
            return _codim_to_global_indices.at(key.codim).at(key.global_index).at(key.sub_index);
        }

     private:
        std::unordered_map<
            int, // codim
            std::unordered_map<
                std::size_t, // entity index
                std::unordered_map<int, std::size_t>  // sub-entity index to global indices
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
    using ElementSeed = typename GV::Grid::template Codim<0>::EntitySeed;
    using LocalPoints = LagrangeDetail::LocalPoints<GV>;
    using Mapper = DUNE::MultipleCodimMultipleGeomTypeMapper<GV>;
    static constexpr int dim = GV::dimension;

 public:
    struct Point { std::size_t index; };
    struct Cell { std::size_t index; };

    using GridView = GV;
    using Element = typename GridView::template Codim<0>::Entity;
    using Position = typename Element::Geometry::GlobalCoordinate;

    explicit LagrangePolynomialGrid(const GridView& grid_view, unsigned int order = 1)
    : _grid_view{grid_view}
    , _order{order} {
        update(grid_view);
        if (order < 1)
            throw InvalidState("Order must be >= 1");
    }

    void update(const GridView& grid_view) {
        clear();
        _grid_view = grid_view;
        _make_codim_mappers();
        _update_local_points();
        _update_mesh();
    }

    void clear() {
        _points.clear();
        _cells.clear();
        _element_seeds.clear();
        _local_points.clear();
        _codim_to_mapper.clear();
        _element_to_running_index.clear();
    }

    std::size_t number_of_cells() const { return _cells.size(); }
    std::size_t number_of_points() const { return _points.size(); }
    std::size_t number_of_points(const Cell& cell) const { return _cells.at(cell.index).size(); }

    auto cells() const {
        return std::views::iota(std::size_t{0}, number_of_cells())
            | std::views::transform([] (const std::size_t i) {
                return Cell{i};
            });
    }

    auto points() const {
        return std::views::iota(std::size_t{0}, number_of_points())
            | std::views::transform([] (const std::size_t i) {
                return Point{i};
            });
    }

    auto points(const Cell& cell) const {
        return _cells.at(cell.index) | std::views::transform([] (const std::size_t i) {
            return Point{i};
        });
    }

    auto points(const Element& e) const {
        return points(Cell{_element_to_running_index.at(_codim_to_mapper.at(0).index(e))});
    }

    const GridView& grid_view() const {
        return _grid_view;
    }

    const Position& position(const Point& p) const {
        return _points.at(p.index);
    }

    Element element(const Cell& cell) const {
        return _grid_view.grid().entity(_element_seeds.at(cell.index));
    }

    auto geometry(const Cell& cell) const {
        return element(cell).geometry();
    }

 private:
    void _update_local_points() {
        for (const auto& gt : _grid_view.indexSet().types(0)) {
            _local_points.emplace(gt, _order);
            _local_points.at(gt).build(gt);
        }
    }

    auto _make_codim_mappers() {
        _codim_to_mapper.clear();
        _codim_to_mapper.emplace(0, Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<0>{})});
        _codim_to_mapper.emplace(1, Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<1>{})});
        if constexpr (int(GridView::dimension) >= 2)
            _codim_to_mapper.emplace(2, Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<2>{})});
        if constexpr (int(GridView::dimension) == 3)
            _codim_to_mapper.emplace(3, Mapper{_grid_view, DUNE::mcmgLayout(DUNE::Codim<3>{})});
    }

    void _update_mesh() {
        LagrangeDetail::PointIndicesHelper point_indices;
        _cells.reserve(Traits::NumberOfCells<GridView>::get(_grid_view));
        _element_seeds.reserve(_cells.size());
        for (const auto& element : Traits::Cells<GridView>::get(_grid_view)) {
            _element_to_running_index[_codim_to_mapper.at(0).index(element)] = _cells.size();
            _element_seeds.push_back(element.seed());
            auto& cell_points = _cells.emplace_back();
            for (const auto& local_point : _local_points.at(element.type()).get()) {
                const auto codim = local_point.localKey().codim();
                const auto sub_entity = local_point.localKey().subEntity();
                const auto sub_index = local_point.localKey().index();
                const auto global_index = _codim_to_mapper.at(codim).subIndex(element, sub_entity, codim);
                if (!point_indices.contains({codim, global_index, sub_index})) {
                    _points.emplace_back(element.geometry().global(local_point.point()));
                    point_indices.insert({codim, global_index, sub_index}, _points.size() - 1);
                    cell_points.push_back(_points.size() - 1);
                } else {
                    cell_points.push_back(point_indices.get({codim, global_index, sub_index}));
                }
            }
        }
    }

    GridView _grid_view;
    unsigned int _order;
    std::vector<Position> _points;
    std::vector<std::vector<std::size_t>> _cells;
    std::vector<ElementSeed> _element_seeds;
    std::map<DUNE::GeometryType, LocalPoints> _local_points;
    std::unordered_map<int, Mapper> _codim_to_mapper;
    std::unordered_map<std::size_t, std::size_t> _element_to_running_index;
};

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
    static auto get(const Dune::LagrangePolynomialGrid<GridView>& mesh,
                    const typename Dune::LagrangePolynomialGrid<GridView>::Cell& cell) {
        return Dune::LagrangeDetail::cell_type(mesh.element(cell).type());
    }
};

template<typename GridView>
struct PointCoordinates<Dune::LagrangePolynomialGrid<GridView>,
                        typename Dune::LagrangePolynomialGrid<GridView>::Point> {
    static const auto& get(const Dune::LagrangePolynomialGrid<GridView>& mesh,
                           const typename Dune::LagrangePolynomialGrid<GridView>::Point& point) {
        return mesh.position(point);
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
 * \brief Implements the field interface for a Dune::Function defined on a GridFormat::Dune::LagrangePolynomialGrid.
 */
template<typename Function, typename GridView, typename T = FunctionDetail::RangeScalar<Function, GridView>>
class FunctionField : public GridFormat::Field {
 public:
    explicit FunctionField(const Function& function,
                           const LagrangePolynomialGrid<GridView>& mesh,
                           const Precision<T>& = {},
                           bool cellwise_constant = false)
    : _function{function}
    , _mesh{mesh}
    , _cellwise_constant{cellwise_constant} {
        if constexpr (requires { function.basis(); }) {
            static_assert(std::is_same_v<typename Function::Basis::GridView, GridView>);
            if (&function.basis().gridView().grid() != &mesh.grid_view().grid())
                throw ValueError("Function and mesh do not use the same underlying grid");
        }
    }

 private:
    MDLayout _layout() const override {
        return get_md_layout<FunctionDetail::RangeType<Function, GridView>>(
            _cellwise_constant ? _mesh.number_of_cells()
                               : _mesh.number_of_points()
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
        auto local_function = localFunction(_function);

        if (_cellwise_constant) {
            std::size_t count = 0;
            for (const auto& element : Traits::Cells<GridView>::get(_mesh.grid_view())) {
                local_function.bind(element);
                const auto& elem_geo = element.geometry();
                const auto& local_pos = elem_geo.local(elem_geo.center());
                std::size_t offset = (count++)*num_entries_per_value;
                _copy_values(local_function(local_pos), out_data, offset);
            }
        } else {
            for (const auto& element : Traits::Cells<GridView>::get(_mesh.grid_view())) {
                local_function.bind(element);
                for (const auto& point : _mesh.points(element)) {
                    const auto& local_pos = element.geometry().local(_mesh.position(point));
                    std::size_t offset = point.index*num_entries_per_value;
                    _copy_values(local_function(local_pos), out_data, offset);
                }
            }
        }

        return result;
    }

    template<std::ranges::range R>
    void _copy_values(R&& range, std::span<T> out, std::size_t& offset) const {
        std::ranges::for_each(range, [&] (const auto& entry) {
            _copy_values(entry, out, offset);
        });
    }

    template<Concepts::Scalar S>
    void _copy_values(const S value, std::span<T> out, std::size_t& offset) const {
        out[offset++] = static_cast<T>(value);
    }

    const Function& _function;
    const LagrangePolynomialGrid<GridView>& _mesh;
    bool _cellwise_constant;
};


#ifndef DOXYGEN
template<typename T> struct IsLagrangeGrid : public std::false_type {};
template<typename GV> struct IsLagrangeGrid<LagrangePolynomialGrid<GV>> : public std::true_type {};

namespace FunctionDetail {
    template<typename F, typename W, typename T>
    void set_function(F&& f, W& w, const std::string& name, const Precision<T>& prec, bool is_cellwise) {
        static_assert(std::is_lvalue_reference_v<F>, "Functions are stored as references and need to be lvalues");
        static_assert(
            IsLagrangeGrid<typename W::Grid>::value,
            "Functions can only be set in writers that were constructed with GridFormat::Dune::LagrangePolynomialGrid"
        );
        if (is_cellwise)
            w.set_cell_field(name, FunctionField{f, w.grid(), prec, true});
        else
            w.set_point_field(name, FunctionField{f, w.grid(), prec, false});
    }

    template<typename F, typename W>
    void set_function(F&& f, W& w, const std::string& name, bool is_cellwise) {
        static_assert(
            IsLagrangeGrid<typename W::Grid>::value,
            "Functions can only be set in writers that were constructed with GridFormat::Dune::LagrangePolynomialGrid"
        );
        using T = RangeScalar<F, typename W::Grid::GridView>;
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
template<typename Function, typename Writer, Concepts::Scalar T>
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
template<typename Writer, typename Function, Concepts::Scalar T>
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
#endif  // GRIDFORMAT_TRAITS_DUNE_HPP_
