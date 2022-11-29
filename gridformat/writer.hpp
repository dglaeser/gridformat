// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_WRITER_HPP_
#define GRIDFORMAT_WRITER_HPP_

#include <cmath>
#include <memory>
#include <concepts>
#include <type_traits>
#include <ranges>
#include <utility>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/cell_type.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/parallel/concepts.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvd_writer.hpp>

namespace GridFormat {

enum class FileFormat {
    vtu,
    vtp
};


template<Concepts::Grid Grid>
class Writer {
 public:
    template<std::derived_from<GridWriter<Grid>> W> requires(!std::is_lvalue_reference_v<W>)
    explicit Writer(W&& writer)
    : _writer(std::make_unique<W>(std::move(writer)))
    {}

    template<typename... Args>
    void set_point_field(Args&&... args) {
        _writer->set_point_field(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void set_cell_field(Args&&... args) {
        _writer->set_cell_field(std::forward<Args>(args)...);
    }

    void write(const std::string& filename) {
        _writer->write(filename);
    }

 private:
    std::unique_ptr<GridWriter<Grid>> _writer;
};


template<Concepts::Grid Grid>
class TimeSeriesWriter {
 public:
    template<std::derived_from<TimeSeriesGridWriter<Grid>> W> requires(!std::is_lvalue_reference_v<W>)
    explicit TimeSeriesWriter(W&& writer)
    : _writer(std::make_unique<W>(std::move(writer)))
    {}

    template<typename... Args>
    void set_point_field(Args&&... args) {
        _writer->set_point_field(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void set_cell_field(Args&&... args) {
        _writer->set_cell_field(std::forward<Args>(args)...);
    }

    void write(double t) {
        _writer->write(t);
    }

 private:
    std::unique_ptr<TimeSeriesGridWriter<Grid>> _writer;
};


#ifndef DOXYGEN
namespace Detail {

template<Concepts::UnstructuredGrid Grid>
inline int max_cell_dim(const Grid& grid) {
    int max_dim = 0;
    std::ranges::for_each(cells(grid), [&] (const auto& cell) {
        max_dim = std::max(dimension(type(grid, cell)), max_dim);
    });
    return max_dim;
}

}  // namespace Detail
#endif  // DOXYGEN


//! Return a default writer for an unstructured grid
template<Concepts::UnstructuredGrid Grid>
Writer<Grid> make_writer(const Grid& grid) {
    if (Detail::max_cell_dim(grid) > 2)
        return Writer<Grid>{VTUWriter{grid}};
    return Writer<Grid>{VTPWriter{grid}};
}


//! Return a writer for the given unstructured grid
template<Concepts::UnstructuredGrid Grid>
Writer<Grid> make_writer(const Grid& grid, FileFormat format) {
    if (format == FileFormat::vtp) {
        if (Detail::max_cell_dim(grid) > 2)
            as_warning("vtp file format only supports up to 2d cells. 3d cells will be omitted");
        return Writer<Grid>{VTPWriter{grid}};
    }
    if (format == FileFormat::vtu)
        return Writer<Grid>{VTUWriter{grid}};

    throw InvalidState(as_error("Factory function not yet available for given file format"));
}


//! Return a default parallel writer for the given unstructured grid & communicator
template<Concepts::UnstructuredGrid Grid, Concepts::Communicator Comm>
Writer<Grid> make_writer(const Grid& grid, const Comm& comm) {
    return Writer<Grid>{PVTUWriter{grid, comm}};
}


//! Return a parallel writer for the given unstructured grid & communicator
template<Concepts::UnstructuredGrid Grid, Concepts::Communicator Comm>
Writer<Grid> make_writer(const Grid& grid, const Comm& comm, FileFormat format) {
    if (format == FileFormat::vtu)
        return Writer<Grid>{PVTUWriter{grid, comm}};
    throw InvalidState(as_error("Factory function not yet available for given file format"));
}


//! Return a default time series writer for unstructured grids
template<Concepts::UnstructuredGrid Grid>
TimeSeriesWriter<Grid> make_time_series_writer(const std::string& base_filename, const Grid& grid) {
    if (Detail::max_cell_dim(grid) > 2)
        return TimeSeriesWriter<Grid>{PVDWriter{VTUWriter{grid}, base_filename}};
    return TimeSeriesWriter<Grid>{PVDWriter{VTPWriter{grid}, base_filename}};
}


//! Return a time series writer
template<Concepts::Grid Grid>
TimeSeriesWriter<Grid> make_time_series_writer(const std::string& base_filename,
                                               const Grid& grid,
                                               FileFormat format) {
    throw NotImplemented(as_error("TimeSeriesWriter Factory function with format specification not yet available"));
}


//! Return a default parallel time series writer for unstructured grids
template<Concepts::UnstructuredGrid Grid, Concepts::Communicator Comm>
TimeSeriesWriter<Grid> make_time_series_writer(const std::string& base_filename, const Grid& grid, const Comm& comm) {
    return TimeSeriesWriter<Grid>{PVDWriter{PVTUWriter{grid, comm}, base_filename}};
}


//! Return a parallel time series writer
template<Concepts::Grid Grid, Concepts::Communicator Comm>
TimeSeriesWriter<Grid> make_time_series_writer(const std::string& base_filename,
                                               const Grid& grid,
                                               const Comm& comm,
                                               FileFormat format) {
    throw NotImplemented(as_error("TimeSeriesWriter Factory function with format specification not yet available"));
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_WRITER_HPP_
