#include "PointCloud.h"

namespace InspectionApp {

    void PointCloud::ComputeBounds() {
        if (m_points.empty()) {
            m_bounds = BoundingBox{};
            m_pointCount = 0;
            return;
        }
        glm::vec3 min = m_points[0].position;
        glm::vec3 max = m_points[0].position;
        for (const auto& p : m_points) {
            min = glm::min(min, p.position);
            max = glm::max(max, p.position);
        }
        m_bounds.min = min;
        m_bounds.max = max;
        m_pointCount = m_points.size();
    }

    void PointCloud::Clear() {
        m_points.clear();
        m_bounds = BoundingBox{};
        m_pointCount = 0;
        m_sourceFile.clear();
    }

    void PointCloud::Reserve(uint64_t count) {
        m_points.reserve(static_cast<size_t>(count));
    }

} // namespace InspectionApp