#version 460 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in float a_Intensity;

uniform mat4 u_PV;
uniform vec3 u_CloudOffset;
uniform int u_HeatmapMode;      // 0=RGB, 1=Intensity, 2=Plane-Deform
uniform int u_PlaneAxis;        // 0=X, 1=Y, 2=Z
uniform int u_PlaneEdgeMode;    // 0=Clamp, 1=Repeat
uniform float u_PlaneOffset;    // Nivel de referencia
uniform float u_PlaneRange;     // Rango total (puede ser negativo)
uniform float u_MinIntensity;
uniform float u_MaxIntensity;

out vec3 v_Color;

// === RAMPA POINTOOLS EXACTA ===
vec3 SampleRamp(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 stops[9] = vec3[](
        vec3(1.00, 0.00, 0.00),   // 0.0 rojo
        vec3(1.00, 0.50, 0.00),   // naranja
        vec3(1.00, 1.00, 0.00),   // amarillo
        vec3(0.00, 1.00, 0.00),   // verde
        vec3(0.00, 1.00, 1.00),   // cyan
        vec3(0.00, 0.00, 1.00),   // azul
        vec3(0.50, 0.00, 1.00),   // morado
        vec3(1.00, 0.00, 1.00),   // magenta
        vec3(1.00, 0.00, 0.00)    // 1.0 rojo
    );
    float idx = t * 8.0;
    int i = int(floor(idx));
    int j = min(i + 1, 8);
    float f = fract(idx);
    return mix(stops[i], stops[j], f);
}

vec3 HeatmapIntensity(float intensity) {
    float t = clamp((intensity - u_MinIntensity) / max(u_MaxIntensity - u_MinIntensity, 0.001), 0.0, 1.0);
    return SampleRamp(t);
}

vec3 HeatmapPlane(vec3 worldPos) {
    float coord;
    if (u_PlaneAxis == 0) coord = worldPos.x;
    else if (u_PlaneAxis == 1) coord = worldPos.y;
    else coord = worldPos.z;

    // Si u_PlaneRange es negativo, la division invierte la rampa automaticamente:
    // valores altos -> azul, valores bajos -> rojo
    float t = (coord - u_PlaneOffset) / u_PlaneRange + 0.5;

    if (u_PlaneEdgeMode == 1) {
        t = fract(t);
    } else {
        t = clamp(t, 0.0, 1.0);
    }
    return SampleRamp(t);
}

uniform int u_PointDensity = 1;
uniform float u_PointSize = 0.5;

void main() {
    if (u_PointDensity > 1 && gl_VertexID % u_PointDensity != 0) {
        gl_Position = vec4(9999.0, 9999.0, 9999.0, 1.0);
        gl_PointSize = 0.0;
        v_Color = vec3(0.0);
        return;
    }

    vec3 worldPos = a_Position + u_CloudOffset;
    gl_Position = u_PV * vec4(worldPos, 1.0);
    gl_PointSize = u_PointSize;

    if (u_HeatmapMode == 0) {
        v_Color = a_Color;
    } else if (u_HeatmapMode == 1) {
        v_Color = HeatmapIntensity(a_Intensity);
    } else {
        v_Color = HeatmapPlane(worldPos);
    }
}