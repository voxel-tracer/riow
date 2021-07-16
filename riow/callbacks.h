#pragma once

#include <vector>
#include <iostream>

#include "tracer_callback.h"

namespace callback {
    class num_inters_callback : public callback {
    public:
        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (std::dynamic_pointer_cast<Hit>(event)) {
                ++count;
            }
        }

        unsigned count = 0;
    };


    inline yocto::vec3f toYocto(const vec3& v) {
        return { (float)v[0], (float)v[1], (float)v[2] };
    }

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
                    if (ss->is_refracted)
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
                segments.push_back({ toYocto(p), toYocto(p + d), toYocto(color(0, 0, 0)) });
            }
        }

        std::vector<tool::path_segment> segments;
    };

    class print_pdf_sample_cb : public callback {
    private:
        bool done = false;

    public:
        virtual void operator ()(std::shared_ptr<Event> e) override {
            if (auto n = cast<New>(e)) {
                sample_id = n->sampleId;
            }
            else if (auto p = cast<PdfSample>(e)) {
                if (p->pdf_name.find("light") != std::string::npos) {
                    done = true;
                    std::cerr << "Sample " << sample_id << " has " << p->pdf_name << std::endl;
                }
            }
        }

        virtual bool terminate() const override {
            return done;
        }

        unsigned sample_id;
    };

    class print_callback : public callback {
    private:
        const bool verbose;
    public:
        print_callback(bool verbose = false) : verbose(verbose) {}

        virtual void operator ()(std::shared_ptr<Event> e) override {
            if (auto b = cast<Bounce>(e)) {
                std::cerr << b->depth << ":\n";
            }
            else {
                std::cerr << "\t";
                if (verbose)
                    e->digest(std::cerr);
                else
                    std::cerr << e->name;
                std::cerr << std::endl;
            }
        }
    };

    class print_sample_callback : public callback {
    private:
        const bool verbose;
        const unsigned target_sample;
        bool target_found = false;

    public:
        print_sample_callback(unsigned target_sample, bool verbose = false) :
            target_sample(target_sample), verbose(verbose) {}

        virtual void operator ()(std::shared_ptr<Event> e) override {
            if (auto n = cast<New>(e)) {
                target_found = n->sampleId == target_sample;
            }
            else if (target_found) {
                if (auto b = cast<Bounce>(e)) {
                    std::cerr << b->depth << ": throughput = " << b->throughput << std::endl;
                }
                else {
                    std::cerr << "\t";
                    if (verbose)
                        e->digest(std::cerr);
                    else
                        std::cerr << e->name;
                    std::cerr << std::endl;
                }
            }
        }
    };

    class count_max_depth : public callback {
    private:
        std::atomic_ulong cnt;

    public:
        virtual void operator ()(std::shared_ptr<Event> e) override {
            if (cast<MaxDepthTerminal>(e)) {
                cnt++;
            }
        }

        unsigned long total() const { return cnt; }
    };

    /*
    * Checks if any ray goes through the a particular surface
    * If it does then eventually it will intersect the same surface again from the inside (rec.front_face == false)
    */
    class dbg_find_gothrough_diffuse : public callback {
    private:
        const bool verbose;
        const bool stopAtFirstFound;
        const std::string target;

        bool foundTarget = false;
        bool foundBug = false;
        std::shared_ptr<New> n;
        unsigned long count = 0;

    public:
        dbg_find_gothrough_diffuse(std::string t, bool verbose, bool stopAtFirst)
            : target(t), verbose(verbose), stopAtFirstFound(stopAtFirst) {}

        virtual void operator ()(event_ptr e) override {
            if (auto ne = cast<New>(e)) {
                n = ne;
                foundTarget = foundBug = false;
            }
            else if (auto b = cast<Bounce>(e)) {
                foundTarget = false;
            }
            else if (auto h = cast<SurfaceHit>(e)) {
                if (!foundTarget && h->rec.obj_ptr->name == target) {
                    foundTarget = true;
                }
            }
            else if (auto ds = cast<DiffuseScatter>(e)) {
                if (foundTarget && !foundBug && dot(ds->rec.normal, ds->d) < 0) {
                    if (verbose) {
                        std::cout << "\nFound a go through ray at (" << n->x << ", " << n->y << "):" << n->sampleId << std::endl;
                    }
                    foundBug = true;
                    ++count;
                }
            }
        }

        virtual bool terminate() const override {
            return stopAtFirstFound && foundBug;
        }

        virtual void alterPixelColor(vec3& clr) const override {
            if (!stopAtFirstFound && foundBug)
                clr = { 1.0, 1.0, 1.0 };
            else
                clr = { 0.0 };
        }

        unsigned long getCount() const { return count; }
    };

}