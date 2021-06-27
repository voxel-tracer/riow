#pragma once

#include <string>
#include <memory>
#include <sstream>

#include "rtweekend.h"
#include "hit_record.h"
#include "hittable.h"
#include "tool/tracer_data.h"

namespace callback {
    class Event {
    public:
        Event(std::string name) : name(name) {}
        virtual ~Event() {}

        virtual std::ostream& digest(std::ostream& o) const {
            return o << name;
        }

        const std::string name;
    };

    typedef std::shared_ptr<Event> event_ptr;

    template<typename T>
    static std::shared_ptr<T> cast(const event_ptr e) {
        return std::dynamic_pointer_cast<T>(e);
    }

    class callback {
    public:
        virtual void operator ()(event_ptr event) {}
        virtual bool terminate() const { return false; }
    };

    typedef std::shared_ptr<callback> callback_ptr;

    class New : public Event {
    public:
        New(const ray& r, unsigned x, unsigned y, unsigned sampleId) :
            Event("new"), r(r), x(x), y(y), sampleId(sampleId) {}

        static event_ptr make(const ray& r, unsigned x, unsigned y, unsigned sampleId) { 
            return std::make_shared<New>(r, x, y, sampleId); 
        }

        const ray r;
        const unsigned x, y, sampleId;
    };

    class Terminal : public Event {
    public:
        Terminal(std::string name) : Event(name + "_terminal") {}
    };

    class CandidateHit : public Event {
    public:
        CandidateHit(const hit_record& rec) : Event("candidate_hit"), rec(rec) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "candidate_hit(" << rec.obj_ptr->name << ")";
        }

        static event_ptr make(const hit_record& rec) { return std::make_shared<CandidateHit>(rec); }

        const hit_record rec;
    };

    class Bounce: public Event {
    public:
        Bounce(unsigned depth, const vec3& throughput) :
            Event("bounce"), depth(depth), throughput(throughput) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "bounce(" << depth << ")";
        }

        static event_ptr make(unsigned depth, const vec3& throughput) { 
            return std::make_shared<Bounce>(depth, throughput); 
        }

        const unsigned depth;
        const vec3 throughput;
    };

    class Hit : public Event {
    public:
        Hit(std::string name, const vec3& p) : Event(name + "_hit"), p(p) {}

        const vec3 p; // hit point
    };

    class MediumHit : public Hit {
    public:
        MediumHit(const vec3& p, double d, double rt) : Hit("medium", p), distance(d), rec_t(rt) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "medium_hit(dist = " << distance << ", rec.t = " << rec_t << ")";
        }

        double distance, rec_t;

        static event_ptr make(const vec3& p, double d, double rt) { 
            return std::make_shared<MediumHit>(p, d, rt); 
        }
    };

    class SurfaceHit : public Hit {
    public:
        SurfaceHit(const hit_record& rec) : Hit("surface", rec.p), rec(rec) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "surface_hit(" << rec.obj_ptr->name << ")";
        }
        const hit_record rec;

        static event_ptr make(const hit_record& rec) { return std::make_shared<SurfaceHit>(rec); }
    };

    class Scatter : public Event {
    public:
        Scatter(std::string name, const vec3 &d) : Event(name + "_scatter"), d(d) {}

        const vec3 d; // scatter direction
    };

    class SpecularScatter : public Scatter {
    public:
        SpecularScatter(const vec3& d, bool refracted) : Scatter("specular", d), is_refracted(refracted) {}

        const bool is_refracted;

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "specular_scatter(" << (is_refracted ? "refracted" : "reflected") << ")";
        }

        static event_ptr make(const vec3& d, bool refracted) { 
            return std::make_shared<SpecularScatter>(d, refracted); 
        }
    };

    class DiffuseScatter : public Scatter {
    public:
        DiffuseScatter(const vec3& d) : Scatter("diffuse", d) {}

        static event_ptr make(const vec3& d) { return std::make_shared<DiffuseScatter>(d); }
    };

    class MediumScatter : public Scatter {
    public:
        MediumScatter(const vec3& d) : Scatter("medium", d) {}

        static event_ptr make(const vec3& d) { return std::make_shared<MediumScatter>(d); }
    };

    class NoHitTerminal : public Terminal {
    public:
        NoHitTerminal() : Terminal("nohit") {}

        static event_ptr make() { return std::make_shared<NoHitTerminal>(); }
    };

    class AbsorbedTerminal : public Terminal {
    public:
        AbsorbedTerminal() : Terminal("absorbed") {}

        static event_ptr make() { return std::make_shared<AbsorbedTerminal>(); }
    };

    class RouletteTerminal : public Terminal {
    public:
        RouletteTerminal() :Terminal("roulette") {}

        static event_ptr make() { return std::make_shared<RouletteTerminal>(); }
    };

    class MaxDepthTerminal : public Terminal {
    public:
        MaxDepthTerminal() : Terminal("maxdepth") {}

        static event_ptr make() { return std::make_shared<MaxDepthTerminal>(); }
    };

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

    /*
    * Checks if any ray goes through the a particular surface
    * If it does then eventually it will intersect the same surface again from the inside (rec.front_face == false)
    */
    class dbg_find_gothrough_diffuse : public callback {
    private:
        const bool verbose;
        const bool stopAtFirstFound;
        const std::string target;

        bool found = false;
        std::shared_ptr<New> n;
        unsigned long count = 0;

    public:
        dbg_find_gothrough_diffuse(std::string t, bool verbose, bool stopAtFirst) 
            : target(t), verbose(verbose), stopAtFirstFound(stopAtFirst) {}

        virtual void operator ()(event_ptr e) override {
            if (auto ne = cast<New>(e)) {
                n = ne;
                found = false; // only report/count one per sample
            } else if (auto h = cast<SurfaceHit>(e)) {
                if (!found && h->rec.obj_ptr->name == target && !h->rec.front_face) {
                    ++count;
                    if (verbose) {
                        std::cerr << "\nFound a go through ray at (" << n->x << ", " << n->y << "):" << n->sampleId << std::endl;
                    }
                    found = true;
                }
            }
        }

        virtual bool terminate() const override {
            return stopAtFirstFound && found;
        }

        unsigned long getCount() const { return count; }
    };

    // TODO add callback that counts how many bounces have very low throughput
}