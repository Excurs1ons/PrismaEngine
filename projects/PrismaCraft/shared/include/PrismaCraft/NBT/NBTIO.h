#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "NBTTag.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <zlib.h>

namespace PrismaCraft {
namespace NBT {

/**
 * Data input stream for NBT reading
 * Corresponds to: net.minecraft.nbt.DataInput
 */
class PRISMACRAFT_API DataInput {
public:
    virtual ~DataInput() = default;

    virtual int8_t readByte() = 0;
    virtual int16_t readShort() = 0;
    virtual int32_t readInt() = 0;
    virtual int64_t readLong() = 0;
    virtual float readFloat() = 0;
    virtual double readDouble() = 0;
    virtual std::string readUtf() = 0;
    virtual std::vector<int8_t> readByteArray(size_t length) = 0;
    virtual std::vector<int32_t> readIntArray(size_t length) = 0;
    virtual std::vector<int64_t> readLongArray(size_t length) = 0;
};

/**
 * Data output stream for NBT writing
 * Corresponds to: net.minecraft.nbt.DataOutput
 */
class PRISMACRAFT_API DataOutput {
public:
    virtual ~DataOutput() = default;

    virtual void writeByte(int8_t value) = 0;
    virtual void writeShort(int16_t value) = 0;
    virtual void writeInt(int32_t value) = 0;
    virtual void writeLong(int64_t value) = 0;
    virtual void writeFloat(float value) = 0;
    virtual void writeDouble(double value) = 0;
    virtual void writeUtf(const std::string& value) = 0;
    virtual void writeByteArray(const int8_t* data, size_t length) = 0;
    virtual void writeIntArray(const int32_t* data, size_t length) = 0;
    virtual void writeLongArray(const int64_t* data, size_t length) = 0;
};

/**
 * Stream-based data input
 */
class PRISMACRAFT_API StreamDataInput : public DataInput {
public:
    explicit StreamDataInput(std::istream& stream) : stream(stream) {}

    int8_t readByte() override {
        int8_t value;
        stream.read(reinterpret_cast<char*>(&value), 1);
        if (stream.fail()) throw std::runtime_error("Failed to read byte");
        return value;
    }

    int16_t readShort() override {
        int16_t value;
        stream.read(reinterpret_cast<char*>(&value), 2);
        if (stream.fail()) throw std::runtime_error("Failed to read short");
        return value;
    }

    int32_t readInt() override {
        int32_t value;
        stream.read(reinterpret_cast<char*>(&value), 4);
        if (stream.fail()) throw std::runtime_error("Failed to read int");
        return value;
    }

    int64_t readLong() override {
        int64_t value;
        stream.read(reinterpret_cast<char*>(&value), 8);
        if (stream.fail()) throw std::runtime_error("Failed to read long");
        return value;
    }

    float readFloat() override {
        float value;
        stream.read(reinterpret_cast<char*>(&value), 4);
        if (stream.fail()) throw std::runtime_error("Failed to read float");
        return value;
    }

    double readDouble() override {
        double value;
        stream.read(reinterpret_cast<char*>(&value), 8);
        if (stream.fail()) throw std::runtime_error("Failed to read double");
        return value;
    }

    std::string readUtf() override {
        uint16_t length = static_cast<uint16_t>(readShort());
        std::string value(length, '\0');
        stream.read(&value[0], length);
        if (stream.fail()) throw std::runtime_error("Failed to read string");
        return value;
    }

    std::vector<int8_t> readByteArray(size_t length) override {
        std::vector<int8_t> data(length);
        stream.read(reinterpret_cast<char*>(data.data()), length);
        if (stream.fail()) throw std::runtime_error("Failed to read byte array");
        return data;
    }

    std::vector<int32_t> readIntArray(size_t length) override {
        std::vector<int32_t> data(length);
        stream.read(reinterpret_cast<char*>(data.data()), length * 4);
        if (stream.fail()) throw std::runtime_error("Failed to read int array");
        return data;
    }

    std::vector<int64_t> readLongArray(size_t length) override {
        std::vector<int64_t> data(length);
        stream.read(reinterpret_cast<char*>(data.data()), length * 8);
        if (stream.fail()) throw std::runtime_error("Failed to read long array");
        return data;
    }

private:
    std::istream& stream;
};

/**
 * Stream-based data output
 */
class PRISMACRAFT_API StreamDataOutput : public DataOutput {
public:
    explicit StreamDataOutput(std::ostream& stream) : stream(stream) {}

    void writeByte(int8_t value) override {
        stream.write(reinterpret_cast<const char*>(&value), 1);
    }

    void writeShort(int16_t value) override {
        stream.write(reinterpret_cast<const char*>(&value), 2);
    }

    void writeInt(int32_t value) override {
        stream.write(reinterpret_cast<const char*>(&value), 4);
    }

    void writeLong(int64_t value) override {
        stream.write(reinterpret_cast<const char*>(&value), 8);
    }

    void writeFloat(float value) override {
        stream.write(reinterpret_cast<const char*>(&value), 4);
    }

    void writeDouble(double value) override {
        stream.write(reinterpret_cast<const char*>(&value), 8);
    }

    void writeUtf(const std::string& value) override {
        uint16_t length = static_cast<uint16_t>(value.length());
        writeShort(length);
        stream.write(value.data(), length);
    }

    void writeByteArray(const int8_t* data, size_t length) override {
        stream.write(reinterpret_cast<const char*>(data), length);
    }

    void writeIntArray(const int32_t* data, size_t length) override {
        stream.write(reinterpret_cast<const char*>(data), length * 4);
    }

    void writeLongArray(const int64_t* data, size_t length) override {
        stream.write(reinterpret_cast<const char*>(data), length * 8);
    }

private:
    std::ostream& stream;
};

/**
 * NBT reader
 * Corresponds to: net.minecraft.nbt.NbtIo
 */
class PRISMACRAFT_API NbtReader {
public:
    // Read named tag from stream
    static std::unique_ptr<Tag> read(DataInput& input);
    static std::unique_ptr<Tag> read(std::istream& stream);

    // Read compound tag from stream (common case)
    static std::unique_ptr<CompoundTag> readCompound(DataInput& input);
    static std::unique_ptr<CompoundTag> readCompound(std::istream& stream);

    // Read tag with specific type (unnamed)
    static std::unique_ptr<Tag> readTag(DataInput& input, TagType type);

    // Read from file
    static std::unique_ptr<CompoundTag> readFile(const std::string& path);

private:
    static std::unique_ptr<Tag> readTagPayload(DataInput& input, TagType type, int depth);
    static constexpr int MAX_DEPTH = 512;
};

/**
 * NBT writer
 * Corresponds to: net.minecraft.nbt.NbtIo
 */
class PRISMACRAFT_API NbtWriter {
public:
    // Write named tag to stream
    static void write(const Tag& tag, DataOutput& output);
    static void write(const Tag& tag, std::ostream& stream);

    // Write compound tag to stream (common case)
    static void writeCompound(const CompoundTag& tag, DataOutput& output);
    static void writeCompound(const CompoundTag& tag, std::ostream& stream);

    // Write tag with specific type (unnamed)
    static void writeTag(const Tag& tag, DataOutput& output);

    // Write to file
    static void writeFile(const CompoundTag& tag, const std::string& path);

private:
    static void writeTagPayload(const Tag& tag, DataOutput& output);
};

/**
 * Compressed NBT I/O
 * Corresponds to: net.minecraft.nbt.NbtIo (compressed versions)
 */
class PRISMACRAFT_API CompressedNbtIO {
public:
    // GZIP compression
    static std::unique_ptr<CompoundTag> readGZip(const std::string& path);
    static std::unique_ptr<CompoundTag> readGZipData(const std::vector<uint8_t>& data);
    static void writeGZip(const CompoundTag& tag, const std::string& path);
    static std::vector<uint8_t> writeGZipData(const CompoundTag& tag);

    // ZLIB compression
    static std::unique_ptr<CompoundTag> readZlib(const std::string& path);
    static std::unique_ptr<CompoundTag> readZlibData(const std::vector<uint8_t>& data);
    static void writeZlib(const CompoundTag& tag, const std::string& path);
    static std::vector<uint8_t> writeZlibData(const CompoundTag& tag);

private:
    // Buffer size for compression
    static constexpr size_t BUFFER_SIZE = 8192;
};

/**
 * NBT utilities for pretty printing
 */
class PRISMACRAFT_API NbtPrinter {
public:
    // Pretty print tag to string
    static std::string prettyPrint(const Tag& tag);
    static std::string prettyPrint(const Tag& tag, int indent);

    // Helper methods for specific tag types
    static std::string tagToString(TagType type);

private:
    static std::string indentStr(int indent);
};

} // namespace NBT
} // namespace PrismaCraft
