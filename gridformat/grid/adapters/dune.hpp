// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Adapters
 * \brief Traits specializations for <a href="https://gitlab.dune-project.org/core/dune-grid">dune grid views</a>
 */
#ifndef GRIDFORMAT_ADAPTERS_DUNE_HPP_
#define GRIDFORMAT_ADAPTERS_DUNE_HPP_

#include <ranges>
#include <cassert>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

// forward declaration
namespace Dune {

class GeometryType;
template<typename Traits> class GridView;

// YaspGrid will also be registered as structured grid
template<class ct, int dim> class EquidistantCoordinates;
template<class ct, int dim> class EquidistantOffsetCoordinates;
template<class ct, int dim> class TensorProductCoordinates;
template<int dim, class Coordinates> class YaspGrid;

}  // namespace Dune


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
        // assumes dune/grid/gridenums.hh header to have been included
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
        // assumes dune/grid/gridenums.hh header to have been included
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

template<typename Traits> requires(
    DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>> and
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

#endif  // GRIDFORMAT_ADAPTERS_DUNE_HPP_
