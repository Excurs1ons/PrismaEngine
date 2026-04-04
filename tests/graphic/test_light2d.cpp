#include "graphic/2d/Light2D.h"
#include <gtest/gtest.h>

using namespace Prisma::Graphic;

TEST(Light2DTest, DefaultConstruction) {
    Light2D light;

    EXPECT_EQ(light.GetType(), Light2D::Type::Point);
    EXPECT_EQ(light.GetPosition(), Prisma::Vector2(0.0f, 0.0f));
    EXPECT_EQ(light.GetColor(), Prisma::Vector3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(light.GetIntensity(), 1.0f);
    EXPECT_FLOAT_EQ(light.GetRadius(), 100.0f);
    EXPECT_FALSE(light.IsCastShadows());
}

TEST(Light2DTest, SetPropertyUpdates) {
    Light2D light;

    light.SetType(Light2D::Type::Spot);
    EXPECT_EQ(light.GetType(), Light2D::Type::Spot);

    light.SetPosition({100.0f, 200.0f});
    EXPECT_EQ(light.GetPosition(), Prisma::Vector2(100.0f, 200.0f));

    light.SetColor({1.0f, 0.0f, 0.0f});
    EXPECT_EQ(light.GetColor(), Prisma::Vector3(1.0f, 0.0f, 0.0f));

    light.SetIntensity(2.5f);
    EXPECT_FLOAT_EQ(light.GetIntensity(), 2.5f);

    light.SetRadius(150.0f);
    EXPECT_FLOAT_EQ(light.GetRadius(), 150.0f);

    light.SetCastShadows(true);
    EXPECT_TRUE(light.IsCastShadows());
}

TEST(Light2DTest, DirectionalLightProperties) {
    Light2D light(Light2D::Type::Directional);

    light.SetDirection({1.0f, -1.0f});
    EXPECT_EQ(light.GetDirection(), Prisma::Vector2(1.0f, -1.0f));

    // 方向光应该忽略位置和半径
    light.SetPosition({100.0f, 100.0f});
    light.SetRadius(50.0f);

    // 方向光的衰减应该基于方向而非距离
    EXPECT_TRUE(light.IsDirectional());
}

TEST(Light2DTest, SpotLightConeAngle) {
    Light2D light(Light2D::Type::Spot);

    light.SetSpotAngle(45.0f);
    EXPECT_FLOAT_EQ(light.GetSpotAngle(), 45.0f);

    light.SetSpotAngle(120.0f);
    EXPECT_FLOAT_EQ(light.GetSpotAngle(), 120.0f);

    // 角度应该限制在合理范围内
    light.SetSpotAngle(200.0f);
    EXPECT_FLOAT_EQ(light.GetSpotAngle(), 180.0f);
}