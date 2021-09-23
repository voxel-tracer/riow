#pragma once

#include <callbacks.h>
#include <vector>

#include "tracer_data.h"

namespace callback {
    class build_segments_cb : public callback {
    private:
        vec3 p;
        color c;
        vec3 d; // direction

    public:
        virtual void operator ()(std::shared_ptr<Event> e) override {
            if (auto n = cast<New>(e)) {
                p = n->r.origin();
                d = n->r.direction();
                c = color(1, 1, 1); // new segments are white
            }
            else if (auto h = cast<Hit>(e)) {
                segments.push_back({ toYocto(p), toYocto(h->p), toYocto(c) });
                p = h->p;
            }
            else if (auto sc = cast<Scatter>(e)) {
                d = sc->d;
                if (auto ss = cast<SpecularScatter>(e)) {
                    if (ss->srec.is_refracted)
                        c = color(1, 1, 0); // refracted specular is yellow
                    else
                        c = color(0, 1, 1); // reflected specular is aqua
                }
                else if (cast<MediumScatter>(e)) {
                    c = color(1, 0, 1); // medium scatter is purple
                }
                else {
                    c = color(1, 0.5, 0); // diffuse is orange
                }
            }
            else if (auto nh = cast<NoHitTerminal>(e)) {
                // generate a segment that point towards the general direction of the scattered ray
                segments.push_back({ toYocto(p), toYocto(p + d), toYocto(c) });
            }
        }

        std::vector<tool::path_segment> segments;
    };
}