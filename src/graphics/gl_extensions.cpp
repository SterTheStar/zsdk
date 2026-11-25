/**
 * @file gl_extensions.cpp
 * @brief GLContext::HasExtension() - whole-token match against glGetString(GL_EXTENSIONS).
 */

#include "zsdk/graphics/gl_context.hpp"
#include "gl_platform.hpp"

#include <string.h>

namespace zsdk {
namespace graphics {

bool GLContext::HasExtension(const char* name) const {
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    const char* list = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (list == nullptr) {
        return false;
    }
    const size_t len = strlen(name);
    const char* p = list;
    while ((p = strstr(p, name)) != nullptr) {
        const bool startOk = (p == list) || (p[-1] == ' ');
        const bool endOk   = (p[len] == ' ') || (p[len] == '\0');
        if (startOk && endOk) {
            return true;
        }
        p += len;
    }
    return false;
}

} // namespace graphics
} // namespace zsdk
