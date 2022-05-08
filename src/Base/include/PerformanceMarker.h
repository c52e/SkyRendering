#pragma once

#include <glad/glad.h>

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define PERF_MARKER(message) DebugGroup CONCAT(__debug_group__, __LINE__)(message);

class DebugGroup {
public:
	DebugGroup(const char* message) {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, message);
	}
	~DebugGroup() {
		glPopDebugGroup();
	}
};
