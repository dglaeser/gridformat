// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTUReader
 */
#ifndef GRIDFORMAT_VTK_PVTU_READER_HPP_
#define GRIDFORMAT_VTK_PVTU_READER_HPP_

#include <optional>

#include <gridformat/vtk/vtu_reader.hpp>
#include <gridformat/vtk/pxml_reader.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .pvtu file format
 * \copydetails VTK::PXMLUnstructuredGridReader
 */
class PVTUReader : public VTK::PXMLUnstructuredGridReader<VTUReader> {
    using ParentType = VTK::PXMLUnstructuredGridReader<VTUReader>;

 public:
    PVTUReader()
    : ParentType("PUnstructuredGrid")
    {}

    explicit PVTUReader(const NullCommunicator&)
    : ParentType("PUnstructuredGrid")
    {}

    template<Concepts::Communicator C>
    explicit PVTUReader(const C& comm, std::optional<bool> merge_exceeding_pieces = {})
    : ParentType("PUnstructuredGrid", comm, merge_exceeding_pieces)
    {}

 private:
    std::string _name() const override {
        return "PVTUReader";
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTU_READER_HPP_
