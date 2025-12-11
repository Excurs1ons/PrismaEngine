#include "Asset.h"
#include "AssetSerializer.h"

namespace Engine {
		bool Asset::SerializeToFile(const std::filesystem::path& filePath, SerializationFormat format) const
		{
			return AssetSerializer::SerializeToFile(*this, filePath, format);
		}
}