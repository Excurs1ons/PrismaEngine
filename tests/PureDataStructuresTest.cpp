#include "TestFramework.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <algorithm>

// 纯数据结构测试 - 无任何外部依赖
class PureDataStructuresTestSuite : public TestFramework::TestSuite {
public:
    PureDataStructuresTestSuite() : TestSuite("Pure Data Structures Tests") {
        AddTest(new VectorTest());
        AddTest(new UnorderedMapTest());
        AddTest(new MemoryManagementTest());
        AddTest(new AlgorithmsTest());
    }

private:
    // 自定义Vector类
    template<typename T>
    class Vector {
    public:
        Vector() = default;
        explicit Vector(size_t size) : m_data(size) {}

        void PushBack(const T& value) {
            m_data.push_back(value);
        }

        void PopBack() {
            if (!m_data.empty()) {
                m_data.pop_back();
            }
        }

        T& operator[](size_t index) {
            return m_data[index];
        }

        const T& operator[](size_t index) const {
            return m_data[index];
        }

        size_t Size() const {
            return m_data.size();
        }

        bool Empty() const {
            return m_data.empty();
        }

        void Clear() {
            m_data.clear();
        }

        void Reserve(size_t capacity) {
            m_data.reserve(capacity);
        }

        auto begin() { return m_data.begin(); }
        auto end() { return m_data.end(); }
        auto begin() const { return m_data.cbegin(); }
        auto end() const { return m_data.cend(); }

    private:
        std::vector<T> m_data;
    };

    // Vector测试
    class VectorTest : public TestFramework::TestCase {
    public:
        VectorTest() : TestFramework::TestCase("Vector Operations") {}
    protected:
        void RunTest() override {
            Vector<int> vec;

            // 测试空Vector
            AssertTrue(vec.Empty());
            AssertEquals(static_cast<size_t>(0), vec.Size());

            // 测试添加元素
            vec.PushBack(1);
            vec.PushBack(2);
            vec.PushBack(3);
            AssertFalse(vec.Empty());
            AssertEquals(static_cast<size_t>(3), vec.Size());
            AssertEquals(1, vec[0]);
            AssertEquals(2, vec[1]);
            AssertEquals(3, vec[2]);

            // 测试移除元素
            vec.PopBack();
            AssertEquals(static_cast<size_t>(2), vec.Size());
            AssertEquals(1, vec[0]);
            AssertEquals(2, vec[1]);

            // 测试清空
            vec.Clear();
            AssertTrue(vec.Empty());
            AssertEquals(static_cast<size_t>(0), vec.Size());

            // 测试保留容量
            Vector<int> vec2;
            vec2.Reserve(100);
            AssertTrue(vec2.Empty());
            AssertTrue(vec2.Size() < 100); // Size仍然是0

            // 测试初始大小构造
            Vector<int> vec3(10);
            AssertEquals(static_cast<size_t>(10), vec3.Size());
        }
    };

    // UnorderedMap测试
    class UnorderedMapTest : public TestFramework::TestCase {
    public:
        UnorderedMapTest() : TestFramework::TestCase("UnorderedMap Operations") {}
    protected:
        void RunTest() override {
            std::unordered_map<std::string, int> map;

            // 测试空map
            AssertEquals(static_cast<size_t>(0), map.size());

            // 测试插入和查找
            map["one"] = 1;
            map["two"] = 2;
            map["three"] = 3;
            AssertEquals(static_cast<size_t>(3), map.size());

            AssertEquals(1, map["one"]);
            AssertEquals(2, map["two"]);
            AssertEquals(3, map["three"]);

            // 测试查找不存在的键
            AssertEquals(0, map["four"]);

            // 测试包含
            AssertTrue(map.count("one"));
            AssertFalse(map.count("four"));

            // 测试删除
            map.erase("two");
            AssertEquals(static_cast<size_t>(2), map.size());
            AssertFalse(map.count("two"));

            // 测试迭代
            int sum = 0;
            for (const auto& pair : map) {
                sum += pair.second;
            }
            AssertEquals(4, sum); // 1 + 3
        }
    };

    // 内存管理测试
    class MemoryManagementTest : public TestFramework::TestCase {
    public:
        MemoryManagementTest() : TestFramework::TestCase("Memory Management") {}
    protected:
        void RunTest() override {
            // 测试智能指针
            auto ptr1 = std::make_unique<int>(42);
            AssertNotNull(ptr1.get());
            AssertEquals(42, *ptr1);

            // 测试所有权转移
            std::unique_ptr<int> ptr2 = std::move(ptr1);
            AssertNull(ptr1.get());
            AssertNotNull(ptr2.get());
            AssertEquals(42, *ptr2);

            // 测试shared_ptr
            auto shared1 = std::make_shared<int>(100);
            auto shared2 = shared1;
            AssertEquals(1, shared1.use_count());
            AssertEquals(2, shared2.use_count());

            {
                std::shared_ptr<int> shared3 = shared1;
                AssertEquals(3, shared1.use_count());
                AssertEquals(3, shared2.use_count());
                AssertEquals(3, shared3.use_count());
            }
            AssertEquals(2, shared1.use_count());
            AssertEquals(2, shared2.use_count());

            // 测试数组智能指针
            std::unique_ptr<int[]> arrPtr = std::make_unique<int[]>(5);
            for (int i = 0; i < 5; ++i) {
                arrPtr[i] = i * i;
            }
            AssertEquals(0, arrPtr[0]);
            AssertEquals(1, arrPtr[1]);
            AssertEquals(4, arrPtr[2]);
            AssertEquals(9, arrPtr[3]);
            AssertEquals(16, arrPtr[4]);
        }
    };

    // 算法测试
    class AlgorithmsTest : public TestFramework::TestCase {
    public:
        AlgorithmsTest() : TestFramework::TestCase("Algorithms") {}
    protected:
        void RunTest() override {
            // 测试排序
            Vector<int> vec;
            vec.PushBack(3);
            vec.PushBack(1);
            vec.PushBack(4);
            vec.PushBack(2);
            vec.PushBack(5);

            std::sort(vec.begin(), vec.end());

            AssertEquals(1, vec[0]);
            AssertEquals(2, vec[1]);
            AssertEquals(3, vec[2]);
            AssertEquals(4, vec[3]);
            AssertEquals(5, vec[4]);

            // 测试二分查找
            auto it = std::lower_bound(vec.begin(), vec.end(), 3);
            AssertTrue(it != vec.end());
            AssertEquals(3, *it);

            it = std::lower_bound(vec.begin(), vec.end(), 6);
            AssertTrue(it == vec.end());

            // 测试max_element
            int maxVal = *std::max_element(vec.begin(), vec.end());
            AssertEquals(5, maxVal);

            // 测试min_element
            int minVal = *std::min_element(vec.begin(), vec.end());
            AssertEquals(1, minVal);

            // 测试find
            auto findIt = std::find(vec.begin(), vec.end(), 4);
            AssertTrue(findIt != vec.end());
            AssertEquals(4, *findIt);

            // 测试计数
            int count = std::count(vec.begin(), vec.end(), 2);
            AssertEquals(1, count);

            // 测试去重
            Vector<int> vecWithDup;
            vecWithDup.PushBack(1);
            vecWithDup.PushBack(2);
            vecWithDup.PushBack(2);
            vecWithDup.PushBack(3);
            vecWithDup.PushBack(1);
            auto newEnd = std::unique(vecWithDup.begin(), vecWithDup.end());
            vecWithDup.erase(newEnd, vecWithDup.end());

            AssertEquals(static_cast<size_t>(3), vecWithDup.Size());
            AssertEquals(1, vecWithDup[0]);
            AssertEquals(2, vecWithDup[1]);
            AssertEquals(3, vecWithDup[2]);
        }
    };
};

// 注册测试套件
REGISTER_TEST_SUITE(PureDataStructuresTestSuite)