// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTIReader
 */
#ifndef GRIDFORMAT_VTK_PVTI_READER_HPP_
#define GRIDFORMAT_VTK_PVTI_READER_HPP_

#include <optional>

#include <gridformat/vtk/vti_reader.hpp>
#include <gridformat/vtk/pxml_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .pvtp file format
 * \copydetails VTK::PXMLStructuredGridReader
 */
class PVTIReader : public VTK::PXMLStructuredGridReader<VTIReader> {
    using ParentType = VTK::PXMLStructuredGridReader<VTIReader>;

 public:
    PVTIReader()
    : ParentType("PImageData")
    {}

    explicit PVTIReader(const NullCommunicator&)
    : ParentType("PImageData")
    {}

    template<Concepts::Communicator C>
    explicit PVTIReader(const C& comm)
    : ParentType("PImageData", comm)
    {}

 private:
    std::string _name() const override {
        return "PVTIReader";
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTI_READER_HPP_
