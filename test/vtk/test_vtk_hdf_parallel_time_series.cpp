// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <cstddef>

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    bool verbose = rank == 0;

    {
        const auto grid = GridFormat::Test::make_unstructured<2, 2>();
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid,
            MPI_COMM_WORLD,
            "pvtk_hdf_time_series_2d_in_2d_unstructured"
        };
        GridFormat::Test::write_test_time_series<2>(writer, 5, {}, verbose);
    }

    {
        const double xoffset = static_cast<double>(rank%2);
        const double yoffset = static_cast<double>(rank/2);
        GridFormat::Test::StructuredGrid<2> structured_grid{
            {1.0, 1.0},
            {5, 7},
            {xoffset, yoffset}
        };
        GridFormat::VTKHDFTimeSeriesWriter writer{
            structured_grid,
            MPI_COMM_WORLD,
            "pvtk_hdf_time_series_2d_in_2d_image"
        };
        GridFormat::Test::write_test_time_series<2>(writer, 5, {}, verbose);
    }

    const auto text_at = [] (std::size_t step) { return "step_" + std::to_string(step); };
    const auto check_transient_strings = [&] (auto&& writer, const std::string& filename) {
        for (std::size_t step = 0; step < 3; ++step) {
            writer.set_meta_data("string", text_at(step));
            writer.write(static_cast<double>(step));
        }
        if (rank == 0) {
            GridFormat::VTKHDFReader reader;
            reader.open(filename);
            if (reader.number_of_steps() != 3)
                throw GridFormat::ValueError("Unexpected number of steps in " + filename);
            for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
                reader.set_step(step);
                const auto value = reader.meta_data_field("string")->template export_to<std::string>();
                if (value != text_at(step))
                    throw GridFormat::ValueError("Unexpected string meta data in " + filename + ": " + value);
            }
        }
    };

    {
        const auto grid = GridFormat::Test::make_unstructured<2, 2>();
        check_transient_strings(GridFormat::VTKHDFTimeSeriesWriter{
            grid,
            MPI_COMM_WORLD,
            "pvtk_hdf_strings_2d_in_2d_unstructured",
            {.static_grid = true, .static_meta_data = false}
        }, "pvtk_hdf_strings_2d_in_2d_unstructured.hdf");
    }

    {
        GridFormat::Test::StructuredGrid<2> structured_grid{
            {1.0, 1.0},
            {5, 7},
            {static_cast<double>(rank%2), static_cast<double>(rank/2)}
        };
        check_transient_strings(GridFormat::VTKHDFTimeSeriesWriter{
            structured_grid,
            MPI_COMM_WORLD,
            "pvtk_hdf_strings_2d_in_2d_image",
            {.static_grid = true, .static_meta_data = false}
        }, "pvtk_hdf_strings_2d_in_2d_image.hdf");
    }

    MPI_Finalize();
    return 0;
}
