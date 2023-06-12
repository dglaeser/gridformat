// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup PredefinedTraits
 * \brief Traits specializations for
 *        <a href="https://docs.mfem.org/html/classmfem_1_1Mesh.html">mfem::Mesh</a>
 */
#ifndef GRIDFORMAT_TRAITS_MFEM_HPP_
#define GRIDFORMAT_TRAITS_MFEM_HPP_

#include <cassert>
#include <ranges>
#include <vector>
#include <array>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/filtered_range.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

// forward declaration of the triangulation classes
// users are expected to include the headers of the actually used classes
namespace mfem {

// TODO
// #ifdef MFEM_USE_MPI
// class ParMesh;
// #endif

class Mesh;
class Element;

}  // namespace mfem

namespace GridFormat::MFEM {

#ifndef DOXYGEN
namespace Detail {
    CellType cell_type(mfem::Element::Type ct) {
        // since mfem supports higher-order cells, let's simply return the lagrange variants always
        switch (ct) {
            case mfem::Element::Type::POINT: return GridFormat::CellType::vertex;
            case mfem::Element::Type::SEGMENT: return GridFormat::CellType::lagrange_segment;
            case mfem::Element::Type::TRIANGLE: return GridFormat::CellType::lagrange_triangle;
            case mfem::Element::Type::QUADRILATERAL: return GridFormat::CellType::lagrange_quadrilateral;
            case mfem::Element::Type::TETRAHEDRON: return GridFormat::CellType::lagrange_tetrahedron;
            case mfem::Element::Type::WEDGE: break;
            case mfem::Element::Type::PYRAMID: break;
            case mfem::Element::Type::HEXAHEDRON: return GridFormat::CellType::lagrange_hexahedron;
        }
        throw NotImplemented("Support for given mfem cell type");
    }
}  // namespace Detail
#endif  // DOXYGEN

using Point = int;
using Cell = int;

} // namespace GridFormat::MFEM

// Specializations of the traits required for the `UnstructuredGrid` concept for mfem::Mesh
namespace GridFormat::Traits {

template<>
struct Points<mfem::Mesh> {
    static std::ranges::range auto get(const mfem::Mesh& mesh) {
        return std::views::iota(0, mesh.GetNV());
    }
};

template<>
struct Cells<mfem::Mesh> {
    static std::ranges::range auto get(const mfem::Mesh& mesh) {
        return std::views::iota(0, mesh.GetNE());
    }
};

template<>
struct CellType<mfem::Mesh, GridFormat::MFEM::Cell> {
    static GridFormat::CellType get(const mfem::Mesh& mesh, const GridFormat::MFEM::Cell& cell) {
        return GridFormat::MFEM::Detail::cell_type(mesh.GetElement(cell)->GetType());
    }
};

template<>
struct CellPoints<mfem::Mesh, GridFormat::MFEM::Cell> {
    static std::ranges::range decltype(auto) get(const mfem::Mesh& mesh, const GridFormat::MFEM::Cell& cell) {
        const auto element = mesh.GetElement(cell);
        auto begin = element->GetVertices();
        return std::ranges::subrange(begin, begin + element->GetNVertices());
    }
};

template<>
struct PointId<mfem::Mesh, GridFormat::MFEM::Point> {
    static auto get(const mfem::Mesh& mesh, const GridFormat::MFEM::Point& point) {
        return point;
    }
};

template<>
struct PointCoordinates<mfem::Mesh, GridFormat::MFEM::Point> {
    static std::ranges::range decltype(auto) get(const mfem::Mesh& mesh, const GridFormat::MFEM::Point& point) {
        std::array<double, 3> coords = {};
        auto begin = mesh.GetVertex(point);
        std::copy(begin, begin + mesh.SpaceDimension(), coords.begin());
        return coords;
    }
};

template<>
struct NumberOfPoints<mfem::Mesh> {
    static std::integral auto get(const mfem::Mesh& mesh) {
        return mesh.GetNV();
    }
};

template<>
struct NumberOfCells<mfem::Mesh> {
    static std::integral auto get(const mfem::Mesh& mesh) {
        return mesh.GetNE();
    }
};

template<>
struct NumberOfCellPoints<mfem::Mesh, mfem::Element> {
    static std::integral auto get(const mfem::Mesh&, const mfem::Element& cell) {
        return cell.GetNVertices();
    }
};

}  // namespace GridFormat::Traits

#endif  // GRIDFORMAT_TRAITS_MFEM_HPP_
