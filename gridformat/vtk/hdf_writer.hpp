// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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

}  // namespace VTKHDFDetail
#endif  // DOXYGEN

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
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

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_WRITER_HPP_
