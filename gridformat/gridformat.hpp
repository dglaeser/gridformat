// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup API
 * \brief This file is the entrypoint to the high-level API exposing all provided
 *        readers/writers through a unified interface.
 *
 *        The individual writer can also be used directly, for instance, if one
 *        wants to minimize compile times. All available writers can be seen in
 *        the lists of includes of this file. For instructions on how to use the
 *        API, have a look at the readme or the provided examples.
 */
#ifndef GRIDFORMAT_GRIDFORMAT_HPP_
#define GRIDFORMAT_GRIDFORMAT_HPP_

#include <memory>
#include <optional>
#include <type_traits>

#include <gridformat/reader.hpp>
#include <gridformat/writer.hpp>

#include <gridformat/grid/converter.hpp>
#include <gridformat/grid/image_grid.hpp>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/parallel/communication.hpp>

#include <gridformat/vtk/vti_reader.hpp>
#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>

#include <gridformat/vtk/vtr_reader.hpp>
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>

#include <gridformat/vtk/vts_reader.hpp>
#include <gridformat/vtk/vts_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>

#include <gridformat/vtk/vtp_reader.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtu_reader.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include <gridformat/vtk/pvd_reader.hpp>
#include <gridformat/vtk/pvd_writer.hpp>

#include <gridformat/vtk/xml_time_series_writer.hpp>

#ifndef DOXYGEN
namespace GridFormat::APIDetail {

    template<typename... T>
    inline constexpr bool always_false = false;

    template<typename... T>
    struct DefaultAsserter {
        static constexpr bool do_assert() {
            static_assert(
                always_false<T...>,
                "\033[1m\033[31mRequested reader/writer is unavailable due to missing dependency\033[0"
            );
            return false;
        }
    };

    // Derive from GridWriter to make this a "valid" writer (and abuse some valid grid impl for that)
    template<template<typename...> typename Asserter = DefaultAsserter>
    class UnavailableWriter : public GridWriter<ImageGrid<2, double>> {
     public:
        template<typename... Args>
        UnavailableWriter(Args&&...) { static_assert(Asserter<Args...>::do_assert()); }
     private:
        void _throw() const { throw GridFormat::NotImplemented("Writer unavailable"); }
        void _write(std::ostream&) const { _throw(); }
    };

    // Derive from Reader to make this a "valid" reader
    template<template<typename...> typename Asserter = DefaultAsserter>
    class UnavailableReader : public GridReader {
     public:
        template<typename... Args>
        UnavailableReader(Args&&...) { static_assert(Asserter<Args...>::do_assert()); }
     private:
        void _throw() const { throw GridFormat::NotImplemented("Reader unavailable"); }
        std::string _name() const override { _throw(); return ""; }
        void _open(const std::string&, typename GridReader::FieldNames&) override { _throw(); }
        void _close() override { _throw(); }
        std::size_t _number_of_cells() const override { _throw(); return 0; }
        std::size_t _number_of_points() const override { _throw(); return 0; }
        FieldPtr _cell_field(std::string_view) const override { _throw(); return nullptr; }
        FieldPtr _point_field(std::string_view) const override { _throw(); return nullptr; }
        FieldPtr _meta_data_field(std::string_view) const override { _throw(); return nullptr; }
        bool _is_sequence() const override { _throw(); return false; }
    };

}  // namespace GridFormat::APIDetail

#if GRIDFORMAT_HAVE_HIGH_FIVE
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/hdf_reader.hpp>
inline constexpr bool _gfmt_api_have_high_five = true;
#else
#include <gridformat/vtk/hdf_common.hpp>
inline constexpr bool _gfmt_api_have_high_five = false;
namespace GridFormat {

template<typename... T>
struct VTKHDFAsserter {
    static constexpr bool do_assert() {
        static_assert(
            APIDetail::always_false<T...>,
            "\033[1m\033[31mVTKHDF reader/writers require HighFive.\033[0"
        );
        return false;
    }
};

using VTKHDFWriter = APIDetail::UnavailableWriter<VTKHDFAsserter>;
using VTKHDFTimeSeriesWriter = APIDetail::UnavailableWriter<VTKHDFAsserter>;
using VTKHDFImageGridWriter = APIDetail::UnavailableWriter<VTKHDFAsserter>;
using VTKHDFUnstructuredGridWriter = APIDetail::UnavailableWriter<VTKHDFAsserter>;
using VTKHDFImageGridTimeSeriesWriter = APIDetail::UnavailableWriter<VTKHDFAsserter>;
using VTKHDFUnstructuredTimeSeriesWriter = APIDetail::UnavailableWriter<VTKHDFAsserter>;

using VTKHDFImageGridReader = APIDetail::UnavailableReader<VTKHDFAsserter>;
template<typename...> using VTKHDFUnstructuredGridReader = APIDetail::UnavailableReader<VTKHDFAsserter>;
template<typename...> using VTKHDFReader = APIDetail::UnavailableReader<VTKHDFAsserter>;

}  // namespace GridFormat
#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // DOXYGEN


namespace GridFormat {

//! Null communicator that can be used e.g. to read parallel file formats sequentially
inline constexpr GridFormat::NullCommunicator null_communicator;

//! Factory class to create a writer for the given file format
template<typename FileFormat> struct WriterFactory;

//! Factory class to create a reader for the given file format
template<typename FileFormat> struct ReaderFactory;

namespace FileFormat {

//! Base class for formats taking options
template<typename Format, typename Opts>
struct FormatWithOptions {
    using Options = Opts;
    std::optional<Options> opts = {};

    //! Construct a new instance of this format with the given options
    constexpr Format operator()(Options _opts) const {
        return with(std::move(_opts));
    }

    //! Construct a new instance of this format with the given options
    constexpr Format with(Options _opts) const {
        Format f;
        f.opts = std::move(_opts);
        return f;
    }
};

//! Base class for VTK-XML formats
template<typename VTKFormat>
struct VTKXMLFormatBase : public FormatWithOptions<VTKFormat, VTK::XMLOptions> {
    //! Construct a new instance of this format with modified encoding option
    constexpr auto with_encoding(VTK::XML::Encoder e) const {
        auto cur_opts = this->opts.value_or(VTK::XMLOptions{});
        Variant::unwrap_to(cur_opts.encoder, e);
        return this->with(std::move(cur_opts));
    }

    //! Construct a new instance of this format with modified data format option
    constexpr auto with_data_format(VTK::XML::DataFormat f) const {
        auto cur_opts = this->opts.value_or(VTK::XMLOptions{});
        Variant::unwrap_to(cur_opts.data_format, f);
        return this->with(std::move(cur_opts));
    }

    //! Construct a new instance of this format with modified compression option
    constexpr auto with_compression(VTK::XML::Compressor c) const {
        auto cur_opts = this->opts.value_or(VTK::XMLOptions{});
        Variant::unwrap_to(cur_opts.compressor, c);
        return this->with(std::move(cur_opts));
    }
};


/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for an unspecified file format. When using this format, GridFormat will
 *        automatically select a format or deduce from a file being read, for instance.
 */
struct Any {};


/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Time series variant of the Any format selector
 */
struct AnyTimeSeries {};


/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the .vti/.pvti image grid file format to be passed to the Writer.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#imagedata">here</a>
 *          or <a href="https://examples.vtk.org/site/VTKFileFormats/#pimagedata">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvti) is only available if MPI is found on the system.
 */
struct VTI : VTKXMLFormatBase<VTI> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the .vtr/.pvtr rectilinear grid file format to be passed to the Writer.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#rectilineargrid">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#prectilineargrid">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvtr) is only available if MPI is found on the system.
 */
struct VTR : VTKXMLFormatBase<VTR> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the .vts/.pvts structured grid file format to be passed to the Writer.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#structuredgrid">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#pstructuredgrid">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvts) is only available if MPI is found on the system.
 */
struct VTS : VTKXMLFormatBase<VTS> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the .vtp/.pvtp file format for two-dimensional unstructured grids.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#polydata">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#ppolydata">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvtp) is only available if MPI is found on the system.
 */
struct VTP : VTKXMLFormatBase<VTP> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the .vtu/.pvtu file format for general unstructured grids.
 * \details For more details on the file format, see
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#unstructuredgrid">here</a> or
 *          <a href="https://examples.vtk.org/site/VTKFileFormats/#punstructuredgrid">here</a>
 *          for the parallel variant.
 * \note The parallel variant (.pvtu) is only available if MPI is found on the system.
 */
struct VTU : VTKXMLFormatBase<VTU> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for a time series of any VTK-XML format.
 */
template<typename VTX>
struct VTKXMLTimeSeries : VTKXMLFormatBase<VTKXMLTimeSeries<VTX>> {};


/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the vtk-hdf file format for image grids.
 *        For more information, see
 *        <a href="https://examples.vtk.org/site/VTKFileFormats/#image-data">here</a>.
 * \note This file format is only available if HighFive is found on the system. If libhdf5 is found on the system,
 *       Highfive is automatically included when pulling the repository recursively, or, when using cmake's
 *       FetchContent mechanism.
 * \note A bug in VTK/ParaView related to reading cell data arrays has been fixed in
 *       <a href="https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10147">VTK merge request 10147</a>.
 *       The fix is included in VTK>9.2.6 and ParaView>5.11.0.
 */
struct VTKHDFImage {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Transient variant of the vtk-hdf image data format
 */
struct VTKHDFImageTransient
: FormatWithOptions<VTKHDFImageTransient, VTK::HDFTransientOptions> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
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
 * \ingroup FileFormats
 * \brief Transient variant of the vtk-hdf unstructured grid format
 */
struct VTKHDFUnstructuredTransient
: FormatWithOptions<VTKHDFUnstructuredTransient, VTK::HDFTransientOptions> {};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the transient vtk-hdf file format with automatic deduction of the flavour.
 *        If the grid for which a writer is constructed is an image grid, it selects the image-grid flavour, otherwise
 *        it selects the flavour for unstructured grids (which requires the respective traits to be specialized).
 * \note This file format is only available if HighFive is found on the system. If libhdf5 is found on the system,
 *       Highfive is automatically included when pulling the repository recursively, or, when using cmake's
 *       FetchContent mechanism.
 */
struct VTKHDFTransient : FormatWithOptions<VTKHDFTransient, VTK::HDFTransientOptions> {
    template<typename Grid>
    constexpr auto from(const Grid&) const {
        // TODO: Once the VTKHDFImageGridReader is stable, use image format for image grids
        return VTKHDFUnstructuredTransient{this->opts};
    }
};

/*!
 * \ingroup API
 * \ingroup FileFormats
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

    //! Return the transient variant of this format with the given options
    constexpr VTKHDFTransient with(VTK::HDFTransientOptions opts) {
        return {std::move(opts)};
    }
};

#ifndef DOXYGEN
namespace Detail {

    template<typename F> struct IsVTKXMLFormat : public std::false_type {};
    template<> struct IsVTKXMLFormat<VTI> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTR> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTS> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTP> : public std::true_type {};
    template<> struct IsVTKXMLFormat<VTU> : public std::true_type {};

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Selector for the .pvd file format for a time series.
 *        For more information, see <a href="https://www.paraview.org/Wiki/ParaView/Data_formats#PVD_File_Format">here</a>.
 * \tparam PieceFormat The underlying file format used for each time step.
 * \note ParaView only supports reading .pvd series if the file format for pieces is one of the VTK-XML formats.
 */
template<typename PieceFormat = Any>
struct PVD {
    PieceFormat piece_format;
};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Closure for selecting the .pvd file format. Takes a VTK-XML format and returns an instance of PVD.
 */
struct PVDClosure {
    template<typename Format>
    constexpr auto operator()(const Format& f) const {
        static_assert(
            Detail::IsVTKXMLFormat<Format>::value,
            "The PVD format is only available with vtk-xml file formats"
        );
        return PVD<Format>{f};
    }
};

/*!
 * \ingroup API
 * \ingroup FileFormats
 * \brief Closure for time series format selection. Takes a sequential format and returns a time series variant.
 */
struct TimeSeriesClosure {
    template<typename Format>
    constexpr auto operator()(const Format& f) const {
        if constexpr (Detail::IsVTKXMLFormat<Format>::value)
            return VTKXMLTimeSeries<Format>{f.opts};
        else if constexpr (std::same_as<VTKHDFImage, Format>)
            return VTKHDFImageTransient{};
        else if constexpr (std::same_as<VTKHDFUnstructured, Format>)
            return VTKHDFUnstructuredTransient{};
        else if constexpr (std::same_as<VTKHDF, Format>)
            return VTKHDFTransient{};
        else if constexpr (std::same_as<Any, Format>)
            return AnyTimeSeries{};
        else {
            static_assert(
                APIDetail::always_false<Format>,
                "Cannot create a time series variant for the given format. This means the format does not "
                "define a variant for time series, or, the selected format is already time series format."
            );
        }
    }
};

}  // namespace FileFormat


#ifndef DOXYGEN
namespace APIDetail {

    // Meta-Reader for file formats that have different format flavours in parallel/sequential
    template<typename SeqReader, typename ParReader>
    class SequentialOrParallelReader : public GridReader {
        template<typename R> using CommunicatorAccess = Traits::CommunicatorAccess<R>;
        template<typename R> using Communicator = decltype(CommunicatorAccess<R>::get(std::declval<const R&>()));
        using ParCommunicator = Communicator<ParReader>;
        using SeqCommunicator = Communicator<SeqReader>;

     public:
        SequentialOrParallelReader(SeqReader&& seq, ParReader&& par)
        : _seq_reader{std::move(seq)}
        , _par_reader{std::move(par)}
        {}

        ParCommunicator communicator() const {
            const bool is_parallel = !_use_sequential.value_or(true);
            if (!is_parallel) {
                if constexpr (!std::is_same_v<ParCommunicator, SeqCommunicator>)
                    throw InvalidState("Cannot access communicator for sequential variant of the reader");
                else
                    return CommunicatorAccess<SeqReader>::get(_seq_reader);
            }
            return CommunicatorAccess<ParReader>::get(_par_reader);
        }

     private:
        SeqReader _seq_reader;
        ParReader _par_reader;
        std::optional<bool> _use_sequential = {};

        std::string _name() const override {
            return _is_set() ? _access().name() : std::string{"undefined"};
        }

        void _open(const std::string& filename, typename GridReader::FieldNames& names) override {
            _close();
            std::string seq_error;
            try {
                _seq_reader.open(filename);
                _use_sequential = true;
            } catch (const std::exception& e) {
                seq_error = e.what();
                _use_sequential.reset();
                _seq_reader.close();
                try {
                    _par_reader.open(filename);
                    _use_sequential = false;
                } catch (const std::exception& par_err) {
                    _use_sequential.reset();
                    _par_reader.close();
                    throw IOError(
                        "Could not read '" + filename + "' neither as sequential nor parallel format.\n"
                        "Error with sequential format (" + _seq_reader.name() + "): " + seq_error + "\n"
                        "Error with parallel format (" + _par_reader.name() + "): " + par_err.what()
                    );
                }
            }

            ReaderDetail::copy_field_names(_access(), names);
        }

        void _close() override {
            if (_is_set())
                _access().close();
        }

        std::size_t _number_of_cells() const override { return _access().number_of_cells(); }
        std::size_t _number_of_points() const override { return _access().number_of_points(); }
        std::size_t _number_of_pieces() const override { return _access().number_of_pieces(); }

        FieldPtr _cell_field(std::string_view n) const override { return _access().cell_field(n); }
        FieldPtr _point_field(std::string_view n) const override { return _access().point_field(n); }
        FieldPtr _meta_data_field(std::string_view n) const override { return _access().meta_data_field(n); }

        FieldPtr _points() const override { return _access().points(); }
        void _visit_cells(const typename GridReader::CellVisitor& v) const override { _access().visit_cells(v); }
        std::vector<double> _ordinates(unsigned int i) const { return _access().ordinates(i); }

        typename GridReader::PieceLocation _location() const override { return _access().location(); }
        typename GridReader::Vector _spacing() const override { return _access().spacing(); }
        typename GridReader::Vector _origin() const override { return _access().origin(); }
        typename GridReader::Vector _basis_vector(unsigned int i) const override { return _access().basis_vector(i); }

        bool _is_sequence() const override { return _access().is_sequence(); }
        std::size_t _number_of_steps() const override { return _access().number_of_steps(); }
        double _time_at_step(std::size_t i) const override { return _access().time_at_step(i); }
        void _set_step(std::size_t i, typename GridReader::FieldNames& names) override {
            _access().set_step(i);
            names.clear();
            ReaderDetail::copy_field_names(_access(), names);
        }

        bool _is_set() const { return _use_sequential.has_value(); }
        void _throw_if_not_set() const {
            if (!_is_set())
                throw IOError("No data has been read");
        }

        GridReader& _access() {
            _throw_if_not_set();
            return _use_sequential.value()
                ? static_cast<GridReader&>(_seq_reader)
                : static_cast<GridReader&>(_par_reader);
        }

        const GridReader& _access() const {
            _throw_if_not_set();
            return _use_sequential.value()
                ? static_cast<const GridReader&>(_seq_reader)
                : static_cast<const GridReader&>(_par_reader);
        }
    };

    template<typename S, typename P>
    SequentialOrParallelReader(S&&, P&&) -> SequentialOrParallelReader<std::remove_cvref_t<S>, std::remove_cvref_t<P>>;

    template<typename Format, typename SeqReader, typename ParReader>
    struct SequentialOrParallelReaderFactory {
        template<Concepts::Communicator C = NullCommunicator>
        static auto make(const Format&, const C& comm = null_communicator) {
            return SequentialOrParallelReader{SeqReader{}, ParReader{comm}};
        }
    };

    template<typename Format, typename SequentialReader, typename ParallelReader>
    struct DefaultReaderFactory {
        static auto make(const Format&) { return SequentialReader{}; }
        static auto make(const Format&, const Concepts::Communicator auto& comm) { return ParallelReader{comm}; }
    };

    // Specialization for cases where the parallel reader is templated on the communicator
    template<typename F, typename S, template<typename> typename P>
    struct DefaultTemplatedReaderFactory {
        static auto make(const F&) { return S{}; }
        template<Concepts::Communicator Communicator>
        static auto make(const F&, const Communicator& comm) { return P<Communicator>{comm}; }
    };

}  // namespace APIDetail
#endif  // DOXYGEN

// Traits specializations for SequentialOrParallelReader
namespace Traits {

template<typename S, typename P>
struct WritesConnectivity<APIDetail::SequentialOrParallelReader<S, P>>
: public std::bool_constant<WritesConnectivity<S>::value || WritesConnectivity<P>::value>
{};

}  // namespace Traits

//! Specialization of the WriterFactory for the .vti format
template<> struct WriterFactory<FileFormat::VTI> {
    static auto make(const FileFormat::VTI& format,
                     const Concepts::ImageGrid auto& grid) {
        return VTIWriter{grid, format.opts.value_or(VTK::XMLOptions{})};
    }

    static auto make(const FileFormat::VTI& format,
                     const Concepts::ImageGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTIWriter{grid, comm, format.opts.value_or(VTK::XMLOptions{})};
    }
};

//! Specialization of the ReaderFactory for the .vti format
template<>
struct ReaderFactory<FileFormat::VTI> : APIDetail::SequentialOrParallelReaderFactory<FileFormat::VTI, VTIReader, PVTIReader> {};

//! Specialization of the WriterFactory for the .vtr format
template<> struct WriterFactory<FileFormat::VTR> {
    static auto make(const FileFormat::VTR& format,
                     const Concepts::RectilinearGrid auto& grid) {
        return VTRWriter{grid, format.opts.value_or(VTK::XMLOptions{})};
    }
    static auto make(const FileFormat::VTR& format,
                     const Concepts::RectilinearGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTRWriter{grid, comm, format.opts.value_or(VTK::XMLOptions{})};
    }
};

//! Specialization of the ReaderFactory for the .vtr format
template<>
struct ReaderFactory<FileFormat::VTR> : APIDetail::SequentialOrParallelReaderFactory<FileFormat::VTR, VTRReader, PVTRReader> {};

//! Specialization of the WriterFactory for the .vts format
template<> struct WriterFactory<FileFormat::VTS> {
    static auto make(const FileFormat::VTS& format,
                     const Concepts::StructuredGrid auto& grid) {
        return VTSWriter{grid, format.opts.value_or(VTK::XMLOptions{})};
    }
    static auto make(const FileFormat::VTS& format,
                     const Concepts::StructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTSWriter{grid, comm, format.opts.value_or(VTK::XMLOptions{})};
    }
};

//! Specialization of the ReaderFactory for the .vts format
template<>
struct ReaderFactory<FileFormat::VTS> : APIDetail::SequentialOrParallelReaderFactory<FileFormat::VTS, VTSReader, PVTSReader> {};

//! Specialization of the WriterFactory for the .vtp format
template<> struct WriterFactory<FileFormat::VTP> {
    static auto make(const FileFormat::VTP& format,
                     const Concepts::UnstructuredGrid auto& grid) {
        return VTPWriter{grid, format.opts.value_or(VTK::XMLOptions{})};
    }
    static auto make(const FileFormat::VTP& format,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTPWriter{grid, comm, format.opts.value_or(VTK::XMLOptions{})};
    }
};

//! Specialization of the ReaderFactory for the .vtp format
template<>
struct ReaderFactory<FileFormat::VTP> : APIDetail::SequentialOrParallelReaderFactory<FileFormat::VTP, VTPReader, PVTPReader> {};

//! Specialization of the WriterFactory for the .vtu format
template<> struct WriterFactory<FileFormat::VTU> {
    static auto make(const FileFormat::VTU& format,
                     const Concepts::UnstructuredGrid auto& grid) {
        return VTUWriter{grid, format.opts.value_or(VTK::XMLOptions{})};
    }
    static auto make(const FileFormat::VTU& format,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm) {
        return PVTUWriter{grid, comm, format.opts.value_or(VTK::XMLOptions{})};
    }
};

//! Specialization of the ReaderFactory for the .vtu format
template<>
struct ReaderFactory<FileFormat::VTU> : APIDetail::SequentialOrParallelReaderFactory<FileFormat::VTU, VTUReader, PVTUReader> {};

//! Specialization of the WriterFactory for vtk-xml time series.
template<typename F> struct WriterFactory<FileFormat::VTKXMLTimeSeries<F>> {
    static auto make(const FileFormat::VTKXMLTimeSeries<F>& format,
                     const Concepts::UnstructuredGrid auto& grid,
                     const std::string& base_filename) {
        return VTKXMLTimeSeriesWriter{
            WriterFactory<F>::make(F{format.opts.value_or(VTK::XMLOptions{})}, grid),
            base_filename
        };
    }
    static auto make(const FileFormat::VTKXMLTimeSeries<F>& format,
                     const Concepts::UnstructuredGrid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        return VTKXMLTimeSeriesWriter{
            WriterFactory<F>::make(F{format.opts.value_or(VTK::XMLOptions{})}, grid, comm),
            base_filename
        };
    }
};

//! Specialization of the WriterFactory for the vtk-hdf image grid format
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

//! Specialization of the ReaderFactory for the vtk-hdf image grid format
template<>
struct ReaderFactory<FileFormat::VTKHDFImage>
: APIDetail::DefaultReaderFactory<FileFormat::VTKHDFImage,
                                  VTKHDFImageGridReader,
                                  VTKHDFImageGridReader>
{};

//! Specialization of the WriterFactory for the transient vtk-hdf image grid format
template<> struct WriterFactory<FileFormat::VTKHDFImageTransient> {
    static auto make(const FileFormat::VTKHDFImageTransient& f,
                     const Concepts::ImageGrid auto& grid,
                     const std::string& base_filename) {
        if (f.opts.has_value())
            return VTKHDFImageGridTimeSeriesWriter{grid, base_filename, f.opts.value()};
        return VTKHDFImageGridTimeSeriesWriter{grid, base_filename};
    }
    static auto make(const FileFormat::VTKHDFImageTransient& f,
                     const Concepts::ImageGrid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        if (f.opts.has_value())
            return VTKHDFImageGridTimeSeriesWriter{grid, comm, base_filename, f.opts.value()};
        return VTKHDFImageGridTimeSeriesWriter{grid, comm, base_filename};
    }
};

//! Specialization of the ReaderFactory for the transient vtk-hdf image grid format
template<>
struct ReaderFactory<FileFormat::VTKHDFImageTransient>
: APIDetail::DefaultReaderFactory<FileFormat::VTKHDFImageTransient,
                                  VTKHDFImageGridReader,
                                  VTKHDFImageGridReader>
{};

//! Specialization of the WriterFactory for the vtk-hdf unstructured grid format
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

//! Specialization of the ReaderFactory for the vtk-hdf unstructured grid format
template<>
struct ReaderFactory<FileFormat::VTKHDFUnstructured>
: APIDetail::DefaultTemplatedReaderFactory<FileFormat::VTKHDFUnstructured,
                                           VTKHDFUnstructuredGridReader<>,
                                           VTKHDFUnstructuredGridReader>
{};

//! Specialization of the WriterFactory for the transient vtk-hdf unstructured grid format
template<> struct WriterFactory<FileFormat::VTKHDFUnstructuredTransient> {
    static auto make(const FileFormat::VTKHDFUnstructuredTransient& f,
                     const Concepts::ImageGrid auto& grid,
                     const std::string& base_filename) {
        if (f.opts.has_value())
            return VTKHDFUnstructuredTimeSeriesWriter{grid, base_filename, f.opts.value()};
        return VTKHDFUnstructuredTimeSeriesWriter{grid, base_filename};
    }
    static auto make(const FileFormat::VTKHDFUnstructuredTransient& f,
                     const Concepts::ImageGrid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        if (f.opts.has_value())
            return VTKHDFUnstructuredTimeSeriesWriter{grid, comm, base_filename, f.opts.value()};
        return VTKHDFUnstructuredTimeSeriesWriter{grid, comm, base_filename};
    }
};

//! Specialization of the ReaderFactory for the vtk-hdf unstructured grid format
template<>
struct ReaderFactory<FileFormat::VTKHDFUnstructuredTransient>
: APIDetail::DefaultTemplatedReaderFactory<FileFormat::VTKHDFUnstructuredTransient,
                                           VTKHDFUnstructuredGridReader<>,
                                           VTKHDFUnstructuredGridReader>
{};

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

//! Specialization of the ReaderFactory for the vtk-hdf file format with automatic flavour selection
template<>
struct ReaderFactory<FileFormat::VTKHDF>
: APIDetail::DefaultTemplatedReaderFactory<FileFormat::VTKHDF, VTKHDFReader<>, VTKHDFReader>
{};

//! Specialization of the WriterFactory for the transient vtk-hdf file format with automatic flavour selection.
template<> struct WriterFactory<FileFormat::VTKHDFTransient> {
    static auto make(const FileFormat::VTKHDFTransient& format,
                     const Concepts::Grid auto& grid,
                     const std::string& base_filename) {
        return _make(format.from(grid), grid, base_filename);
    }
    static auto make(const FileFormat::VTKHDFTransient& format,
                     const Concepts::Grid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        return _make(format.from(grid), grid, comm, base_filename);
    }
 private:
    template<typename F, typename... Args>
    static auto _make(F&& format, Args&&... args) {
        return WriterFactory<F>::make(format, std::forward<Args>(args)...);
    }
};

//! Specialization of the ReaderFactory for the transient vtk-hdf file format with automatic flavour selection
template<>
struct ReaderFactory<FileFormat::VTKHDFTransient>
: APIDetail::DefaultTemplatedReaderFactory<FileFormat::VTKHDFTransient, VTKHDFReader<>, VTKHDFReader>
{};

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

//! Specialization of the ReaderFactory for the .pvd time series file format.
template<typename PieceFormat>
struct ReaderFactory<FileFormat::PVD<PieceFormat>>
: APIDetail::DefaultTemplatedReaderFactory<FileFormat::PVD<PieceFormat>, PVDReader<>, PVDReader>
{};

//! Specialization of the ReaderFactory for the .pvd time series closure type.
template<>
struct ReaderFactory<FileFormat::PVDClosure>
: APIDetail::DefaultTemplatedReaderFactory<FileFormat::PVDClosure, PVDReader<>, PVDReader>
{};


//! Specialization of the WriterFactory for the any format selector.
template<> struct WriterFactory<FileFormat::Any> {
 private:
    template<typename G>
    static constexpr bool is_converter_grid = std::same_as<G, ConverterDetail::ConverterGrid>;

    template<typename F, typename... Args>
    static auto _make(F&& format, Args&&... args) {
        return WriterFactory<F>::make(format, std::forward<Args>(args)...);
    }

 public:
    template<Concepts::Grid G>
    static constexpr auto default_format_for() {
        if constexpr (Concepts::ImageGrid<G> && !is_converter_grid<G>)
            return FileFormat::VTI{};
        else if constexpr (Concepts::RectilinearGrid<G> && !is_converter_grid<G>)
            return FileFormat::VTR{};
        else if constexpr (Concepts::StructuredGrid<G> && !is_converter_grid<G>)
            return FileFormat::VTS{};
        else if constexpr (Concepts::UnstructuredGrid<G>)
            return FileFormat::VTU{};
        else
            static_assert(APIDetail::always_false<G>, "Cannot deduce a default format for the given grid");
    }

    template<Concepts::Grid G>
    static auto make(const FileFormat::Any&, const G& grid) {
        return _make(default_format_for<G>(), grid);
    }

    template<Concepts::Grid G, Concepts::Communicator C>
    static auto make(const FileFormat::Any&, const G& grid, const C& comm) {
        return _make(default_format_for<G>(), grid, comm);
    }
};

//! Specialization of the WriterFactory for the time series variant of the any format selector.
template<> struct WriterFactory<FileFormat::AnyTimeSeries> {
    static auto make(const FileFormat::AnyTimeSeries&,
                     const Concepts::Grid auto& grid,
                     const std::string& base_filename) {
        return _make(grid, base_filename);
    }

    static auto make(const FileFormat::AnyTimeSeries&,
                     const Concepts::Grid auto& grid,
                     const Concepts::Communicator auto& comm,
                     const std::string& base_filename) {
        return _make(grid, comm, base_filename);
    }

 private:
    template<typename Grid, typename... Args>
    static auto _make(const Grid& grid, Args&&... args) {
        auto fmt = WriterFactory<FileFormat::Any>::template default_format_for<Grid>();
        auto ts_fmt = FileFormat::TimeSeriesClosure{}(fmt);
        return WriterFactory<decltype(ts_fmt)>::make(ts_fmt, grid, std::forward<Args>(args)...);
    }
};

#ifndef DOXYGEN
namespace APIDetail {
    bool has_hdf_file_extension(const std::string& filename) {
        return filename.ends_with(".hdf")
            || filename.ends_with(".hdf5")
            || filename.ends_with(".he5")
            || filename.ends_with(".h5");
    }

    template<std::derived_from<GridReader> Reader, typename Communicator>
    std::unique_ptr<Reader> make_reader(const Communicator& c) {
        if constexpr (std::is_same_v<Communicator, NullCommunicator> ||
                        !std::is_constructible_v<Reader, const Communicator&>)
            return std::make_unique<Reader>();
        else
            return std::make_unique<Reader>(c);
    }

    template<Concepts::Communicator C>
    std::unique_ptr<GridReader> make_reader_for(const std::string& filename, const C& c) {
        if (filename.ends_with(".vtu")) return make_reader<VTUReader>(c);
        else if (filename.ends_with(".vtp")) return make_reader<VTPReader>(c);
        else if (filename.ends_with(".vti")) return make_reader<VTIReader>(c);
        else if (filename.ends_with(".vtr")) return make_reader<VTRReader>(c);
        else if (filename.ends_with(".vts")) return make_reader<VTSReader>(c);
        else if (filename.ends_with(".pvtu")) return make_reader<PVTUReader>(c);
        else if (filename.ends_with(".pvtp")) return make_reader<PVTPReader>(c);
        else if (filename.ends_with(".pvti")) return make_reader<PVTIReader>(c);
        else if (filename.ends_with(".pvtr")) return make_reader<PVTRReader>(c);
        else if (filename.ends_with(".pvts")) return make_reader<PVTSReader>(c);
        else if (filename.ends_with(".pvd")) return make_reader<PVDReader<C>>(c);
#if GRIDFORMAT_HAVE_HIGH_FIVE
        else if (has_hdf_file_extension(filename)) return make_reader<VTKHDFReader<C>>(c);
#endif
        throw IOError("Could not deduce an available file format for '" + filename + "'");
    }

}  // namespace APIDetail
#endif  // DOXYGEN

// Implementation of the AnyReaderFactory function
template<Concepts::Communicator C>
std::unique_ptr<GridReader> AnyReaderFactory<C>::make_for(const std::string& filename) const {
    return APIDetail::make_reader_for(filename, _comm);
}

//! Options for format conversions
template<typename OutFormat, typename InFormat = FileFormat::Any>
struct ConversionOptions {
    OutFormat out_format = {};
    InFormat in_format = {};
    int verbosity = 0;
};

/*!
 * \ingroup API
 * \brief Convert between parallel grid file formats.
 * \param in The input filename.
 * \param out The output filename.
 * \param opts Conversion options.
 * \param communicator The communicator (for parallel I/O).
 * \note Converting into .vtp file format does not work yet.
 */
template<typename OutFormat,
         typename InFormat,
         typename Communicator = None>
std::string convert(const std::string& in,
                    const std::string& out,
                    const ConversionOptions<OutFormat, InFormat>& opts,
                    const Communicator& communicator = {}) {
    static constexpr bool use_communicator = !std::same_as<Communicator, None>;
    static_assert(
        !use_communicator || Concepts::Communicator<Communicator>,
        "Given communicator does not satisfy the communicator concepts."
    );

    using WriterDetail::has_parallel_factory;
    using WriterDetail::has_sequential_factory;
    using WriterDetail::has_parallel_time_series_factory;
    using WriterDetail::has_sequential_time_series_factory;
    using CG = ConverterDetail::ConverterGrid;

    static constexpr bool is_single_file_out_format = [&] () {
        if constexpr (use_communicator) return has_parallel_factory<OutFormat, CG, Communicator>;
        else return has_sequential_factory<OutFormat, CG>;
    } ();

    static constexpr bool is_time_series_out_format = [&] () {
        if constexpr (use_communicator) return has_parallel_time_series_factory<OutFormat, CG, Communicator>;
        else return has_sequential_time_series_factory<OutFormat, CG>;
    } ();

    if (opts.verbosity > 0)
        std::cout << "Opening '" << in << "'" << std::endl;

    auto reader = [&] () {
        if constexpr (use_communicator) return Reader{opts.in_format, communicator};
        else return Reader{opts.in_format};
    } ();
    reader.open(in);
    if constexpr (use_communicator)
        if (reader.number_of_pieces() > 1
            && reader.number_of_pieces() != static_cast<unsigned>(Parallel::size(communicator)))
            throw IOError(
                "Can only convert parallel files if the number of processes matches "
                "the number of processes that were used to write the original file "
                "(" + std::to_string(reader.number_of_pieces()) + ")"
            );

    const bool print_progress_output = opts.verbosity > 1 && [&] () {
        if constexpr (use_communicator) return GridFormat::Parallel::rank(communicator) == 0;
        else return true;
    } ();

    const auto step_call_back = [&] (std::size_t step_idx, const std::string& filename) {
        if (print_progress_output)
            std::cout << "Wrote step " << step_idx << " to '" << filename << "'" << std::endl;
    };

    const auto invoke_factory = [&] <typename Fmt, typename... T> (const Fmt& fmt, const auto& grid, T&&... args) {
        if constexpr (use_communicator)
            return WriterFactory<Fmt>::make(fmt, grid, communicator, std::forward<T>(args)...);
        else
            return WriterFactory<Fmt>::make(fmt, grid, std::forward<T>(args)...);
    };

    if constexpr (is_single_file_out_format) {
        if (reader.is_sequence())
            return convert(reader, [&] (const auto& grid) {
                return invoke_factory(FileFormat::TimeSeriesClosure{}(opts.out_format), grid, out);
            }, step_call_back);

        const auto filename = convert(reader, out, [&] (const auto& grid) {
            return invoke_factory(opts.out_format, grid);
        });
        if (print_progress_output)
            std::cout << "Wrote '" << filename << "'" << std::endl;
        return filename;
    } else if constexpr (is_time_series_out_format) {
        return convert(reader, [&] (const auto& grid) {
            return invoke_factory(opts.out_format, grid, out);
        }, step_call_back);
    } else {
        static_assert(
            APIDetail::always_false<OutFormat>,
            "No viable factory found for the requested format"
        );
    }
}


// We place the format instances in a namespace different from FileFormats, in which
// the format types are defined above. Further below, we make these instances available
// in the GridFormat namespace directly. Having a separate namespace allows downstream ´
// projects to expose the format instances in their own namespace without having to expose
// all of GridFormat. Also, separating the format types from the instances further allows
// "hiding" the former and only expose the latter.
namespace Formats {

//! \addtogroup API
//! \{
//! \addtogroup FormatSelectors
//! \{
//! \name File Format Selectors
//! \{

inline constexpr FileFormat::Any any;
inline constexpr FileFormat::VTI vti;
inline constexpr FileFormat::VTR vtr;
inline constexpr FileFormat::VTS vts;
inline constexpr FileFormat::VTP vtp;
inline constexpr FileFormat::VTU vtu;
inline constexpr FileFormat::PVD pvd;
inline constexpr FileFormat::PVDClosure pvd_with;
inline constexpr FileFormat::TimeSeriesClosure time_series;
inline constexpr FileFormat::VTKHDF vtk_hdf;
inline constexpr FileFormat::VTKHDFTransient vtk_hdf_transient;

//! \} name File Format Selectors
//! \} group FormatSelectors
//! \} group API

/*!
 * \ingroup API
 * \brief Selects a default format suitable to write the given grid
 * \tparam G The grid type for which to select a file format.
 */
template<Concepts::Grid G>
constexpr auto default_for() { return WriterFactory<FileFormat::Any>::template default_format_for<G>(); }

/*!
 * \ingroup API
 * \brief Selects a default format suitable to write the given grid
 * \tparam G The grid type for which to select a file format.
 */
template<Concepts::Grid G>
constexpr auto default_for(const G&) { return default_for<G>(); }

}  // namespace Formats

// expose format instances
using namespace Formats;

// bring the format instances into the FileFormat namespace
namespace FileFormat { using namespace Formats; }

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRIDFORMAT_HPP_
