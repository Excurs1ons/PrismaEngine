#include "TestFramework.h"
#include <DirectXMath.h>

using namespace DirectX;

// 数学测试套件
class MathTestSuite : public TestFramework::TestSuite {
public:
    MathTestSuite() : TestSuite("Math Tests") {
        AddTest(new Vector3Test());
        AddTest(new MatrixTest());
        AddTest(new QuaternionTest());
        AddTest(new MathFunctionsTest());
    }

private:
    // Vector3测试
    class Vector3Test : public TestCase {
    public:
        Vector3Test() : TestCase("Vector3") {}
    protected:
        void RunTest() override {
            XMVECTOR v1 = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
            XMVECTOR v2 = XMVectorSet(4.0f, 5.0f, 6.0f, 0.0f);

            // 加法
            XMVECTOR sum = XMVectorAdd(v1, v2);
            AssertNear(5.0f, XMVectorGetX(sum), 0.001f, "Vector addition X component");
            AssertNear(7.0f, XMVectorGetY(sum), 0.001f, "Vector addition Y component");
            AssertNear(9.0f, XMVectorGetZ(sum), 0.001f, "Vector addition Z component");

            // 点乘
            float dot = XMVectorGetX(XMVector3Dot(v1, v2));
            AssertNear(32.0f, dot, 0.001f, "Vector dot product");

            // 叉乘
            XMVECTOR v3 = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
            XMVECTOR v4 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            XMVECTOR cross = XMVector3Cross(v3, v4);
            XMVECTOR expected = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

            AssertNear(0.0f, XMVectorGetX(cross), 0.001f, "Cross product X");
            AssertNear(0.0f, XMVectorGetY(cross), 0.001f, "Cross product Y");
            AssertNear(1.0f, XMVectorGetZ(cross), 0.001f, "Cross product Z");

            // 长度
            float length = XMVectorGetX(XMVector3Length(v1));
            AssertNear(sqrtf(14.0f), length, 0.001f, "Vector length");
        }
    };

    // 矩阵测试
    class MatrixTest : public TestCase {
    public:
        MatrixTest() : TestCase("Matrix") {}
    protected:
        void RunTest() override {
            XMMATRIX identity = XMMatrixIdentity();

            // 单位矩阵性质
            XMFLOAT3X3 result;
            XMStoreFloat3x3(&result, identity);

            AssertNear(1.0f, result._11, 0.001f, "Identity matrix (1,1)");
            AssertNear(0.0f, result._12, 0.001f, "Identity matrix (1,2)");
            AssertNear(1.0f, result._33, 0.001f, "Identity matrix (3,3)");

            // 矩阵乘法
            XMMATRIX m1 = XMMatrixTranslation(1.0f, 2.0f, 3.0f);
            XMMATRIX m2 = XMMatrixRotationY(XM_PIDIV2);
            XMMATRIX product = XMMatrixMultiply(m1, m2);

            // 检查平移后的旋转
            XMFLOAT4X4 res;
            XMStoreFloat4x4(&res, product);
            AssertNear(-1.0f, res._41, 0.001f, "Matrix multiplication translation");
        }
    };

    // 四元数测试
    class QuaternionTest : public TestCase {
    public:
        QuaternionTest() : TestCase("Quaternion") {}
    protected:
        void RunTest() override {
            // 单位四元数
            XMMATRIX identity = XMMatrixIdentity();
            XMVECTOR q = XMQuaternionRotationMatrix(identity);
            AssertNear(0.0f, XMVectorGetX(q), 0.001f, "Identity quaternion X");
            AssertNear(0.0f, XMVectorGetY(q), 0.001f, "Identity quaternion Y");
            AssertNear(0.0f, XMVectorGetZ(q), 0.001f, "Identity quaternion Z");
            AssertNear(1.0f, XMVectorGetW(q), 0.001f, "Identity quaternion W");

            // 四元数归一化
            XMVECTOR unnormalized = XMVectorSet(2.0f, 2.0f, 2.0f, 2.0f);
            XMVECTOR normalized = XMQuaternionNormalize(unnormalized);
            float length = XMVectorGetX(XMVector3Length(normalized));
            AssertNear(1.0f, length, 0.001f, "Normalized quaternion length");

            // 四元数乘法
            XMVECTOR q1 = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_PIDIV2);
            XMVECTOR q2 = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), XM_PIDIV2);
            XMVECTOR q3 = XMQuaternionMultiply(q1, q2);

            // 验证四元数乘法结果
            XMMATRIX m = XMMatrixRotationQuaternion(q3);
            XMFLOAT4X4 matrixResult;
            XMStoreFloat4x4(&matrixResult, m);
            AssertNear(0.0f, matrixResult._11, 0.001f, "Quaternion multiplication matrix");
        }
    };

    // 数学函数测试
    class MathFunctionsTest : public TestCase {
    public:
        MathFunctionsTest() : TestCase("MathFunctions") {}
    protected:
        void RunTest() override {
            // 三角函数
            AssertNear(0.0f, sinf(0.0f), 0.001f, "sin(0)");
            AssertNear(1.0f, sinf(XM_PIDIV2), 0.001f, "sin(π/2)");
            AssertNear(0.0f, cosf(XM_PIDIV2), 0.001f, "cos(π/2)");

            // 插值
            float a = 0.0f;
            float b = 10.0f;
            AssertNear(5.0f, a + (b - a) * 0.5f, 0.001f, "Linear interpolation");

            // 夹角
            XMVECTOR v1 = XMVectorSet(1, 0, 0, 0);
            XMVECTOR v2 = XMVectorSet(0, 1, 0, 0);
            float angle = XMVectorGetX(XMVector3AngleBetweenVectors(v1, v2));
            AssertNear(XM_PIDIV2, angle, 0.001f, "Angle between vectors");
        }
    };
};

// 注册测试套件
REGISTER_TEST_SUITE(MathTestSuite)