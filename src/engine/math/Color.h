// #pragma once
//
// #include "MathTypes.h"
// #include "Math.h"
// #include <string>
// #include <cstdint>
// #include <cmath>
//
//
// namespace Engine::Math {
//
// /**
//  * @brief 颜色结构体 - Prisma::Vector4的包装器，类似Unity的Color类型
//  *
//  * 提供RGBA颜色值的便捷操作，支持多种颜色空间和格式转换
//  * 跨平台兼容：Windows使用DirectXMath，Android使用GLM
//  */
// class Color {
// public:
//     // ========== 构造函数 ==========
//
//     /**
//      * @brief 默认构造 - 白色 (1, 1, 1, 1)
//      */
//     Color() : m_value(PrismaMath::vec4(1.0f, 1.0f, 1.0f, 1.0f)) {}
//
//     /**
//      * @brief RGB构造 - Alpha = 1.0
//      */
//     Color(float r, float g, float b) : m_value(PrismaMath::vec4(r, g, b, 1.0f)) {}
//
//     /**
//      * @brief RGBA构造
//      */
//     Color(float r, float g, float b, float a) : m_value(PrismaMath::vec4(r, g, b, a)) {}
//
//     /**
//      * @brief 从Prisma::Vector4构造
//      */
//     explicit Color(const Prisma::Vector4& vec) : m_value(vec) {}
//
//     /**
//      * @brief 从32位ARGB整数构造 (0xAARRGGBB)
//      */
//     explicit Color(uint32_t argb)
//         : m_value(PrismaMath::vec4(static_cast<float>((argb >> 16) & 0xFF) / 255.0f,  // R
//                                   static_cast<float>((argb >> 8) & 0xFF) / 255.0f,   // G
//                                   static_cast<float>(argb & 0xFF) / 255.0f,          // B
//                                   static_cast<float>((argb >> 24) & 0xFF) / 255.0f   // A
//                                   )) {}
//
//     // ========== 静态工厂方法 ==========
//
//     /**
//      * @brief 从HSV创建颜色
//      */
//     static Color FromHSV(float h, float s, float v, float a = 1.0f);
//
//     /**
//      * @brief 从HEX字符串创建颜色 (#RRGGBB 或 #AARRGGBB)
//      */
//     static Color FromHex(const std::string& hex);
//
//     /**
//      * @brief 从32位整数创建颜色
//      */
//     static Color FromARGB(uint32_t argb) { return Color(argb); }
//     static Color FromRGBA(uint32_t rgba);
//
//     // ========== 预定义颜色常量 ==========
//
//     static Color Clear() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
//     static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
//     static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
//     static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
//     static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
//     static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
//     static Color Yellow() { return Color(1.0f, 1.0f, 0.0f, 1.0f); }
//     static Color Magenta() { return Color(1.0f, 0.0f, 1.0f, 1.0f); }
//     static Color Cyan() { return Color(0.0f, 1.0f, 1.0f, 1.0f); }
//     static Color Gray() { return Color(0.5f, 0.5f, 0.5f, 1.0f); }
//     static Color Grey() { return Gray(); }
//     static Color Orange() { return Color(1.0f, 0.5f, 0.0f, 1.0f); }
//     static Color Purple() { return Color(0.5f, 0.0f, 0.5f, 1.0f); }
//
//     // ========== 访问器 ==========
//
//     float r() const { return m_value.x; }
//     float g() const { return m_value.y; }
//     float b() const { return m_value.z; }
//     float a() const { return m_value.w; }
//
//     void SetR(float r) { m_value.x = r; }
//     void SetG(float g) { m_value.y = g; }
//     void SetB(float b) { m_value.z = b; }
//     void SetA(float a) { m_value.w = a; }
//
//     void SetRGBA(float r, float g, float b, float a) { m_value = PrismaMath::vec4(r, g, b, a); }
//
//     // ========== 运算符重载 ==========
//
//     // 相等性比较
//     bool operator==(const Color& other) const {
//         const float epsilon = 0.001f;
//         return std::abs(r() - other.r()) < epsilon && std::abs(g() - other.g()) < epsilon &&
//                std::abs(b() - other.b()) < epsilon && std::abs(a() - other.a()) < epsilon;
//     }
//
//     bool operator!=(const Color& other) const { return !(*this == other); }
//
//     // 算术运算
//     Color operator+(const Color& other) const { return Color(m_value + other.m_value); }
//
//     Color operator-(const Color& other) const { return Color(m_value - other.m_value); }
//
//     Color operator*(const Color& other) const { return Color(m_value * other.m_value); }
//
//     Color operator*(float scalar) const { return Color(m_value * scalar); }
//
//     Color operator/(const Color& other) const { return Color(m_value / other.m_value); }
//
//     Color operator/(float scalar) const { return Color(m_value * (1.0f / scalar)); }
//
//     // 赋值运算
//     Color& operator+=(const Color& other) {
//         m_value = m_value + other.m_value;
//         return *this;
//     }
//
//     Color& operator-=(const Color& other) {
//         m_value = m_value - other.m_value;
//         return *this;
//     }
//
//     Color& operator*=(const Color& other) {
//         m_value = m_value * other.m_value;
//         return *this;
//     }
//
//     Color& operator*=(float scalar) {
//         m_value = Prisma::Math::Multiply(m_value, scalar);
//         return *this;
//     }
//
//     Color& operator/=(const Color& other) {
//         m_value = m_value / other.m_value;
//         return *this;
//     }
//
//     Color& operator/=(float scalar) {
//         m_value = Prisma::Math::Multiply(m_value, 1.0f / scalar);
//         return *this;
//     }
//
//     // ========== 颜色操作 ==========
//
//     /**
//      * @brief 线性插值
//      */
//     static Color Lerp(const Color& a, const Color& b, float t) { return Color(PrismaMath::mix(a.m_value, b.m_value, t)); }
//
//     /**
//      * @brief 平滑插值 (Smoothstep)
//      */
//     static Color SmoothStep(const Color& a, const Color& b, float t);
//
//     /**
//      * @brief 获取亮度 (灰度值)
//      */
//     float GetLuminance() const {
//         // 使用标准亮度权重: 0.2126*R + 0.7152*G + 0.0722*B
//         return r() * 0.2126f + g() * 0.7152f + b() * 0.0722f;
//     }
//
//     /**
//      * @brief 转换为灰度颜色
//      */
//     Color ToGrayscale() const {
//         float luminance = GetLuminance();
//         return Color(luminance, luminance, luminance, a());
//     }
//
//     /**
//      * @brief 转换为HSV
//      */
//     void ToHSV(float& h, float& s, float& v) const;
//
//     /**
//      * @brief 调整亮度
//      */
//     Color AdjustBrightness(float factor) const { return Color(r() * factor, g() * factor, b() * factor, a()); }
//
//     /**
//      * @brief 调整饱和度
//      */
//     Color AdjustSaturation(float factor) const;
//
//     /**
//      * @brief 调整对比度
//      */
//     Color AdjustContrast(float factor) const;
//
//     /**
//      * @brief 反转颜色
//      */
//     Color Inverted() const { return Color(1.0f - r(), 1.0f - g(), 1.0f - b(), a()); }
//
//     // ========== 颜色空间转换 ==========
//
//     /**
//      * @brief Gamma校正
//      */
//     Color GammaCorrect(float gamma = 2.2f) const {
//         return Color(std::pow(r(), 1.0f / gamma), std::pow(g(), 1.0f / gamma), std::pow(b(), 1.0f / gamma), a());
//     }
//
//     /**
//      * @brief Linear到sRGB转换
//      */
//     Color LinearToSRGB() const;
//
//     /**
//      * @brief sRGB到Linear转换
//      */
//     Color SRGBToLinear() const;
//
//     // ========== 格式转换 ==========
//
//     /**
//      * @brief 转换为Prisma::Vector4 (隐式转换)
//      */
//     operator Prisma::Vector4() const { return m_value; }
//
//     /**
//      * @brief 转换为32位ARGB整数
//      */
//     uint32_t ToARGB() const {
//         return ((static_cast<uint32_t>(a() * 255.0f) << 24) | (static_cast<uint32_t>(r() * 255.0f) << 16) |
//                 (static_cast<uint32_t>(g() * 255.0f) << 8) | (static_cast<uint32_t>(b() * 255.0f)));
//     }
//
//     /**
//      * @brief 转换为32位RGBA整数
//      */
//     uint32_t ToRGBA() const {
//         return ((static_cast<uint32_t>(r() * 255.0f) << 24) | (static_cast<uint32_t>(g() * 255.0f) << 16) |
//                 (static_cast<uint32_t>(b() * 255.0f) << 8) | (static_cast<uint32_t>(a() * 255.0f)));
//     }
//
//     /**
//      * @brief 转换为HEX字符串
//      */
//     std::string ToHex(bool includeAlpha = false) const;
//
//     // ========== 调试和工具 ==========
//
//     /**
//      * @brief 获取字符串表示
//      */
//     std::string ToString() const;
//
//     /**
//      * @brief 验证颜色值是否在有效范围内 [0, 1]
//      */
//     bool IsValid() const {
//         return r() >= 0.0f && r() <= 1.0f && g() >= 0.0f && g() <= 1.0f && b() >= 0.0f && b() <= 1.0f && a() >= 0.0f &&
//                a() <= 1.0f;
//     }
//
//     /**
//      * @brief 限制颜色值到有效范围 [0, 1]
//      */
//     Color& Clamp() {
//         m_value.x = Prisma::Math::Clamp(m_value.x, 0.0f, 1.0f);
//         m_value.y = Prisma::Math::Clamp(m_value.y, 0.0f, 1.0f);
//         m_value.z = Prisma::Math::Clamp(m_value.z, 0.0f, 1.0f);
//         m_value.w = Prisma::Math::Clamp(m_value.w, 0.0f, 1.0f);
//         return *this;
//     }
//
//     /**
//      * @brief 获取限制后的颜色
//      */
//     Color Clamped() const {
//         return Color(Prisma::Math::Clamp(r(), 0.0f, 1.0f),
//                      Prisma::Math::Clamp(g(), 0.0f, 1.0f),
//                      Prisma::Math::Clamp(b(), 0.0f, 1.0f),
//                      Prisma::Math::Clamp(a(), 0.0f, 1.0f));
//     }
//
// private:
//     Prisma::Vector4 m_value;
// };
//
// // ========== 全局运算符 ==========
//
// inline Color operator*(float scalar, const Color& color) {
//     return color * scalar;
// }
//
// // ========== RGBA构造函数实现 ==========
//
// Color Color::FromRGBA(uint32_t rgba) {
//     return Color(
//         static_cast<float>((rgba >> 24) & 0xFF) / 255.0f,  // R
//         static_cast<float>((rgba >> 16) & 0xFF) / 255.0f,  // G
//         static_cast<float>((rgba >> 8) & 0xFF) / 255.0f,   // B
//         static_cast<float>(rgba & 0xFF) / 255.0f           // A
//     );
// }
//
// } // namespace Engine::Math
