// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTSReader
 */
#ifndef GRIDFORMAT_VTK_PVTS_READER_HPP_
#define GRIDFORMAT_VTK_PVTS_READER_HPP_

#include <gridformat/vtk/vts_reader.hpp>
#include <gridformat/vtk/pxml_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .pvts file format
 * \copydetails VTK::PXMLStructuredGridReader
 */
class PVTSReader : public VTK::PXMLStructuredGridReader<VTSReader> {
    using ParentType = VTK::PXMLStructuredGridReader<VTSReader>;

 public:
    PVTSReader()
    : ParentType("PStructuredGrid")
    {}

    explicit PVTSReader(const NullCommunicator&)
    : ParentType("PStructuredGrid")
    {}

    template<Concepts::Communicator C>
    explicit PVTSReader(const C& comm)
    : ParentType("PStructuredGrid", comm)
    {}

 private:
    std::string _name() const override {
        return "PVTSReader";
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTS_READER_HPP_
