#pragma once

#include <vector>
#include <iostream>
#include <limits>

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
                segments.push_back({ toYocto(p), toYocto(p + d), toYocto(c) });
            }
        }

        std::vector<tool::path_segment> segments;
    };

    /*
    * draws all paths that scatter inside the medium
    * only draws the first time the rays enters a medium, once it exits it won't draw it 
    * again if it re-enters
    */
    class draw_medium_scatters : public callback {
    private:
        bool found = false; // current sample is or was in a medium
        bool inside = false; // current sample is in a medium
        vec3 p;
        color c{ 1.0 };

        void add(vec3 n) {
            segments.push_back({ toYocto(p), toYocto(n), toYocto(c) });
            p = n;
        }
    public:
        virtual void operator ()(std::shared_ptr<Event> e) override {
            if (auto n = cast<New>(e)) {
                found = inside = false;
                c = { 1.0 };
            }
            if (auto t = cast<Transmitted>(e)) {
                if (inside) {
                    c *= max(t->transmission);
                }
            }
            else if (auto s = cast<SpecularScatter>(e)) {
                if (s->is_refracted) { // ray traversed the surface
                    // only track the first entry/exit
                    if (!found) {
                        found = inside = true;
                    }
                    else if (inside)
                        inside = false;
                }
            }
            else if (auto h = cast<SurfaceHit>(e)) {
                if (inside) add(h->p);
                else p = h->p;
            }
            else if (auto m = cast<MediumHit>(e)) {
                if (inside) add(m->p);
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
        count_max_depth() : callback(true) {}

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

    // compute histogram for values in the range [0, maxVal]
    class transmitted_dist_histo : public callback {
    private:
        double dist = 0.0; // current sample transmitted distance
        bool found = false; // did current sample traverse medium ?
        unsigned total = 0;

        const double maxVal;
        const int numBins;
        std::vector<unsigned> bins;

        void add(double d) {
            int binIdx = (d / maxVal) * numBins;
            if (binIdx >= numBins) binIdx = numBins - 1;
            bins[binIdx]++;

            total++;
        }

    public:
        transmitted_dist_histo(int numBins, double maxVal) : 
            numBins(numBins), maxVal(maxVal), bins(numBins, 0) {}

        virtual void operator ()(event_ptr e) override {
            if (cast<New>(e)) {
                dist = 0.0;
                found = false;
            }
            else if (auto t = cast<Transmitted>(e)) {
                found = true;
                dist += t->distance;
            }
            else if (cast<Terminal>(e)) {
                if (found) {
                    add(dist);
                }
            }
        }

        std::vector<float> getNormalizedBins() const {
            std::vector<float> out;
            out.reserve(numBins);

            //std::cerr << "histo: total = " << total << std::endl;
            //float t = 0.0f;
            for (auto b : bins) {
                float f = (float)b / total;
                out.push_back(f);
                //std::cerr << b << ", " << f << std::endl;
                //t += f;
            }
            //std::cerr << "total = " << t << std::endl;
            return out;
        }
    };

    class average_transmitted_dist : public callback {
    private:
        double dist = 0.0; // current sample transmitted distance
        bool found = false; // did current sample traverse medium ?

        double min_dist = std::numeric_limits<double>::max();
        double max_dist = 0.0;

        double total_dist = 0.0;
        unsigned long count;

        void add(double d) {
            if (d < min_dist) min_dist = d;
            if (d > max_dist) max_dist = d;
            total_dist += d;
            count++;
        }

    public:
        virtual void operator ()(event_ptr e) override {
            if (cast<New>(e)) {
                dist = 0.0;
                found = false;
            }
            else if (auto t = cast<Transmitted>(e)) {
                found = true;
                dist += t->distance;
            }
            else if (cast<Terminal>(e)) {
                if (found) {
                    add(dist);
                }
            }
        }

        std::ostream& digest(std::ostream& o) const {
            return o << "average transmitted dist = " << (total_dist / count) <<
                "\nfor a total of " << count << " samples" <<
                "\n[" << min_dist << ", " << max_dist << "]";
        }
    };

    class num_medium_scatter_stats : public callback {
    private:
        bool inMedium = false; // did current sample enter a medium ?
        long scatterCount = 0; // how many times did the sample scatter inside a medium ?

        long minScatterCount = std::numeric_limits<long>::max();
        long maxScatterCount = 0;
        long totalScatterCount = 0;
        long numSamples = 0; // how many samples entered a medium ?

        void add(long count) {
            totalScatterCount += count;
            if (count < minScatterCount) minScatterCount = count;
            if (count > maxScatterCount) maxScatterCount = count;
        }

    public:
        virtual void operator ()(event_ptr e) override {
            if (cast<New>(e)) {
                inMedium = false;
                scatterCount = 0;
            }
            else if (auto t = cast<Transmitted>(e)) {
                if (!inMedium) {
                    // just entered the medium
                    inMedium = true;
                    numSamples++;
                }
            }
            else if (cast<MediumScatter>(e)) {
                scatterCount++;
            }
            else if (cast<Terminal>(e)) {
                if (inMedium) add(scatterCount);
            }
        }

        std::ostream& digest(std::ostream& o) const {
            return o << "average scatter count = " << (totalScatterCount / numSamples) <<
                "\nfor a total of " << numSamples << " samples" <<
                "\n[" << minScatterCount << ", " << maxScatterCount << "]";
        }
    };

    class num_medium_scatter_histo : public callback {
    private:
        bool inMedium = false; // did current sample enter a medium ?
        long scatterCount = 0; // how many times did the sample scatter inside a medium ?

        long numSamples = 0; // how many samples entered a medium ?

        const long maxVal;
        const int numBins;
        std::vector<unsigned> bins;

        void add(long count) {
            int binIdx = ((double)count / maxVal) * numBins;
            if (binIdx >= numBins) binIdx = numBins - 1;
            bins[binIdx]++;

            numSamples++;
        }

    public:
        num_medium_scatter_histo(int numBins, long maxVal) :
            numBins(numBins), maxVal(maxVal), bins(numBins, 0) {}

        virtual void operator ()(event_ptr e) override {
            if (cast<New>(e)) {
                inMedium = false;
                scatterCount = 0;
            }
            else if (auto t = cast<Transmitted>(e)) {
                if (!inMedium) {
                    // just entered the medium
                    inMedium = true;
                    numSamples++;
                }
            }
            else if (cast<MediumScatter>(e)) {
                scatterCount++;
            }
            else if (cast<Terminal>(e)) {
                if (inMedium) add(scatterCount);
            }
        }

        std::vector<float> getNormalizedBins() const {
            std::vector<float> out;
            out.reserve(numBins);

            for (auto b : bins) {
                float f = (float)b / numSamples;
                out.push_back(f);
            }
            return out;
        }
    };
}