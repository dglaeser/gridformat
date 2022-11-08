// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_COMMON_DETAIL_CRTP_HPP_
#define GRIDFORMAT_COMMON_DETAIL_CRTP_HPP_

#ifndef DOXYGEN_SKIP_DETAILS
namespace GridFormat::Detail {

template<typename Impl, typename Base>
Impl& cast_to_impl_ref(Base* base_ptr) {
    return static_cast<Impl&>(*base_ptr);
}

template<typename Impl, typename Base>
Impl* cast_to_impl_ptr(Base* base_ptr) {
    return static_cast<Impl*>(base_ptr);
}

template<typename Impl>
class CRTPBase {
 protected:
    Impl& _impl() { return cast_to_impl_ref<Impl>(this); }
    Impl* _pimpl() { return cast_to_impl_ptr<Impl>(this); }

    const Impl& _impl() const { return cast_to_impl_ref<const Impl>(this); }
    const Impl* _pimpl() const { return cast_to_impl_ptr<const Impl>(this); }
};

}  // end namespace GridFormat::Detail

#endif  // DOXYGEN_SKIP_DETAILS
#endif  // GRIDFORMAT_COMMON_DETAIL_CRTP_HPP_