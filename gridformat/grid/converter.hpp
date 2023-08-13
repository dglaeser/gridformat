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
#include <cstdint>

#include <gridformat/common/field.hpp>
#include <gridformat/common/exceptions.hpp>

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
            points.clear();
            cells.clear();
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
    concept PieceWriter = Writer<T> and std::derived_from<std::remove_cvref_t<T>, GridWriter<typename T::Grid>>;

    template<typename T>
    concept TimeSeriesWriter = Writer<T> and std::derived_from<std::remove_cvref_t<T>, TimeSeriesGridWriter<typename T::Grid>>;

    template<typename T>
    concept PieceWriterFactory = requires (const T& factory, const ConverterGrid& grid) {
        { factory(grid) } -> PieceWriter;
    };

    template<typename T>
    concept TimeSeriesWriterFactory = requires (const T& factory, const ConverterGrid& grid, const std::string& filename) {
        { factory(grid) } -> TimeSeriesWriter;
    };

    template<typename T>
    concept WriterFactory = PieceWriterFactory<T> or TimeSeriesWriterFactory<T>;

    template<typename Reader, Writer Writer>
    void add_piece_fields(const Reader& reader, Writer& writer) {
        writer.clear();
        for (auto [name, field_ptr] : cell_fields(reader))
            writer.set_cell_field(std::move(name), std::move(field_ptr));
        for (auto [name, field_ptr] : point_fields(reader))
            writer.set_point_field(std::move(name), std::move(field_ptr));
        for (auto [name, field_ptr] : meta_data_fields(reader))
            writer.set_meta_data(std::move(name), std::move(field_ptr));
    }

    template<typename Reader, PieceWriter Writer>
    std::string write_piece(const Reader& reader, Writer& writer, const std::string& filename) {
        add_piece_fields(reader, writer);
        return writer.write(filename);
    }

    template<typename Reader, TimeSeriesWriter Writer>
    std::string write_piece(const Reader& reader, Writer& writer, double time_step) {
        add_piece_fields(reader, writer);
        return writer.write(time_step);
    }

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
    if (reader.filename() == filename + writer.extension())
        throw GridFormat::IOError("Cannot read/write from/to the same file");
    if constexpr (Traits::WritesConnectivity<std::remove_cvref_t<decltype(writer)>>::value)
        grid.make_grid();
    return ConverterDetail::write_piece(reader, writer, filename);
}

/*!
 * \ingroup Grid
 * \brief Overload for time series formats.
 * \param reader A grid reader on which a file was opened.
 * \param factory A factory to construct a time series writer with the desired output format.
 * \param call_back (optional) A callback that is invoked after writing each step.
 */
template<std::derived_from<GridReader> Reader,
         ConverterDetail::TimeSeriesWriterFactory Factory,
         std::invocable<std::size_t, const std::string&> StepCallBack = decltype([] (std::size_t, const std::string&) {})>
std::string convert(Reader& reader,
                    const Factory& factory,
                    const StepCallBack& call_back = {}) {
    if (!reader.is_sequence())
        throw ValueError("Cannot convert data from reader to a sequence as the file read is no sequence.");

    ConverterDetail::ConverterGrid grid{reader};
    auto writer = factory(grid);
    std::string filename;
    for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
        reader.set_step(step);
        if constexpr (Traits::WritesConnectivity<std::remove_cvref_t<decltype(writer)>>::value)
            grid.make_grid();
        filename = ConverterDetail::write_piece(reader, writer, reader.time_at_step(step));
        call_back(step, filename);
    }
    return filename;
}

namespace Traits {

// to distinguish points/cells we use different integer types

template<>
struct Points<ConverterDetail::ConverterGrid> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid) {
        return std::views::iota(std::size_t{0}, grid.reader.number_of_points());
    }
};

template<>
struct Cells<ConverterDetail::ConverterGrid> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid) {
        const auto max = static_cast<std::int64_t>(grid.reader.number_of_cells());
        if (max < 0)
            throw TypeError("Integer overflow. Too many grid cells.");
        return std::views::iota(std::int64_t{0}, max);
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
struct CellPoints<ConverterDetail::ConverterGrid, std::int64_t> {
    static std::ranges::range auto get(const ConverterDetail::ConverterGrid& grid, const std::int64_t i) {
        return grid.cells.at(i).second | std::views::all;
    }
};

template<>
struct CellType<ConverterDetail::ConverterGrid, std::int64_t> {
    static GridFormat::CellType get(const ConverterDetail::ConverterGrid& grid, const std::int64_t i) {
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
struct NumberOfCellPoints<ConverterDetail::ConverterGrid, std::int64_t> {
    static std::size_t get(const ConverterDetail::ConverterGrid& grid, const std::int64_t i) {
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
    static std::array<std::size_t, 3> get(const ConverterDetail::ConverterGrid& grid, const std::size_t point) {
        return _get(Ranges::incremented(Extents<ConverterDetail::ConverterGrid>::get(grid), 1), point);
    }

    static std::array<std::size_t, 3> get(const ConverterDetail::ConverterGrid& grid, const std::int64_t cell) {
        return _get(Extents<ConverterDetail::ConverterGrid>::get(grid), cell);
    }

 private:
    static std::array<std::size_t, 3> _get(std::ranges::range auto extents, std::integral auto index) {
        // avoid zero extents
        std::ranges::for_each(extents, [] <std::integral T> (T& e) { e = std::max(e, T{1}); });
        const auto accumulate_until = [&] (int dim) {
            auto range = extents | std::views::take(dim);
            return std::accumulate(
                std::ranges::begin(range),
                std::ranges::end(range),
                std::size_t{1},
                std::multiplies{}
            );
        };

        const auto divisor1 = accumulate_until(1);
        const auto divisor2 = accumulate_until(2);
        return {
            index%divisor2%divisor1,
            index%divisor2/divisor1,
            index/divisor2
        };
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CONVERTER_HPP_
