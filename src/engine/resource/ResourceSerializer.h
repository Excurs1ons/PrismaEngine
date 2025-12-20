#pragma once
#include "Mesh.h"
#include "ResourceBase.h"
#include "../math/MathTypes.h"
#include <filesystem>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace nlohmann {
    // 第三方类型的序列化集中在这里

    template <>
    struct adl_serializer<PrismaMath::vec2> {
        static void to_json(json& j, const PrismaMath::vec2& v) {
            j = json{ {"x", v.x}, {"y", v.y} };
        }
        static void from_json(const json& j, PrismaMath::vec2& v) {
            j.at("x").get_to(v.x);
            j.at("y").get_to(v.y);
		}
    };

    template <>
    struct adl_serializer<PrismaMath::vec3> {
        static void to_json(json& j, const PrismaMath::vec3& v) {
            j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
        }
        static void from_json(const json& j, PrismaMath::vec3& v) {
            j.at("x").get_to(v.x);
            j.at("y").get_to(v.y);
            j.at("z").get_to(v.z);
		}
    };

    template <>
    struct adl_serializer<PrismaMath::vec4> {
        static void to_json(json& j, const PrismaMath::vec4& v) {
            j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w} };
        }
        static void from_json(const json& j, PrismaMath::vec4& v) {
            j.at("x").get_to(v.x);
            j.at("y").get_to(v.y);
            j.at("z").get_to(v.z);
            j.at("w").get_to(v.w);
		}
    };

    template <>
    struct adl_serializer<std::filesystem::path> {
        static void to_json(json& j, const std::filesystem::path& p) {
            j = p.string();
        }
        static void from_json(const json& j, std::filesystem::path& p) {
            p = j.get<std::string>();
        }
    };

    template <>
    struct adl_serializer<Vertex> {
        static void to_json(json& j, const Vertex& vertex) {
            j = json{
                {"position", vertex.position},
                {"normal", vertex.normal},
                {"texCoord", vertex.texCoord},
                {"tangent", vertex.tangent},
                {"color", vertex.color}
            };
        }
        static void from_json(const json& j, Vertex& vertex) {
            j.at("position").get_to(vertex.position);
			j.at("normal").get_to(vertex.normal);
			j.at("texCoord").get_to(vertex.texCoord);
			j.at("tangent").get_to(vertex.tangent);
			j.at("color").get_to(vertex.color);
        }
    };


    template <>
    struct adl_serializer<SubMesh> {
        static void to_json(json& j, const SubMesh& subMesh) {
            j = json{
                {"name", subMesh.name},
                {"materialIndex", subMesh.materialIndex},
                {"vertices", subMesh.vertices},
                {"indices", subMesh.indices}
            };
        }
        static void from_json(const json& j, SubMesh& subMesh) {
            j.at("name").get_to(subMesh.name);
        }
    };
}
