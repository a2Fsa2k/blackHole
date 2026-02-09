#include <cmath>
#include <fstream>
#include <iostream>

// --------------------
// Math helpers
// --------------------

constexpr float PI = 3.1415926538f;

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;

    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
};

float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

float length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

Vec3 normalize(const Vec3& v) {
    float len = length(v);
    return v * (1.0f / len);
}

// --------------------
// Checkerboard (CPU version)
// --------------------

float checkerAA(const Vec2& p) {
    // No GPU derivatives here — hard checker
    float sx = std::sin(PI * p.x * 20.0f);
    float sy = std::sin(PI * p.y * 10.0f);
    float m = sx * sy;
    return (m > 0.0f) ? 1.0f : 0.0f;
}

// --------------------
// Render one pixel
// --------------------

Vec3 renderPixel(int px, int py, int width, int height, float time) {
    // === Shadertoy mapping ===
    // fragCoord -> (px, py)
    // iResolution -> (width, height)
    // iTime -> time

    Vec2 fragCoord = { float(px), float(py) };
    Vec2 resolution = { float(width), float(height) };

    Vec2 uv;
    uv.x = (2.0f * fragCoord.x - resolution.x) / resolution.x;
    uv.y = (2.0f * fragCoord.y - resolution.y) / resolution.x;

    float hfov = 2.3f;
    float dist = 5.0f;

    Vec3 vel = normalize({
        1.0f,
        -uv.x * std::tan(hfov / 2.0f),
        -uv.y * std::tan(hfov / 2.0f)
    });

    Vec3 pos = { -dist, 0.0f, 0.0f };
    float r = length(pos);
    float dtau = 0.2f;

    while (r < dist * 2.0f && r > 1.0f) {
        float ddtau = dtau * r;
        pos += vel * ddtau;
        r = length(pos);

        Vec3 er = pos * (1.0f / r);
        Vec3 c = cross(vel, er);

        vel -= er * (ddtau * dot(c, c) / (r * r));
    }

    float phi1 = 1.0f - std::atan2(vel.y, vel.x) / (2.0f * PI);
    float theta1 = 1.0f - std::atan2(std::sqrt(vel.x*vel.x + vel.y*vel.y), vel.z) / PI;

    Vec2 UV = {
        phi1 + time * 0.01f,
        theta1
    };

    float checker = checkerAA({
        UV.x * 180.0f / PI / 30.0f,
        UV.y * 180.0f / PI / 30.0f
    });

    float visible = (r > 1.0f) ? 1.0f : 0.0f;
    float v = checker * visible;

    return { v, v, v };
}

// --------------------
// Main
// --------------------

int main() {
    const int WIDTH = 1280;
    const int HEIGHT = 720;
    const float time = 0.0f; // single frame

    std::ofstream out("frame.ppm", std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }

    // PPM header
    out << "P6\n" << WIDTH << " " << HEIGHT << "\n255\n";

    for (int y = HEIGHT - 1; y >= 0; --y) {
        for (int x = 0; x < WIDTH; ++x) {
            Vec3 col = renderPixel(x, y, WIDTH, HEIGHT, time);

            unsigned char r = (unsigned char)(std::fmin(col.x, 1.0f) * 255.0f);
            unsigned char g = (unsigned char)(std::fmin(col.y, 1.0f) * 255.0f);
            unsigned char b = (unsigned char)(std::fmin(col.z, 1.0f) * 255.0f);

            out.write((char*)&r, 1);
            out.write((char*)&g, 1);
            out.write((char*)&b, 1);
        }
    }

    out.close();
    std::cout << "Rendered frame.ppm\n";
    return 0;
}
