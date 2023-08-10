// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \ingroup XML
 * \copydoc GridFormat::XMLElement
 */
#ifndef GRIDFORMAT_XML_ELEMENT_HPP_
#define GRIDFORMAT_XML_ELEMENT_HPP_

#include <ranges>
#include <utility>
#include <string>
#include <ostream>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <memory>
#include <list>
#include <any>

#include <gridformat/common/path.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/indentation.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/optional_reference.hpp>
#include <gridformat/xml/tag.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

    template<typename Child>
    auto _child_find_lambda(std::string_view name) {
        return [name] (const Child& child) { return child.name() == name; };
    }

}  // end namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup XML
 * \brief Class to represent an XML element, i.e. an XML tag with a data body.
 */
class XMLElement : public XMLTag {
    using ChildStorage = std::list<XMLElement>;

    struct Content {
        virtual ~Content() {}
        virtual void stream(std::ostream& os) const = 0;
        virtual std::any get_as_any() const = 0;
    };

    template<Concepts::StreamableWith<std::ostream> C>
    struct ContentImpl : public Content {
        explicit ContentImpl(C&& c)
        : _c(std::forward<C>(c))
        {}

        void stream(std::ostream& os) const override {
            os << _c;
        }

        std::any get_as_any() const override {
            if constexpr (std::is_constructible_v<std::any, C>)
                return {_c};
            else
                throw InvalidState("Cannot parse content");
        }

     private:
        C _c;
    };

 public:
    explicit XMLElement(std::string name)
    : XMLTag(std::move(name))
    {}

    XMLElement(XMLElement&&) = default;
    XMLElement(const XMLElement&) = delete;
    XMLElement() = delete;

    XMLElement& parent() {
        _check_parent();
        return *_parent;
    }

    const XMLElement& parent() const {
        _check_parent();
        return *_parent;
    }

    bool has_parent() const {
        return _parent;
    }

    XMLElement& add_child(std::string name) {
        XMLElement child(std::move(name));
        child._parent = this;
        _children.emplace_back(std::move(child));
        return _children.back();
    }

    bool remove_child(std::string_view child_name) {
        return std::erase_if(
            _children,
            Detail::_child_find_lambda<XMLElement>(child_name)
        );
    }

    bool has_child(std::string_view child_name) const {
        return std::ranges::any_of(
            _children,
            Detail::_child_find_lambda<XMLElement>(child_name)
        );
    }

    XMLElement& get_child(std::string_view child_name) {
        auto it = std::ranges::find_if(
            _children,
            Detail::_child_find_lambda<XMLElement>(child_name)
        );
        if (it == _children.end())
            throw ValueError("XMLElement has no child '" + std::string{child_name} + "'");
        return *it;
    }

    const XMLElement& get_child(std::string_view child_name) const {
        auto it = std::ranges::find_if(
            _children,
            Detail::_child_find_lambda<XMLElement>(child_name)
        );
        if (it == _children.end())
            throw ValueError("XMLElement has no child '" + std::string{child_name} + "'");
        return *it;
    }

    std::size_t number_of_children() const {
        return _children.size();
    }

    template<Concepts::StreamableWith<std::ostream> C>
    void set_content(C&& content) {
        _content = std::make_unique<ContentImpl<C>>(std::forward<C>(content));
    }

    void stream_content(std::ostream& s) const {
        _content->stream(s);
    }

    bool has_content() const {
        return static_cast<bool>(_content);
    }

    template<typename T>
    T get_content() const {
        if (!has_content())
            throw ValueError("XMLElement has no content");
        return std::any_cast<T>(_content->get_as_any());
    }

    friend ChildStorage& children(XMLElement& e) {
        return e._children;
    }

    friend const ChildStorage& children(const XMLElement& e) {
        return e._children;
    }

 private:
    void _check_parent() const {
        if (!_parent)
            throw InvalidState("This xml element has no parent");
    }

    XMLElement* _parent{nullptr};
    ChildStorage _children;
    std::unique_ptr<Content> _content{nullptr};
};


/*!
 * \ingroup XML
 * \brief Return a reference to the element resulting from successively
 *        accessing the child elements as given by the provided path.
 * \param path The relative path starting from the given element
 * \param element The element at which to start traversing
 * \param delimiter The delimiter used to separate path entries.
 * \note If an element has multiple children with the same (matching) name,
 *       the first one will be selected.
 */
template<typename Element> requires(std::same_as<XMLElement, std::remove_cvref_t<Element>>)
OptionalReference<Element> access_at(std::string_view path, Element& element, char delimiter = '/') {
    if (path == "")
        return OptionalReference<Element>{element};
    Element* result = &element;
    for (const auto& name : Path::elements_of(path, delimiter)) {
        if (!result->has_child(name))
            return {};
        result = &result->get_child(name);
    }
    return OptionalReference<Element>{*result};
}

/*!
 * \ingroup XML
 * \brief Return a reference to the element resulting from successively
 *        accessing or creating the child elements as given by the provided path.
 * \param path The relative path starting from the given element
 * \param element The element at which to start traversing
 * \param delimiter The delimiter used to separate path entries.
 * \note If an element has multiple children with the same (matching) name,
 *       the first one will be selected.
 */
XMLElement& access_or_create_at(std::string_view path, XMLElement& element, char delimiter = '/') {
    if (path == "")
        return element;
    XMLElement* result = &element;
    std::ranges::for_each(Path::elements_of(path, delimiter), [&] (const auto& name) {
        result = result->has_child(name) ? &result->get_child(name)
                                         : &result->add_child(name);
    });
    return *result;
}


#ifndef DOXYGEN
namespace XML::Detail {

void write_xml_tag_open(const XMLElement& e,
                        std::ostream& s,
                        std::string_view close_char) {
    s << "<" << e.name();
    std::ranges::for_each(attributes(e),
        [&] (const std::string& attr_name) {
            s << " " << attr_name
                     << "=\""
                     << e.get_attribute(attr_name)
                     << "\"";
    });
    s << close_char;
}

void write_xml_tag_open(const XMLElement& e,
                        std::ostream& s) {
    write_xml_tag_open(e, s, ">");
}

void write_empty_xml_tag(const XMLElement& e,
                         std::ostream& s) {
    write_xml_tag_open(e, s, "/>");
}

void write_xml_tag_close(const XMLElement& e,
                        std::ostream& s) {
    s << "</" << e.name() << ">";
}

void write_xml_element(const XMLElement& e,
                       std::ostream& s,
                       Indentation& ind) {
    if (!e.has_content() && e.number_of_children() == 0) {
        s << ind;
        write_empty_xml_tag(e, s);
    } else {
        s << ind;
        write_xml_tag_open(e, s);
        s << "\n";

        if (e.has_content()) {
            e.stream_content(s);
            s << "\n";
        }

        ++ind;
        for (const auto& c : children(e)) {
            write_xml_element(c, s, ind);
            s << "\n";
        }
        --ind;

        s << ind;
        write_xml_tag_close(e, s);
    }
}

}  // end namespace XML::Detail
#endif  // DOXYGEN

inline void write_xml(const XMLElement& e,
                      std::ostream& s,
                      Indentation ind = {}) {
    XML::Detail::write_xml_element(e, s, ind);
}

inline void write_xml_with_version_header(const XMLElement& e,
                                          std::ostream& s,
                                          Indentation ind = {}) {
    s << "<?xml version=\"1.0\"?>\n";
    write_xml(e, s, ind);
}

}  // end namespace GridFormat

#endif  // GRIDFORMAT_XML_ELEMENT_HPP_
