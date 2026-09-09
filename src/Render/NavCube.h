#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <cmath>

namespace InspectionApp {

    struct NavCube {
        glm::vec3 verts[8] = {
            {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
            {-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1}
        };

        struct Face {
            int idx[4];
            const char* name;
            ImU32 color;
        };

        Face faces[6] = {
            {{1,2,6,5}, "R",  IM_COL32(220,100,100,255)},
            {{0,4,7,3}, "L",  IM_COL32(220,100,100,255)},
            {{3,2,6,7}, "F",  IM_COL32(100,220,100,255)},
            {{0,1,5,4}, "Bk", IM_COL32(100,220,100,255)},
            {{4,5,6,7}, "T",  IM_COL32(100,150,220,255)},
            {{0,3,2,1}, "Bt", IM_COL32(100,150,220,255)}
        };

        float size = 70.0f;
        glm::vec2 screenPos{ 0,0 };

        bool isDragging = false;
        bool wasDragged = false;
        float dragStartX = 0.0f;
        float dragStartY = 0.0f;
        float dragStartYaw = 0.0f;
        float dragStartPitch = 0.0f;
        float dragThreshold = 3.0f;

        glm::vec2 GetScreenPos() const {
            ImVec2 canvas = ImGui::GetIO().DisplaySize;
            return glm::vec2(canvas.x - 90.0f, 90.0f);
        }

        void Draw(float yaw, float pitch, float rotZ) {
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            if (!dl) return;

            ImVec2 canvas = ImGui::GetIO().DisplaySize;
            if (canvas.x <= 0 || canvas.y <= 0) return;

            screenPos = GetScreenPos();

            dl->AddCircleFilled(
                ImVec2(screenPos.x, screenPos.y),
                size * 0.65f,
                IM_COL32(40, 40, 40, 180),
                32
            );

            // === MATRIZ DE ORIENTACION 1:1 CON LA CAMARA ===
            // Construimos la direccion exacta que usa la camara
            float yawRad = glm::radians(yaw);
            float pitchRad = glm::radians(pitch);
            glm::vec3 forward(
                std::cos(pitchRad) * std::cos(yawRad),
                std::cos(pitchRad) * std::sin(yawRad),
                std::sin(pitchRad)
            );
            // La camara mira en -forward (desde eye hacia center)
            glm::vec3 cubeForward = -forward;

            // Up y Right calculados igual que Camera::GetViewMatrix
            glm::vec3 worldUp = (std::abs(forward.z) < 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
            glm::vec3 cubeRight = glm::normalize(glm::cross(worldUp, cubeForward));
            glm::vec3 cubeUp = glm::normalize(glm::cross(cubeForward, cubeRight));

            // Aplicar rotZ de la camara alrededor del eje de vista
            if (std::abs(rotZ) > 0.01f) {
                float rad = glm::radians(-rotZ); // negativo para coincidir con la camara
                float c = std::cos(rad), s = std::sin(rad);
                glm::vec3 newRight = cubeRight * c + cubeUp * s;
                glm::vec3 newUp = -cubeRight * s + cubeUp * c;
                cubeRight = glm::normalize(newRight);
                cubeUp = glm::normalize(newUp);
            }

            // Matriz de orientacion del cubo (column-major)
            glm::mat4 orient(1.0f);
            orient[0] = glm::vec4(cubeRight, 0);
            orient[1] = glm::vec4(cubeUp, 0);
            orient[2] = glm::vec4(cubeForward, 0);

            // Base isometrica decorativa (aplicada antes de la orientacion de camara)
            glm::mat4 base = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(1, 0, 0));
            base = glm::rotate(base, glm::radians(45.0f), glm::vec3(0, 1, 0));

            glm::mat4 R = orient * base;

            // Proyeccion de vertices
            glm::vec2 proj[8];
            for (int i = 0; i < 8; i++) {
                glm::vec4 v = R * glm::vec4(verts[i], 1.0f);
                proj[i] = screenPos + glm::vec2(v.x, -v.y) * (size * 0.38f);
            }

            // Aristas
            int edges[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
            for (auto& e : edges) {
                dl->AddLine(
                    ImVec2(proj[e[0]].x, proj[e[0]].y),
                    ImVec2(proj[e[1]].x, proj[e[1]].y),
                    IM_COL32(180, 180, 180, 200), 1.5f
                );
            }

            // Caras
            for (int f = 0; f < 6; f++) {
                glm::vec2 c = (proj[faces[f].idx[0]] + proj[faces[f].idx[1]] + proj[faces[f].idx[2]] + proj[faces[f].idx[3]]) * 0.25f;

                glm::vec2 e1 = proj[faces[f].idx[1]] - proj[faces[f].idx[0]];
                glm::vec2 e2 = proj[faces[f].idx[3]] - proj[faces[f].idx[0]];
                if (e1.x * e2.y - e1.y * e2.x <= 0) continue;

                ImVec2 poly[4];
                for (int i = 0; i < 4; i++) poly[i] = ImVec2(proj[faces[f].idx[i]].x, proj[faces[f].idx[i]].y);

                dl->AddConvexPolyFilled(poly, 4, IM_COL32(
                    (faces[f].color >> IM_COL32_R_SHIFT) & 0xFF,
                    (faces[f].color >> IM_COL32_G_SHIFT) & 0xFF,
                    (faces[f].color >> IM_COL32_B_SHIFT) & 0xFF,
                    40
                ));

                for (int i = 0; i < 4; i++) {
                    int j = (i + 1) % 4;
                    dl->AddLine(poly[i], poly[j], faces[f].color, 1.5f);
                }

                ImVec2 ts = ImGui::CalcTextSize(faces[f].name);
                dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), IM_COL32(255, 255, 255, 255), faces[f].name);
            }
        }

        bool IsOver(const glm::vec2& mouse) const {
            glm::vec2 pos = GetScreenPos();
            return glm::distance(mouse, pos) < size * 0.8f;
        }

        bool BeginDrag(float yaw, float pitch, const glm::vec2& mouse) {
            if (!IsOver(mouse)) return false;
            isDragging = true;
            wasDragged = false;
            dragStartX = mouse.x;
            dragStartY = mouse.y;
            dragStartYaw = yaw;
            dragStartPitch = pitch;
            return true;
        }

        bool UpdateDrag(const glm::vec2& mouse, float& outYaw, float& outPitch) {
            if (!isDragging) return false;
            float dx = mouse.x - dragStartX;
            float dy = mouse.y - dragStartY;
            if (std::abs(dx) > dragThreshold || std::abs(dy) > dragThreshold) {
                wasDragged = true;
            }
            if (wasDragged) {
                // === SIGNOS DEL DRAG ===
                // Cambia solo el signo de dx para invertir izquierda/derecha.
                // +dx = estilo Revit (agarras la escena, arrastras izq -> gira izq)
                // -dx = estilo deslizar foto (arrastras izq -> gira der)
                outYaw = dragStartYaw - dx * 0.3f;    // <-- cambia a -dx para probar
                outPitch = dragStartPitch + dy * 0.3f;  // <-- Y ya confirmado que esta bien
                if (outPitch > 89.0f) outPitch = 89.0f;
                if (outPitch < -89.0f) outPitch = -89.0f;
            }
            return wasDragged;
        }

        void EndDrag() {
            isDragging = false;
            wasDragged = false;
        }
    };

} // namespace