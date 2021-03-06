#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <atomic>

#include "rtweekend.h"
#include "hit_record.h"
#include "hittable.h"
#include "material.h"

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
        callback(bool threadsafe = false): threadsafe(threadsafe) {}

        virtual void operator ()(event_ptr event) {}
        virtual bool terminate() const { return false; }
        virtual void alterPixelColor(vec3 &clr) const {}

        virtual std::ostream& digest(std::ostream& o) const {
            return o << "";
        }

        const bool threadsafe;
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
            o << "candidate_hit(" << rec.obj_ptr->name;
            if (rec.element != -1)
                o << ", element = " << rec.element;
            return o << ", t = " << rec.t << ", normal = " << rec.normal << 
                ", front_face = " << rec.front_face << 
                ", uv = (" << rec.u << ", " << rec.v << "))";
        }

        static event_ptr make(const hit_record& rec) { return std::make_shared<CandidateHit>(rec); }

        const hit_record rec;
    };

    class Skip : public Event {
    public:
        Skip(std::string name) : Event(name + "_skip") {}
    };

    class HitSkip : public Skip {
    public:
        HitSkip(bool front_face) : Skip("hit"), front_face(front_face) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "hit_skip(front_face = " << front_face << ")";
        }

        static event_ptr make(bool front_face) { return std::make_shared<HitSkip>(front_face); }

        const bool front_face;
    };

    class MediumSkip : public Skip {
    private:
        const std::string reason;
    public:
        MediumSkip(std::string reason) : Skip("medium"), reason(reason) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "medium_skip(reason = " << reason << ")";
        }

        static event_ptr make(std::string reason) { return std::make_shared<MediumSkip>(reason); }
    };

    class Bounce: public Event {
    public:
        Bounce(unsigned depth, const vec3& throughput) :
            Event("bounce"), depth(depth), throughput(throughput) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "bounce(depth = " << depth << ", throughput = " << throughput << ")";
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

    class Transmitted : public Event {
    public:
        Transmitted(double distance, const color& transmission) :
            Event("transmitted"), distance(distance), transmission(transmission) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "transmitted(dist = " << distance << ", transmission = " << transmission << ")";
        }

        static event_ptr make(double distance, const color& transmission) {
            return make_shared<Transmitted>(distance, transmission);
        }

        const double distance;
        const color transmission;
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
        SpecularScatter(const vec3& d, hit_record rec, const scatter_record& srec) : 
            Scatter("specular", d), rec(rec), srec(srec) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "specular_scatter(" <<
                (srec.is_refracted ? "refracted" : "reflected") <<
                ", dot(scatter.dir, norm) = " << dot(d, rec.normal) << ")";
        }

        static event_ptr make(const vec3& d, hit_record rec, const scatter_record& srec) { 
            return std::make_shared<SpecularScatter>(d, rec, srec); 
        }

        const scatter_record srec;
        const hit_record rec;
    };

    class DiffuseScatter : public Scatter {
    public:
        DiffuseScatter(const vec3& d, const hit_record& rec) : Scatter("diffuse", d), rec(rec) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "diffuse_scatter(d= " << d << ", dot(d,n)= " << (dot(d, rec.normal)) << ")";
        }

        static event_ptr make(const vec3& d, const hit_record& rec) { 
            return std::make_shared<DiffuseScatter>(d, rec); 
        }

        const hit_record rec;
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

    class PdfSample : public Event {
    public:
        PdfSample(std::string pdf_name, double pdf_val) : 
            Event("pdf_sample"), pdf_name(pdf_name), pdf_val(pdf_val) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "pdf_sample(name = " << pdf_name << ", val = " << pdf_val << ")\n";
        }

        static event_ptr make(std::string pdf_name, double pdf_val) { 
            return std::make_shared<PdfSample>(pdf_name, pdf_val); 
        }

        const std::string pdf_name;
        const double pdf_val;
    };

    class Emitted : public Event {
    public:
        Emitted(std::string emitter, const vec3& emitted) : 
            Event("emitted"), emitter(emitter), emitted(emitted) {}

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "emitted[emitter= " << emitter << ", value = " << emitted << "]\n";
        }

        static event_ptr make(std::string emitter, const vec3& emitted) { return std::make_shared<Emitted>(emitter, emitted); }

        const std::string emitter;
        const vec3 emitted;
    };
}
