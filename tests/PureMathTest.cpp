#include "TestFramework.h"
#include <cmath>
#include <vector>
#include <string>

// 纯数学测试 - 无平台依赖
class PureMathTestSuite : public TestFramework::TestSuite {
public:
    PureMathTestSuite() : TestSuite("Pure Math Tests") {
        AddTest(new VectorTest());
        AddTest(new MatrixTest());
        AddTest(new MathUtilsTest());
    }

private:
    // 简单Vector3类（不使用DirectXMath）
    struct Vector3 {
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        Vector3 operator+(const Vector3& other) const {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        Vector3 operator-(const Vector3& other) const {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }

        Vector3 operator*(float s) const {
            return Vector3(x * s, y * s, z * s);
        }

        float Dot(const Vector3& other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        float Length() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        Vector3 Cross(const Vector3& other) const {
            return Vector3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }

        Vector3 Normalize() const {
            float len = Length();
            return Vector3(x / len, y / len, z / len);
        }

        static Vector3 Zero() { return Vector3(0, 0, 0); }
        static Vector3 One() { return Vector3(1, 1, 1); }
        static Vector3 Forward() { return Vector3(0, 0, 1); }
        static Vector3 Up() { return Vector3(0, 1, 0); }
        static Vector3 Right() { return Vector3(1, 0, 0); }
    };

    // 简单Matrix4类（4x4矩阵）
    struct Matrix4 {
        float m[16];

        Matrix4() {
            Identity();
        }

        explicit Matrix4(const float* data) {
            for (int i = 0; i < 16; ++i) {
                m[i] = data[i];
            }
        }

        void Identity() {
            for (int i = 0; i < 16; ++i) {
                m[i] = 0.0f;
            }
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }

        float& At(int row, int col) {
            return m[row * 4 + col];
        }

        const float& At(int row, int col) const {
            return m[row * 4 + col];
        }

        Vector3 TransformPoint(const Vector3& p) const {
            float x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
            float y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
            float z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
            return Vector3(x, y, z);
        }

        Vector3 TransformDirection(const Vector3& d) const {
            float x = m[0] * d.x + m[4] * d.y + m[8] * d.z;
            float y = m[1] * d.x + m[5] * d.y + m[9] * d.z;
            float z = m[2] * d.x + m[6] * d.y + m[10] * d.z;
            return Vector3(x, y, z);
        }
    };

    // Vector3测试
    class VectorTest : public TestFramework::TestCase {
    public:
        VectorTest() : TestFramework::TestCase("Vector Operations") {}
    protected:
        void RunTest() override {
            Vector3 v1(1.0f, 2.0f, 3.0f);
            Vector3 v2(4.0f, 5.0f, 6.0f);

            // 加法
            Vector3 sum = v1 + v2;
            AssertEquals(5.0f, sum.x);
            AssertEquals(7.0f, sum.y);
            AssertEquals(9.0f, sum.z);

            // 减法
            Vector3 diff = v2 - v1;
            AssertEquals(3.0f, diff.x);
            AssertEquals(3.0f, diff.y);
            AssertEquals(3.0f, diff.z);

            // 缩放
            Vector3 scaled = v1 * 2.0f;
            AssertEquals(2.0f, scaled.x);
            AssertEquals(4.0f, scaled.y);
            AssertEquals(6.0f, scaled.z);

            // 点乘
            float dot = v1.Dot(v2);
            AssertEquals(32.0f, dot, 0.001f);

            // 叉乘
            Vector3 v3(1.0f, 0.0f, 0.0f);
            Vector3 v4(0.0f, 1.0f, 0.0f);
            Vector3 cross = v3.Cross(v4);
            AssertNear(0.0f, cross.x, 0.001f);
            AssertNear(0.0f, cross.y, 0.001f);
            AssertNear(1.0f, cross.z, 0.001f);

            // 长度
            float length = v1.Length();
            AssertNear(std::sqrt(14.0f), length, 0.001f);

            // 归一化
            Vector3 normalized = Vector3(2.0f, 0.0f, 0.0f).Normalize();
            AssertNear(1.0f, normalized.Length(), 0.001f);
            AssertNear(1.0f, normalized.x, 0.001f);
            AssertNear(0.0f, normalized.y, 0.001f);
            AssertNear(0.0f, normalized.z, 0.001f);
        }
    };

    // Matrix4测试
    class MatrixTest : public TestFramework::TestCase {
    public:
        MatrixTest() : TestFramework::TestCase("Matrix Operations") {}
    protected:
        void RunTest() override {
            Matrix4 m;

            // 单位矩阵测试
            AssertEquals(1.0f, m.At(0, 0));
            AssertEquals(1.0f, m.At(1, 1));
            AssertEquals(1.0f, m.At(2, 2));
            AssertEquals(1.0f, m.At(3, 3));

            // 确保其他位置为0
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (i == j) continue;
                    AssertEquals(0.0f, m.At(i, j));
                }
            }

            // 单位矩阵不改变点
            Vector3 point(5.0f, 3.0f, 2.0f);
            Vector3 transformed = m.TransformPoint(point);
            AssertEquals(5.0f, transformed.x);
            AssertEquals(3.0f, transformed.y);
            AssertEquals(2.0f, transformed.z);

            // 单位矩阵不改变方向
            Vector3 direction(1.0f, 1.0f, 1.0f);
            Vector3 transformedDir = m.TransformDirection(direction);
            AssertEquals(1.0f, transformedDir.x);
            AssertEquals(1.0f, transformedDir.y);
            AssertEquals(1.0f, transformedDir.z);
        }
    };

    // 数学工具函数测试
    class MathUtilsTest : public TestFramework::TestCase {
    public:
        MathUtilsTest() : TestFramework::TestCase("Math Utilities") {}
    protected:
        void RunTest() override {
            // 线性插值
            auto Lerp = [](float a, float b, float t) {
                return a + (b - a) * t;
            };

            AssertNear(0.0f, Lerp(0.0f, 10.0f, 0.0f), 0.001f);
            AssertNear(5.0f, Lerp(0.0f, 10.0f, 0.5f), 0.001f);
            AssertNear(10.0f, Lerp(0.0f, 10.0f, 1.0f), 0.001f);

            // 夹钳
            auto Clamp = [](float value, float min, float max) {
                if (value < min) return min;
                if (value > max) return max;
                return value;
            };

            AssertEquals(5.0f, Clamp(5.0f, 0.0f, 10.0f));
            AssertEquals(0.0f, Clamp(-5.0f, 0.0f, 10.0f));
            AssertEquals(10.0f, Clamp(15.0f, 0.0f, 10.0f));

            // 绝对值
            AssertEquals(5.0f, std::abs(-5.0f));
            AssertEquals(5.0f, std::abs(5.0f));
            AssertEquals(0.0f, std::abs(0.0f));

            // 最小值
            AssertEquals(3.0f, std::min(3.0f, 7.0f));
            AssertEquals(3.0f, std::min(7.0f, 3.0f));

            // 最大值
            AssertEquals(7.0f, std::max(3.0f, 7.0f));
            AssertEquals(7.0f, std::max(7.0f, 3.0f));

            // 弧度转弧度
            auto DegToRad = [](float degrees) {
                return degrees * M_PI / 180.0f;
            };

            AssertNear(M_PI, DegToRad(180.0f), 0.001f);
            AssertNear(M_PI / 2.0f, DegToRad(90.0f), 0.001f);

            // 三角函数
            AssertNear(0.0f, std::sin(0.0f), 0.001f);
            AssertNear(1.0f, std::sin(M_PI / 2.0f), 0.001f);
            AssertNear(1.0f, std::cos(0.0f), 0.001f);
            AssertNear(0.0f, std::cos(M_PI / 2.0f), 0.001f);
        }
    };
};

// 注册测试套件
REGISTER_TEST_SUITE(PureMathTestSuite)