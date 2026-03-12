#include "ColorUtils.hpp"
#include <random>

ccColor3B ColorUtils::generateRandomColor() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(50, 205); // Avoid too dark/light colors
    
    return ccColor3B{
        static_cast<GLubyte>(dis(gen)),
        static_cast<GLubyte>(dis(gen)),
        static_cast<GLubyte>(dis(gen))
    };
}

ccColor3B ColorUtils::lightenColor(ccColor3B color, float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    
    return ccColor3B{
        static_cast<GLubyte>(color.r + (255 - color.r) * factor),
        static_cast<GLubyte>(color.g + (255 - color.g) * factor),
        static_cast<GLubyte>(color.b + (255 - color.b) * factor)
    };
}

ccColor3B ColorUtils::darkenColor(ccColor3B color, float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    
    return ccColor3B{
        static_cast<GLubyte>(color.r * (1.0f - factor)),
        static_cast<GLubyte>(color.g * (1.0f - factor)),
        static_cast<GLubyte>(color.b * (1.0f - factor))
    };
}

float ColorUtils::getColorBrightness(ccColor3B color) {
    return (color.r + color.g + color.b) / (3.0f * 255.0f);
}

bool ColorUtils::isColorLight(ccColor3B color) {
    return getColorBrightness(color) > 0.5f;
}

ccColor3B ColorUtils::getContrastColor(ccColor3B color) {
    return isColorLight(color) ? ccColor3B{0, 0, 0} : ccColor3B{255, 255, 255};
}
