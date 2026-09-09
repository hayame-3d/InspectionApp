#pragma once
#include "PointCloud.h"
#include <string>

namespace InspectionApp {

    class CloudSerializer {
    public:
        static constexpr uint32_t CURRENT_VERSION = 1;
        static constexpr char MAGIC[4] = { 'C', 'L', 'O', 'D' };

        static bool Save(const PointCloud& cloud, const std::string& filepath);
        static bool Load(const std::string& filepath, PointCloud& outCloud);
        static bool IsValidCloudFile(const std::string& filepath);
    };

} // namespace InspectionApp