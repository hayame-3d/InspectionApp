#include "PTSImporter.h"
#include "Core/Logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>

namespace InspectionApp {

    static inline std::string Trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return s.substr(start, end - start);
    }

    // Quitar BOM UTF-8 si existe
    static inline std::string RemoveBOM(const std::string& s) {
        if (s.size() >= 3 &&
            static_cast<unsigned char>(s[0]) == 0xEF &&
            static_cast<unsigned char>(s[1]) == 0xBB &&
            static_cast<unsigned char>(s[2]) == 0xBF) {
            return s.substr(3);
        }
        return s;
    }

    static inline bool ParseLineFast(const std::string& line, Point& pt,
        bool hasIntensity, bool hasColor,
        double& outIntensity) {
        const char* p = line.c_str();
        char* end = nullptr;

        while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
        if (!*p) return false;

        float x = std::strtof(p, &end);
        if (p == end) return false; p = end;
        float y = std::strtof(p, &end);
        if (p == end) return false; p = end;
        float z = std::strtof(p, &end);
        if (p == end) return false; p = end;

        pt.position = glm::vec3(x, y, z);

        if (hasIntensity && hasColor) {
            float i = std::strtof(p, &end);
            if (p != end) {
                outIntensity = i;
                p = end;
                long r = std::strtol(p, &end, 10); if (p == end) return true; p = end;
                long g = std::strtol(p, &end, 10); if (p == end) return true; p = end;
                long b = std::strtol(p, &end, 10); if (p == end) return true; p = end;
                pt.color = glm::u8vec3(
                    static_cast<uint8_t>(std::clamp(r, 0L, 255L)),
                    static_cast<uint8_t>(std::clamp(g, 0L, 255L)),
                    static_cast<uint8_t>(std::clamp(b, 0L, 255L))
                );
            }
        }
        else if (hasColor) {
            long r = std::strtol(p, &end, 10); if (p == end) return true; p = end;
            long g = std::strtol(p, &end, 10); if (p == end) return true; p = end;
            long b = std::strtol(p, &end, 10); if (p == end) return true; p = end;
            pt.color = glm::u8vec3(
                static_cast<uint8_t>(std::clamp(r, 0L, 255L)),
                static_cast<uint8_t>(std::clamp(g, 0L, 255L)),
                static_cast<uint8_t>(std::clamp(b, 0L, 255L))
            );
        }
        else if (hasIntensity) {
            float i = std::strtof(p, &end);
            if (p != end) outIntensity = i;
        }

        return true;
    }

    bool PTSImporter::Import(const std::string& filepath, PointCloud& outCloud,
        PTSImportStats& outStats, ProgressCallback progress,
        uint64_t pointLimit) {
        auto t0 = std::chrono::high_resolution_clock::now();

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open PTS file: " + filepath);
            return false;
        }

        outCloud.Clear();
        outCloud.SetSourceFile(filepath);

        std::string line;
        if (!std::getline(file, line)) {
            LOG_ERROR("PTS file is empty: " + filepath);
            return false;
        }

        // Quitar BOM y espacios
        line = RemoveBOM(line);
        std::string headerStr = Trim(line);

        // === VALIDACION DEFENSIVA DEL HEADER ===
        // Verificar que la primera linea sea SOLO un numero entero positivo
        bool headerIsValidNumber = !headerStr.empty();
        for (char c : headerStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                headerIsValidNumber = false;
                break;
            }
        }

        uint64_t declaredCount = 0;
        bool hasHeader = false;

        if (headerIsValidNumber) {
            try {
                declaredCount = std::stoull(headerStr);
                if (declaredCount > 0 && declaredCount <= 500'000'000) { // Max 500M puntos
                    hasHeader = true;
                }
                else if (declaredCount > 500'000'000) {
                    LOG_WARN("PTS header declares " + std::to_string(declaredCount) +
                        " points (suspicious). Parsing without header limit.");
                    hasHeader = false;
                }
            }
            catch (...) {
                hasHeader = false;
            }
        }

        // Si no hay header valido, volver al inicio y parsear todo
        if (!hasHeader) {
            LOG_INFO("PTS without valid header detected. Parsing all lines.");
            file.clear();
            file.seekg(0, std::ios::beg);
            declaredCount = 0; // desconocido
        }

        uint64_t targetCount = declaredCount;
        if (pointLimit > 0 && pointLimit < declaredCount) {
            targetCount = pointLimit;
        }

        // Reserve solo si tenemos un numero razonable
        if (targetCount > 0 && targetCount <= 500'000'000) {
            try {
                outCloud.Reserve(static_cast<size_t>(targetCount));
            }
            catch (const std::exception& e) {
                LOG_WARN("Could not reserve " + std::to_string(targetCount) +
                    " points: " + e.what() + ". Using default growth.");
            }
        }

        LOG_INFO("Importing PTS: " + filepath +
            (hasHeader ? " | Declared points: " + std::to_string(declaredCount) : " | No header"));

        uint64_t readCount = 0;
        uint64_t processedCount = 0;
        double minI = 1e300, maxI = -1e300;
        bool hasColor = false;
        bool hasIntensity = false;

        // Detectar columnas leyendo la primera linea de datos
        if (std::getline(file, line)) {
            auto trimmed = Trim(line);
            if (!trimmed.empty()) {
                int colCount = 0;
                const char* p = trimmed.c_str();
                while (*p) {
                    while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
                    if (!*p) break;
                    ++colCount;
                    while (*p && !std::isspace(static_cast<unsigned char>(*p))) ++p;
                }

                if (colCount >= 7) {
                    hasColor = true;
                    hasIntensity = true;
                }
                else if (colCount >= 6) {
                    hasColor = true;
                }
                else if (colCount >= 4) {
                    hasIntensity = true;
                }
            }

            // Volver al inicio (o despues del header si existe)
            file.clear();
            file.seekg(0, std::ios::beg);
            if (hasHeader) {
                std::getline(file, line); // saltar header
            }
        }

        const uint64_t reportInterval = (targetCount > 0)
            ? std::max<uint64_t>(1, targetCount / 50)
            : 1000;

        while (std::getline(file, line)) {
            if (targetCount > 0 && processedCount >= targetCount) break;

            ++readCount;
            auto trimmed = Trim(line);
            if (trimmed.empty()) continue;

            Point pt;
            double intensity = 0.0;

            if (!ParseLineFast(trimmed, pt, hasIntensity, hasColor, intensity)) {
                ++outStats.pointsSkipped;
                continue;
            }

            if (hasIntensity) {
                minI = std::min(minI, intensity);
                maxI = std::max(maxI, intensity);
                pt.intensity = static_cast<float>(intensity);
            }

            outCloud.GetPoints().push_back(pt);
            ++processedCount;

            if (progress && (processedCount % reportInterval == 0)) {
                progress(processedCount, targetCount > 0 ? targetCount : processedCount, "Parsing PTS...");
            }
        }

        outCloud.ComputeBounds();

        auto t1 = std::chrono::high_resolution_clock::now();
        outStats.parseTimeSeconds = std::chrono::duration<float>(t1 - t0).count();
        outStats.pointsRead = processedCount;
        outStats.pointsSkipped = outStats.pointsSkipped + (readCount - processedCount);
        outStats.hasColor = hasColor;
        outStats.hasIntensity = hasIntensity;
        outStats.minIntensity = minI;
        outStats.maxIntensity = (maxI < -1e200) ? 0.0 : maxI;

        LOG_INFO("PTS imported: " + std::to_string(processedCount) + " points in " +
            std::to_string(outStats.parseTimeSeconds) + "s");

        return processedCount > 0;
    }

} // namespace InspectionApp