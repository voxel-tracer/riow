#pragma once

#include <vector>
#include <iostream>
#include <limits>

#include "tracer_callback.h"

namespace callback {
    class collect_hits : public callback {
    public:
        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (auto hit = cast<Hit>(event)) {
                hits.push_back(hit);
            }
        }

        std::vector<shared_ptr<Hit>> hits{};
    };

    class validate_model : public callback {
    private:
        const std::string target{};
        const int stopAtBug;
        const bool printBugs;

        std::shared_ptr<SurfaceHit> hit;
        std::shared_ptr<New> currentSample;
        std::shared_ptr<New> lastBuggySample;

        bool inside = false; // true if ray entered the target
        bool foundBadFront = false; // true if front face hit inside target
        bool foundBadBack  = false; // true if back face hit outside target
        bool targetHit = false; // true if current sample hit target
        bool mediumSkip = false; // true if current medium transmittance was skipped
                                 // this means a bad front hit was properly handled

        long numFoundBugs = 0; // total number of samples with bugs

    public:
        validate_model(std::string target, int stopAtBug = -1, bool printBugs = false) :
            target(target), stopAtBug(stopAtBug), printBugs(printBugs) {}

        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (auto n = cast<New>(event)) {
                currentSample   = n;
                inside          = false;
                foundBadBack    = false;
                foundBadFront   = false;
                targetHit       = false;
                mediumSkip      = false;
            }
            else if (auto h = cast<SurfaceHit>(event)) {
                if (h->rec.obj_ptr->name == target) {
                    targetHit = true;
                    hit = h;
                }
                else {
                    targetHit = false;
                    //TODO if inside, this is a bug
                }
            }
            else if (cast<MediumSkip>(event)) {
                mediumSkip = true;
            }
            else if (auto s = cast<SpecularScatter>(event)) {
                if (targetHit && s->srec.is_refracted) {
                    if (inside) {
                        if (hit->rec.front_face) {
                            if (mediumSkip) // we can ignore this case as it was properly handled
                                mediumSkip = false; // reset this flag as it only fixes one bad front hit
                            else
                                foundBadFront = true;
                        }
                        else
                            inside = false;
                    }
                    else {
                        if (hit->rec.front_face)
                            inside = true;
                        else
                            foundBadBack = true;
                    }
                }
            }
            else if (cast<Terminal>(event)) {
                if (foundBadBack || foundBadFront || inside) {
                    numFoundBugs++;
                    lastBuggySample = currentSample;
                    if (printBugs)
                        std::cerr << "bug at (" << currentSample->x << 
                                     ", " << currentSample->y << 
                                     ", " << currentSample->sampleId << ")\n";
                }
            }
        }

        virtual void alterPixelColor(vec3& clr) const override {
            if (foundBadBack)
                clr = { 1.0, 0.5, 0.5 };
            else if (foundBadFront)
                clr = { 0.5, 0.5, 1.0 };
            else if (inside)
                clr = { 1.0, 0.1, 0.1 };
            else
                clr = { 0.5, 1.0, 0.5 };
        }

        virtual std::ostream& digest(std::ostream& o) const override {
            o << "found " << numFoundBugs << " buggy samples";
            if (stopAtBug > 0 && numFoundBugs >= stopAtBug)
                o << ", at(" << lastBuggySample->x << ", " << lastBuggySample->y << ") at sample: " << lastBuggySample->sampleId;
            return o;
        }

        virtual bool terminate() const override { 
            return stopAtBug > 0 && numFoundBugs >= stopAtBug; 
        }

    };

    class multi : public callback {
    private:
        std::vector<shared_ptr<callback>> all{};

    public:
        void add(shared_ptr<callback> cb) { all.push_back(cb); }

        virtual void operator ()(std::shared_ptr<Event> event) override {
            for (auto cb : all) (*cb)(event);
        }

        virtual void alterPixelColor(vec3& clr) const override {
            for (auto cb : all) cb->alterPixelColor(clr);
        }
    };

    class normals_falsecolor : public callback {
    private:
        bool found = false; // true if we found first intersection normal
        vec3 dir; // current ray direction
        color falsecolor;

    public:
        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (auto n = cast<New>(event)) {
                found = false;
                dir = n->r.direction();
            }
            else if (auto hit = cast<CandidateHit>(event)) {
                if (!found) {
                    found = true;
                    // compute geometry normal (account for normals flipped when hit from back)
                    auto normal = hit->rec.normal;
                    if (!hit->rec.front_face)
                        normal = -normal;
                    // compute dot product. from back (-1) to front (+1)
                    auto t = dot(-normal, dir);
                    // normalize t from [0,1]
                    t = (t + 1) / 2;
                    // color ranges from red (back) to green (front)
                    falsecolor = (1 - t) * color(1.0, 0.0, 0.0) + t * color(0.0, 1.0, 0.0);
                }
            }
        }

        virtual void alterPixelColor(vec3& clr) const override {
            if (found)
                clr = falsecolor;
        }
    };

    class highlight_element : public callback {
    private:
        bool found = false;

    public:
        highlight_element(int element, color c = { 1.0, 0.0, 0.0 }) : element(element), c(c) {}

        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (cast<New>(event)) {
                found = false;
            }
            else if (auto hit = cast<CandidateHit>(event)) {
                if (hit->rec.element == element)
                    found = true;
            }
        }

        virtual void alterPixelColor(vec3& clr) const override {
            if (found)
                clr = c;
        }

        const int element;
        const color c;
    };

    // a buggy specular sample intersects a target model an odd number of times
    // this should only happen if the sample was terminated prematurly (max depth or russian roulette)
    // as such only account for samples that terminate with a NoHitTerminal
    class buggy_specular : public callback {
    private:
        const std::string target;
        const bool colorize;

        unsigned count = 0;
        bool foundBug = false; // true if bug found in current sample
        long numFoundBugs = 0; // total number of samples with bug
        bool hitTarget = false; // true if current bounce hit target

    public:
        buggy_specular(std::string target, bool colorize) : target(target), colorize(colorize) {}

        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (cast<New>(event)) {
                count = 0;
                foundBug = false;
            }
            else if (cast<Bounce>(event)) {
                hitTarget = false;
            }
            else if (cast<NoHitTerminal>(event)) {
                if (count % 2) {
                    foundBug = true;
                    numFoundBugs++;
                }
            }
            else if (auto h = cast<CandidateHit>(event)) {
                if (h->rec.obj_ptr->name == target) {
                    hitTarget = true;
                }
            }
            else if (auto ss = cast<SpecularScatter>(event)) {
                if (hitTarget) {
                    if (ss->srec.is_refracted) count++;
                }
            }
        }

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "found " << numFoundBugs << " self-intersected samples";
        }

        virtual void alterPixelColor(vec3& clr) const override {
            if (colorize) {
                if (foundBug)
                    clr = { 1.0, 0.0, 0.0 };
                else
                    clr *= 0.1;
            }
        }
    };

    class self_intersection : public callback {
    private:
        const std::string target;
        const double threshold;
        const bool colorize;

        unsigned numHits = 0; // how many times current sample hit any object
        bool hitTarget = false; // true if current sample hit target object before any other object
        bool foundBug = false; // true if current sample self intersected

        long numBugsFound = 0;

    public:
        self_intersection(std::string target, double threshold, bool colorize) : 
            target(target), threshold(threshold), colorize(colorize) {}

        virtual void operator ()(std::shared_ptr<Event> event) override {
            if (cast<New>(event)) {
                numHits = 0;
                hitTarget = false;
                foundBug = false;
            }
            else if (auto h = cast<CandidateHit>(event)) {
                if (numHits == 0) {
                    if (h->rec.obj_ptr->name == target)
                        hitTarget = true;
                }
                else if (numHits == 1 && hitTarget) {
                    if (h->rec.t < threshold)
                        foundBug = true;
                }

                numHits = true;
            }
            else if (cast<Terminal>(event)) {
                if (foundBug) numBugsFound++;
            }
        }

        virtual std::ostream& digest(std::ostream& o) const override {
            return o << "found " << numBugsFound << " self-intersected samples";
        }

        virtual void alterPixelColor(vec3& clr) const override {
            if (colorize) {
                if (foundBug)
                    clr = { 1.0, 0.0, 0.0 };
                else
                    clr *= 0.1;
            }
        }

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