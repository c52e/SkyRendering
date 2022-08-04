#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {
	struct StbiImageFlip{
		StbiImageFlip() {
			stbi_set_flip_vertically_on_load(true);
		}
	} _;
}
