#include <gtest/gtest.h>
#include "graphic/Material.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IResource.h"

namespace Prisma::Graphic {
namespace {

// 最小化 IShader 测试替身：仅用于 Material 构造
class TestShader : public IShader {
public:
    TestShader() { m_id = 42; m_isLoaded = true; }

    ResourceType GetResourceType() const override { return ResourceType::Shader; }
    ShaderType GetShaderType() const override { return ShaderType::Vertex; }
    ShaderLanguage GetLanguage() const override { return ShaderLanguage::SPIRV; }
    const std::string& GetEntryPoint() const override { static std::string s = "main"; return s; }
    const std::string& GetTarget() const override { static std::string s = "spv"; return s; }
    const std::string& GetSource() const override { static std::string s; return s; }
    const std::vector<uint8_t>& GetBytecode() const override { static std::vector<uint8_t> b; return b; }
    const std::string& GetFilename() const override { static std::string s = "test.shader"; return s; }
    uint64_t GetCompileTimestamp() const override { return 0; }
    uint64_t GetCompileHash() const override { return 0; }
    const ShaderCompileOptions& GetCompileOptions() const override { static ShaderCompileOptions o; return o; }
    const ShaderReflection& GetReflection() const override { static ShaderReflection r; return r; }
    bool HasReflection() const override { return false; }
    const ShaderResource* FindResource(const std::string&) const override { return nullptr; }
    const ShaderResource* FindResourceByBindPoint(uint32_t, uint32_t) const override { return nullptr; }
    bool Recompile(const ShaderCompileOptions*, std::string&) override { return false; }
    bool RecompileFromSource(const std::string&, const ShaderCompileOptions*, std::string&) override { return false; }
    bool ReloadFromFile(std::string&) override { return false; }
    void EnableHotReload(bool) override {}
    bool IsFileModified() const override { return false; }
    bool NeedsReload() const override { return false; }
    uint64_t GetFileModificationTime() const override { return 0; }
    const std::string& GetCompileLog() const override { static std::string s; return s; }
    bool HasWarnings() const override { return false; }
    bool HasErrors() const override { return false; }
    bool Validate() override { return true; }
    std::string Disassemble() const override { return {}; }
    bool DebugSaveToFile(const std::string&, bool, bool) const override { return false; }
    const std::vector<std::string>& GetDependencies() const override { static std::vector<std::string> d; return d; }
    const std::vector<std::string>& GetIncludes() const override { static std::vector<std::string> i; return i; }
    const std::vector<std::string>& GetDefines() const override { static std::vector<std::string> d; return d; }
};

// 空 shader 构造测试
TEST(MaterialTest, ConstructWithNullShader) {
    auto material = std::make_shared<Material>(nullptr);
    EXPECT_NE(material, nullptr);
    EXPECT_FALSE(material->IsLoaded());
    EXPECT_EQ(material->GetShader(), nullptr);
    EXPECT_EQ(material->GetType(), AssetType::Material);
}

TEST(MaterialTest, ConstructWithShader) {
    auto shader = std::make_shared<TestShader>();
    auto material = std::make_shared<Material>(shader);
    EXPECT_TRUE(material->IsLoaded());
    EXPECT_EQ(material->GetShader(), shader);
}

TEST(MaterialTest, SetAndGetParam) {
    auto material = std::make_shared<Material>(nullptr);

    material->SetParam("FloatParam", 3.14f);
    material->SetParam("Vec3Param", PrismaMath::vec3(1.0f, 2.0f, 3.0f));
    material->SetParam("Vec4Param", PrismaMath::vec4(0.1f, 0.2f, 0.3f, 1.0f));

    auto* floatVal = material->GetParam("FloatParam");
    ASSERT_NE(floatVal, nullptr);
    ASSERT_TRUE(std::holds_alternative<float>(*floatVal));
    EXPECT_FLOAT_EQ(std::get<float>(*floatVal), 3.14f);

    auto* vec3Val = material->GetParam("Vec3Param");
    ASSERT_NE(vec3Val, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrismaMath::vec3>(*vec3Val));
    auto v3 = std::get<PrismaMath::vec3>(*vec3Val);
    EXPECT_FLOAT_EQ(v3.x, 1.0f);
    EXPECT_FLOAT_EQ(v3.y, 2.0f);

    auto* vec4Val = material->GetParam("Vec4Param");
    ASSERT_NE(vec4Val, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrismaMath::vec4>(*vec4Val));
    auto v4 = std::get<PrismaMath::vec4>(*vec4Val);
    EXPECT_FLOAT_EQ(v4.w, 1.0f);
}

TEST(MaterialTest, GetParamMissing) {
    auto material = std::make_shared<Material>(nullptr);
    auto* val = material->GetParam("NonExistentParam");
    EXPECT_EQ(val, nullptr);
}

TEST(MaterialTest, OverwriteParam) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetParam("Value", 1.0f);
    material->SetParam("Value", 99.9f);

    auto* val = material->GetParam("Value");
    ASSERT_NE(val, nullptr);
    EXPECT_FLOAT_EQ(std::get<float>(*val), 99.9f);
}

TEST(MaterialTest, SetBaseColor) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetBaseColor(0.2f, 0.4f, 0.6f, 1.0f);

    auto* val = material->GetParam("BaseColor");
    ASSERT_NE(val, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrismaMath::vec4>(*val));
    auto v = std::get<PrismaMath::vec4>(*val);
    EXPECT_FLOAT_EQ(v.x, 0.2f);
    EXPECT_FLOAT_EQ(v.y, 0.4f);
    EXPECT_FLOAT_EQ(v.z, 0.6f);
    EXPECT_FLOAT_EQ(v.w, 1.0f);
}

TEST(MaterialTest, SetMetallic) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetMetallic(0.75f);

    auto* val = material->GetParam("Metallic");
    ASSERT_NE(val, nullptr);
    EXPECT_FLOAT_EQ(std::get<float>(*val), 0.75f);
}

TEST(MaterialTest, SetRoughness) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetRoughness(0.3f);

    auto* val = material->GetParam("Roughness");
    ASSERT_NE(val, nullptr);
    EXPECT_FLOAT_EQ(std::get<float>(*val), 0.3f);
}

TEST(MaterialTest, SetAO) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetAO(0.8f);

    auto* val = material->GetParam("AO");
    ASSERT_NE(val, nullptr);
    EXPECT_FLOAT_EQ(std::get<float>(*val), 0.8f);
}

TEST(MaterialTest, SetEmissiveIntensity) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetEmissiveIntensity(2.5f);

    auto* val = material->GetParam("EmissiveIntensity");
    ASSERT_NE(val, nullptr);
    EXPECT_FLOAT_EQ(std::get<float>(*val), 2.5f);
}

TEST(MaterialTest, SetEmissiveColor) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetEmissiveColor(PrismaMath::vec3(0.1f, 0.5f, 0.9f));

    auto* val = material->GetParam("EmissiveColor");
    ASSERT_NE(val, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrismaMath::vec3>(*val));
    auto v = std::get<PrismaMath::vec3>(*val);
    EXPECT_FLOAT_EQ(v.x, 0.1f);
    EXPECT_FLOAT_EQ(v.y, 0.5f);
}

TEST(MaterialTest, UnloadClearsParams) {
    auto material = std::make_shared<Material>(nullptr);
    material->SetParam("Test", 42.0f);
    material->Unload();

    EXPECT_FALSE(material->IsLoaded());
    EXPECT_EQ(material->GetParam("Test"), nullptr);
    EXPECT_EQ(material->GetShader(), nullptr);
}

TEST(MaterialTest, SetBaseColorWithVector4) {
    auto material = std::make_shared<Material>(nullptr);
    PrismaMath::vec4 color(0.1f, 0.2f, 0.3f, 0.4f);
    material->SetBaseColor(color);

    auto* val = material->GetParam("BaseColor");
    ASSERT_NE(val, nullptr);
    ASSERT_TRUE(std::holds_alternative<PrismaMath::vec4>(*val));
    auto v = std::get<PrismaMath::vec4>(*val);
    EXPECT_FLOAT_EQ(v.x, 0.1f);
    EXPECT_FLOAT_EQ(v.y, 0.2f);
    EXPECT_FLOAT_EQ(v.z, 0.3f);
    EXPECT_FLOAT_EQ(v.w, 0.4f);
}

} // namespace
} // namespace Prisma::Graphic
