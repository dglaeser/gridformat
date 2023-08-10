// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::PVTRReader
 */
#ifndef GRIDFORMAT_VTK_PVTR_READER_HPP_
#define GRIDFORMAT_VTK_PVTR_READER_HPP_

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <ranges>
#include <array>

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
    using IndexInterval = std::array<std::size_t, 2>;

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

    std::vector<double> _ordinates(unsigned int i) const override {
        if (this->_num_process_pieces() == 0)
            return {};
        if (this->_num_process_pieces() == 1)
            return this->_readers().front().ordinates(i);

        std::unordered_map<std::size_t, IndexInterval> piece_to_interval;
        std::ranges::for_each(this->_readers(), [&, reader_idx=0] (const GridReader& reader) mutable {
            auto interval = _to_interval(reader.location(), i);
            const bool should_insert = std::ranges::none_of(
                piece_to_interval | std::views::values,
                [&] (const auto& inserted_interval) {
                    if (_is_same(inserted_interval, interval))
                        return true;
                    else if (_overlap(inserted_interval, interval))
                        throw IOError("Cannot determine ordinates for pieces with overlapping intervals");
                    return false;
                }
            );
            if (should_insert)
                piece_to_interval.emplace(std::make_pair(reader_idx, std::move(interval)));
            reader_idx++;
        });


        std::vector<unsigned int> sorted_piece_indices(piece_to_interval.size());
        std::ranges::copy(std::views::iota(std::size_t{0}, sorted_piece_indices.size()), sorted_piece_indices.begin());
        std::ranges::sort(sorted_piece_indices, [&] (unsigned int a, unsigned int b) {
            return piece_to_interval.at(a)[0] < piece_to_interval.at(b)[0];
        });

        std::vector<double> result;
        std::ranges::for_each(sorted_piece_indices, [&] (unsigned int piece_idx) {
            auto piece_ordinates = this->_readers().at(piece_idx).ordinates(i);
            if (result.size() > 0)
                result.pop_back();  // avoid duplicate points at overlaps
            result.reserve(result.size() + piece_ordinates.size());
            std::ranges::move(std::move(piece_ordinates), std::back_inserter(result));
        });
        return result;
    }

    bool _is_same(const IndexInterval& a, const IndexInterval& b) const {
        return std::ranges::equal(a, b);
    }

    bool _overlap(const IndexInterval& a, const IndexInterval& b) const {
        return (a[0] < b[0] && a[1] > b[0]) || (b[0] < a[0] && b[1] > a[0]);
    }

    IndexInterval _to_interval(const typename ParentType::PieceLocation& loc, unsigned int i) const {
        return {loc.lower_left.at(i), loc.upper_right.at(i)};
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_PVTR_READER_HPP_
