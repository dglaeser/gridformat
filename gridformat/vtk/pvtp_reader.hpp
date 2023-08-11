// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTPReader
 */
#ifndef GRIDFORMAT_VTK_PVTP_READER_HPP_
#define GRIDFORMAT_VTK_PVTP_READER_HPP_

#include <optional>

#include <gridformat/vtk/vtp_reader.hpp>
#include <gridformat/vtk/pxml_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .pvtp file format
 * \copydetails VTK::PXMLUnstructuredGridReader
 */
class PVTPReader : public VTK::PXMLUnstructuredGridReader<VTPReader> {
    using ParentType = VTK::PXMLUnstructuredGridReader<VTPReader>;

 public:
    PVTPReader()
    : ParentType("PPolyData")
    {}

    explicit PVTPReader(const NullCommunicator&)
    : ParentType("PPolyData")
    {}

    template<Concepts::Communicator C>
    explicit PVTPReader(const C& comm, std::optional<bool> merge_exceeding_pieces = {})
    : ParentType("PPolyData", comm, merge_exceeding_pieces)
    {}

 private:
    std::string _name() const override {
        return "PVTPReader";
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTP_READER_HPP_
