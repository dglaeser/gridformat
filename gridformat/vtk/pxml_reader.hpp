// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTK::PXMLReader
 */
#ifndef GRIDFORMAT_VTK_PXML_READER_HPP_
#define GRIDFORMAT_VTK_PXML_READER_HPP_

#include <string>
#include <optional>
#include <iterator>
#include <filesystem>
#include <algorithm>
#include <utility>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/empty_field.hpp>
#include <gridformat/common/field_transformations.hpp>
#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/parallel/communication.hpp>

#include <gridformat/vtk/vtu_reader.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief Reader for parallel vtk-xml file formats for unstructured grids.
 * \details TODO
 */
template<std::derived_from<GridReader> PieceReader>
class PXMLReader : public GridReader {
 public:
    PXMLReader(std::string vtk_grid_type)
    : _vtk_grid_type{std::move(vtk_grid_type)}
    {}

    explicit PXMLReader(std::string vtk_grid_type, const NullCommunicator&)
    : PXMLReader(std::move(vtk_grid_type))
    {}

    template<Concepts::Communicator C>
    explicit PXMLReader(std::string vtk_grid_type,
                        const C& comm,
                        std::optional<bool> merge_exceeding_pieces = {})
    : PXMLReader(std::move(vtk_grid_type)) {
        _num_ranks = Parallel::size(comm);
        _rank = Parallel::rank(comm);
        _merge_exceeding = merge_exceeding_pieces;
    }

 private:
    std::ranges::range auto _pieces_paths(const VTK::XMLReaderHelper& helper) const {
        return children(helper.get(_vtk_grid_type))
            | std::views::filter([&] (const XMLElement& e) { return e.name() == "Piece"; })
            | std::views::transform([&] (const XMLElement& p) { return _get_piece_path(p.get_attribute("Source")); });
    }

    std::filesystem::path _get_piece_path(const std::string& piece_filename) const {
        std::filesystem::path piece_path{piece_filename};
        if (piece_path.is_absolute())
            return piece_path;
        return std::filesystem::path{_filename.value()}.parent_path() / piece_filename;
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        _filename = filename;
        _read_pieces();

        std::ranges::copy(point_field_names(_piece_readers.front()), std::back_inserter(fields.point_fields));
        std::ranges::copy(cell_field_names(_piece_readers.front()), std::back_inserter(fields.cell_fields));
        std::ranges::copy(meta_data_field_names(_piece_readers.front()), std::back_inserter(fields.meta_data_fields));

        if (std::ranges::any_of(_piece_readers | std::views::drop(1), [&] (const auto& reader) {
            return !std::ranges::equal(point_field_names(reader), fields.point_fields);
        }))
            throw IOError("All pieces must define the same point fields");
        if (std::ranges::any_of(_piece_readers | std::views::drop(1), [&] (const auto& reader) {
            return !std::ranges::equal(cell_field_names(reader), fields.cell_fields);
        }))
            throw IOError("All pieces must define the same cell fields");
    }

    void _read_pieces() {
        auto helper = VTK::XMLReaderHelper::make_from(_filename.value(), _vtk_grid_type);
        if (_num_ranks) {
            _read_parallel_piece(helper);
        } else {
            std::ranges::for_each(_pieces_paths(helper), [&] (const std::filesystem::path& path) {
                _piece_readers.emplace_back(PieceReader{}).open(path);
            });
        }
    }

    void _read_parallel_piece(const VTK::XMLReaderHelper& helper) {
        const auto num_pieces = Ranges::size(_pieces_paths(helper));
        if (num_pieces < _num_ranks.value() && _rank.value() == 0)
            log_warning(
                "PVTK file defines less pieces than there are ranks. The grids on some ranks will be empty."
            );
        if (num_pieces > _num_ranks.value() && !_merge_exceeding.has_value() && _rank.value() == 0)
            log_warning(
                "PVTK file defines more pieces than used ranks. Will only read the first "
                + std::to_string(_num_ranks.value()) + " pieces"
            );

        const std::size_t my_num_pieces =
            _rank.value() == _num_ranks.value() - 1 && _merge_exceeding.value_or(false)
                ? _num_ranks.value() - _rank.value()
                : 1;

        std::ranges::for_each(
            _pieces_paths(helper)
            | std::views::drop(_rank.value())
            | std::views::take(my_num_pieces),
            [&] (const std::filesystem::path& path) {
                _piece_readers.emplace_back(PieceReader{}).open(path);
            }
        );
    }

    void _close() override {
        _num_ranks.reset();
        _rank.reset();
        _merge_exceeding.reset();
        _filename.reset();
        _piece_readers.clear();
    }

    std::string _name() const override {
        return "VTK::PXMLReader";
    }

    std::size_t _number_of_cells() const override {
        auto num_cells_view = _piece_readers | std::views::transform([] (const auto& reader) {
            return reader.number_of_cells();
        });
        return std::accumulate(
            std::ranges::begin(num_cells_view),
            std::ranges::end(num_cells_view),
            std::size_t{0}
        );
    }

    std::size_t _number_of_points() const override {
        auto num_points_view = _piece_readers | std::views::transform([] (const auto& reader) {
            return reader.number_of_points();
        });
        return std::accumulate(
            std::ranges::begin(num_points_view),
            std::ranges::end(num_points_view),
            std::size_t{0}
        );
    }

    bool _is_sequence() const override {
        return false;
    }

    FieldPtr _points() const override {
        return _merge([] (const PieceReader& reader) { return reader.points(); });
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        std::size_t offset = 0;
        std::ranges::for_each(_piece_readers, [&] (const PieceReader& reader) {
            reader.visit_cells([&] (CellType ct, std::vector<std::size_t> corners) {
                std::ranges::for_each(corners, [&] (auto& value) { value += offset; });
                visitor(ct, corners);
            });
            offset += reader.number_of_points();
        });
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _merge([&] (const PieceReader& reader) { return reader.cell_field(name); });
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _merge([&] (const PieceReader& reader) { return reader.point_field(name); });
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _piece_readers.front().meta_data_field(name);
    }

    template<std::invocable<const PieceReader&> FieldGetter>
        requires(std::same_as<std::invoke_result_t<FieldGetter, const PieceReader&>, FieldPtr>)
    FieldPtr _merge(const FieldGetter& get_field) const {
        if (_piece_readers.empty())
            return make_field_ptr(EmptyField{float64});
        if (_piece_readers.size() == 1)
            return get_field(_piece_readers.front());

        std::vector<FieldPtr> result;
        std::ranges::copy(
            _piece_readers
            | std::views::transform([&] (const PieceReader& reader) { return get_field(reader); }),
            std::back_inserter(result)
        );
        return make_field_ptr(MergedField{std::move(result)});
    }

    std::string _vtk_grid_type;

    std::optional<unsigned int> _num_ranks;
    std::optional<unsigned int> _rank;
    std::optional<bool> _merge_exceeding;

    std::optional<std::string> _filename;
    std::vector<PieceReader> _piece_readers;
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_PXML_READER_HPP_
