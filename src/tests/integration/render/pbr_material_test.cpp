#include <gtest/gtest.h>
#include "graphic/Material.h"
#include "graphic/interfaces/IShader.h"

namespace Prisma::Graphic {
namespace {

// ============================================================================
// PBR Material — Parameter Round-trip
// ============================================================================
TEST(DISABLED_TestPBRMaterialParams, MetallicRoughnessAO) {
    auto material = Material::CreatePBR();
    ASSERT_NE(material, nullptr);
    ASSERT_TRUE(material->IsLoaded());

    material->SetMetallic(0.8f);
    const auto* metallicParam = material->GetParam("Metallic");
    ASSERT_NE(metallicParam, nullptr);
    ASSERT_TRUE(std::holds_alternative<float>(*metallicParam));
    EXPECT_FLOAT_EQ(std::get<float>(*metallicParam), 0.8f);

    material->SetRoughness(0.3f);
    const auto* roughnessParam = material->GetParam("Roughness");
    ASSERT_NE(roughnessParam, nullptr);
    ASSERT_TRUE(std::holds_alternative<float>(*roughnessParam));
    EXPECT_FLOAT_EQ(std::get<float>(*roughnessParam), 0.3f);

    material->SetAO(0.5f);
    const auto* aoParam = material->GetParam("AO");
    ASSERT_NE(aoParam, nullptr);
    ASSERT_TRUE(std::holds_alternative<float>(*aoParam));
    EXPECT_FLOAT_EQ(std::get<float>(*aoParam), 0.5f);
}

// ============================================================================
// PBR Material — Descriptor Set After Parameter Update
// ============================================================================
TEST(DISABLED_TestPBRDescriptorSet, DescriptorSetAfterUpdate) {
    auto material = Material::CreatePBR();
    ASSERT_NE(material, nullptr);

    IDescriptorSet* initialDesc = material->GetDescriptorSet();

    material->SetMetallic(0.5f);
    material->SetRoughness(0.2f);
    material->SetAO(1.0f);

    material->SetBaseColor(0.2f, 0.4f, 0.6f, 1.0f);

    const auto* baseColor = material->GetParam("BaseColor");
    ASSERT_NE(baseColor, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrismaMath::vec4>(*baseColor));
    auto color = std::get<PrismaMath::vec4>(*baseColor);
    EXPECT_FLOAT_EQ(color.x, 0.2f);
    EXPECT_FLOAT_EQ(color.y, 0.4f);
    EXPECT_FLOAT_EQ(color.z, 0.6f);

    IDescriptorSet* updatedDesc = material->GetDescriptorSet();
    EXPECT_EQ(updatedDesc, initialDesc);
}

// ============================================================================
// PBR Material — Material Type Identity
// ============================================================================
TEST(DISABLED_TestPBRMaterialTextures, TypeAndShader) {
    auto material = Material::CreatePBR();
    ASSERT_NE(material, nullptr);

    EXPECT_EQ(material->GetType(), AssetType::Material);

    auto shader = material->GetShader();
    EXPECT_NE(shader, nullptr) << "PBR material should have an associated shader";

    if (shader) {
        EXPECT_EQ(shader->GetResourceType(), ResourceType::Shader);
    }
}

} // namespace
} // namespace Prisma::Graphic
