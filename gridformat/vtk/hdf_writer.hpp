// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Writers for the VTK HDF file formats.
 */
#ifndef GRIDFORMAT_VTK_HDF_WRITER_HPP_
#define GRIDFORMAT_VTK_HDF_WRITER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <type_traits>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/concepts.hpp>

#include <gridformat/vtk/hdf_common.hpp>
#include <gridformat/vtk/hdf_image_grid_writer.hpp>
#include <gridformat/vtk/hdf_unstructured_grid_writer.hpp>

namespace GridFormat {

// forward declarations
template<Concepts::ImageGrid Grid, Concepts::Communicator C>
class VTKHDFImageGridWriter;
template<Concepts::UnstructuredGrid Grid, Concepts::Communicator C>
class VTKHDFUnstructuredGridWriter;

#ifndef DOXYGEN
namespace VTKHDFDetail {

    template<typename Grid, typename C>
    struct VTKHDFWriterSelector;
    template<Concepts::UnstructuredGrid G, typename C> requires(!Concepts::ImageGrid<G>)
    struct VTKHDFWriterSelector<G, C> : public std::type_identity<VTKHDFUnstructuredGridWriter<G, C>> {};
    template<Concepts::ImageGrid G, typename C>
    struct VTKHDFWriterSelector<G, C> : public std::type_identity<VTKHDFImageGridWriter<G, C>> {};

    template<typename Grid, typename C>
    struct VTKHDFTimeSeriesWriterSelector;
    template<Concepts::UnstructuredGrid G, typename C> requires(!Concepts::ImageGrid<G>)
    struct VTKHDFTimeSeriesWriterSelector<G, C> : public std::type_identity<VTKHDFUnstructuredTimeSeriesWriter<G, C>> {};
    template<Concepts::ImageGrid G, typename C>
    struct VTKHDFTimeSeriesWriterSelector<G, C> : public std::type_identity<VTKHDFImageGridTimeSeriesWriter<G, C>> {};

}  // namespace VTKHDFDetail
#endif  // DOXYGEN

/*!
 * \ingroup VTK
 * \brief Convenience writer for the vtk-hdf file format, automatically selecting the image
 *        or unstructured grid format depending on the given grid type.
 */
template<Concepts::Grid Grid, typename Communicator = GridFormat::NullCommunicator>
class VTKHDFWriter : public VTKHDFDetail::VTKHDFWriterSelector<Grid, Communicator>::type {
    using ParentType = typename VTKHDFDetail::VTKHDFWriterSelector<Grid, Communicator>::type;

 public:
    using ParentType::ParentType;
};

template<typename Grid>
VTKHDFWriter(const Grid&) -> VTKHDFWriter<Grid>;

template<typename Grid, Concepts::Communicator Comm>
VTKHDFWriter(const Grid&, const Comm&) -> VTKHDFWriter<Grid, Comm>;


/*!
 * \ingroup VTK
 * \brief Convenience writer for the vtk-hdf file format, automatically selecting the image
 *        or unstructured grid format depending on the given grid type.
 */
template<Concepts::Grid Grid, Concepts::Communicator C = GridFormat::NullCommunicator>
class VTKHDFTimeSeriesWriter : public VTKHDFDetail::VTKHDFTimeSeriesWriterSelector<Grid, C>::type {
    using ParentType = typename VTKHDFDetail::VTKHDFTimeSeriesWriterSelector<Grid, C>::type;

 public:
    using ParentType::ParentType;
};

template<Concepts::Grid Grid>
VTKHDFTimeSeriesWriter(const Grid&, std::string, VTK::HDFTransientOptions = {}) -> VTKHDFTimeSeriesWriter<Grid>;
template<Concepts::Grid Grid, Concepts::Communicator C>
VTKHDFTimeSeriesWriter(const Grid&, C, std::string, VTK::HDFTransientOptions = {}) -> VTKHDFTimeSeriesWriter<Grid, C>;

namespace Traits {

template<Concepts::ImageGrid Grid, typename... Args>
struct WritesConnectivity<VTKHDFWriter<Grid, Args...>> : public std::false_type {};

template<Concepts::ImageGrid Grid, typename... Args>
struct WritesConnectivity<VTKHDFTimeSeriesWriter<Grid, Args...>> : public std::false_type {};

}  // namespace Traits

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_WRITER_HPP_
