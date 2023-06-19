// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup PredefinedTraits
 * \brief Traits specializations for
 *        <a href="https://www.dealii.org/current/doxygen/deal.II/group__grid.html">dealii triangulations</a>
 */
#ifndef GRIDFORMAT_TRAITS_DEAL_II_HPP_
#define GRIDFORMAT_TRAITS_DEAL_II_HPP_

#include <cassert>
#include <ranges>
#include <vector>
#include <array>
#include <type_traits>

#include <deal.II/grid/tria.h>
#include <deal.II/distributed/tria.h>
#include <deal.II/distributed/fully_distributed_tria.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/filtered_range.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>


namespace GridFormat {

#ifndef DOXYGEN
namespace DealII {

template<int dim, int space_dim> using Tria = dealii::Triangulation<dim, space_dim>;
template<int dim, int space_dim> using DTria = dealii::parallel::distributed::Triangulation<dim, space_dim>;
template<int dim, int space_dim> using FDTria = dealii::parallel::fullydistributed::Triangulation<dim, space_dim>;

template<typename T> struct IsTriangulation : public std::false_type {};
template<typename T> struct IsParallelTriangulation : public std::false_type {};

template<int d, int sd> struct IsTriangulation<Tria<d, sd>> : public std::true_type {};
template<int d, int sd> struct IsTriangulation<DTria<d, sd>> : public std::true_type {};
template<int d, int sd> struct IsTriangulation<FDTria<d, sd>> : public std::true_type {};
template<int d, int sd> struct IsParallelTriangulation<DTria<d, sd>> : public std::true_type {};
template<int d, int sd> struct IsParallelTriangulation<FDTria<d, sd>> : public std::true_type {};

template<typename T> inline constexpr bool is_triangulation = IsTriangulation<T>::value;
template<typename T> inline constexpr bool is_parallel_triangulation = IsParallelTriangulation<T>::value;

// We need to wrap the deal.ii iterators because they do not fulfill the
// requirements for the std::ranges concepts. The issue comes from the
// std::indirectly_readable concept (https://en.cppreference.com/w/cpp/iterator/indirectly_readable)
// which expects that the reference type obtained from Iterator& and const Iterator&
// is the same: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1878r0.pdf.
// However, deal.ii iterators return refs or const refs depending on the constness of the iterator.
// For the traits, we only need read-access to cells/points, so we wrap the iterator and expose only
// the const interface in order to be compatible with std::ranges.
template<typename Iterator>
class ForwardIteratorWrapper
: public ForwardIteratorFacade<ForwardIteratorWrapper<Iterator>,
                               typename Iterator::value_type,
                               const typename Iterator::value_type&,
                               typename Iterator::pointer,
                               typename Iterator::difference_type> {
    using Reference = const typename Iterator::value_type&;

 public:
    ForwardIteratorWrapper() = default;
    ForwardIteratorWrapper(Iterator it) : _it{it} {}

    // we need to explicitly implement the equality operators because with
    // deal.ii we have compare iterator types that are not convertible into
    // each other (which is a requirement for the generic implementation provided
    // in GridFormat::IteratorFacade). For instance, active_cell_iterator has to
    // be compared against cell_iterator which is used as sentinel.
    template<typename I>
    friend bool operator==(const ForwardIteratorWrapper& self, const ForwardIteratorWrapper<I>& other) {
        return self._it == other._get_it();
    }

    template<typename I>
    friend bool operator!=(const ForwardIteratorWrapper& self, const ForwardIteratorWrapper<I>& other) {
        return self._it != other._get_it();
    }

    const Iterator& _get_it() const {
        return _it;
    }

 private:
    friend IteratorAccess;

    Reference _dereference() const { return *_it; }
    bool _is_equal(const ForwardIteratorWrapper& other) { return _it == other._it; }
    void _increment() { ++_it; }

    Iterator _it;
};

template<typename It>
ForwardIteratorWrapper(It&& it) -> ForwardIteratorWrapper<std::remove_cvref_t<It>>;

template<typename T> requires(is_triangulation<T>)
using Cell = typename T::active_cell_iterator::AccessorType;

template<typename T> requires(is_triangulation<T>)
using Point = typename T::active_vertex_iterator::AccessorType;

const std::vector<int>& cell_corners_in_gridformat_order(unsigned int cell_dimension,
                                                         unsigned int number_of_cell_corners) {
    if (cell_dimension == 0) {
        assert(number_of_cell_corners == 1);
        static const std::vector<int> corners{0};
        return corners;
    } else if (cell_dimension == 1) {
        assert(number_of_cell_corners == 2);
        static const std::vector<int> corners{0, 1};
        return corners;
    } else if (cell_dimension == 2) {
        if (number_of_cell_corners == 3) {  // triangles
            static const std::vector<int> corners{0, 1, 2};
            return corners;
        } else if (number_of_cell_corners == 4) {  // quads
            static const std::vector<int> corners{0, 1, 3, 2};
            return corners;
        }
    } else if (cell_dimension == 3) {
        if (number_of_cell_corners == 4) {  // tets
            static const std::vector<int> corners{0, 1, 2, 3};
            return corners;
        } else if (number_of_cell_corners == 8) {  // hexes
            static const std::vector<int> corners{0, 1, 3, 2, 4, 5, 7, 6};
            return corners;
        }
    }

    throw NotImplemented(
        "deal.ii cell corner indices for cell of dimension " + std::to_string(cell_dimension) +
        " and " + std::to_string(number_of_cell_corners) + " corners"
    );
    // In order to make compilers happy
    static const std::vector<int> v{};
    return v;
}

}  // namespace DealII
#endif  // DOXYGEN

// Specializations of the traits required for the `UnstructuredGrid` concept for dealii triangulations
namespace Traits {

template<typename T> requires(DealII::is_triangulation<T>)
struct Points<T> {
    static std::ranges::range decltype(auto) get(const T& grid) {
        return std::ranges::subrange(
            DealII::ForwardIteratorWrapper{grid.begin_active_vertex()},
            DealII::ForwardIteratorWrapper{grid.end_vertex()}
        );
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct Cells<T> {
    static std::ranges::range auto get(const T& grid) {
        if constexpr (DealII::is_parallel_triangulation<T>)
            return Ranges::filter_by(
                [] (const auto& cell) { return cell.is_locally_owned(); },
                std::ranges::subrange(
                    DealII::ForwardIteratorWrapper{grid.begin_active()},
                    DealII::ForwardIteratorWrapper{grid.end()}
                )
            );
        else
            return std::ranges::subrange(
                DealII::ForwardIteratorWrapper{grid.begin_active()},
                DealII::ForwardIteratorWrapper{grid.end()}
            );
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct CellType<T, DealII::Cell<T>> {
    static GridFormat::CellType get(const T&, const DealII::Cell<T>& cell) {
        using CT = GridFormat::CellType;
        static constexpr std::array cubes{CT::vertex, CT::segment, CT::quadrilateral, CT::hexahedron};
        static constexpr std::array simplices{CT::vertex, CT::segment, CT::triangle, CT::tetrahedron};
        const auto& ref_cell = cell.reference_cell();
        if (ref_cell.is_hyper_cube())
            return cubes[ref_cell.get_dimension()];
        else if (ref_cell.is_simplex())
            return simplices[ref_cell.get_dimension()];
        throw NotImplemented("CellType only implemented for hypercubes & simplices");
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct CellPoints<T, DealII::Cell<T>> {
    static std::ranges::range decltype(auto) get(const T&, const DealII::Cell<T>& cell) {
        return DealII::cell_corners_in_gridformat_order(cell.reference_cell().get_dimension(), cell.n_vertices())
            | std::views::transform([&] (int i) { return *cell.vertex_iterator(i); });
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct PointId<T, DealII::Point<T>> {
    static auto get(const T&, const DealII::Point<T>& point) {
        return point.index();
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct PointCoordinates<T, DealII::Point<T>> {
    static std::ranges::range decltype(auto) get(const T&, const DealII::Point<T>& point) {
        static_assert(T::dimension >= 1 && T::dimension <= 3);
        const auto& center = point.center();
        if constexpr (T::dimension == 1)
            return std::array{center[0]};
        else if constexpr (T::dimension == 2)
            return std::array{center[0], center[1]};
        else if constexpr (T::dimension == 3)
            return std::array{center[0], center[1], center[2]};
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct NumberOfPoints<T> {
    static std::integral auto get(const T& grid) {
        return grid.n_used_vertices();
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct NumberOfCells<T> {
    static std::integral auto get(const T& grid) {
        if constexpr (DealII::is_parallel_triangulation<T>)
            return grid.n_locally_owned_active_cells();
        else
            return grid.n_active_cells();
    }
};

template<typename T> requires(DealII::is_triangulation<T>)
struct NumberOfCellPoints<T, DealII::Cell<T>> {
    static std::integral auto get(const T&, const DealII::Cell<T>& cell) {
        return cell.n_vertices();
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_TRAITS_DEAL_II_HPP_
