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
        return std::views::iota(std::size_t{0}, grid.reader.number_of_points());
    }
};

template<>
struct Cells<ConverterDetail::ConverterGrid> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid) {
        return std::views::iota(std::size_t{0}, grid.reader.number_of_cells());
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
struct CellPoints<ConverterDetail::ConverterGrid, std::size_t> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid, const std::size_t i) {
        return grid.cells.at(i).second | std::views::all;
    }
};

template<>
struct CellType<ConverterDetail::ConverterGrid, std::size_t> {
    static GridFormat::CellType get(const ConverterDetail::ConverterGrid& grid, const std::size_t i) {
        return grid.cells.at(i).first;
    }
};

template<>
struct PointCoordinates<ConverterDetail::ConverterGrid, std::size_t> {
    static std::array<double, 3> get(const ConverterDetail::ConverterGrid& grid, const std::size_t i) {
        return grid.points.at(i);
    }
};

template<>
struct PointId<ConverterDetail::ConverterGrid, std::size_t> {
    static std::size_t get(const ConverterDetail::ConverterGrid&, const std::size_t i) {
        return i;
    }
};

template<>
struct NumberOfCellPoints<ConverterDetail::ConverterGrid, std::size_t> {
    static std::size_t get(const ConverterDetail::ConverterGrid& grid, const std::size_t i) {
        return grid.cells.at(i).second.size();
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

template<typename Entity>
struct Location<ConverterDetail::ConverterGrid, Entity> {
    static std::array<std::size_t, 3> get(const ConverterDetail::ConverterGrid&, const Entity&) {
        throw NotImplemented("Entity location for converter grid");
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CONVERTER_HPP_
