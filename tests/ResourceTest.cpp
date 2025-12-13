#include "TestFramework.h"
#include "../src/engine/core/ResourceManager.h"

using namespace Engine::Core;

// 测试资源类型
class TestResource : public IResource {
public:
    TestResource(const std::string& path, int value)
        : m_path(path), m_value(value) {}

    const std::string& GetPath() const override { return m_path; }
    ResourceType GetType() const override { return ResourceType::Texture; }
    uint64_t GetSize() const override { return sizeof(TestResource); }
    ResourceState GetState() const override { return m_state; }
    bool Load() override {
        m_state = ResourceState::Loaded;
        return true;
    }
    void Unload() override {
        m_state = ResourceState::Unloaded;
    }
    bool Reload() override {
        m_state = ResourceState::Loaded;
        return true;
    }
    bool IsValid() const override { return m_state == ResourceState::Loaded; }
    uint32_t GetRefCount() const override { return 1; }
    double GetLastUsedTime() const override { return 0.0; }

    int GetValue() const { return m_value; }

private:
    void SetState(ResourceState state) override { m_state = state; }

    std::string m_path;
    int m_value;
    ResourceState m_state = ResourceState::Unloaded;
};

// 测试资源加载器
class TestResourceLoader : public IResourceLoader {
public:
    std::vector<std::string> GetSupportedExtensions() const override {
        return {".test"};
    }

    std::shared_ptr<IResource> CreateResource(const std::string& path) override {
        // 根据路径生成值
        int value = path.length(); // 简单的测试值
        return std::make_shared<TestResource>(path, value);
    }

    std::future<bool> LoadResourceAsync(std::shared_ptr<IResource> resource) override {
        return std::async(std::launch::deferred, [resource]() {
            return resource->Load();
        });
    }
};

// 资源管理器测试套件
class ResourceTestSuite : public TestFramework::TestSuite {
public:
    ResourceTestSuite() : TestSuite("Resource Manager Tests") {
        AddTest(new LoadResourceTest());
        AddTest(new CacheTest());
        AddTest(new AsyncLoadTest());
        AddTest(new MemoryLimitTest());
    }

private:
    // 加载资源测试
    class LoadResourceTest : public TestFramework::TestCase {
    public:
        LoadResourceTest() : TestFramework::TestCase("LoadResource") {}
    protected:
        void RunTest() override {
            auto& manager = ResourceManager::GetInstance();
            manager.RegisterLoader(ResourceType::Texture, std::make_unique<TestResourceLoader>());

            auto resource = manager.LoadResource("test.test");
            AssertNotNull(resource.get(), "Resource should not be null");
            AssertTrue(resource->IsValid(), "Resource should be loaded");
            AssertEquals("test.test", resource->GetPath());
        }
    };

    // 缓存测试
    class CacheTest : public TestFramework::TestCase {
    public:
        CacheTest() : TestFramework::TestCase("Cache") {}
    protected:
        void RunTest() override {
            auto& manager = ResourceManager::GetInstance();

            // 首次加载
            auto resource1 = manager.LoadResource("cache.test");
            auto resource2 = manager.LoadResource("cache.test");

            // 应该返回相同的实例
            Assert(resource1.get() == resource2.get(), "Should return cached resource");
        }
    };

    // 异步加载测试
    class AsyncLoadTest : public TestFramework::TestCase {
    public:
        AsyncLoadTest() : TestFramework::TestCase("AsyncLoad") {}
    protected:
        void RunTest() override {
            auto& manager = ResourceManager::GetInstance();

            auto future = manager.LoadResourceAsync("async.test");
            auto resource = future.get();

            AssertNotNull(resource.get(), "Async loaded resource should not be null");
            AssertTrue(resource->IsValid(), "Async loaded resource should be valid");
        }
    };

    // 内存限制测试
    class MemoryLimitTest : public TestFramework::TestCase {
    public:
        MemoryLimitTest() : TestFramework::TestCase("MemoryLimit") {}
    protected:
        void RunTest() override {
            auto& manager = ResourceManager::GetInstance();
            manager.SetMemoryLimit(1024); // 1KB

            // 加载资源直到达到限制
            manager.LoadResource("limit1.test");
            manager.LoadResource("limit2.test");
            manager.LoadResource("limit3.test");

            auto stats = manager.GetStats();
            AssertTrue(stats.totalMemoryUsage <= 1024, "Memory usage should not exceed limit");
        }
    };
};

// 注册测试套件
REGISTER_TEST_SUITE(ResourceTestSuite)