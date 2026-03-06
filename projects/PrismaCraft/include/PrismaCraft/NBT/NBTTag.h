#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace PrismaCraft {
namespace NBT {

/**
 * NBT tag type enum
 * Corresponds to: net.minecraft.nbt.Tag
 */
enum class TagType : int8_t {
    END = 0,
    BYTE = 1,
    SHORT = 2,
    INT = 3,
    LONG = 4,
    FLOAT = 5,
    DOUBLE = 6,
    BYTE_ARRAY = 7,
    STRING = 8,
    LIST = 9,
    COMPOUND = 10,
    INT_ARRAY = 11,
    LONG_ARRAY = 12
};

/**
 * Base tag class
 * Corresponds to: net.minecraft.nbt.Tag
 */
class PRISMACRAFT_API Tag {
public:
    virtual ~Tag() = default;

    virtual TagType getId() const = 0;
    virtual std::string toString() const = 0;
    virtual std::unique_ptr<Tag> copy() const = 0;

    // Type checking
    bool is(TagType type) const { return getId() == type; }

    // Value access (throws if wrong type)
    virtual int8_t getAsByte() const { throw std::runtime_error("Not a byte tag"); }
    virtual int16_t getAsShort() const { throw std::runtime_error("Not a short tag"); }
    virtual int32_t getAsInt() const { throw std::runtime_error("Not an int tag"); }
    virtual int64_t getAsLong() const { throw std::runtime_error("Not a long tag"); }
    virtual float getAsFloat() const { throw std::runtime_error("Not a float tag"); }
    virtual double getAsDouble() const { throw std::runtime_error("Not a double tag"); }
    virtual std::string getAsString() const { throw std::runtime_error("Not a string tag"); }

protected:
    Tag() = default;
};

/**
 * Byte tag (int8_t)
 * Corresponds to: net.minecraft.nbt.ByteTag
 */
class PRISMACRAFT_API ByteTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::BYTE;

    explicit ByteTag(int8_t data = 0) : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<ByteTag>(data);
    }

    int8_t getAsByte() const override { return data; }
    int8_t getData() const { return data; }
    void setData(int8_t value) { data = value; }

private:
    int8_t data;
};

/**
 * Short tag (int16_t)
 * Corresponds to: net.minecraft.nbt.ShortTag
 */
class PRISMACRAFT_API ShortTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::SHORT;

    explicit ShortTag(int16_t data = 0) : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<ShortTag>(data);
    }

    int16_t getAsShort() const override { return data; }
    int16_t getData() const { return data; }
    void setData(int16_t value) { data = value; }

private:
    int16_t data;
};

/**
 * Int tag (int32_t)
 * Corresponds to: net.minecraft.nbt.IntTag
 */
class PRISMACRAFT_API IntTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::INT;

    explicit IntTag(int32_t data = 0) : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<IntTag>(data);
    }

    int32_t getAsInt() const override { return data; }
    int32_t getData() const { return data; }
    void setData(int32_t value) { data = value; }

private:
    int32_t data;
};

/**
 * Long tag (int64_t)
 * Corresponds to: net.minecraft.nbt.LongTag
 */
class PRISMACRAFT_API LongTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::LONG;

    explicit LongTag(int64_t data = 0) : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<LongTag>(data);
    }

    int64_t getAsLong() const override { return data; }
    int64_t getData() const { return data; }
    void setData(int64_t value) { data = value; }

private:
    int64_t data;
};

/**
 * Float tag
 * Corresponds to: net.minecraft.nbt.FloatTag
 */
class PRISMACRAFT_API FloatTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::FLOAT;

    explicit FloatTag(float data = 0.0f) : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<FloatTag>(data);
    }

    float getAsFloat() const override { return data; }
    float getData() const { return data; }
    void setData(float value) { data = value; }

private:
    float data;
};

/**
 * Double tag
 * Corresponds to: net.minecraft.nbt.DoubleTag
 */
class PRISMACRAFT_API DoubleTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::DOUBLE;

    explicit DoubleTag(double data = 0.0) : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<DoubleTag>(data);
    }

    double getAsDouble() const override { return data; }
    double getData() const { return data; }
    void setData(double value) { data = value; }

private:
    double data;
};

/**
 * String tag
 * Corresponds to: net.minecraft.nbt.StringTag
 */
class PRISMACRAFT_API StringTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::STRING;

    explicit StringTag(const std::string& data = "") : data(data) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<StringTag>(data);
    }

    std::string getAsString() const override { return data; }
    const std::string& getData() const { return data; }
    void setData(const std::string& value) { data = value; }

private:
    std::string data;
};

/**
 * Byte array tag
 * Corresponds to: net.minecraft.nbt.ByteArrayTag
 */
class PRISMACRAFT_API ByteArrayTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::BYTE_ARRAY;

    ByteArrayTag() = default;
    explicit ByteArrayTag(std::vector<int8_t> data) : data(std::move(data)) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<ByteArrayTag>(data);
    }

    const std::vector<int8_t>& getData() const { return data; }
    void setData(std::vector<int8_t> value) { data = std::move(value); }
    size_t size() const { return data.size(); }

private:
    std::vector<int8_t> data;
};

/**
 * Int array tag
 * Corresponds to: net.minecraft.nbt.IntArrayTag
 */
class PRISMACRAFT_API IntArrayTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::INT_ARRAY;

    IntArrayTag() = default;
    explicit IntArrayTag(std::vector<int32_t> data) : data(std::move(data)) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<IntArrayTag>(data);
    }

    const std::vector<int32_t>& getData() const { return data; }
    void setData(std::vector<int32_t> value) { data = std::move(value); }
    size_t size() const { return data.size(); }

private:
    std::vector<int32_t> data;
};

/**
 * Long array tag
 * Corresponds to: net.minecraft.nbt.LongArrayTag
 */
class PRISMACRAFT_API LongArrayTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::LONG_ARRAY;

    LongArrayTag() = default;
    explicit LongArrayTag(std::vector<int64_t> data) : data(std::move(data)) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override {
        return std::make_unique<LongArrayTag>(data);
    }

    const std::vector<int64_t>& getData() const { return data; }
    void setData(std::vector<int64_t> value) { data = std::move(value); }
    size_t size() const { return data.size(); }

private:
    std::vector<int64_t> data;
};

/**
 * List tag (heterogeneous list of tags)
 * Corresponds to: net.minecraft.nbt.ListTag
 */
class PRISMACRAFT_API ListTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::LIST;

    ListTag() = default;
    explicit ListTag(TagType listType) : listType(listType) {}

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override;

    const std::vector<std::unique_ptr<Tag>>& getData() const { return tags; }
    size_t size() const { return tags.size(); }
    bool isEmpty() const { return tags.empty(); }
    TagType getListType() const { return listType; }

    void add(std::unique_ptr<Tag> tag);
    void add(size_t index, std::unique_ptr<Tag> tag);
    std::unique_ptr<Tag> remove(size_t index);
    void set(size_t index, std::unique_ptr<Tag> tag);
    Tag* get(size_t index) const { return tags[index].get(); }

    // Helper methods for adding primitive values
    void addByte(int8_t value);
    void addShort(int16_t value);
    void addInt(int32_t value);
    void addLong(int64_t value);
    void addFloat(float value);
    void addDouble(double value);
    void addString(const std::string& value);

private:
    std::vector<std::unique_ptr<Tag>> tags;
    TagType listType = TagType::END;
};

/**
 * Compound tag (key-value map of tags)
 * Corresponds to: net.minecraft.nbt.CompoundTag
 */
class PRISMACRAFT_API CompoundTag : public Tag {
public:
    static constexpr TagType TYPE = TagType::COMPOUND;

    CompoundTag() = default;

    TagType getId() const override { return TYPE; }
    std::string toString() const override;
    std::unique_ptr<Tag> copy() const override;

    const std::unordered_map<std::string, std::unique_ptr<Tag>>& getData() const { return tags; }
    size_t size() const { return tags.size(); }
    bool isEmpty() const { return tags.empty(); }
    bool contains(const std::string& key) const { return tags.find(key) != tags.end(); }

    // Getters
    Tag* get(const std::string& key) const;
    template<typename T>
    T* getOfType(const std::string& key) const {
        auto it = tags.find(key);
        if (it != tags.end()) {
            return dynamic_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    // Primitive getters
    bool getBoolean(const std::string& key) const;
    int8_t getByte(const std::string& key) const;
    int16_t getShort(const std::string& key) const;
    int32_t getInt(const std::string& key) const;
    int64_t getLong(const std::string& key) const;
    float getFloat(const std::string& key) const;
    double getDouble(const std::string& key) const;
    std::string getString(const std::string& key) const;

    // Setters
    void put(const std::string& key, std::unique_ptr<Tag> tag);
    void putBoolean(const std::string& key, bool value);
    void putByte(const std::string& key, int8_t value);
    void putShort(const std::string& key, int16_t value);
    void putInt(const std::string& key, int32_t value);
    void putLong(const std::string& key, int64_t value);
    void putFloat(const std::string& key, float value);
    void putDouble(const std::string& key, double value);
    void putString(const std::string& key, const std::string& value);

    // Remove
    bool remove(const std::string& key);
    std::unique_ptr<Tag> take(const std::string& key);

    // Merge
    void merge(CompoundTag& other);

    // Keys
    std::vector<std::string> getAllKeys() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Tag>> tags;
};

} // namespace NBT
} // namespace PrismaCraft
