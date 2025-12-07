#pragma once
#include "Resources.h"
#include "RenderCommandContext.h"

namespace Engine {

    class Material :public Resource
    {
    public:
        void Apply(RenderCommandContext* context);
        ~Material() override;
        bool Load(const std::filesystem::path& path);
        void Unload();
        bool IsLoaded() const;
        ResourceType GetType() const override {
            return ResourceType::Material;
        }

    };


}
