# YAGE 资产序列化系统

## 概述

YAGE资产序列化系统提供了一套完整的接口，用于将资产对象转换为可存储的二进制或JSON格式，并能从序列化数据中还原为原始对象。该系统支持版本控制、错误处理，并提供了简洁高效的API。

## 核心组件

### 1. 序列化接口

#### `Serializable` 类
- 所有可序列化对象的基类
- 定义了 `Serialize` 和 `Deserialize` 纯虚函数

#### `OutputArchive` 和 `InputArchive` 类
- 提供序列化和反序列化的抽象接口
- 支持基本数据类型、字符串、数组和对象的序列化

### 2. 具体实现

#### 二进制格式
- `BinaryOutputArchive` 和 `BinaryInputArchive`
- 高效的二进制序列化，适合性能敏感场景

#### JSON格式
- `JsonOutputArchive` 和 `JsonInputArchive`
- 人类可读的格式，便于调试和编辑

### 3. 资产基类

#### `Asset` 类
- 继承自 `IResource` 和 `Serializable`
- 提供资产元数据支持
- 实现了通用的序列化方法

#### `TextureAsset` 和 `MeshAsset` 类
- 具体的资产类型实现
- 展示了如何为特定资产类型实现序列化

## 使用示例

### 基本序列化和反序列化

```cpp
#include "AssetSerializer.h"
#include "TextureAsset.h"

// 创建资产
Resources::TextureAsset texture;
texture.SetMetadata("MyTexture", "A sample texture");
texture.SetDimensions(256, 256, 4);
// ... 设置纹理数据 ...

// 序列化到JSON文件
texture.SerializeToFile("my_texture.json", SerializationFormat::JSON);

// 从JSON文件反序列化
Resources::TextureAsset loadedTexture;
loadedTexture.DeserializeFromFile("my_texture.json", SerializationFormat::JSON);
```

### 使用AssetSerializer

```cpp
// 序列化到文件
AssetSerializer::SerializeToFile(texture, "texture.bin", SerializationFormat::Binary);

// 从文件反序列化
auto loadedTexture = AssetSerializer::DeserializeFromFile<Resources::TextureAsset>(
    "texture.bin", SerializationFormat::Binary);

// 序列化到内存
auto data = AssetSerializer::SerializeToMemory(texture, SerializationFormat::JSON);

// 从内存反序列化
auto loadedTexture = AssetSerializer::DeserializeFromMemory<Resources::TextureAsset>(
    data, SerializationFormat::JSON);
```

### 自定义资产类型

```cpp
class CustomAsset : public Asset {
public:
    // 实现IResource接口
    bool Load(const std::filesystem::path& path) override { /* ... */ }
    void Unload() override { /* ... */ }
    bool IsLoaded() const override { /* ... */ }
    ResourceType GetType() const override { /* ... */ }
    
    // 实现Serializable接口
    void Serialize(OutputArchive& archive) const override {
        archive.BeginObject();
        archive("metadata", m_metadata);
        archive("customData", m_customData);
        // ... 序列化其他字段 ...
        archive.EndObject();
    }
    
    void Deserialize(InputArchive& archive) override {
        size_t fieldCount = archive.BeginObject();
        
        for (size_t i = 0; i < fieldCount; ++i) {
            if (archive.HasNextField("metadata")) {
                m_metadata.Deserialize(archive);
            } else if (archive.HasNextField("customData")) {
                m_customData = archive.ReadString();
            }
            // ... 处理其他字段 ...
        }
        
        archive.EndObject();
    }
    
    // 实现Asset特定方法
    std::string GetAssetType() const override { return "Custom"; }
    
private:
    std::string m_customData;
    // ... 其他字段 ...
};
```

## 版本控制

序列化系统支持版本控制，确保不同版本的序列化格式可以兼容：

```cpp
// 使用特定版本序列化
SerializationVersion version = {2, 1, 0};
AssetSerializer::SerializeToFile(asset, "asset_v2.1.json", 
                                SerializationFormat::JSON, version);
```

版本信息会作为元数据存储在序列化文件的开头，便于后续版本兼容性处理。

## 错误处理

系统提供了完善的错误处理机制：

```cpp
try {
    auto asset = AssetSerializer::DeserializeFromFile<CustomAsset>("invalid_file.json");
    if (!asset) {
        // 处理反序列化失败
    }
} catch (const SerializationException& e) {
    // 处理序列化异常
    std::cerr << "Serialization error: " << e.what() << std::endl;
}
```

## 性能考虑

- 二进制格式比JSON格式更紧凑，加载速度更快
- 内存序列化避免了文件I/O，适合网络传输或缓存
- 大型资产（如高分辨率纹理）应考虑分块序列化

## 测试

项目包含完整的单元测试，验证序列化系统的正确性：

```bash
# 构建并运行测试
cd Tests
mkdir build && cd build
cmake ..
make
./AssetSerializerTest
```

测试覆盖了以下场景：
- JSON和二进制格式的序列化/反序列化
- 内存序列化
- 版本控制
- 错误处理
- 具体资产类型的序列化

## 扩展指南

要添加对新资产类型的支持：

1. 创建继承自 `Asset` 的新类
2. 实现 `IResource` 和 `Serializable` 接口
3. 在 `Serialize` 和 `Deserialize` 方法中处理特定数据
4. 为新类型添加单元测试

## 注意事项

- 确保序列化和反序列化的字段顺序一致
- 处理可选字段时，使用 `HasNextField` 检查字段是否存在
- 对于大型数据，考虑使用流式处理而非一次性加载
- 版本升级时，确保向后兼容性