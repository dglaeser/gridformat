// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Common functionality for writing VTK HDF files.
 */
#ifndef GRIDFORMAT_VTK_HDF_COMMON_HPP_
#define GRIDFORMAT_VTK_HDF_COMMON_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <type_traits>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <highfive/H5Easy.hpp>
#include <highfive/H5File.hpp>
#pragma GCC diagnostic pop

#if GRIDFORMAT_HAVE_MPI
#include <mpi.h>
namespace GridFormat::VTKHDFDetail {
    using _MPICommunicator = MPI_Comm;

    template<typename T>
    static constexpr bool is_mpi_comm = std::is_same_v<T, MPI_Comm>;
}
#else
namespace GridFormat::VTKHDFDetail {
    using _MPICommunicator = NullCommunicator;

    template<typename T>
    static constexpr bool is_mpi_comm = false;
}
#endif

#include <gridformat/common/logging.hpp>
#include <gridformat/grid/grid.hpp>

namespace GridFormat::VTKHDF {

struct DataSetSlice {
    std::vector<std::size_t> size;
    std::vector<std::size_t> offset;
    std::vector<std::size_t> count;
};

struct DataSetPath {
    std::string group_path;
    std::string dataset_name;
};

struct IOContext {
    const int my_rank;
    const int num_ranks;
    const bool is_parallel;
    const std::vector<std::size_t> rank_cells;
    const std::vector<std::size_t> rank_points;
    const std::size_t num_cells_total;
    const std::size_t num_points_total;
    const std::size_t my_cell_offset;
    const std::size_t my_point_offset;

    IOContext(int _my_rank,
              int _num_ranks,
              std::vector<std::size_t> _rank_cells,
              std::vector<std::size_t> _rank_points)
    : my_rank{_my_rank}
    , num_ranks{_num_ranks}
    , is_parallel{_num_ranks > 1}
    , rank_cells{std::move(_rank_cells)}
    , rank_points{std::move(_rank_points)}
    , num_cells_total{_accumulate(rank_cells)}
    , num_points_total{_accumulate(rank_points)}
    , my_cell_offset{_accumulate(rank_cells.begin(), std::next(rank_cells.begin(), my_rank))}
    , my_point_offset{_accumulate(rank_points.begin(), std::next(rank_points.begin(), my_rank))} {
        if (my_rank >= num_ranks)
            throw ValueError("Given rank is not within communicator size");
        if (num_ranks != static_cast<int>(rank_cells.size()))
            throw ValueError("Cells vector does not match communicator size");
        if (num_ranks != static_cast<int>(rank_points.size()))
            throw ValueError("Points vector does not match communicator size");
    }

    template<Concepts::Grid Grid, Concepts::Communicator Communicator>
    static IOContext from(const Grid& grid, const Communicator& comm, int root_rank = 0) {
        const int size = Parallel::size(comm);
        const int rank = Parallel::rank(comm);
        const std::size_t num_points = number_of_points(grid);
        const std::size_t num_cells = number_of_cells(grid);
        if (size == 1)
            return IOContext{rank, size, std::vector{num_cells}, std::vector{num_points}};

        const auto all_num_points = Parallel::gather(comm, num_points, root_rank);
        const auto all_num_cells = Parallel::gather(comm, num_cells, root_rank);
        const auto my_all_num_points = Parallel::broadcast(comm, all_num_points, root_rank);
        const auto my_all_num_cells = Parallel::broadcast(comm, all_num_cells, root_rank);
        return IOContext{rank, size, my_all_num_cells, my_all_num_points};
    }

 private:
    std::size_t _accumulate(const std::vector<std::size_t>& in) const {
        return _accumulate(in.begin(), in.end());
    }

    template<typename I>
    std::size_t _accumulate(I begin, I end) const {
        return std::accumulate(begin, end, std::size_t{0});
    }
};

// Custom string data type using ascii encoding: VTKHDF uses ascii, but HighFive uses UTF-8.
struct AsciiString : public HighFive::DataType {
    explicit AsciiString(std::size_t n) {
        _hid = H5Tcopy(H5T_C_S1);
        if (H5Tset_size(_hid, n) < 0) {
            HighFive::HDF5ErrMapper::ToException<HighFive::DataTypeException>(
                "Unable to define datatype size to " + std::to_string(n)
            );
        }
        // define encoding to ASCII
        H5Tset_cset(_hid, H5T_CSET_ASCII);
        H5Tset_strpad(_hid, H5T_STR_SPACEPAD);
    }

    template<std::size_t N>
    static AsciiString from(const char (&input)[N]) {
        if (input[N-1] == '\0')
            return AsciiString{N-1};
        return AsciiString{N};
    }

    static AsciiString from(const std::string& n) {
        return AsciiString{n.size()};
    }
};

template<Concepts::Communicator Communicator>
auto open_file(const std::string& filename,
               const Communicator& communicator,
               const IOContext& context) { // TODO: avoid context...
    if (!context.is_parallel)
        return HighFive::File{filename, HighFive::File::Overwrite};

    if constexpr (!VTKHDFDetail::is_mpi_comm<Communicator>)
        throw TypeError("Only MPI_Comm can be used as communicator for parallel HDF5 I/O");
    else {
        HighFive::FileAccessProps fapl;
        fapl.add(HighFive::MPIOFileAccess{communicator, MPI_INFO_NULL});
        fapl.add(HighFive::MPIOCollectiveMetadata{});
        return HighFive::File{filename, HighFive::File::Overwrite, fapl};
    }
}

void check_successful_collective_io(const HighFive::DataTransferProps& xfer_props) {
    auto mnccp = HighFive::MpioNoCollectiveCause(xfer_props);
    if (mnccp.getLocalCause() || mnccp.getGlobalCause())
        log_warning(
            std::string{"The operation was successful, but couldn't use collective MPI-IO. "}
            + "Local cause: " + std::to_string(mnccp.getLocalCause()) + " "
            + "Global cause:" + std::to_string(mnccp.getGlobalCause()) + "\n"
        );
}

}  // namespace GridFormat::VTKHDF

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_COMMON_HPP_
