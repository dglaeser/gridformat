// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTIReader
 */
#ifndef GRIDFORMAT_VTK_PVTI_READER_HPP_
#define GRIDFORMAT_VTK_PVTI_READER_HPP_

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

    std::vector<double> _ordinates(unsigned int direction) const override {
        if (this->_num_process_pieces() == 0)
            return {};
        if (this->_num_process_pieces() == 1)
            return this->_readers().front().ordinates(direction);

        const auto extents = this->_specs().extents;
        const auto extent_begin = this->_specs().extents[2*direction];
        const auto origin = this->origin();
        const auto spacing = this->spacing();
        const std::size_t num_ordinates(extents[2*direction + 1] - extent_begin + 1);
        std::vector<double> result(num_ordinates);
        std::ranges::copy(
            std::views::iota(std::size_t{0}, num_ordinates) | std::views::transform([&] (const std::size_t i) {
                return origin[direction] + (extent_begin + i)*spacing[direction];
            }),
            result.begin()
        );
        return result;
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTI_READER_HPP_
