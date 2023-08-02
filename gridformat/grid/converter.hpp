// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \brief Converter between grid formats.
 */
#ifndef GRIDFORMAT_GRID_CONVERTER_HPP_
#define GRIDFORMAT_GRID_CONVERTER_HPP_

#include <array>
#include <vector>
#include <ranges>
#include <concepts>
#include <utility>
#include <numeric>
#include <optional>

#include <gridformat/common/field.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/reader.hpp>
#include <gridformat/grid/writer.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace ConverterDetail {

    struct ConverterGrid {
        struct Cell { std::size_t i; };
        struct Point { std::size_t i; };

        const GridReader& reader;
        std::vector<std::array<double, 3>> points;
        std::vector<std::pair<CellType, std::vector<std::size_t>>> cells;

        explicit ConverterGrid(const GridReader& r) : reader{r} {}

        void make_grid() {
            _make_points();
            _make_cells();
        }

     private:
        void _make_points() {
            const auto in_points = reader.points();
            const auto in_layout = in_points->layout();
            const auto in_np = in_layout.extent(0);
            const auto in_dim = in_layout.dimension() > 1 ? in_layout.extent(1) : 0;
            if (in_np != reader.number_of_points())
                throw SizeError("Mismatch between stored and defined number of points.");

            points.reserve(in_np);
            in_points->visit_field_values([&] <typename T> (std::span<const T> values) {
                std::ranges::for_each(std::views::iota(std::size_t{0}, in_np), [&] (auto p_idx) {
                    points.push_back({});
                    std::ranges::for_each(std::views::iota(std::size_t{0}, in_dim), [&] (auto dim) {
                        points.back().at(dim) = static_cast<double>(values[p_idx*in_dim + dim]);
                    });
                });
            });
        }

        void _make_cells() {
            cells.reserve(reader.number_of_cells());
            reader.visit_cells([&] (CellType ct, std::vector<std::size_t> corners) {
                cells.emplace_back(std::pair{std::move(ct), std::move(corners)});
            });
            if (cells.size() != reader.number_of_cells())
                throw SizeError("Mismatch between stored and defined number of cells.");
        }
    };

    template<typename T>
    concept Writer
        = requires { typename std::remove_cvref_t<T>::Grid; }
        and std::derived_from<std::remove_cvref_t<T>, GridWriterBase<typename T::Grid>>;

    template<typename T>
    concept WriterFactory = requires (const T& factory, const ConverterGrid& grid) {
        { factory(grid) } -> Writer;
    };

}  // namespace ConverterDetail
#endif  // DOXYGEN

/*!
 * \ingroup Grid
 * \brief Convert between grid formats.
 * \param reader A grid reader on which a file was opened.
 * \param factory A factory to construct a writer with the desired output format.
 * \param filename The name of the file to be written.
 */
template<std::derived_from<GridReader> Reader, ConverterDetail::WriterFactory Factory>
std::string convert(const Reader& reader, const std::string& filename, const Factory& factory) {
    ConverterDetail::ConverterGrid grid{reader};
    auto writer = factory(grid);
    if constexpr (Traits::WritesConnectivity<std::remove_cvref_t<decltype(writer)>>::value)
        grid.make_grid();
    for (auto [name, field_ptr] : cell_fields(reader))
        writer.set_cell_field(std::move(name), std::move(field_ptr));
    for (auto [name, field_ptr] : point_fields(reader))
        writer.set_point_field(std::move(name), std::move(field_ptr));
    for (auto [name, field_ptr] : meta_data_fields(reader))
        writer.set_meta_data(std::move(name), std::move(field_ptr));
    return writer.write(filename);
}

namespace Traits {

template<>
struct Points<ConverterDetail::ConverterGrid> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid) {
        return std::views::iota(std::size_t{0}, grid.reader.number_of_points())
            | std::views::transform([] (std::size_t i) { return ConverterDetail::ConverterGrid::Point{i}; });
    }
};

template<>
struct Cells<ConverterDetail::ConverterGrid> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid) {
        return std::views::iota(std::size_t{0}, grid.reader.number_of_cells())
            | std::views::transform([] (std::size_t i) { return ConverterDetail::ConverterGrid::Cell{i}; });
    }
};

template<>
struct NumberOfPoints<ConverterDetail::ConverterGrid> {
    static std::size_t get(const ConverterDetail::ConverterGrid& grid) {
        return grid.reader.number_of_points();
    }
};

template<>
struct NumberOfCells<ConverterDetail::ConverterGrid> {
    static std::size_t get(const ConverterDetail::ConverterGrid& grid) {
        return grid.reader.number_of_cells();
    }
};

template<>
struct CellPoints<ConverterDetail::ConverterGrid,
                  typename ConverterDetail::ConverterGrid::Cell> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid,
                                       const typename ConverterDetail::ConverterGrid::Cell& cell) {
        return grid.cells.at(cell.i).second | std::views::transform([] (std::size_t i) {
            return ConverterDetail::ConverterGrid::Point{i};
        });
    }
};

template<>
struct CellType<ConverterDetail::ConverterGrid,
                typename ConverterDetail::ConverterGrid::Cell> {
    static GridFormat::CellType get(const ConverterDetail::ConverterGrid& grid,
                                    const typename ConverterDetail::ConverterGrid::Cell& cell) {
        return grid.cells.at(cell.i).first;
    }
};

template<>
struct PointCoordinates<ConverterDetail::ConverterGrid,
                        typename ConverterDetail::ConverterGrid::Point> {
    static std::array<double, 3> get(const ConverterDetail::ConverterGrid& grid,
                                     const typename ConverterDetail::ConverterGrid::Point& point) {
        return grid.points.at(point.i);
    }
};

template<>
struct PointId<ConverterDetail::ConverterGrid,
               typename ConverterDetail::ConverterGrid::Point> {
    static std::size_t get(const ConverterDetail::ConverterGrid&,
                           const typename ConverterDetail::ConverterGrid::Point& point) {
        return point.i;
    }
};

template<>
struct NumberOfCellPoints<ConverterDetail::ConverterGrid,
                          typename ConverterDetail::ConverterGrid::Cell> {
    static std::size_t get(const ConverterDetail::ConverterGrid& grid,
                           const typename ConverterDetail::ConverterGrid::Cell& cell) {
        return grid.cells.at(cell.i).second.size();
    }
};

template<>
struct Origin<ConverterDetail::ConverterGrid> {
    static std::array<double, 3> get(const ConverterDetail::ConverterGrid& grid) {
        return grid.reader.origin();
    }
};

template<>
struct Spacing<ConverterDetail::ConverterGrid> {
    static std::array<double, 3> get(const ConverterDetail::ConverterGrid& grid) {
        return grid.reader.spacing();
    }
};

template<>
struct Basis<ConverterDetail::ConverterGrid> {
    static std::array<std::array<double, 3>, 3> get(const ConverterDetail::ConverterGrid& grid) {
        std::array<std::array<double, 3>, 3> result;
        for (unsigned int i = 0; i < 3; ++i)
            result[i] = grid.reader.basis_vector(i);
        return result;
    }
};

template<>
struct Extents<ConverterDetail::ConverterGrid> {
    static std::array<std::size_t, 3> get(const ConverterDetail::ConverterGrid& grid) {
        return grid.reader.extents();
    }
};

template<>
struct Ordinates<ConverterDetail::ConverterGrid> {
    static std::vector<double> get(const ConverterDetail::ConverterGrid& grid, unsigned int i) {
        return grid.reader.ordinates(i);
    }
};

// TODO: Maybe let this throw? Should not be needed?
template<typename Entity>
struct Location<ConverterDetail::ConverterGrid, Entity> {
    static std::array<std::size_t, 3> get(const ConverterDetail::ConverterGrid& grid,
                                          const typename ConverterDetail::ConverterGrid::Point& point) {
        return _get(Ranges::incremented(Extents<ConverterDetail::ConverterGrid>::get(grid), 1), point.i);
    }

    static std::array<std::size_t, 3> get(const ConverterDetail::ConverterGrid& grid,
                                          const typename ConverterDetail::ConverterGrid::Cell& cell) {
        return _get(Extents<ConverterDetail::ConverterGrid>::get(grid), cell.i);
    }

 private:
    static std::array<std::size_t, 3> _get(const std::ranges::range auto& extents, std::size_t index) {
        const auto num_actives = std::ranges::count_if(extents, [] (auto e) { return e != 0; });
        const auto accumulator = [] (const auto& range, int non_zero_count) {
            std::vector<std::size_t> filtered;
            std::ranges::copy(
                range | std::views::filter([] (auto v) { return v != 0; })
                      | std::views::take(non_zero_count),
                std::back_inserter(filtered)
            );
            return std::accumulate(
                std::ranges::begin(filtered),
                std::ranges::end(filtered),
                std::size_t{1},
                std::multiplies{}
            );
        };

        if (num_actives == 1)
            return {index, 0, 0};
        if (num_actives == 2) {
            const auto divisor = accumulator(extents, 1);
            return {
                index%divisor,
                index/divisor,
                0
            };
        }
        if (num_actives == 3) {
            const auto divisor1 = accumulator(extents, 1);
            const auto divisor2 = accumulator(extents, 2);
            return {
                index%divisor2%divisor1,
                index&divisor2/divisor1,
                index/divisor2
            };
        }
        throw InvalidState("Unexpected number of active extents");
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CONVERTER_HPP_
