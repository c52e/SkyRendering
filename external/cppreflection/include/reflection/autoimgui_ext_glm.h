#pragma once

#include "autoimgui.h"

#include <glm/glm.hpp>

namespace reflection {

template<glm::length_t L, typename T, glm::qualifier Q>
class Type<IAutoImGui, glm::vec<L, T, Q>>
    : public TypeBase<IAutoImGui, glm::vec<L, T, Q>> {
public:
    void DrawAutoImGui(void* addr, const char* name, const UserdataBase* userdata) const override {
        _AutoImGuiArrayTypeHelper<T, L>::DrawAutoImGui(addr, name, userdata);
    }
};

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
class Type<IAutoImGui, glm::mat<C, R, T, Q>>
    : public TypeBase<IAutoImGui, glm::mat<C, R, T, Q>> {
public:
    using ValueType = glm::mat<C, R, T, Q>;
    using LineT = typename ValueType::col_type;

    void DrawAutoImGui(void* addr, const char* name, const UserdataBase* userdata) const override {
        _AutoImGuiArrayTypeHelper<LineT, ValueType::length()>::DrawAutoImGui(addr, name, userdata);
    }
};

} // namespace reflection
