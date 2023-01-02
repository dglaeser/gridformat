// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Adapters
 * \brief Traits specializations for [`Dune::GridView`](https://gitlab.dune-project.org/core/dune-grid)
 */
#ifndef GRIDFORMAT_ADAPTERS_DUNE_HPP_
#define GRIDFORMAT_ADAPTERS_DUNE_HPP_

#include <ranges>
#include <cassert>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

// forward declaration
namespace Dune {

class GeometryType;

template<typename Traits>
class GridView;

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
        return grid_view.size(point_codim);
    }
};

template<typename Traits>
struct NumberOfCells<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        return grid_view.size(0);
    }
};

template<typename Traits>
struct NumberOfCellCorners<Dune::GridView<Traits>> {
    using _Cell = typename Dune::GridView<Traits>::template Codim<0>::Entity;
    static auto get(const Dune::GridView<Traits>&, const _Cell& cell) {
        return cell.subEntities(Dune::GridView<Traits>::dimension);
    }
};

template<typename Traits>
struct CellPoints<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Element<Dune::GridView<Traits>>& element) {
        static constexpr int dim = Dune::GridView<Traits>::dimension;
        return std::views::transform(
            std::views::iota(unsigned{0}, element.subEntities(dim)),
            [elem=element] (int i) {
                return elem.template subEntity<dim>(DuneDetail::map_corner_index(elem.type(), i));
            }
        );
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

}  // namespace GridFormat::Traits

#endif  // GRIDFORMAT_ADAPTERS_DUNE_HPP_
