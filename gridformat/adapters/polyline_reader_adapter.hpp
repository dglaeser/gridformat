// SPDX-FileCopyrightText: 2025 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Adapters
 * \copydoc GridFormat::PolyLineReaderAdapter
 */
#ifndef GRIDFORMAT_ADAPTER_POLYLINE_READER_ADAPTER_HPP_
#define GRIDFORMAT_ADAPTER_POLYLINE_READER_ADAPTER_HPP_

#include <concepts>
#include <algorithm>
#include <iterator>
#include <memory>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/buffer_field.hpp>
#include <gridformat/grid/reader.hpp>

namespace GridFormat {

/*!
* \ingroup Adapters
* \brief Adapter for readers that subdivides polyline cells into collections of segments.
* \note The adapter takes ownership over the reader provided upon construction.
*/
class PolyLineReaderAdapter : public GridReader {
 public:
    template<std::derived_from<GridReader> Reader>
        requires(not std::is_lvalue_reference_v<Reader>)
    explicit PolyLineReaderAdapter(Reader&& reader)
    : _reader{std::make_unique<std::remove_cvref_t<Reader>>(std::move(reader))}
    {}

    template<std::derived_from<GridReader> Reader>
    explicit PolyLineReaderAdapter(std::unique_ptr<Reader>&& reader)
    : _reader{std::move(reader)}
    {}

 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        _reader->open(filename);
        std::ranges::copy(point_field_names(*_reader), std::back_inserter(fields.point_fields));
        std::ranges::copy(cell_field_names(*_reader), std::back_inserter(fields.cell_fields));
        std::ranges::copy(meta_data_field_names(*_reader), std::back_inserter(fields.meta_data_fields));
    }

    void _close() override {
        _reader->close();
    }

    std::string _name() const override {
        return "PolyLineReaderAdapter<" + _reader->name() + ">";
    }

    std::size_t _number_of_cells() const override {
        std::size_t result = 0;
        _reader->visit_cells([&] (CellType ct, const std::vector<std::size_t>& corners) {
            result += ct == CellType::polyline ? corners.size() - 1 : 1;
        });
        return result;
    }

    std::size_t _number_of_points() const override {
        return _reader->number_of_points();
    }

    std::size_t _number_of_pieces() const override {
        return _reader->number_of_pieces();
    }

    bool _is_sequence() const override {
        return _reader->is_sequence();
    }

    FieldPtr _points() const override {
        return _reader->points();
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        _reader->visit_cells([&] (CellType ct, const std::vector<std::size_t>& corners) {
            if (ct == CellType::polyline) {
                std::vector<std::size_t> sub_segment_corners(2);
                for (std::size_t i = 0; i < corners.size() - 1; ++i) {
                    sub_segment_corners[0] = corners[i];
                    sub_segment_corners[1] = corners[i + 1];
                    visitor(CellType::segment, sub_segment_corners);
                }
            } else {
                visitor(ct, corners);
            }
        });
    }

    FieldPtr _cell_field(std::string_view name) const override {
        FieldPtr raw_field = _reader->cell_field(name);
        const auto raw_layout = raw_field->layout();
        const auto raw_data = raw_field->serialized();
        auto adapted_layout = raw_layout.dimension() > 1
            ? MDLayout{_number_of_cells()}.with_sub_layout(raw_layout.sub_layout(1))
            : MDLayout{_number_of_cells()};

        return raw_field->precision().visit([&] <typename T> (const Precision<T>&) -> FieldPtr {
            const std::size_t number_of_components = raw_layout.dimension() > 1 ? raw_layout.number_of_entries(1) : 1;
            const std::span raw_buffer = raw_data.template as_span_of<T>();
            std::vector<T> adapted_buffer(adapted_layout.number_of_entries());

            std::size_t raw_index = 0;
            std::size_t adapted_index = 0;
            const auto copy = [&] () {
                std::copy_n(
                    raw_buffer.begin() + raw_index*number_of_components,
                    number_of_components,
                    adapted_buffer.begin() + adapted_index*number_of_components
                );
            };

            _reader->visit_cells([&] (CellType ct, const std::vector<std::size_t>& corners) {
                if (ct == CellType::polyline) {
                    for (std::size_t i = 0; i < corners.size() - 1; ++i) {
                        copy();
                        adapted_index++;
                    }
                } else {
                    copy();
                }
                raw_index++;
            });

            return make_field_ptr(BufferField{
                std::move(adapted_buffer),
                std::move(adapted_layout)
            });
        });
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _reader->point_field(name);
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _reader->meta_data_field(name);
    }

    std::unique_ptr<GridReader> _reader;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_ADAPTER_POLYLINE_READER_ADAPTER_HPP_
