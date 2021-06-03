#pragma once

#include <vector>
#include "vec3.h"
#include "ray.h"
#include "tool/tracer_data.h"

inline yocto::vec3f toYocto(const vec3& v) {
    return { (float)v[0], (float)v[1], (float)v[2] };
}

enum class render_state {
    SPECULAR, DIFFUSE, NOHIT, ABSORBED, MAXDEPTH, RROULETTE
};

struct callback_data {
    ray r;
    vec3 p;
    color albedo;
    render_state state;

    callback_data(render_state s, const color& a = { 0.0f, 0.0f, 0.0f }) :
        state(s), albedo(a) {}

    callback_data(const ray& _r, const vec3& _p, const color& a, render_state s) :
        r(_r), p(_p), albedo(a), state(s) {}

    tool::path_segment toSegment() const {
        if (state == render_state::NOHIT)
            return { toYocto(r.origin()), toYocto(r.at(1)), toYocto(albedo) };
        return { toYocto(r.origin()), toYocto(p), toYocto(albedo) };
    }
};

class render_callback {
public:
    virtual void operator ()(const callback_data&) {}
};

class num_inters_callback : public render_callback {
public:
    virtual void operator()(const callback_data& data) override {
        if (data.state != render_state::MAXDEPTH &&
            data.state != render_state::RROULETTE) {
            ++count;
        }
    }

    unsigned long count = 0;
};

class tracer {
public:
    virtual void Render(render_callback &callback) = 0;
    virtual void Render() = 0;
    virtual void DebugPixel(unsigned x, unsigned y, std::vector<tool::path_segment>& segments) = 0;
};