// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"
#include "../reader_tests.hpp"

#ifndef RUN_PARALLEL
#define RUN_PARALLEL 0
#endif

static constexpr bool is_parallel{RUN_PARALLEL};

std::string filename_prefix() {
    if constexpr (is_parallel)
        return "generic_parallel_converter_";
    return "generic_converter_";
}

template<typename Fmt, typename Grid, typename Communicator>
auto make_writer(const Fmt& format, const Grid& grid, [[maybe_unused]] const Communicator& comm) {
    if constexpr (is_parallel)
        return GridFormat::Writer{format, grid, comm};
    else
        return GridFormat::Writer{format, grid};
}

template<typename Grid, typename Format, typename Communicator>
std::string write(const Grid& grid,
                  const Format& fmt,
                  const std::string& suffix,
                  const Communicator& comm) {
    auto writer = make_writer(fmt, grid, comm);
    writer.set_point_field("pscalar", [&] (const auto& p) {
        return GridFormat::Test::test_function<double>(
            GridFormat::Test::evaluation_position(grid, p)
        );
    });
    const auto filename = writer.write("_no_regression_" + filename_prefix() + suffix + "_in");
    std::cout << "Wrote '" << filename << "'" << std::endl;
    return filename;
}

template<typename Grid, typename InFmt, typename OutFmt, typename Communicator>
void test(const Grid& grid,
          const InFmt& in_fmt,
          const OutFmt& out_fmt,
          const std::string& suffix,
          const Communicator& comm) {
    const auto in_filename = write(grid, in_fmt, suffix, comm);
    const auto out_filename = filename_prefix() + suffix + "_out_2d_in_2d";
    const auto converted = [&] () {
        if constexpr (is_parallel)
            return GridFormat::convert(in_filename, out_filename, GridFormat::ConversionOptions<OutFmt>{}, comm);
        else
            return GridFormat::convert(in_filename, out_filename, GridFormat::ConversionOptions<OutFmt>{});
    } ();
    std::cout << "Wrote '" << converted << "'" << std::endl;

    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    auto reader = [&] () {
        if constexpr (is_parallel)
            return GridFormat::Reader{out_fmt, comm};
        else
            return GridFormat::Reader{out_fmt};
    } ();
    reader.open(converted);
    expect(eq(reader.number_of_cells(), GridFormat::number_of_cells(grid)));
    expect(eq(reader.number_of_points(), GridFormat::number_of_points(grid)));
    expect(GridFormat::Test::test_field_values<2>(
        "pscalar", reader.point_field("pscalar"), grid, GridFormat::points(grid)
    ));
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
#if RUN_PARALLEL
    MPI_Init(&argc, &argv);
    const auto comm = MPI_COMM_WORLD;
#else
    const GridFormat::NullCommunicator comm{};
#endif

    const auto num_ranks = GridFormat::Parallel::size(comm);
    const auto rank = GridFormat::Parallel::rank(comm);
    if (is_parallel && num_ranks%2 != 0)
        throw std::runtime_error("This test requires the number of ranks to be divisible by 2");

    const double x_offset = static_cast<double>(rank%2);
    const double y_offset = static_cast<double>(rank/2);
    GridFormat::ImageGrid<2, double> grid{{x_offset, y_offset}, {1.0, 1.0}, {10, 15}};

    using GridFormat::Testing::operator""_test;
    "vtp_to_vtu"_test = [&] () { test(grid, GridFormat::vtp, GridFormat::vtu, "vtp_to_vtu", comm); };
    "vti_to_vtu"_test = [&] () { test(grid, GridFormat::vti, GridFormat::vtu, "vti_to_vtu", comm); };
    "vtr_to_vtu"_test = [&] () { test(grid, GridFormat::vtr, GridFormat::vtu, "vtr_to_vtu", comm); };

#if GRIDFORMAT_HAVE_HIGH_FIVE
    "vtu_to_vtk_hdf"_test = [&] () {
        test(grid, GridFormat::vtu, GridFormat::vtk_hdf, "vtu_to_vtk_hdf_unstructured", comm);
    };
    "vtk_hdf_to_vtu"_test = [&] () {
        test(grid, GridFormat::FileFormat::VTKHDFUnstructured{}, GridFormat::vtu, "vtk_hdf_unstructured_to_vtu", comm);
    };
#endif

    // test merging of sequential files into one parallel file
    if (is_parallel)
        "generic_converter_sequential_to_parallel"_test = [&] () {
            GridFormat::VTUWriter sequential_writer{grid};
            const auto seq_file = sequential_writer.write("_generic_converter_vtu_per_rank-" + std::to_string(rank));
            const auto converted_parallel_file = GridFormat::convert(
                seq_file,
                "generic_parallel_converter_sequential_files_to_parallel_file_2d_in_2d_out",
                GridFormat::ConversionOptions<GridFormat::FileFormat::VTU>{},
                comm
            );
            if (rank == 0)
                std::cout << "Wrote sequential file converted to parallel '" << converted_parallel_file << "'" << std::endl;
        };

    // test time series conversion
    const auto parallel_suffix = is_parallel ? std::string{"parallel_"} : std::string{""};
    const auto ts_in_filename = "generic_" + parallel_suffix + "ts_converter_in";
    auto ts_writer = is_parallel
        ? GridFormat::Writer{GridFormat::pvd, grid, comm, ts_in_filename}
        : GridFormat::Writer{GridFormat::pvd, grid, ts_in_filename};

    const std::string ts_filename = [&] () {
        std::string _ts_filename;
        for (unsigned int step = 0; step < 5; step++) {
            const auto time_step = static_cast<double>(step)*0.2;
            ts_writer.set_point_field("pscalar", [&] (const auto& p) {
                const auto eval_pos = GridFormat::Test::evaluation_position(grid, p);
                return GridFormat::Test::test_function<double>(eval_pos, time_step);
            });
            ts_writer.set_cell_field("cscalar", [&] (const auto& c) {
                const auto eval_pos = GridFormat::Test::evaluation_position(grid, c);
                return GridFormat::Test::test_function<double>(eval_pos, time_step);
            });
            _ts_filename = ts_writer.write(time_step);
        }
        return _ts_filename;
    } ();

    const auto convert_time_series_to = [&] <typename O> (const O&, const std::string& out_name) {
        return is_parallel
            ? GridFormat::convert(ts_filename, out_name, GridFormat::ConversionOptions<O>{}, comm)
            : GridFormat::convert(ts_filename, out_name, GridFormat::ConversionOptions<O>{});
    };

    const auto ts_converted_filename = convert_time_series_to(
        GridFormat::pvd_with(GridFormat::vtu),
        "generic_" + parallel_suffix + "time_series_converter_2d_in_2d"
    );
    if (rank == 0)
        std::cout << "Wrote converted time series to '" << ts_converted_filename << "'" << std::endl;

    // test automatic format conversion to time series when non-time-series format is given
    const auto ts_auto_converted_filename = convert_time_series_to(
        GridFormat::vtu,
        "generic_" + parallel_suffix + "automatic_time_series_converter_2d_in_2d"
    );
    if (rank == 0)
        std::cout << "Wrote converted (automatic) time series to '" << ts_auto_converted_filename << "'" << std::endl;

    // test automatic time series when no format is given
    const auto ts_any_converted_filename = convert_time_series_to(
        GridFormat::any,
        "generic_" + parallel_suffix + "any_time_series_converter_2d_in_2d"
    );
    if (rank == 0)
        std::cout << "Wrote converted (to any format) time series to '" << ts_any_converted_filename << "'" << std::endl;

#if RUN_PARALLEL
    MPI_Finalize();
#endif

    return 0;
}
