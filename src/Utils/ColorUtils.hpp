#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class ColorUtils {
public:
    static ccColor3B generateRandomColor();
    static ccColor3B lightenColor(ccColor3B color, float factor = 0.3f);
    static ccColor3B darkenColor(ccColor3B color, float factor = 0.3f);
    static float getColorBrightness(ccColor3B color);
    static bool isColorLight(ccColor3B color);
    static ccColor3B getContrastColor(ccColor3B color);
};
