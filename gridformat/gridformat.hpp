// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief This file is the entrypoint to the high-level API exposing all provided
 *        writers through a unified interface.
 *
 *        The individual writer can also be used directly, for instance, if one
 *        wants to minimize compile times. All available writers can be seen in
 *        the lists of includes of this file. For instructions on how to use the
 *        API, have a look at the readme or the provided examples.
 */
#ifndef GRIDFORMAT_GRIDFORMAT_HPP_
#define GRIDFORMAT_GRIDFORMAT_HPP_

#include <type_traits>

#include <gridformat/writer.hpp>
#include <gridformat/grid/image_grid.hpp>

#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/vts_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/time_series_writer.hpp>


#ifndef DOXYGEN
namespace GridFormat::APIDetail {
    class Unavailable {
        template<typename... Args>
        Unavailable(Args&&...) {
            throw NotImplemented("Required writer is not available due to missing dependency");
        }
    };
}  // namespace GridFormat::APIDetail

#if GRIDFORMAT_HAVE_HIGH_FIVE
#include <gridformat/vtk/hdf_writer.hpp>
inline constexpr bool _gfmt_api_have_high_five = true;
#else
inline constexpr bool _gfmt_api_have_high_five = false;
namespace GridFormat {

using VTKHDFWriter = APIDetail::Unavailable;
using VTKHDFImageGridWriter = APIDetail::Unavailable;
using VTKHDFUnstructuredGridWriter = APIDetail::Unavailable;

}  // namespace GridFormat
#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // DOXYGEN


namespace GridFormat {

//! Factory class to create a writer for the given file format
template<typename FileFormat> struct WriterFactory;

namespace FileFormat {

//! Base class for VTK-XML formats
template<typename VTKFormat>
struct VTKXMLFormatBase {
    //! Construct a new instance of this format with the given options
    constexpr auto operator()(VTK::XMLOptions opts) const {
        auto f = VTKFormat{};
        f.opts = std::move(opts);
        return f;
    }

    //! Construct a new instance of this format with the given options
    constexpr auto with(VTK::XMLOptions opts) const {
        return (*this)(std::move(opts));
    }

    //! Construct a new instance of this format with modified encoding option
    constexpr auto with_encoding(VTK::XML::Encoder e) const {
        auto opts = _cur_opts();
        Variant::unwrap_to(opts.encoder, e);
        return (*this)(std::move(opts));
    }

    //! Construct a new instance of this format with modified data format option
    constexpr auto with_data_format(VTK::XML::DataFormat f) const {
        auto opts = _cur_opts();
        Variant::unwrap_to(opts.data_format, f);
        return (*this)(std::move(opts));
    }

    //! Construct a new instance of this format with modified compression option
    constexpr auto with_compression(VTK::XML::Compressor c) const {
        auto opts = _cur_opts();
        Variant::unwrap_to(opts.compressor, c);
        return (*this)(std::move(opts));
    }

 private:
    VTK::XMLOptions _cur_opts() const {
        return static_cast<const VTKFormat&>(*this).opts;
    }
};


/*!
 * \ingroup API
 * \brief Selector for the .vti/.pvti image grid file format to be passed to the Writer.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#imagedata">here</a>
 *          or <a href="https://examples.vtk.org/site/VTKFileFormats/#pimagedata">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvti) is only available if MPI is found on the system.
 */
struct VTI : VTKXMLFormatBase<VTI> { VTK::XMLOptions opts = {}; };

/*!
 * \ingroup API
 * \brief Selector for the .vtr/.pvtr rectilinear grid file format to be passed to the Writer.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#rectilineargrid">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#prectilineargrid">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvtr) is only available if MPI is found on the system.
 */
struct VTR : VTKXMLFormatBase<VTR> { VTK::XMLOptions opts = {}; };

/*!
 * \ingroup API
 * \brief Selector for the .vts/.pvts structured grid file format to be passed to the Writer.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#structuredgrid">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#pstructuredgrid">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvts) is only available if MPI is found on the system.
 */
struct VTS : VTKXMLFormatBase<VTS> { VTK::XMLOptions opts = {}; };

/*!
 * \ingroup API
 * \brief Selector for the .vtp/.pvtp file format for two-dimensional unstructured grids.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#polydata">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#ppolydata">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvtp) is only available if MPI is found on the system.
 */
struct VTP : VTKXMLFormatBase<VTP> { VTK::XMLOptions opts = {}; };

/*!
 * \ingroup API
 * \brief Selector for the .vtu/.pvtu file format for general unstructured grids.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#unstructuredgrid">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#punstructuredgrid">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvtu) is only available if MPI is found on the system.
 */
struct VTU : VTKXMLFormatBase<VTU> { VTK::XMLOptions opts = {}; };

#if GRIDFORMAT_HAVE_HIGH_FIVE
/*!
 * \ingroup API
 * \brief Selector for the vtk-hdf file format for image grids.
 *        For more information, see
 *        <a href="https://examples.vtk.org/site/VTKFileFormats/#image-data">here</a>.
 * \note This file format is only available if HighFive is found on the system. If libhdf5 is found on the system,
 *       Highfive is automatically included when pulling the repository recursively, or, when using cmake's
 *       FetchContent mechanism.
 */
struct VTKHDFImage {};

/*!
 * \ingroup API
 * \brief Selector for the vtk-hdf file format for unstructured grids.
 *        For more information, see
 *        <a href="https://examples.vtk.org/site/VTKFileFormats/#unstructured-grid">here</a>.
 * \note This file format is only available if HighFive is found on the system. If libhdf5 is found on the system,
 *       Highfive is automatically included when pulling the repository recursively, or, when using cmake's
 *       FetchContent mechanism.
 */
struct VTKHDFUnstructured {};

/*!
 * \ingroup API
 * \brief Selector for the vtk-hdf file format with automatic deduction of the flavour.
 *        If the grid for which a writer is constructed is an image grid, it selects the image-grid flavour, otherwise
 *        it selects the flavour for unstructured grids (which requires the respective traits to be specialized).
 * \note This file format is only available if HighFive is found on the system. If libhdf5 is found on the system,
 *       Highfive is automatically included when pulling the repository recursively, or, when using cmake's
 *       FetchContent mechanism.
 */
struct VTKHDF {
    template<typename Grid>
    static constexpr auto from(const Grid&) {
        // TODO: Once the VTKHDFImageGridReader is stable, use image format for image grids
        return VTKHDFUnstructured{};
    }
};
#endif  // GRIDFORMAT_HAVE_HIGH_FIVE

/*!
 * \ingroup API
 * \brief Selector for the .pvd file format for a time series.
 *        For more information, see <a href="https://www.paraview.org/Wiki/ParaView/Data_formats#PVD_File_Format">here</a>.
 * \tparam PieceFormat The underlying file format used for each time step.
 * \note ParaView only supports reading .pvd series if the file format for pieces is one of the VTK-XML formats.
 */
template<typename PieceFormat>
struct PVD { PieceFormat piece_format; };

/*!
 * \ingroup API
 * \brief Convenience selector for a time series flavour of the given file format.
 *        This is unavailable if the given format does not provide a flavour for time series.
 * \tparam Format The file format for which a time-series flavour is requested.
 */
template<typename Format>
struct TimeSeries { Format format; };


#ifndef DOXYGEN
namespace Detail {

    template<typename F> struct IsVTKXMLFormat : public std::false_type {};
    template<> struct IsVTKXMLFormat<VTI> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTR> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTS> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTP> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTU> : public std::true_type {};

    struct PVDClosure {
        template<typename Format>
        constexpr auto operator()(const Format& f) const {
            static_assert(
                IsVTKXMLFormat<Format>::value,
                "The PVD format is only available with vtk-xml file formats"
            );
            return PVD<Format>{f};
        }
    };

    struct TimeSeriesClosure {
        template<typename Format>
        constexpr auto operator()(const Format& f) const {
            return TimeSeries<Format>{f};
        }
    };

}  // namespace Detail
#endif  // DOXYGEN

}  // namespace FileFormat


//! Specialization of the WriterFactory for the .vti format
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

//! Specialization of the WriterFactory for the .vtr format
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

//! Specialization of the WriterFactory for the .vts format
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

//! Specialization of the WriterFactory for the .vtp format
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

//! Specialization of the WriterFactory for the .vtu format
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

#if GRIDFORMAT_HAVE_HIGH_FIVE
//! Specialization of the WriterFactory for the vtk-hdf image grid format
template<> struct WriterFactory<FileFormat::VTKHDFImage> {
    static auto make(const FileFormat::VTKHDFImage&,
                     const Concepts::ImageGrid auto& grid) {
        static_assert(_gfmt_api_have_high_five, "HighFive is required for this file format");
        return VTKHDFImageGridWriter{grid};
    }
    static auto make(const FileFormat::VTKHDFImage&,
                     const Concepts::ImageGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return VTKHDFImageGridWriter{grid, comm};
    }
};

//! Specialization of the WriterFactory for the vtk-hdf unstructured grid format
template<> struct WriterFactory<FileFormat::VTKHDFUnstructured> {
    static auto make(const FileFormat::VTKHDFUnstructured&,
                     const Concepts::UnstructuredGrid auto& grid) {
        static_assert(_gfmt_api_have_high_five, "HighFive is required for this file format");
        return VTKHDFUnstructuredGridWriter{grid};
    }
    static auto make(const FileFormat::VTKHDFUnstructured&,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        static_assert(_gfmt_api_have_high_five, "HighFive is required for this file format");
        return VTKHDFUnstructuredGridWriter{grid, comm};
    }
};

//! Specialization of the WriterFactory for the vtk-hdf file format with automatic flavour selection.
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
#endif  // GRIDFORMAT_HAVE_HIGH_FIVE

//! Specialization of the WriterFactory for the .pvd time series format.
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

//! Specialization of the WriterFactory for the time series formats with automatic selection.
template<typename F>
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


// We place the format instances in a namespace different from FileFormats, in which
// the format types are defined above. Further below, we make these instances available
// in the GridFormat namespace directly. Having a separate namespace allows downstream ´
// projects to expose the format instances in their own namespace without having to expose
// all of GridFormat. Also, separating the format types from the instances further allows
// "hiding" the former and only expose the latter.
namespace Formats {

inline constexpr FileFormat::VTI vti;
inline constexpr FileFormat::VTR vtr;
inline constexpr FileFormat::VTS vts;
inline constexpr FileFormat::VTP vtp;
inline constexpr FileFormat::VTU vtu;
inline constexpr FileFormat::Detail::PVDClosure pvd;
inline constexpr FileFormat::Detail::TimeSeriesClosure time_series;

#if GRIDFORMAT_HAVE_HIGH_FIVE
inline constexpr FileFormat::VTKHDF vtk_hdf;
#endif

#ifndef DOXYGEN
namespace APIDetail { template<typename T> struct False : public std::false_type {}; }
#endif  // DOXYGEN

/*!
 * \ingroup API
 * \brief Selects a default format suitable to write the given grid
 * \tparam G The grid type for which to select a file format.
 */
template<Concepts::Grid G>
constexpr auto default_for() {
    if constexpr (Concepts::ImageGrid<G>)
        return vti;
    else if constexpr (Concepts::RectilinearGrid<G>)
        return vtr;
    else if constexpr (Concepts::StructuredGrid<G>)
        return vts;
    else if constexpr (Concepts::UnstructuredGrid<G>)
        return vtu;
    else
        static_assert(APIDetail::False<G>::value, "Cannot deduce a default format for the given grid");
}

/*!
 * \ingroup API
 * \brief Selects a default format suitable to write the given grid
 * \tparam G The grid type for which to select a file format.
 */
template<Concepts::Grid G>
constexpr auto default_for(const G&) {
    return default_for<G>();
}

}  // namespace Formats

// expose format instances
using namespace Formats;

// bring the format instances into the FileFormat namespace
namespace FileFormat { using namespace Formats; }

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRIDFORMAT_HPP_
