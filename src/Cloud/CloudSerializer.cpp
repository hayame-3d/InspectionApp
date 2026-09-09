#include "CloudSerializer.h"
#include "Core/Logger.h"
#include <fstream>
#include <cstring>

namespace InspectionApp {

#pragma pack(push, 1)
    struct CloudHeader {
        char magic[4];
        uint32_t version;
        uint64_t pointCount;
        uint32_t flags;
        float minX, minY, minZ;
        float maxX, maxY, maxZ;
        uint32_t sourceFileLen;
        char reserved[64];
    };
    struct CloudPointRaw {
        float x, y, z;
        uint8_t r, g, b;
        float intensity;
    };
#pragma pack(pop)

    bool CloudSerializer::Save(const PointCloud& cloud, const std::string& filepath) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file) { LOG_ERROR("Cannot write CLOUD file: " + filepath); return false; }
        const auto& pts = cloud.GetPoints();
        const auto& b = cloud.GetBounds();
        CloudHeader header{};
        std::memcpy(header.magic, MAGIC, 4);
        header.version = CURRENT_VERSION;
        header.pointCount = pts.size();
        header.flags = 0x3;
        header.minX = b.min.x; header.minY = b.min.y; header.minZ = b.min.z;
        header.maxX = b.max.x; header.maxY = b.max.y; header.maxZ = b.max.z;
        const std::string& src = cloud.GetSourceFile();
        header.sourceFileLen = static_cast<uint32_t>(src.size());
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(src.data(), src.size());
        for (const auto& p : pts) {
            CloudPointRaw raw;
            raw.x = p.position.x; raw.y = p.position.y; raw.z = p.position.z;
            raw.r = p.color.r; raw.g = p.color.g; raw.b = p.color.b;
            raw.intensity = p.intensity;
            file.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
        }
        LOG_INFO("CLOUD saved: " + filepath + " | Points: " + std::to_string(pts.size()));
        return true;
    }

    bool CloudSerializer::Load(const std::string& filepath, PointCloud& outCloud) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) { LOG_ERROR("Cannot open CLOUD file: " + filepath); return false; }
        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        CloudHeader header;
        if (!file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
            LOG_ERROR("CLOUD file too small (header)"); return false;
        }
        if (std::memcmp(header.magic, MAGIC, 4) != 0) {
            LOG_ERROR("Invalid CLOUD magic number"); return false;
        }
        if (header.version != CURRENT_VERSION) {
            LOG_ERROR("Unsupported CLOUD version: " + std::to_string(header.version)); return false;
        }
        file.seekg(header.sourceFileLen, std::ios::cur);
        auto dataSize = static_cast<size_t>(fileSize - file.tellg());
        auto expectedSize = header.pointCount * sizeof(CloudPointRaw);
        if (dataSize < expectedSize) { LOG_ERROR("CLOUD file truncated"); return false; }
        outCloud.Clear();
        outCloud.Reserve(header.pointCount);
        outCloud.GetPoints().resize(header.pointCount);
        // Leer punto por punto para evitar problemas de padding entre structs
        for (uint64_t i = 0; i < header.pointCount; ++i) {
            CloudPointRaw raw;
            file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
            auto& p = outCloud.GetPoints()[i];
            p.position = glm::vec3(raw.x, raw.y, raw.z);
            p.color = glm::u8vec3(raw.r, raw.g, raw.b);
            p.intensity = raw.intensity;
        }
        outCloud.SetBounds(
            glm::vec3(header.minX, header.minY, header.minZ),
            glm::vec3(header.maxX, header.maxY, header.maxZ)
        );
        LOG_INFO("CLOUD loaded: " + filepath + " | Points: " + std::to_string(header.pointCount));
        return true;
    }

    bool CloudSerializer::IsValidCloudFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return false;
        char magic[4];
        file.read(magic, 4);
        return std::memcmp(magic, MAGIC, 4) == 0;
    }

}