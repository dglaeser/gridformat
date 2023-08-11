// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Helper function for writing parallel VTK files
 */
#ifndef GRIDFORMAT_VTK_PARALLEL_HPP_
#define GRIDFORMAT_VTK_PARALLEL_HPP_

#include <array>
#include <vector>
#include <string>
#include <utility>
#include <concepts>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <tuple>
#include <cmath>

#include <gridformat/common/math.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/helpers.hpp>

#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/attributes.hpp>

namespace GridFormat::PVTK {

//! Return the piece filename (w/o extension) for the given rank
 std::string piece_basefilename(const std::string& par_filename, int rank) {
    const std::string base_name = par_filename.substr(0, par_filename.find_last_of("."));
    return base_name + "-" + std::to_string(rank);
}

//! Helper to add a PDataArray child to an xml element
template<typename Encoder, typename DataFormat>
class PDataArrayHelper {
 public:
    PDataArrayHelper(const Encoder& e,
                     const DataFormat& df,
                     XMLElement& element)
    : _encoder(e)
    , _data_format(df)
    , _element(element)
    {}

    void add(const std::string& name, const Field& field) {
        // vtk always uses 3d, this assumes that the field
        // is transformed accordingly in the piece writers
        static constexpr std::size_t vtk_space_dim = 3;
        const auto layout = field.layout();
        std::size_t ncomps = std::pow(vtk_space_dim, field.layout().dimension() - 1);

        // leave higher-dimensional vectors untouched
        if (layout.dimension() == 2 && layout.extent(1) > vtk_space_dim)
            ncomps = layout.extent(1);

        XMLElement& arr = _element.add_child("PDataArray");
        arr.set_attribute("Name", name);
        arr.set_attribute("type", VTK::attribute_name(field.precision()));
        arr.set_attribute("format", VTK::data_format_name(_encoder, _data_format));
        arr.set_attribute("NumberOfComponents", ncomps);
    }

 private:
    const Encoder& _encoder;
    const DataFormat& _data_format;
    XMLElement& _element;
};

template<std::size_t dim>
class StructuredGridMapper {
 public:
    using Location = std::array<std::size_t, dim>;

    explicit StructuredGridMapper(std::vector<Location>&& map)
    : _map{std::move(map)}
    {}

    const auto& location(std::integral auto rank) const {
        return _map[rank];
    }

    std::ranges::view auto ranks_below(const Location& loc, unsigned direction) const {
        return std::views::filter(
            std::views::iota(std::size_t{0}, _map.size()),
            [&, loc=loc, direction=direction] (const std::size_t rank) {
                const auto& rank_loc = _map[rank];
                for (unsigned dir = 0; dir < dim; ++dir)
                    if (dir != direction && rank_loc[dir] != loc[dir]) {
                        return false;
                    }
                return rank_loc[direction] < loc[direction];
            }
        );
    }

 private:
    std::vector<Location> _map;
};

//! Helper for finding the locations of the sub-grids
//! associated with each rank in structured parallel grids
template<typename T, std::size_t dim>
class StructuredGridMapperHelper {
 public:
    using Origin = std::array<T, dim>;
    using Basis = std::array<Origin, dim>;

    explicit StructuredGridMapperHelper(std::integral auto ranks,
                                        const Basis& basis,
                                        T default_epsilon = 1e-6)
    : _basis(basis)
    , _origins(ranks)
    , _set(ranks, false)
    , _default_epsilon{default_epsilon} {
        std::ranges::fill(_reverse, false);
    }

    void reverse(int direction) {
        _reverse[direction] = !_reverse[direction];
    }

    void set_origin_for(int rank, Origin origin) {
        if (_set[rank])
            throw ValueError("Origin for given rank already set");
        _origins[rank] = std::move(origin);
        _set[rank] = true;
    }

    auto make_mapper() const {
        if (_origins.empty())
            throw InvalidState("No origins have been set");

        std::vector<std::array<std::size_t, dim>> map(_origins.size());
        for (unsigned dir = 0; dir < dim; ++dir) {
            auto ordinates = _get_ordinates(dir);
            const auto eps = _epsilon(ordinates);
            _sort_ordinates(ordinates, eps, _reverse[dir]);

            if (ordinates.size() > 1) {
                for (unsigned rank = 0; rank < _origins.size(); ++rank) {
                    const auto rank_origin_ordinate = _get_ordinate(_origins[rank], dir);
                    auto it = std::ranges::find_if(ordinates, [&] (const T o) {
                        using std::abs;
                        return abs(o - rank_origin_ordinate) < eps;
                    });
                    if (it == ordinates.end())
                        throw InvalidState("Could not determine rank ordinate");
                    map[rank][dir] = std::distance(ordinates.begin(), it);
                }
            } else {
                for (unsigned rank = 0; rank < _origins.size(); ++rank)
                    map[rank][dir] = 0;
            }
        }
        return StructuredGridMapper<dim>{std::move(map)};
    }

    Origin compute_origin() const {
        if (_origins.empty())
            throw InvalidState("No origins have been set");

        Origin result;
        for (unsigned dir = 0; dir < dim; ++dir) {
            auto ordinates = _get_ordinates(dir);
            _sort_ordinates(ordinates, _reverse[dir]);
            result[dir] = ordinates[0];
        }
        return result;
    }

 private:
    auto _get_ordinates(unsigned int axis) const {
        std::vector<T> result;
        result.reserve(_origins.size());
        std::ranges::copy(
            std::views::transform(_origins, [&] (const Origin& o) {
                return _get_ordinate(o, axis);
            }),
            std::back_inserter(result)
        );
        return result;
    }

    auto _get_ordinate(const Origin& o, unsigned int axis) const {
        return dot_product(o, _basis[axis]);
    }

    void _sort_ordinates(std::vector<T>& ordinates, bool reverse) const {
        if (ordinates.size() > 1)
            _sort_ordinates(ordinates, _epsilon(ordinates), reverse);
    }

    void _sort_ordinates(std::vector<T>& ordinates, const T& eps, bool reverse) const {
        if (ordinates.size() > 1) {
            auto [it, _] = Ranges::sort_and_unique(ordinates, {}, [&] (T a, T b) {
                using std::abs;
                return abs(a - b) < eps;
            });
            ordinates.erase(it, ordinates.end());
            if (reverse)
                std::ranges::reverse(ordinates);
        }
    }

    T _epsilon(const std::vector<T>& ordinates) const {
        const auto max = *std::ranges::max_element(ordinates);
        const auto min = *std::ranges::min_element(ordinates);
        const auto size = max - min;

        if (size > _default_epsilon) {
            std::vector<T> dx; dx.reserve(ordinates.size());
            std::adjacent_difference(ordinates.begin(), ordinates.end(), std::back_inserter(dx));
            std::ranges::for_each(dx, [] (auto& v) { v = std::abs(v); });
            std::ranges::sort(dx);
            for (T _dx : dx)
                if (_dx > 1e-8*size)
                    return _dx*0.1;
        }

        return _default_epsilon;
    }

    const Basis& _basis;
    std::vector<Origin> _origins;
    std::vector<bool> _set;
    std::array<bool, dim> _reverse;
    T _default_epsilon;
};


template<typename Communicator>
class StructuredParallelGridHelper {
 public:
    StructuredParallelGridHelper(const Communicator& comm, int root_rank = 0)
    : _comm{comm}
    , _root_rank{root_rank}
    {}

    template<Concepts::Scalar CT,
             std::size_t dim,
             Concepts::StaticallySizedMDRange<2> B = std::array<std::array<CT, dim>, dim>>
    auto compute_extents_and_origin(const std::vector<CT>& all_origins,
                                    const std::vector<std::size_t>& all_extents,
                                    const std::array<bool, dim>& is_negative_axis,
                                    const B& basis = GridDetail::standard_basis<CT, dim>()) const {
        const auto num_ranks = Parallel::size(_comm);
        std::vector<std::array<std::size_t, dim>> pieces_begin(num_ranks);
        std::vector<std::array<std::size_t, dim>> pieces_end(num_ranks);
        std::array<std::size_t, dim> whole_extent;
        std::array<CT, dim> origin;

        if (Parallel::rank(_comm) == _root_rank) {
            const auto default_epsilon = 1e-6*std::ranges::max(all_origins | std::views::transform([] (CT v) {
                using std::abs;
                return abs(v);
            }));
            const auto mapper_helper = _make_mapper_helper(basis, all_origins, is_negative_axis, default_epsilon);
            const auto rank_mapper = mapper_helper.make_mapper();
            origin = mapper_helper.compute_origin();

            for (unsigned dir = 0; dir < dim; ++dir) {
                for (int rank = 0; rank < num_ranks; ++rank) {
                    auto ranks_below = rank_mapper.ranks_below(rank_mapper.location(rank), dir);
                    const std::size_t offset = std::accumulate(
                        std::ranges::begin(ranks_below),
                        std::ranges::end(ranks_below),
                        std::size_t{0},
                        [&] (const std::size_t current, int r) {
                            return current + Parallel::access_gathered<dim>(all_extents, _comm, {dir, r});
                        }
                    );
                    pieces_begin[rank][dir] = offset;
                    pieces_end[rank][dir] = offset + Parallel::access_gathered<dim>(all_extents, _comm, {dir, rank});
                }

                whole_extent[dir] = (*std::max_element(
                    pieces_end.begin(), pieces_end.end(),
                    [&] (const auto& a1, const auto& a2) { return a1[dir] < a2[dir]; }
                ))[dir];
            }
        }

        return std::make_tuple(
            std::move(pieces_begin),
            std::move(pieces_end),
            std::move(whole_extent),
            std::move(origin)
        );
    }

 private:
    template<Concepts::StaticallySizedMDRange<2> Basis, Concepts::Scalar CT, std::size_t dim>
    auto _make_mapper_helper(const Basis& basis,
                             const std::vector<CT>& all_origins,
                             const std::array<bool, dim>& is_negative_axis,
                             CT default_eps) const {
        StructuredGridMapperHelper<CT, dim> helper(Parallel::size(_comm), basis, default_eps);
        std::ranges::for_each(Parallel::ranks(_comm), [&] (int rank) {
            helper.set_origin_for(rank, Parallel::access_gathered<dim>(all_origins, _comm, rank));
        });

        for (unsigned dir = 0; dir < dim; ++dir)
            if (is_negative_axis[dir])
                helper.reverse(dir);

        return helper;
    }

    const Communicator& _comm;
    int _root_rank;
};

}  // namespace GridFormat::PVTK

#endif  // GRIDFORMAT_VTK_PARALLEL_HPP_
