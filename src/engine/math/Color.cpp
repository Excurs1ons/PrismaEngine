#include "Color.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Engine {
namespace Math {

Color Color::FromHSV(float h, float s, float v, float a)
{
    // H: [0, 360), S: [0, 1], V: [0, 1]
    h = std::fmod(h, 360.0f);
    if (h < 0) h += 360.0f;

    s = std::clamp(s, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    a = std::clamp(a, 0.0f, 1.0f);

    float c = v * s; // Chroma
    float hPrime = h / 60.0f;
    float x = c * (1.0f - std::abs(std::fmod(hPrime, 2.0f) - 1.0f));
    float m = v - c;

    float rPrime = 0, gPrime = 0, bPrime = 0;

    if (hPrime >= 0 && hPrime < 1) {
        rPrime = c; gPrime = x;
    } else if (hPrime >= 1 && hPrime < 2) {
        rPrime = x; gPrime = c;
    } else if (hPrime >= 2 && hPrime < 3) {
        gPrime = c; bPrime = x;
    } else if (hPrime >= 3 && hPrime < 4) {
        gPrime = x; bPrime = c;
    } else if (hPrime >= 4 && hPrime < 5) {
        rPrime = x; bPrime = c;
    } else if (hPrime >= 5 && hPrime < 6) {
        rPrime = c; bPrime = x;
    }

    return Color(rPrime + m, gPrime + m, bPrime + m, a);
}

Color Color::FromHex(const std::string& hex)
{
    std::string trimmed = hex;

    // 移除#前缀
    if (trimmed.size() > 0 && trimmed[0] == '#') {
        trimmed = trimmed.substr(1);
    }

    // 验证长度
    if (trimmed.size() != 6 && trimmed.size() != 8) {
        throw std::invalid_argument("Invalid hex color format. Expected 6 or 8 hexadecimal digits.");
    }

    // 验证字符
    for (char c : trimmed) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            throw std::invalid_argument("Invalid hex color format. Contains non-hexadecimal characters.");
        }
    }

    try {
        uint32_t value = std::stoul(trimmed, nullptr, 16);

        if (trimmed.size() == 6) {
            // RGB格式，Alpha设为255
            return FromRGBA((value << 8) | 0xFF);
        } else {
            // ARGB格式
            return FromARGB(value);
        }
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid hex color format. Could not parse hexadecimal value.");
    }
}

Color Color::SmoothStep(const Color& a, const Color& b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    float smoothT = t * t * (3.0f - 2.0f * t); // Smoothstep function
    return Lerp(a, b, smoothT);
}

void Color::ToHSV(float& h, float& s, float& v) const
{
    float rNorm = std::clamp(r(), 0.0f, 1.0f);
    float gNorm = std::clamp(g(), 0.0f, 1.0f);
    float bNorm = std::clamp(b(), 0.0f, 1.0f);

    float maxVal = std::max({rNorm, gNorm, bNorm});
    float minVal = std::min({rNorm, gNorm, bNorm});
    float delta = maxVal - minVal;

    // Value
    v = maxVal;

    // Saturation
    if (maxVal == 0.0f) {
        s = 0.0f;
    } else {
        s = delta / maxVal;
    }

    // Hue
    if (delta == 0.0f) {
        h = 0.0f;
    } else {
        if (maxVal == rNorm) {
            h = 60.0f * std::fmod((gNorm - bNorm) / delta, 6.0f);
        } else if (maxVal == gNorm) {
            h = 60.0f * ((bNorm - rNorm) / delta + 2.0f);
        } else { // maxVal == bNorm
            h = 60.0f * ((rNorm - gNorm) / delta + 4.0f);
        }

        if (h < 0) h += 360.0f;
    }
}

Color Color::AdjustSaturation(float factor) const
{
    float h, s, v;
    ToHSV(h, s, v);

    s = std::clamp(s * factor, 0.0f, 1.0f);

    return FromHSV(h, s, v, a());
}

Color Color::AdjustContrast(float factor) const
{
    // 使用简单的对比度调整: color * (1 + factor) - 0.5 * factor
    float adjustedFactor = 1.0f + factor;
    float offset = -0.5f * factor;

    return Color(
        std::clamp(r() * adjustedFactor + offset, 0.0f, 1.0f),
        std::clamp(g() * adjustedFactor + offset, 0.0f, 1.0f),
        std::clamp(b() * adjustedFactor + offset, 0.0f, 1.0f),
        a()
    );
}

Color Color::LinearToSRGB() const
{
    auto linearToSrgb = [](float linear) -> float {
        if (linear <= 0.0031308f) {
            return 12.92f * linear;
        } else {
            return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
        }
    };

    return Color(
        linearToSrgb(r()),
        linearToSrgb(g()),
        linearToSrgb(b()),
        a()
    );
}

Color Color::SRGBToLinear() const
{
    auto srgbToLinear = [](float srgb) -> float {
        if (srgb <= 0.04045f) {
            return srgb / 12.92f;
        } else {
            return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
        }
    };

    return Color(
        srgbToLinear(r()),
        srgbToLinear(g()),
        srgbToLinear(b()),
        a()
    );
}

std::string Color::ToHex(bool includeAlpha) const
{
    std::stringstream ss;

    if (includeAlpha) {
        ss << std::hex << std::setfill('0') << std::setw(8) << ToARGB();
    } else {
        uint32_t rgb = ToARGB() & 0x00FFFFFF;
        ss << std::hex << std::setfill('0') << std::setw(6) << rgb;
    }

    std::string hexStr = ss.str();
    std::transform(hexStr.begin(), hexStr.end(), hexStr.begin(), ::toupper);

    return "#" + hexStr;
}

std::string Color::ToString() const
{
    std::stringstream ss;
    ss << "Color(" << std::fixed << std::setprecision(3)
       << r() << ", " << g() << ", " << b() << ", " << a() << ")";
    return ss.str();
}

} // namespace Math
} // namespace Engine