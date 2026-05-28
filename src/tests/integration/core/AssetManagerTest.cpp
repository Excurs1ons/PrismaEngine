#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include "core/AssetManager.h"

namespace Prisma {
namespace {

class TestAsset : public Asset {
public:
    AssetType GetType() const override { return AssetType::None; }
    std::string GetAssetType() const override { return "TestAsset"; }

    bool Load(const std::filesystem::path& path) override {
        m_IsLoaded = std::filesystem::exists(path);
        return m_IsLoaded;
    }

    void Unload() override {
        m_IsLoaded = false;
    }
};

class TempFile {
public:
    std::filesystem::path m_path;

    TempFile(const std::string& name, const std::string& content = "test") {
        m_path = std::filesystem::temp_directory_path() / name;
        std::ofstream ofs(m_path);
        ofs << content;
        ofs.close();
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(m_path, ec);
    }
};

class AssetManagerTest : public ::testing::Test {
protected:
    AssetManager* m_am = nullptr;

    void SetUp() override {
        m_am = new AssetManager();
    }

    void TearDown() override {
        delete m_am;
        m_am = nullptr;
    }
};

TEST_F(AssetManagerTest, InitiallyNotInitialized) {
    EXPECT_FALSE(m_am->IsInitialized());
}

TEST_F(AssetManagerTest, InitializeSucceeds) {
    bool ok = m_am->Initialize(std::filesystem::current_path());
    EXPECT_TRUE(ok);
    EXPECT_TRUE(m_am->IsInitialized());
}

TEST_F(AssetManagerTest, DefaultInitializeUsesCurrentDir) {
    int result = m_am->Initialize();
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(m_am->IsInitialized());
}

TEST_F(AssetManagerTest, ShutdownClearsState) {
    m_am->Initialize();
    ASSERT_TRUE(m_am->IsInitialized());
    m_am->Shutdown();
    EXPECT_FALSE(m_am->IsInitialized());
}

TEST_F(AssetManagerTest, ShutdownIsIdempotent) {
    m_am->Shutdown();
    m_am->Shutdown();
    SUCCEED();
}

TEST_F(AssetManagerTest, MultipleInitializationsAreSafe) {
    EXPECT_TRUE(m_am->Initialize(std::filesystem::current_path()));
    EXPECT_TRUE(m_am->Initialize(std::filesystem::current_path()));
    EXPECT_TRUE(m_am->IsInitialized());
}

TEST_F(AssetManagerTest, AddAndUseSearchPath) {
    m_am->Initialize();
    TempFile tmp("prisma_test_asset.txt");

    auto notFound = m_am->FindResource("prisma_test_asset.txt");
    EXPECT_FALSE(notFound.has_value());

    m_am->AddSearchPath(tmp.m_path.parent_path());
    auto found = m_am->FindResource("prisma_test_asset.txt");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->filename(), "prisma_test_asset.txt");
}

TEST_F(AssetManagerTest, FindResourceReturnsExistingFile) {
    m_am->Initialize();
    TempFile tmp("prisma_test_resource.bin");

    m_am->AddSearchPath(tmp.m_path.parent_path());
    auto result = m_am->FindResource("prisma_test_resource.bin");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(*result));
}

TEST_F(AssetManagerTest, FindResourceReturnsNulloptForMissingFile) {
    m_am->Initialize();
    auto result = m_am->FindResource("non_existent_file_xyz.bin");
    EXPECT_FALSE(result.has_value());
}

TEST_F(AssetManagerTest, LoadAssetCachesAndReturnsHandle) {
    m_am->Initialize();
    TempFile tmp("prisma_test_asset_load.txt");

    m_am->AddSearchPath(tmp.m_path.parent_path());

    auto handle = m_am->Load<TestAsset>("prisma_test_asset_load.txt");
    ASSERT_TRUE(handle.IsValid());
    EXPECT_TRUE(handle->IsLoaded());
    EXPECT_EQ(handle->GetName(), "prisma_test_asset_load.txt");
}

TEST_F(AssetManagerTest, LoadReturnsCachedAssetOnSecondCall) {
    m_am->Initialize();
    TempFile tmp("prisma_test_cache_test.txt");

    m_am->AddSearchPath(tmp.m_path.parent_path());

    auto first = m_am->Load<TestAsset>("prisma_test_cache_test.txt");
    ASSERT_TRUE(first.IsValid());

    auto second = m_am->Load<TestAsset>("prisma_test_cache_test.txt");
    ASSERT_TRUE(second.IsValid());

    EXPECT_EQ(first.Get(), second.Get());
}

TEST_F(AssetManagerTest, LoadReturnsInvalidHandleForMissingFile) {
    m_am->Initialize();
    auto handle = m_am->Load<TestAsset>("definitely_missing_file.dat");
    EXPECT_FALSE(handle.IsValid());
}

TEST_F(AssetManagerTest, LoadBeforeInitStillWorks) {
    TempFile tmp("prisma_test_preinit.txt");
    m_am->AddSearchPath(tmp.m_path.parent_path());

    auto handle = m_am->Load<TestAsset>("prisma_test_preinit.txt");
    EXPECT_TRUE(handle.IsValid());
    EXPECT_TRUE(handle->IsLoaded());
}

TEST_F(AssetManagerTest, UnloadRemovesFromCache) {
    m_am->Initialize();
    TempFile tmp("prisma_test_unload.txt");

    m_am->AddSearchPath(tmp.m_path.parent_path());
    auto handle = m_am->Load<TestAsset>("prisma_test_unload.txt");
    ASSERT_TRUE(handle.IsValid());

    m_am->Unload("prisma_test_unload.txt");
    auto handle2 = m_am->Load<TestAsset>("prisma_test_unload.txt");
    ASSERT_TRUE(handle2.IsValid());
    EXPECT_NE(handle.Get(), handle2.Get());
}

TEST_F(AssetManagerTest, UnloadAllEmptiesCache) {
    m_am->Initialize();
    TempFile tmp1("prisma_test_ua1.txt");
    TempFile tmp2("prisma_test_ua2.txt");

    m_am->AddSearchPath(tmp1.m_path.parent_path());
    auto h1 = m_am->Load<TestAsset>("prisma_test_ua1.txt");
    auto h2 = m_am->Load<TestAsset>("prisma_test_ua2.txt");
    ASSERT_TRUE(h1.IsValid());
    ASSERT_TRUE(h2.IsValid());

    m_am->UnloadAll();

    auto h1r = m_am->Load<TestAsset>("prisma_test_ua1.txt");
    auto h2r = m_am->Load<TestAsset>("prisma_test_ua2.txt");
    EXPECT_NE(h1.Get(), h1r.Get());
    EXPECT_NE(h2.Get(), h2r.Get());
}

TEST_F(AssetManagerTest, MultipleSearchPathsAreSearchedInOrder) {
    m_am->Initialize();

    TempFile tmp1("prisma_test_order.txt");
    auto dir1 = tmp1.m_path.parent_path();

    m_am->AddSearchPath(dir1);
    auto result = m_am->FindResource("prisma_test_order.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->parent_path(), dir1);
}

} // namespace
} // namespace Prisma
