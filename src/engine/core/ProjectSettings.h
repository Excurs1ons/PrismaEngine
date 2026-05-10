#pragma once
#include "../resource/Archive.h"
#include "../serialization/Serializable.h"
#include <string>

namespace Prisma {
namespace Core {
struct ProjectSettings : public Serialization::ISerializable {
    std::string companyName = "DefaultCompany";
    std::string productName = "PrismaProject";
    std::string version     = "1.0.0";

    int32_t screenWidth  = 1280;
    int32_t screenHeight = 720;
    bool fullscreen      = false;
    bool resizable       = true;

    // Serialization implementation
    void Serialize(Serialization::OutputArchive& archive) const override {
        archive("companyName", companyName);
        archive("productName", productName);
        archive("version", version);
        archive("screenWidth", screenWidth);
        archive("screenHeight", screenHeight);
        archive("fullscreen", fullscreen);
        archive("resizable", resizable);
    }

    void Deserialize(Serialization::InputArchive& archive) override {
        archive("companyName", companyName);
        archive("productName", productName);
        archive("version", version);
        archive("screenWidth", screenWidth);
        archive("screenHeight", screenHeight);
        archive("fullscreen", fullscreen);
        archive("resizable", resizable);
    }
};
}  // namespace Core
}  // namespace Prisma