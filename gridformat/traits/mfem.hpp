// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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

#include <mfem/mesh/mesh.hpp>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/filtered_range.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat::MFEM {

#ifndef DOXYGEN
namespace Detail {
    CellType cell_type(mfem::Element::Type ct) {
        switch (ct) {
            case mfem::Element::Type::POINT: return GridFormat::CellType::vertex;
            case mfem::Element::Type::SEGMENT: return GridFormat::CellType::segment;
            case mfem::Element::Type::TRIANGLE: return GridFormat::CellType::triangle;
            case mfem::Element::Type::QUADRILATERAL: return GridFormat::CellType::quadrilateral;
            case mfem::Element::Type::TETRAHEDRON: return GridFormat::CellType::tetrahedron;
            case mfem::Element::Type::WEDGE: break;
            case mfem::Element::Type::PYRAMID: break;
            case mfem::Element::Type::HEXAHEDRON: return GridFormat::CellType::hexahedron;
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
    static auto get(const mfem::Mesh&, const GridFormat::MFEM::Point& point) {
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
struct NumberOfCellPoints<mfem::Mesh, GridFormat::MFEM::Cell> {
    static std::integral auto get(const mfem::Mesh& mesh, const GridFormat::MFEM::Cell& cell) {
        return mesh.GetElement(cell)->GetNVertices();
    }
};

}  // namespace GridFormat::Traits

#endif  // GRIDFORMAT_TRAITS_MFEM_HPP_
