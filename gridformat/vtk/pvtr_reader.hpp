// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTRReader
 */
#ifndef GRIDFORMAT_VTK_PVTR_READER_HPP_
#define GRIDFORMAT_VTK_PVTR_READER_HPP_

#include <optional>

#include <gridformat/vtk/vtr_reader.hpp>
#include <gridformat/vtk/pxml_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .pvtr file format
 * \copydetails VTK::PXMLStructuredGridReader
 */
class PVTRReader : public VTK::PXMLStructuredGridReader<VTRReader> {
    using ParentType = VTK::PXMLStructuredGridReader<VTRReader>;

 public:
    PVTRReader()
    : ParentType("PRectilinearGrid")
    {}

    explicit PVTRReader(const NullCommunicator&)
    : ParentType("PRectilinearGrid")
    {}

    template<Concepts::Communicator C>
    explicit PVTRReader(const C& comm)
    : ParentType("PRectilinearGrid", comm)
    {}

 private:
    std::string _name() const override {
        return "PVTRReader";
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTR_READER_HPP_
