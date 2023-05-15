// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRIDFORMAT_HPP_
#define GRIDFORMAT_GRIDFORMAT_HPP_

#include <gridformat/writer.hpp>
#include <gridformat/grid/image_grid.hpp>

#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/vts_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/time_series_writer.hpp>

namespace GridFormat {

template<typename FileFormat>
struct WriterFactory;

namespace FileFormat {

#ifndef DOXYGEN
namespace Detail {

    template<typename VTKFormat>
    struct VTKXMLFormatBase {
        constexpr auto operator()(VTK::XMLOptions opts = {}) const {
            auto f = VTKFormat{};
            f.opts = std::move(opts);
            return f;
        }
    };

}  // namespace Detail
#endif  // DOXYGEN

struct VTI : Detail::VTKXMLFormatBase<VTI> { VTK::XMLOptions opts = {}; };
struct VTR : Detail::VTKXMLFormatBase<VTR> { VTK::XMLOptions opts = {}; };
struct VTS : Detail::VTKXMLFormatBase<VTS> { VTK::XMLOptions opts = {}; };
struct VTP : Detail::VTKXMLFormatBase<VTP> { VTK::XMLOptions opts = {}; };
struct VTU : Detail::VTKXMLFormatBase<VTU> { VTK::XMLOptions opts = {}; };

struct VTKHDFImage {};
struct VTKHDFUnstructured {};
struct VTKHDF {
    template<typename Grid>
    static constexpr auto from(const Grid&) {
        // TODO: Once the VTKHDFImageGridReader is stable, reduce image format for image grids
        return VTKHDFUnstructured{};
    }
};

template<typename PieceFormat>
struct PVD { PieceFormat piece_format; };

template<typename PieceFormat>
struct TimeSeries { PieceFormat piece_format; };


#ifndef DOXYGEN
namespace Detail {

    struct PVDClosure {
        template<typename Format>
        constexpr auto operator()(Format&& f) const {
            return PVD{std::forward<Format>(f)};
        }
    };

    template<typename F> struct IsVTKFormat : public std::false_type {};
    template<> struct IsVTKFormat<VTI> : public std::true_type {};
    template<> struct IsVTKFormat<VTR> : public std::true_type {};
    template<> struct IsVTKFormat<VTS> : public std::true_type {};
    template<> struct IsVTKFormat<VTP> : public std::true_type {};
    template<> struct IsVTKFormat<VTU> : public std::true_type {};
    template<> struct IsVTKFormat<VTKHDF> : public std::true_type {};
    template<> struct IsVTKFormat<VTKHDFImage> : public std::true_type {};
    template<> struct IsVTKFormat<VTKHDFUnstructured> : public std::true_type {};

    struct TimeSeriesClosure {
        template<typename Format> requires(IsVTKFormat<Format>::value)
        constexpr auto operator()(const Format& f) const {
            return TimeSeries{f};
        }
    };

}  // namespace Detail
#endif  // DOXYGEN

}  // namespace FileFormat


template<> struct WriterFactory<FileFormat::VTI> {
    static auto make(const FileFormat::VTI& format,
                     const Concepts::ImageGrid auto& grid) {
        return VTIWriter{grid, format.opts};
    }
    static auto make(const FileFormat::VTI& format,
                     const Concepts::ImageGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTIWriter{grid, comm, format.opts};
    }
};
template<> struct WriterFactory<FileFormat::VTR> {
    static auto make(const FileFormat::VTR& format,
                     const Concepts::RectilinearGrid auto& grid) {
        return VTRWriter{grid, format.opts};
    }
    static auto make(const FileFormat::VTR& format,
                     const Concepts::RectilinearGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTRWriter{grid, comm, format.opts};
    }
};
template<> struct WriterFactory<FileFormat::VTS> {
    static auto make(const FileFormat::VTS& format,
                     const Concepts::StructuredGrid auto& grid) {
        return VTSWriter{grid, format.opts};
    }
    static auto make(const FileFormat::VTS& format,
                     const Concepts::StructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTSWriter{grid, comm, format.opts};
    }
};
template<> struct WriterFactory<FileFormat::VTP> {
    static auto make(const FileFormat::VTP& format,
                     const Concepts::UnstructuredGrid auto& grid) {
        return VTPWriter{grid, format.opts};
    }
    static auto make(const FileFormat::VTP& format,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTPWriter{grid, comm, format.opts};
    }
};
template<> struct WriterFactory<FileFormat::VTU> {
    static auto make(const FileFormat::VTU& format,
                     const Concepts::UnstructuredGrid auto& grid) {
        return VTUWriter{grid, format.opts};
    }
    static auto make(const FileFormat::VTU& format,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTUWriter{grid, comm, format.opts};
    }
};
template<> struct WriterFactory<FileFormat::VTKHDFImage> {
    static auto make(const FileFormat::VTKHDFImage&,
                     const Concepts::ImageGrid auto& grid) {
        return VTKHDFImageGridWriter{grid};
    }
    static auto make(const FileFormat::VTKHDFImage&,
                     const Concepts::ImageGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return VTKHDFImageGridWriter{grid, comm};
    }
};
template<> struct WriterFactory<FileFormat::VTKHDFUnstructured> {
    static auto make(const FileFormat::VTKHDFUnstructured&,
                     const Concepts::UnstructuredGrid auto& grid) {
        return VTKHDFUnstructuredGridWriter{grid};
    }
    static auto make(const FileFormat::VTKHDFUnstructured&,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return VTKHDFUnstructuredGridWriter{grid, comm};
    }
};
template<> struct WriterFactory<FileFormat::VTKHDF> {
    static auto make(const FileFormat::VTKHDF& format,
                     const Concepts::Grid auto& grid) {
        return _make(format.from(grid), grid);
    }
    static auto make(const FileFormat::VTKHDF& format,
                     const Concepts::Grid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return _make(format.from(grid), grid, comm);
    }
 private:
    template<typename F, typename... Args>
    static auto _make(F&& format, Args&&... args) {
        return WriterFactory<F>::make(format, std::forward<Args>(args)...);
    }
};
template<typename F>
struct WriterFactory<FileFormat::PVD<F>> {
    static auto make(const FileFormat::PVD<F>& format,
                     const Concepts::Grid auto& grid,
                     const std::string& base_filename) {
        return PVDWriter{WriterFactory<F>::make(format.piece_format, grid), base_filename};
    }
    static auto make(const FileFormat::PVD<F>& format,
                     const Concepts::Grid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        return PVDWriter{WriterFactory<F>::make(format.piece_format, grid, comm), base_filename};
    }
};
template<typename F> requires(FileFormat::Detail::IsVTKFormat<F>::value)
struct WriterFactory<FileFormat::TimeSeries<F>> {
    static auto make(const FileFormat::TimeSeries<F>& format,
                     const Concepts::Grid auto& grid,
                     const std::string& base_filename) {
        return VTKTimeSeriesWriter{WriterFactory<F>::make(format.piece_format, grid), base_filename};
    }
    static auto make(const FileFormat::TimeSeries<F>& format,
                     const Concepts::Grid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        return VTKTimeSeriesWriter{WriterFactory<F>::make(format.piece_format, grid, comm), base_filename};
    }
};

inline constexpr FileFormat::VTI vti;
inline constexpr FileFormat::VTR vtr;
inline constexpr FileFormat::VTS vts;
inline constexpr FileFormat::VTP vtp;
inline constexpr FileFormat::VTU vtu;
inline constexpr FileFormat::VTKHDF vtk_hdf;
inline constexpr FileFormat::Detail::PVDClosure pvd;
inline constexpr FileFormat::Detail::TimeSeriesClosure time_series;

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRIDFORMAT_HPP_
