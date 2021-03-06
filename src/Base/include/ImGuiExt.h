#pragma once

#include <magic_enum.hpp>
#include <imgui.h>

#include <reflection/util.h>

template<class Enum>
void EnumSelectable(const char* name, Enum* penum) {
    static_assert(std::is_enum_v<Enum>);
    static_assert(magic_enum::enum_count<Enum>() > 0);
    static std::array<std::string, magic_enum::enum_count<Enum>()> enums;
    if (enums[0].empty()) {
        constexpr auto& names = magic_enum::enum_names<Enum>();
        auto i = enums.begin();
        auto j = names.begin();
        while (i < enums.end())
            *i++ = *j++;
    }
    char buf[MaxEnumStringViewSize<Enum>() + 1];
    auto curstrview = magic_enum::enum_name(*penum);
    memcpy(buf, &curstrview[0], curstrview.size());
    buf[curstrview.size()] = 0;
    if (ImGui::BeginCombo(name, buf)) {
        for (const auto& enumstr : enums) {
            bool is_selected = strcmp(buf, enumstr.c_str()) == 0;
            if (ImGui::Selectable(enumstr.c_str(), is_selected))
                *penum = magic_enum::enum_cast<Enum>(enumstr).value();
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}
