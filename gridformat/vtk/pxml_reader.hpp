// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTK::PXMLReader
 */
#ifndef GRIDFORMAT_VTK_PXML_READER_HPP_
#define GRIDFORMAT_VTK_PXML_READER_HPP_

#include <array>
#include <string>
#include <optional>
#include <iterator>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <utility>
#include <cmath>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/md_index.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/flat_index_mapper.hpp>
#include <gridformat/common/empty_field.hpp>
#include <gridformat/common/field_transformations.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/parallel/communication.hpp>

#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief Base class for readers of parallel vtk-xml file formats.
 * \details Constructors of derived classes for readers of unstructured
 *          grids may expose the `merge_exceeding_pieces` option. If set
 *          to true, then parallel I/O with less ranks than pieces in the
 *          PVTK file is done such that the last rank reads in and merges
 *          all remaining pieces. Otherwise, only as many pieces as ranks
 *          are read. On the other hand, if there are more ranks than pieces,
 *          some ranks will not read in any data (i.e. the grids are empty).
 */
template<std::derived_from<GridReader> PieceReader>
class PXMLReaderBase : public GridReader {
 public:
    PXMLReaderBase(std::string vtk_grid_type)
    : _vtk_grid_type{std::move(vtk_grid_type)}
    {}

    explicit PXMLReaderBase(std::string vtk_grid_type, const NullCommunicator&)
    : PXMLReaderBase(std::move(vtk_grid_type))
    {}

    template<Concepts::Communicator C>
    explicit PXMLReaderBase(std::string vtk_grid_type,
                            const C& comm,
                            std::optional<bool> merge_exceeding_pieces = {})
    : PXMLReaderBase(std::move(vtk_grid_type)) {
        _num_ranks = Parallel::size(comm);
        _rank = Parallel::rank(comm);
        _merge_exceeding = merge_exceeding_pieces;
    }

 protected:
    enum class FieldType { point, cell };

    const std::vector<PieceReader>& _readers() const {
        return _piece_readers;
    }

    const std::string _grid_type() const {
        return _vtk_grid_type;
    }

    std::size_t _num_process_pieces() const {
        return _piece_readers.size();
    }

    std::optional<bool> _merge_exceeding_pieces_option() const {
        return _merge_exceeding;
    }

    XMLReaderHelper _read_pvtk_file(const std::string& filename, typename GridReader::FieldNames& fields) {
        _filename = filename;
         auto helper = XMLReaderHelper::make_from(filename, _vtk_grid_type);
        _num_pieces_in_file = Ranges::size(_pieces_paths(helper));
        _read_pieces(helper);
        if (_piece_readers.size() > 0) {
            std::ranges::copy(point_field_names(_piece_readers.front()), std::back_inserter(fields.point_fields));
            std::ranges::copy(cell_field_names(_piece_readers.front()), std::back_inserter(fields.cell_fields));
            std::ranges::copy(meta_data_field_names(_piece_readers.front()), std::back_inserter(fields.meta_data_fields));
        }

        if (std::ranges::any_of(_piece_readers | std::views::drop(1), [&] (const auto& reader) {
            return !std::ranges::equal(point_field_names(reader), fields.point_fields);
        }))
            throw IOError("All pieces must define the same point fields");
        if (std::ranges::any_of(_piece_readers | std::views::drop(1), [&] (const auto& reader) {
            return !std::ranges::equal(cell_field_names(reader), fields.cell_fields);
        }))
            throw IOError("All pieces must define the same cell fields");
        return helper;
    }

    void _close_pvtk_file() {
        _filename.reset();
        _piece_readers.clear();
        _num_pieces_in_file = 0;
    }

 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        _read_pvtk_file(filename, fields);
    }

    void _close() override {
        _close_pvtk_file();
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

    std::size_t _number_of_pieces() const override {
        return _num_pieces_in_file;
    }

    bool _is_sequence() const override {
        return false;
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _piece_readers.front().meta_data_field(name);
    }

    FieldPtr _points() const override {
        return _merge([] (const PieceReader& reader) { return reader.points(); }, FieldType::point);
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _merge([&] (const PieceReader& reader) { return reader.cell_field(name); }, FieldType::cell);
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _merge([&] (const PieceReader& reader) { return reader.point_field(name); }, FieldType::point);
    }

    template<std::invocable<const PieceReader&> FieldGetter>
        requires(std::same_as<std::invoke_result_t<FieldGetter, const PieceReader&>, FieldPtr>)
    FieldPtr _merge(const FieldGetter& get_field, FieldType type) const {
        if (_num_process_pieces() == 0)
            return make_field_ptr(EmptyField{float64});
        if (_num_process_pieces() == 1)
            return get_field(_piece_readers.front());

        std::vector<FieldPtr> field_pieces;
        std::ranges::copy(
            _piece_readers
            | std::views::transform([&] (const PieceReader& reader) { return get_field(reader); }),
            std::back_inserter(field_pieces)
        );
        return _merge_field_pieces(std::move(field_pieces), type);
    }

    virtual FieldPtr _merge_field_pieces(std::vector<FieldPtr>&&, FieldType) const = 0;

    std::ranges::range auto _pieces_paths(const XMLReaderHelper& helper) const {
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

    void _read_pieces(const XMLReaderHelper& helper) {
        if (_num_ranks)
            _read_parallel_piece(helper);
        else
            std::ranges::for_each(_pieces_paths(helper), [&] (const std::filesystem::path& path) {
                _piece_readers.emplace_back(PieceReader{}).open(path);
            });
    }

    void _read_parallel_piece(const XMLReaderHelper& helper) {
        const auto num_pieces = Ranges::size(_pieces_paths(helper));
        if (num_pieces < _num_ranks.value() && _rank.value() == 0)
            this->_log_warning(
                "PVTK file defines less pieces than there are ranks. The grids on some ranks will be empty."
            );
        if (num_pieces > _num_ranks.value() && !_merge_exceeding.has_value() && _rank.value() == 0)
            this->_log_warning(
                "PVTK file defines more pieces than used ranks. Will only read the first "
                + std::to_string(_num_ranks.value()) + " pieces"
            );

        const bool is_last_rank = _rank.value() == _num_ranks.value() - 1;
        const bool merge_final_pieces = is_last_rank && _merge_exceeding.value_or(false);
        const std::size_t my_num_pieces = merge_final_pieces ? Ranges::size(_pieces_paths(helper)) - _rank.value() : 1;

        std::ranges::for_each(
            _pieces_paths(helper)
            | std::views::drop(_rank.value())
            | std::views::take(my_num_pieces),
            [&] (const std::filesystem::path& path) {
                _piece_readers.emplace_back(PieceReader{}).open(path);
            }
        );
    }

    std::string _vtk_grid_type;

    std::optional<unsigned int> _num_ranks;
    std::optional<unsigned int> _rank;
    std::optional<bool> _merge_exceeding;

    std::optional<std::string> _filename;
    std::vector<PieceReader> _piece_readers;
    std::size_t _num_pieces_in_file = 0;
};


/*!
 * \ingroup VTK
 * \brief Base class for readers of parallel vtk-xml file formats for unstructured grids.
 * \copydetails PXMLReaderBase
 */
template<std::derived_from<GridReader> PieceReader>
class PXMLUnstructuredGridReader : public PXMLReaderBase<PieceReader> {
    using ParentType = PXMLReaderBase<PieceReader>;
 public:
    using ParentType::ParentType;

 private:
    std::size_t _number_of_points() const override {
        auto num_points_view = this->_readers() | std::views::transform([] (const auto& reader) {
            return reader.number_of_points();
        });
        return std::accumulate(
            std::ranges::begin(num_points_view),
            std::ranges::end(num_points_view),
            std::size_t{0}
        );
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        std::size_t offset = 0;
        std::ranges::for_each(this->_readers(), [&] (const PieceReader& reader) {
            reader.visit_cells([&] (CellType ct, std::vector<std::size_t> corners) {
                std::ranges::for_each(corners, [&] (auto& value) { value += offset; });
                visitor(ct, corners);
            });
            offset += reader.number_of_points();
        });
    }

    FieldPtr _merge_field_pieces(std::vector<FieldPtr>&& pieces, ParentType::FieldType) const override {
        return make_field_ptr(MergedField{std::move(pieces)});
    }
};


/*!
 * \ingroup VTK
 * \brief Base class for readers of parallel vtk-xml file formats for structured grids.
 * \copydetails PXMLReaderBase
 * \note This implementation does not support overlapping partitions
 */
template<std::derived_from<GridReader> PieceReader>
class PXMLStructuredGridReader : public PXMLReaderBase<PieceReader> {
    using ParentType = PXMLReaderBase<PieceReader>;
 public:
    using ParentType::ParentType;

 protected:
    struct StructuredGridSpecs {
        std::array<std::size_t, 6> extents;
        std::optional<std::array<double, 3>> spacing;   // for image grids
        std::optional<std::array<double, 3>> origin;    // for image grids
        std::array<double, 9> direction{                // for image grids
            1., 0., 0., 0., 1., 0., 0., 0., 1.
        };
    };

    const StructuredGridSpecs& _specs() const {
        if (!_grid_specs.has_value())
            throw InvalidState("No data has been read");
        return _grid_specs.value();
    }

 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& names) override {
        if (this->_merge_exceeding_pieces_option().value_or(false))
            throw IOError("Parallel I/O of structured vtk files does not support the 'merge_exceeding_pieces' option");

        auto helper = ParentType::_read_pvtk_file(filename, names);
        const XMLElement& vtk_grid = helper.get(this->_grid_type());
        if (vtk_grid.get_attribute_or(std::size_t{0}, "GhostLevel") > 0)
            throw IOError("GhostLevel > 0 not yet supported for parallel I/O of structured vtk files.");

        StructuredGridSpecs& specs = _grid_specs.emplace();
        specs.extents = Ranges::array_from_string<std::size_t, 6>(vtk_grid.get_attribute("WholeExtent"));
        if (specs.extents[0] != 0 || specs.extents[2] != 0 || specs.extents[4] != 0)
            throw ValueError("'WholeExtent' is expected to have no offset (e.g. have the shape 0 X 0 Y 0 Z)");
        if (vtk_grid.has_attribute("Origin"))
            specs.origin = Ranges::array_from_string<double, 3>(vtk_grid.get_attribute("Origin"));
        if (vtk_grid.has_attribute("Spacing"))
            specs.spacing = Ranges::array_from_string<double, 3>(vtk_grid.get_attribute("Spacing"));
        if (vtk_grid.has_attribute("Direction"))
            specs.direction = Ranges::array_from_string<double, 9>(vtk_grid.get_attribute("Direction"));
    }

    void _close() override {
        ParentType::_close_pvtk_file();
        _grid_specs.reset();
    }

    typename GridReader::Vector _origin() const override {
        if (!_specs().origin.has_value())
            throw ValueError("PVTK file does not define the origin for '" + this->_grid_type() + "'");
        if (this->_num_process_pieces() == 1) {
            const auto& specs = _specs();
            return CommonDetail::compute_piece_origin(
                specs.origin.value(),
                _spacing(),
                this->_readers().at(0).location().lower_left,
                specs.direction
            );
        } else {  // use global origin
            return _specs().origin.value();
        }
    }

    typename GridReader::Vector _spacing() const override {
        if (!_specs().spacing.has_value())
            throw ValueError("PVTK file does not define the spacing for '" + this->_grid_type() + "'");
        return _specs().spacing.value();
    }

    typename GridReader::Vector _basis_vector(unsigned int i) const override {
        const auto& direction = _specs().direction;
        return {direction.at(i), direction.at(i+3), direction.at(i+6)};
    }

    typename GridReader::PieceLocation _location() const override {
        typename GridReader::PieceLocation result;

        if (this->_num_process_pieces() == 1) {
            result = this->_readers().at(0).location();
        } else {  // use "WholeExtent"
            const auto& specs = _specs();
            result.lower_left = {specs.extents[0], specs.extents[2], specs.extents[4]};
            result.upper_right = {specs.extents[1], specs.extents[3], specs.extents[5]};
        }

        return result;
    }

    std::size_t _number_of_points() const override {
        if (this->_num_process_pieces() == 0)
            return 0;
        if (this->_num_process_pieces() == 1)
            return this->_readers().at(0).number_of_points();
        return CommonDetail::number_of_entities(_whole_point_extents());
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        if (this->_num_process_pieces() == 1)
            this->_readers().at(0).visit_cells(visitor);
        else
            CommonDetail::visit_structured_cells(visitor, _specs().extents);
    }

    FieldPtr _merge_field_pieces(std::vector<FieldPtr>&& pieces, ParentType::FieldType type) const override {
        auto whole_grid_extents = type == ParentType::FieldType::point ? _whole_point_extents() : [&] () {
            auto result = _specs().extents;
            // avoid zeroes s.t. the index mappers map properly
            result[1] = std::max(result[1], std::size_t{1});
            result[3] = std::max(result[3], std::size_t{1});
            result[5] = std::max(result[5], std::size_t{1});
            return result;
        } ();
        auto num_entities = CommonDetail::number_of_entities(whole_grid_extents);
        auto precision = pieces.at(0)->precision();
        MDLayout whole_field_layout = [&] () {
            const auto first_layout = pieces.at(0)->layout();
            std::vector<std::size_t> dims(first_layout.begin(), first_layout.end());
            dims.at(0) = num_entities;
            return MDLayout{std::move(dims)};
        } ();
        _check_fields_compatibility(pieces, whole_field_layout, precision);

        std::vector<MDLayout> pieces_layouts;
        std::vector<MDIndex> pieces_offsets;
        std::ranges::for_each(this->_readers(), [&] (const PieceReader& reader) {
            pieces_layouts.push_back(MDLayout{reader.extents()});
            pieces_offsets.push_back(MDIndex{reader.location().lower_left});
            if (type == ParentType::FieldType::point)
                std::ranges::for_each(pieces_layouts.back(), [] (auto& o) { o += 1; });
            else
                std::ranges::for_each(pieces_layouts.back(), [] (auto& o) { o = std::max(o, std::size_t{1}); });
        });

        return make_field_ptr(LazyField{
            int{},  // dummy source
            whole_field_layout,
            precision,
            [
                _whole_ex=std::move(whole_grid_extents), _prec=precision, _field_layout=whole_field_layout,
                _pieces=std::move(pieces), _offsets=std::move(pieces_offsets), _piece_layouts=std::move(pieces_layouts)
            ] (const int&) {
                const auto num_entities = _field_layout.extent(0);
                const auto num_comps = _field_layout.dimension() > 1 ? _field_layout.number_of_entries(1) : 1;
                const FlatIndexMapper global_mapper{std::array{_whole_ex[1], _whole_ex[3], _whole_ex[5]}};
                return _prec.visit([&] <typename T> (const Precision<T>& prec) {
                    Serialization result(num_entities*num_comps*sizeof(T));
                    auto result_span = result.as_span_of(prec);

                    for (unsigned int i = 0; i < _pieces.size(); ++i) {
                        auto piece_serialization = _pieces.at(i)->serialized();
                        auto piece_span = piece_serialization.as_span_of(prec);

                        std::size_t piece_offset = 0;
                        const auto local_to_global_offset = _offsets.at(i);
                        for (auto piece_index : MDIndexRange{_piece_layouts.at(i)}) {
                            piece_index += local_to_global_offset;
                            const auto global_offset = global_mapper.map(piece_index)*num_comps;
                            assert(global_offset + num_comps <= result_span.size());
                            assert(piece_offset + num_comps <= piece_span.size());
                            std::copy_n(
                                piece_span.data() + piece_offset,
                                num_comps,
                                result_span.data() + global_offset
                            );
                            piece_offset += num_comps;
                        }
                    }
                    return result;
                });
            }
        });
    }

    void _check_fields_compatibility(const std::vector<FieldPtr>& pieces,
                                     const MDLayout& whole_field_layout,
                                     const DynamicPrecision& precision) const {
        const auto has_compatible_layout = [&] (const FieldPtr& piece_field) {
            const auto piece_layout = piece_field->layout();
            if (piece_layout.dimension() != whole_field_layout.dimension())
                return false;
            if (whole_field_layout.dimension() > 1)
                return whole_field_layout.sub_layout(1) == piece_layout.sub_layout(1);
            return true;
        };
        if (!std::ranges::all_of(pieces, has_compatible_layout))
            throw ValueError("Fields to be merged have incompatible layouts");

        const auto has_compatible_precision = [&] (const FieldPtr& piece_field) {
            return piece_field->precision() == precision;
        };
        if (!std::ranges::all_of(pieces, has_compatible_precision))
            throw ValueError("Fields to be merged have incompatible precisions");
    }

    std::array<std::size_t, 6> _whole_point_extents() const {
        auto result = _specs().extents;
        result[1] += 1;
        result[3] += 1;
        result[5] += 1;
        return result;
    }

    std::optional<StructuredGridSpecs> _grid_specs;
};

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_PXML_READER_HPP_
