#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define sprintf sprintf_s
#include <stb_image_write.h>
#undef sprintf

namespace {
	struct StbiImageFlip{
		StbiImageFlip() {
			stbi_set_flip_vertically_on_load(true);
			stbi_flip_vertically_on_write(true);
		}
	} _;
}
