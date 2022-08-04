#pragma once

#include "serialization.h"

#include <glm/glm.hpp>

namespace reflection {

template<glm::length_t L, typename T, glm::qualifier Q>
class Type<ISerialization, glm::vec<L, T, Q>>
    : public TypeBase<ISerialization, glm::vec<L, T, Q>> {
public:
    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        _SerializationArrayTypeHelper<T, L>::Serialize(addr, writer);
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        _SerializationArrayTypeHelper<T, L>::Deserialize(addr, value);
    }
};

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
class Type<ISerialization, glm::mat<C, R, T, Q>>
    : public TypeBase<ISerialization, glm::mat<C, R, T, Q>> {
public:
    using ValueType = glm::mat<C, R, T, Q>;
    using LineT = typename ValueType::col_type;

    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        _SerializationArrayTypeHelper<LineT, ValueType::length()>::Serialize(addr, writer);
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        _SerializationArrayTypeHelper<LineT, ValueType::length()>::Deserialize(addr, value);
    }
};

} // namespace reflection
