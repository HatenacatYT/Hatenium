#pragma once
#include <vector>
#include "core/Settings.h"

namespace hat {

struct Vec2 { float x = 0.f, y = 0.f; };
struct Rgba { float r = 1.f, g = 1.f, b = 1.f, a = 1.f; };

// Backend-independent drawing target (ImGui draw list for the preview, GDI+ for the real overlay).
class DrawSink {
public:
    virtual ~DrawSink() = default;
    virtual void Line(Vec2 a, Vec2 b, float thickness, Rgba c) = 0;
    virtual void Circle(Vec2 center, float radius, Rgba c, bool filled, float thickness) = 0;
    virtual void Polygon(const Vec2* pts, int count, Rgba c, bool filled, float thickness) = 0;   // star-shaped polygons
    virtual void RoundRect(Vec2 min, Vec2 max, float radius, Rgba fill, Rgba border, float borderThickness) = 0;
    // align: 0 = left, 1 = centred horizontally; the text is always centred vertically on pos.y
    virtual void Text(Vec2 pos, const char* utf8, float pixelSize, Rgba c, int align, bool bold) = 0;
};

Rgba HsvToRgba(float h, float s, float v, float a);

// Simulates the mouse trail and click particles. All pixel values are multiplied by `scale`
// (1 for the real overlay, canvasHeight/screenHeight for the preview).
class EffectsEngine {
public:
    void Update(float dt, Vec2 cursor, bool cursorValid, bool leftDown, bool rightDown,
                const MouseTrailSettings& trail, const ClickParticleSettings& pc, float scale);
    void Render(DrawSink& sink, const MouseTrailSettings& trail, float scale) const;
    void RenderParticles(DrawSink& sink, float scale) const;
    void Clear();
    bool Active() const { return !trail_.empty() || !particles_.empty(); }

private:
    struct TrailPoint { Vec2 p; float age; };
    struct Particle {
        Vec2 pos, vel;
        float age, life, size, rot, rotSpeed, opacity, hue;
        int shape, mode;
        Rgba c0, c1;
    };
    void Spawn(Vec2 at, const ClickParticleSettings& pc, float scale);

    std::vector<TrailPoint> trail_;
    std::vector<Particle> particles_;
    bool prevLeft_ = false, prevRight_ = false;
    float clock_ = 0.f;
    unsigned rng_ = 12345u;
    float Rand01() { rng_ = rng_ * 1664525u + 1013904223u; return ((rng_ >> 8) & 0xFFFFFF) / 16777216.f; }
};

} // namespace hat
