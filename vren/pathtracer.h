#pragma once

#include "rtweekend.h"
#include "tracer.h"
#include "color.h"

#include "thread_pool.hpp"
#include "envmap.h"
#include "Film.h"

#include <atomic>


struct scene_desc {
    color background;
    hittable& world;
    EnvMap* envmap;

    pdf* getSceneLightPdf(const point3& origin) const {
        if (envmap) return envmap->pdf.get();
        else return nullptr;
    }
};

class pathtracer: public tracer {
private:
    camera& cam;
    const scene_desc scene;
    const unsigned max_depth;
    const unsigned rroulette_depth;

    std::vector<unsigned> seeds;
    thread_pool pool;

    Film& film;

    color ray_color(const ray& r, rnd& rng, callback::callback* cb) {
        const double epsilon = 0.001;

        color throughput = { 1, 1, 1 };
        color emitted = { 0, 0, 0 };
        ray curRay = r;

        Medium* medium = nullptr;
        hittable* medium_obj = nullptr;

        for (auto depth = 0; depth < max_depth; ++depth) {
            if (cb && cb->terminate()) break;
            if (cb) (*cb)(callback::Bounce::make(depth, throughput));

            hit_record rec;
            if (!scene.world.hit(curRay, epsilon, infinity, rec)) {
                if (scene.envmap) {
                    color e = scene.envmap->value(curRay.direction());
                    emitted += throughput * e;
                    if (cb && max(e) > 0.0)
                        (*cb)(callback::Emitted::make("env_light", e));
                }
                else {
                    emitted += throughput * scene.background;
                    if (cb && max(scene.background) > 0.0)
                        (*cb)(callback::Emitted::make("background", scene.background));
                }

                if (cb) (*cb)(callback::NoHitTerminal::make());

                return emitted;
            }

            if (cb) (*cb)(callback::CandidateHit::make(rec));

            bool hitSurface = true;
            if (medium && rec.obj_ptr == medium_obj && rec.front_face) {
                // once ray enters a medium it can't hit the front surface of the same medium
                // when this happens we assume the ray exited the medium in the previous bounce
                // we still allow other objects to be inside the medium but we don't yet support
                // overlapping mediums
                medium = nullptr;
                medium_obj = nullptr;
                if (cb) (*cb)(callback::MediumSkip::make("internal face"));
            }

            // take current medium into account
            if (medium) {
                // check if there is an internal scattering
                double distance;
                color transmission = medium->SampleDistance(rec.t, distance, rng);
                throughput *= transmission;

                if (cb) (*cb)(callback::Transmitted::make(distance, transmission));

                if ((distance + epsilon) < rec.t) {
                    vec3 sampled;
                    medium->SampleDirection(curRay.direction(), sampled, rng);
                    ray scattered = ray(curRay.at(distance), sampled);

                    // if the scattered ray is too close to the surface it is possible
                    // it will miss it, in that case ignore the medium scattering
                    hit_record tmp;
                    if (medium_obj->hit(scattered, epsilon, infinity, tmp)) {
                        // ray scattered inside the medium
                        hitSurface = false;
                        curRay = scattered;

                        if (cb) {
                            (*cb)(callback::MediumHit::make(scattered.origin(), distance, rec.t));
                            (*cb)(callback::MediumScatter::make(scattered.direction()));
                        }
                    }
                    else {
                        if (cb) (*cb)(callback::MediumSkip::make("scattered ray misses medium_obj"));
                    }
                }
                else {
                    // we are ignoring the medium scatter and treating this as a surface hit instead
                    if (cb) (*cb)(callback::MediumSkip::make("scatter beyond surface"));
                }
            }

            if (hitSurface) {
                if (!medium && !rec.front_face) {
                    // back hits only allowed inside mediums
                    // otherwise we assume it's a precision issue and we ignore the hit
                    if (cb) (*cb)(callback::HitSkip::make(rec.front_face));
                    curRay = { rec.p, curRay.direction() };
                    continue;
                }

                // only account for hit, when we actually hit the surface
                if (cb) (*cb)(callback::SurfaceHit::make(rec));

                scatter_record srec;
                color e = rec.mat_ptr->emitted(curRay, rec, rec.u, rec.v, rec.p);
                if (cb && max(e) > 0.0)
                    (*cb)(callback::Emitted::make(rec.obj_ptr->name, e));

                emitted += e * throughput;

                if (!rec.mat_ptr->scatter(curRay, rec, srec, rng)) {
                    if (cb) (*cb)(callback::AbsorbedTerminal::make());
                    return emitted;
                }

                if (medium && srec.is_specular && !srec.is_refracted) {
                    // even though reflected rays should remain inside the medium
                    // it is possible for the ray to miss the next intersection with the surface
                    // the sample color will still be computed correctly but this will cause our
                    // validation_callback to detect this as a bug
                    // to avoid that, we check that we can hit the medium_obj in a back face
                    // or change the scattered ray to refracted
                    hit_record trec;
                    if (!medium_obj->hit(srec.specular_ray, epsilon, infinity, trec) || trec.front_face) {
                        srec.is_refracted = true; // this will make the ray exit the medium
                        if (cb) (*cb)(callback::MediumSkip::make("swap reflected to refracted"));
                    }
                }

                // check if we entered or exited a medium
                // we assume that mediums can't overlap
                if (srec.is_refracted) {
                    if (medium) {
                        // we are exiting the medium
                        medium = nullptr;
                        medium_obj = nullptr;
                        //TODO report event
                    }
                    else {
                        // we are entering the medium
                        medium = srec.medium_ptr;
                        medium_obj = rec.obj_ptr;
                        //TODO if medium null report a WARN event
                    }
                }

                if (srec.is_specular) {
                    throughput *= srec.attenuation;
                    curRay = srec.specular_ray;

                    if (cb) (*cb)(callback::SpecularScatter::make(curRay.direction(), rec, srec));

                    continue;
                }

                pdf* mat_pdf = srec.pdf_ptr.get();
                bool using_mixture = false;
                {
                    pdf* light_pdf = scene.getSceneLightPdf(rec.p);
                    if (light_pdf) {
                        mat_pdf = new mixture_pdf(light_pdf, mat_pdf);
                        using_mixture = true;
                    }
                }

                ray scattered = ray(rec.p, mat_pdf->generate(rng));
                double pdf_val = mat_pdf->value(scattered.direction());
                if (cb)(*cb)(callback::PdfSample::make(mat_pdf->name(), pdf_val));

                if (using_mixture)
                    delete mat_pdf;

                double scattering_pdf = rec.mat_ptr->scattering_pdf(curRay, rec, scattered);
                // when sampling lights it is possible to generate scattered rays that go inside the surface
                // those will be absorbed by the surface
                if (scattering_pdf <= 0.0) {
                    if (cb) (*cb)(callback::AbsorbedTerminal::make());
                    return emitted;
                }

                throughput *= srec.attenuation * scattering_pdf / pdf_val;

                if (cb) (*cb)(callback::DiffuseScatter::make(scattered.direction(), rec));

                curRay = scattered;
            }

            // Russian roulette
            if (depth > rroulette_depth) {
                double m = max(throughput);
                if (rng.random_double() > m) {
                    if (cb) (*cb)(callback::RouletteTerminal::make());

                    return emitted;
                }
                throughput *= 1 / m;
            }
        }

        // if we reach this point, we've exceeded the ray bounce limit, no more lights gathered
        if (cb) (*cb)(callback::MaxDepthTerminal::make());
        return emitted;

    }

    unsigned pixelIdx(unsigned i, unsigned j) const {
        return i + j * film.width;
    }

    unsigned computeSeed(int i, int j) const {
        return (xor_rnd::wang_hash(j * film.width + i) * 336343633) | 1;
    }

    void initSeeds() {
        for (auto j = 0; j != film.height; ++j) {
            for (auto i = 0; i != film.width; ++i) {
                seeds[pixelIdx(i, j)] = computeSeed(i, j);
            }
        }
    }

    color RenderPixel(unsigned i, unsigned j, unsigned spp, callback::callback* cb, bool force_reset_seed = false) {
        color pixel_color{ 0, 0, 0 };

        unsigned seed = force_reset_seed ? computeSeed(i, j) : seeds[pixelIdx(i, j)];
        xor_rnd local_rng{ seed };

        for (auto s = 0; s != spp; ++s) {
            auto u = (i + local_rng.random_double()) / (film.width - 1);
            auto v = (j + local_rng.random_double()) / (film.height - 1);
            ray r = cam.get_ray(u, v, local_rng);

            if (cb) (*cb)(callback::New::make(r, i, (film.height - 1) - j, s));

            pixel_color += ray_color(r, local_rng, cb);
            if (cb && cb->terminate()) break;
        }

        if (!force_reset_seed)
            seeds[pixelIdx(i, j)] = local_rng.getState();

        return pixel_color;
    }

    void RenderLine(unsigned j, unsigned startX, unsigned endX, unsigned spp, callback::callback* cb) {
        for (auto i = startX; i < endX; ++i) {
            const unsigned idx = i + j * film.width;

            color clr = RenderPixel(i, j, spp, cb);
            if (cb) cb->alterPixelColor(clr);
            film.AddSample(i, (film.height - 1) - j, toYocto(clr), spp);

            if (cb && cb->terminate()) return;
        }
    }

public:
    pathtracer(camera& c, Film& film, scene_desc sc, unsigned md, unsigned rrd)
        : cam(c), film(film), scene(sc), max_depth(md), rroulette_depth(rrd), 
        seeds(film.width * film.height, 0) {

        initSeeds();

        yocto::print_info("thread pool size = " + std::to_string(pool.get_thread_count()));
    }

    virtual void Render(unsigned spp, bool parallel, callback::callback* cb) override {
        if (parallel) {
            std::atomic_int next_line(0);
            for (auto t = 0; t < pool.get_thread_count(); ++t) {
                pool.push_task([&] {
                    while (true) {
                        auto j = next_line.fetch_add(1);
                        if (j >= film.height) break;
                        RenderLine(j, 0, film.width, spp, cb);
                    }
                    });
            }

            pool.wait_for_tasks();
        }
        else {
            for (unsigned j = 0; j < film.height; j++) RenderLine(j, 0, film.width, spp, cb);
        }
    }

    virtual void DebugPixel(unsigned x, unsigned y, unsigned spp, callback::callback* cb) override {
        std::cerr << "\nDebugPixel(" << x << ", " << y << ")\n";

        // to render pixel (x, y)
        auto i = x;
        auto j = (film.height - 1) - y;
        RenderPixel(i, j, spp, cb, true);
    }

    virtual void updateCamera(
        double from_x, double from_y, double from_z,
        double at_x, double at_y, double at_z) override {
        cam.update({ from_x, from_y, from_z }, { at_x, at_y, at_z });
        Reset();
    }

    virtual void Reset() override {
        initSeeds();
        film.Clear();
    }
};