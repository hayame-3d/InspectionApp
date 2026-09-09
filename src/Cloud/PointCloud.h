#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace InspectionApp {

    struct Point {
        glm::vec3 position;
        glm::u8vec3 color{ 255, 255, 255 };
        float intensity = 0.0f;
    };

    struct BoundingBox {
        glm::vec3 min{ 0.0f };
        glm::vec3 max{ 0.0f };
        bool IsValid() const { return min.x <= max.x; }
        glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
        glm::vec3 GetSize() const { return max - min; }
    };

    class PointCloud {
    public:
        PointCloud() = default;
        const std::vector<Point>& GetPoints() const { return m_points; }
        std::vector<Point>& GetPoints() { return m_points; }
        const BoundingBox& GetBounds() const { return m_bounds; }
        uint64_t GetPointCount() const { return m_pointCount; }
        const std::string& GetSourceFile() const { return m_sourceFile; }
        void SetSourceFile(const std::string& path) { m_sourceFile = path; }
        void SetBounds(const glm::vec3& min, const glm::vec3& max) { m_bounds.min = min; m_bounds.max = max; }
        void ComputeBounds();
        void Clear();
        void Reserve(uint64_t count);
        const Point* Data() const { return m_points.data(); }
        size_t DataSizeBytes() const { return m_points.size() * sizeof(Point); }
    private:
        std::vector<Point> m_points;
        BoundingBox m_bounds;
        uint64_t m_pointCount = 0;
        std::string m_sourceFile;
    };

    inline void PointCloud::ComputeBounds() {
        if (m_points.empty()) { m_bounds = BoundingBox{}; m_pointCount = 0; return; }
        glm::vec3 bmin = m_points[0].position, bmax = m_points[0].position;
        for (const auto& p : m_points) { bmin = glm::min(bmin, p.position); bmax = glm::max(bmax, p.position); }
        m_bounds.min = bmin; m_bounds.max = bmax;
        m_pointCount = static_cast<uint64_t>(m_points.size());
    }

    inline void PointCloud::Clear() {
        m_points.clear(); m_bounds = BoundingBox{}; m_pointCount = 0; m_sourceFile.clear();
    }

    inline void PointCloud::Reserve(uint64_t count) {
        m_points.reserve(static_cast<size_t>(count));
    }

}