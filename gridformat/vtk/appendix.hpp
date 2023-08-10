// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Helper classes for writing VTK appendices of xml formats
 */
#ifndef GRIDFORMAT_VTK_APPENDIX_HPP_
#define GRIDFORMAT_VTK_APPENDIX_HPP_

#include <concepts>
#include <utility>
#include <ostream>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <type_traits>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/indentation.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/data_array.hpp>
#include <gridformat/vtk/attributes.hpp>

namespace GridFormat::VTK {

/*!
 * \ingroup VTK
 * \brief Observer for appendices.
 *        Allows registering the offsets when streaming
 *        all fields that are part of the appendix.
 */
class AppendixStreamObserver {
 public:
    void register_offset(std::size_t offset) {
        _offsets.push_back(offset);
    }

    const std::vector<std::size_t>& offsets() const {
        return _offsets;
    }

 private:
    std::vector<std::size_t> _offsets;
};

/*!
 * \ingroup VTK
 * \brief Stores vtk data arrays to be exported as vtk-xml appendix.
 */
class Appendix {
    struct DataArrayModel {
        virtual ~DataArrayModel() = default;
        friend std::ostream& operator<<(std::ostream& s, const DataArrayModel& arr) {
            arr.stream(s);
            return s;
        }

     private:
        virtual void stream(std::ostream& s) const = 0;
    };

    template<Concepts::StreamableWith<std::ostream> DataArray>
    class DataArrayImpl : public DataArrayModel {
     public:
        explicit DataArrayImpl(DataArray&& arr)
        : _data_array(std::move(arr))
        {}

     private:
        DataArray _data_array;

        void stream(std::ostream& s) const override {
            s << _data_array;
        }
    };

 public:
    template<typename... Args>
    void add(DataArray<Args...>&& data_array) {
        _emplace_back(DataArrayImpl{std::move(data_array)});
    }

    friend std::ostream& operator<<(std::ostream& s, const Appendix& app) {
        const auto start_pos = s.tellp();
        for (const auto& array_ptr : app._content) {
            const auto pos_before = s.tellp();
            s << *array_ptr;
            if (app._observer)
                app._observer->register_offset(pos_before - start_pos);
        }
        return s;
    }

    void set_observer(AppendixStreamObserver* observer) {
        _observer = observer;
    }

 private:
    template<typename DA>
    void _emplace_back(DataArrayImpl<DA>&& impl) {
        _content.emplace_back(
            std::make_unique<DataArrayImpl<DA>>(std::move(impl))
        );
    }

    std::vector<std::unique_ptr<const DataArrayModel>> _content;
    AppendixStreamObserver* _observer{nullptr};
};


#ifndef DOXYGEN
namespace XML::Detail {

    struct XMLAppendixContent {
        const Appendix& appendix;

        friend std::ostream& operator<<(std::ostream& s, const XMLAppendixContent& c) {
            s << " _";  // prefix required by vtk
            s << c.appendix;
            s << "\n";  // newline such that closing xml element is easily visible
            return s;
        }
    };

    void write_xml_element_with_offsets(const XMLElement& e,
                                        std::ostream& s,
                                        Indentation& ind,
                                        std::vector<std::size_t>& offset_positions) {
        const std::string empty_string_for_max_header(
            std::to_string(std::numeric_limits<std::size_t>::max()).size(),
            ' '
        );

        const auto cache_offset_xml_pos = [&] () {
            s << " offset=\"";
            offset_positions.push_back(s.tellp());
            s << empty_string_for_max_header;
            s << "\"";
        };

        if (!e.has_content() && e.number_of_children() == 0) {
            s << ind;
            GridFormat::XML::Detail::write_xml_tag_open(e, s, "");
            if (e.name() == "DataArray")
                cache_offset_xml_pos();
            s << "/>";
        } else {
            s << ind;
            GridFormat::XML::Detail::write_xml_tag_open(e, s);
            if (e.name() == "DataArray")
                cache_offset_xml_pos();
            s << "\n";

            if (e.has_content()) {
                e.stream_content(s);
                s << "\n";
            }

            ++ind;
            for (const auto& c : children(e)) {
                write_xml_element_with_offsets(c, s, ind, offset_positions);
                s << "\n";
            }
            --ind;

            s << ind;
            GridFormat::XML::Detail::write_xml_tag_close(e, s);
        }
    }

    std::vector<std::size_t> write_xml_element_with_offsets(const XMLElement& e,
                                                            std::ostream& s,
                                                            Indentation& ind) {
        std::vector<std::size_t> offset_positions;
        write_xml_element_with_offsets(e, s, ind, offset_positions);
        return offset_positions;
    }

    template<typename Context, typename Encoder>
        requires(!std::is_const_v<std::remove_reference_t<Context>>)
    inline void write_with_appendix(Context&& context,
                                    std::ostream& s,
                                    const Encoder& encoder,
                                    Indentation indentation = {}) {
        if (produces_valid_xml(encoder))
            s << "<?xml version=\"1.0\"?>\n";

        AppendixStreamObserver observer;
        context.appendix.set_observer(&observer);

        auto& app_element = context.xml_representation.add_child("AppendedData");
        app_element.set_attribute("encoding", attribute_name(encoder));
        app_element.set_content(Detail::XMLAppendixContent{context.appendix});

        const auto offset_positions = write_xml_element_with_offsets(
            context.xml_representation, s, indentation
        );
        const auto& offsets = observer.offsets();
        if (offsets.size() != offset_positions.size())
            throw SizeError("Number of written & registered offsets does not match");

        const auto cur_pos = s.tellp();
        for (std::size_t i = 0; i < offsets.size(); ++i) {
            s.seekp(offset_positions[i]);
            const std::string offset_str = std::to_string(offsets[i]);
            s.write(offset_str.data(), offset_str.size());
        }
        s.seekp(cur_pos);
    }

}  // namespace XML::Detail
#endif  // DOXYGEN

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_APPENDIX_HPP_
