#pragma once

#include "../geometry/GXGeometryEnums.hpp"
#include <cstdint>

struct GXVertexAttributeList {
	EGXAttribute Attribute;
	EGXComponentCount ComponentCount;
	EGXComponentType ComponentType;
	uint8_t Fraction;
};