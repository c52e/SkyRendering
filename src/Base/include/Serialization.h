#pragma once

#include <reflection/serialization_ext_glm.h>

using ISerializable = reflection::ISerialization;

#define FIELD_DECLARE(name, ...) FIELD_DECLARATION(#name, name, __VA_ARGS__)
