#pragma once
#include <cstdint>
#include <string>

namespace InspectionApp {

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    using f32 = float;
    using f64 = double;

    struct Vec3 {
        f32 x, y, z;
    };

    struct Vec4 {
        f32 x, y, z, w;
    };

    struct Mat4 {
        f32 m[16];
    };

} // namespace InspectionApp