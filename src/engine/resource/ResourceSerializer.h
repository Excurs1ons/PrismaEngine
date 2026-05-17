#pragma once
#include "Mesh.h"
#include "ResourceBase.h"
#include "../math/MathTypes.h"
#include <filesystem>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

namespace Prisma::Serialization {
    // Note: Glaze uses glz::meta for custom types. 
    // Most vector types are handled explicitly in ArchiveJson.
}
