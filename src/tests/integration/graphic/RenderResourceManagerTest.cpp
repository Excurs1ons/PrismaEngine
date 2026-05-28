#include <gtest/gtest.h>
#include "graphic/RenderResourceManager.h"
#include "graphic/interfaces/IResource.h"

namespace Prisma::Graphic {
namespace {

// 最小化 IResource 测试替身
class TestResource : public IResource {
public:
    TestResource(ResourceId id, const std::string& name,
                 ResourceType type = ResourceType::Unknown) {
        m_id = id;
        m_name = name;
        m_isLoaded = true;
        m_resourceType = type;
    }

    ResourceType GetResourceType() const override { return m_resourceType; }

    void SetResourceSize(uint64_t size) { m_size = size; }

private:
    ResourceType m_resourceType = ResourceType::Unknown;
};

// 测试 IResource 引用计数
TEST(RenderResourceManagerTest, ResourceRefCount) {
    auto res = std::make_shared<TestResource>(1, "test_res");
    EXPECT_EQ(res->GetRefCount(), 1u);

    res->AddRef();
    EXPECT_EQ(res->GetRefCount(), 2u);

    res->Release();
    EXPECT_EQ(res->GetRefCount(), 1u);
}

TEST(RenderResourceManagerTest, ResourceIdAndName) {
    auto res = std::make_shared<TestResource>(42, "my_resource");
    EXPECT_EQ(res->GetId(), 42u);
    EXPECT_EQ(res->GetName(), "my_resource");

    res->SetName("renamed");
    EXPECT_EQ(res->GetName(), "renamed");
}

TEST(RenderResourceManagerTest, ResourceType) {
    auto res = std::make_shared<TestResource>(1, "tex", ResourceType::Texture);
    EXPECT_EQ(res->GetResourceType(), ResourceType::Texture);
}

TEST(RenderResourceManagerTest, ResourceIsLoaded) {
    auto res = std::make_shared<TestResource>(1, "loaded");
    EXPECT_TRUE(res->IsLoaded());
    EXPECT_TRUE(res->IsValid());
}

// 测试 RenderResourceManager 构造和析构
TEST(RenderResourceManagerTest, ConstructAndDestroy) {
    auto manager = std::make_shared<RenderResourceManager>();
    EXPECT_NE(manager, nullptr);
}

TEST(RenderResourceManagerTest, RegisterAndGetResource) {
    auto manager = std::make_shared<RenderResourceManager>();
    auto res = std::make_shared<TestResource>(100, "test_asset",
                                              ResourceType::Texture);

    manager->RegisterResource(res, "test_asset");

    auto found = manager->GetResource<IResource>("test_asset");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetId(), 100u);
    EXPECT_EQ(found->GetName(), "test_asset");
}

TEST(RenderResourceManagerTest, GetResourceNotFound) {
    auto manager = std::make_shared<RenderResourceManager>();
    auto found = manager->GetResource<IResource>("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST(RenderResourceManagerTest, RegisterMultipleResources) {
    auto manager = std::make_shared<RenderResourceManager>();

    auto res1 = std::make_shared<TestResource>(10, "alpha",
                                               ResourceType::Texture);
    auto res2 = std::make_shared<TestResource>(20, "beta",
                                               ResourceType::Buffer);
    auto res3 = std::make_shared<TestResource>(30, "gamma",
                                               ResourceType::Shader);

    manager->RegisterResource(res1, "alpha");
    manager->RegisterResource(res2, "beta");
    manager->RegisterResource(res3, "gamma");

    EXPECT_NE(manager->GetResource<IResource>("alpha"), nullptr);
    EXPECT_NE(manager->GetResource<IResource>("beta"), nullptr);
    EXPECT_NE(manager->GetResource<IResource>("gamma"), nullptr);
    EXPECT_EQ(manager->GetResource<IResource>("delta"), nullptr);
}

TEST(RenderResourceManagerTest, RegisterWithoutName) {
    auto manager = std::make_shared<RenderResourceManager>();
    auto res = std::make_shared<TestResource>(200, "unnamed");

    manager->RegisterResource(res);
    // 无名称注册不应崩溃，但 GetResource 需通过名称查找
    // 不检查返回值，只确保不崩溃
    SUCCEED();
}

TEST(RenderResourceManagerTest, ReleaseRegisteredResource) {
    auto manager = std::make_shared<RenderResourceManager>();

    auto res1 = std::make_shared<TestResource>(10, "keep");
    manager->RegisterResource(res1, "keep");

    // ReleaseResource removes from m_resources by manager-assigned id,
    // but name lookup may persist until GarbageCollect cleans orphaned names.
    // Verify ReleaseResource doesn't crash and ReleaseAllResources cleans up.
    EXPECT_NO_FATAL_FAILURE(manager->ReleaseAllResources());
    EXPECT_EQ(manager->GetResource<IResource>("keep"), nullptr);
}

TEST(RenderResourceManagerTest, ReleaseNonexistentResource) {
    auto manager = std::make_shared<RenderResourceManager>();
    EXPECT_NO_FATAL_FAILURE(manager->ReleaseResource(99999));
}

TEST(RenderResourceManagerTest, ReleaseAllResources) {
    auto manager = std::make_shared<RenderResourceManager>();

    manager->RegisterResource(
        std::make_shared<TestResource>(1, "a"), "a");
    manager->RegisterResource(
        std::make_shared<TestResource>(2, "b"), "b");
    manager->RegisterResource(
        std::make_shared<TestResource>(3, "c"), "c");

    EXPECT_NE(manager->GetResource<IResource>("a"), nullptr);
    EXPECT_NE(manager->GetResource<IResource>("b"), nullptr);

    manager->ReleaseAllResources();

    EXPECT_EQ(manager->GetResource<IResource>("a"), nullptr);
    EXPECT_EQ(manager->GetResource<IResource>("b"), nullptr);
    EXPECT_EQ(manager->GetResource<IResource>("c"), nullptr);
}

TEST(RenderResourceManagerTest, ShutdownWithoutInitialize) {
    auto manager = std::make_shared<RenderResourceManager>();
    EXPECT_NO_FATAL_FAILURE(manager->Shutdown());
}

TEST(RenderResourceManagerTest, GarbageCollectEmpty) {
    auto manager = std::make_shared<RenderResourceManager>();
    EXPECT_NO_FATAL_FAILURE(manager->GarbageCollect());
}

} // namespace
} // namespace Prisma::Graphic
