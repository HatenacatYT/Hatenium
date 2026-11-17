#include "rendering/Effects.h"
#include <algorithm>
#include <cmath>

namespace hat {

namespace {
constexpr float kPi = 3.14159265f;
Rgba Col(const Col3& c, float a) { return Rgba{c[0], c[1], c[2], a}; }
Rgba MixC(const Rgba& a, const Rgba& b, float t) {
    return Rgba{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}
} // namespace

Rgba HsvToRgba(float h, float s, float v, float a) {
    h = h - std::floor(h);
    float r = 0, g = 0, b = 0;
    int i = static_cast<int>(h * 6.f);
    float f = h * 6.f - i, p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
    return Rgba{r, g, b, a};
}

void EffectsEngine::Clear() { trail_.clear(); particles_.clear(); prevLeft_ = prevRight_ = false; }

void EffectsEngine::Spawn(Vec2 at, const ClickParticleSettings& pc, float scale) {
    for (int i = 0; i < pc.count && particles_.size() < 800; ++i) {
        Particle p;
        float ang = Rand01() * 2.f * kPi;
        float spd = pc.speed * scale * (0.35f + 0.65f * Rand01());
        p.pos = at;
        p.vel = Vec2{std::cos(ang) * spd, std::sin(ang) * spd};
        p.age = 0.f;
        p.life = pc.lifetimeMs * 0.001f * (0.7f + 0.3f * Rand01());
        p.size = pc.size * scale * (0.6f + 0.4f * Rand01());
        p.rot = Rand01() * 2.f * kPi;
        p.rotSpeed = (Rand01() - 0.5f) * 10.f;
        p.opacity = pc.opacity;
        p.hue = Rand01();
        p.shape = pc.shape;
        p.mode = pc.colorMode;
        p.c0 = Col(pc.color, 1.f);
        p.c1 = Col(pc.color2, 1.f);
        particles_.push_back(p);
    }
}

void EffectsEngine::Update(float dt, Vec2 cursor, bool cursorValid, bool leftDown, bool rightDown,
                           const MouseTrailSettings& trail, const ClickParticleSettings& pc, float scale) {
    clock_ += dt;
    // ---- trail ----
    float life = trail.lifetimeMs * 0.001f;
    for (auto& t : trail_) t.age += dt;
    trail_.erase(std::remove_if(trail_.begin(), trail_.end(), [&](const TrailPoint& t) { return t.age > life; }), trail_.end());
    if (!trail.enable) trail_.clear();
    else if (cursorValid) {
        if (trail_.empty()) trail_.push_back({cursor, 0.f});
        else {
            const Vec2& l = trail_.back().p;
            float dx = cursor.x - l.x, dy = cursor.y - l.y;
            if (dx * dx + dy * dy >= 1.5f * 1.5f) {
                trail_.push_back({cursor, 0.f});
                if (trail_.size() > 700) trail_.erase(trail_.begin(), trail_.begin() + 100);
            }
        }
    }

    // ---- click particles ----
    if (pc.enable && cursorValid) {
        if (pc.onLeft && leftDown && !prevLeft_) Spawn(cursor, pc, scale);
        if (pc.onRight && rightDown && !prevRight_) Spawn(cursor, pc, scale);
    }
    prevLeft_ = leftDown; prevRight_ = rightDown;
    float g = pc.gravity * scale;
    for (auto& p : particles_) {
        p.age += dt;
        p.vel.y += g * dt;
        float drag = std::max(0.f, 1.f - 1.8f * dt);
        p.vel.x *= drag; p.vel.y *= drag;
        p.pos.x += p.vel.x * dt; p.pos.y += p.vel.y * dt;
        p.rot += p.rotSpeed * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) { return p.age >= p.life; }), particles_.end());
    if (!pc.enable && particles_.size() > 0 && !(pc.onLeft || pc.onRight)) particles_.clear();
}

void EffectsEngine::Render(DrawSink& sink, const MouseTrailSettings& tr, float scale) const {
    size_t n = trail_.size();
    if (!tr.enable || n < 2) return;
    float life = std::max(tr.lifetimeMs * 0.001f, 0.001f);
    float th = tr.thickness * scale;

    auto colorAt = [&](float a, size_t i) {
        Rgba c;
        if (tr.colorMode == 0) c = Col(tr.color, 1.f);
        else if (tr.colorMode == 1) c = MixC(Col(tr.color, 1.f), Col(tr.color2, 1.f), a);
        else c = HsvToRgba(clock_ * 0.25f * tr.rainbowSpeed + a * 0.6f + static_cast<float>(i) * 0.0f, 0.85f, 1.f, 1.f);
        c.a = (1.f - a) * tr.opacity;
        return c;
    };

    for (int pass = (tr.glow ? 0 : 1); pass < 2; ++pass) {
        for (size_t i = 1; i < n; ++i) {
            float a = std::min((trail_[i].age + trail_[i - 1].age) * 0.5f / life, 1.f);
            float w = std::max(0.6f, th * (tr.taper || tr.style == 2 ? (1.f - a) : 1.f));
            Rgba c = colorAt(a, i);
            if (pass == 0) { c.a *= 0.18f; w *= 2.6f; }
            if (tr.style == 1) sink.Circle(trail_[i].p, w * 0.5f * (pass == 0 ? 1.f : 1.f), c, true, 0.f);
            else sink.Line(trail_[i - 1].p, trail_[i].p, w, c);
        }
    }
    if (tr.style == 2) {   // bright comet head
        Rgba c = colorAt(0.f, n - 1);
        sink.Circle(trail_.back().p, th * 0.8f, c, true, 0.f);
    }
}

void EffectsEngine::RenderParticles(DrawSink& sink, float scale) const {
    (void)scale;
    for (const auto& p : particles_) {
        float t = std::min(p.age / p.life, 1.f);
        Rgba c;
        switch (p.mode) {
        case 0: c = p.c0; break;
        case 1: c = MixC(p.c0, p.c1, t); break;
        case 2: c = HsvToRgba(p.hue + t * 0.4f + clock_ * 0.2f, 0.85f, 1.f, 1.f); break;
        default: c = HsvToRgba(p.hue, 0.85f, 1.f, 1.f); break;
        }
        c.a = p.opacity * std::pow(1.f - t, 0.8f);
        float sz = std::max(0.8f, p.size * (1.f - 0.6f * t));
        switch (p.shape) {
        case 0: sink.Circle(p.pos, sz, c, true, 0.f); break;
        case 1: {
            Vec2 q[4];
            for (int k = 0; k < 4; ++k) {
                float a = p.rot + k * kPi * 0.5f + kPi * 0.25f;
                q[k] = Vec2{p.pos.x + std::cos(a) * sz * 1.35f, p.pos.y + std::sin(a) * sz * 1.35f};
            }
            sink.Polygon(q, 4, c, true, 0.f);
            break; }
        case 2: {
            Vec2 q[10];
            for (int k = 0; k < 10; ++k) {
                float a = p.rot + k * kPi / 5.f - kPi * 0.5f;
                float r = (k & 1) ? sz * 0.55f : sz * 1.5f;
                q[k] = Vec2{p.pos.x + std::cos(a) * r, p.pos.y + std::sin(a) * r};
            }
            sink.Polygon(q, 10, c, true, 0.f);
            break; }
        default:
            sink.Circle(p.pos, sz * (0.6f + 1.6f * t), c, false, std::max(1.f, sz * 0.35f));
            break;
        }
    }
}

} // namespace hat
