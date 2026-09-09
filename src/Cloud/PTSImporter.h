#pragma once
#include "PointCloud.h"
#include <string>
#include <functional>

namespace InspectionApp {

    struct PTSImportStats {
        uint64_t pointsRead = 0;
        uint64_t pointsSkipped = 0;
        double minIntensity = 0.0;
        double maxIntensity = 0.0;
        bool hasColor = false;
        bool hasIntensity = false;
        float parseTimeSeconds = 0.0f;
    };

    class PTSImporter {
    public:
        using ProgressCallback = std::function<void(uint64_t current, uint64_t total, const std::string& phase)>;

        static bool Import(const std::string& filepath, PointCloud& outCloud,
            PTSImportStats& outStats,
            ProgressCallback progress = nullptr,
            uint64_t pointLimit = 0);
    };

} // namespace InspectionApp