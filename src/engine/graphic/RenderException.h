#pragma once
#include <stdexcept>
#include <string>

namespace Engine::Graphic {

class RenderException : public std::runtime_error {
public:
    RenderException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace Engine::Graphic