#pragma once
#include "rtweekend.h"
#include "tracer.h"
#include "color.h"

#include <yocto/yocto_parallel.h>


struct scene_desc {
    color background;
    shared_ptr<hittable> world;
    shared_ptr<hittable_list> lights;
};

class pathtracer: public tracer {
private:
    const shared_ptr<camera> cam;
    shared_ptr<yocto::color_image> image;
    const scene_desc scene;
    const unsigned samples_per_pixel;
    const unsigned max_depth;
    const unsigned rroulette_depth;

    color ray_color(const ray& r, shared_ptr<rnd> rng, callback::callback_ptr cb) {
        const double epsilon = 0.001;

        color throughput = { 1, 1, 1 };
        color emitted = { 0, 0, 0 };
        shared_ptr<Medium> medium = {};
        shared_ptr<hittable> medium_obj = {};
        ray curRay = r;

        for (auto depth = 0; depth < max_depth; ++depth) {
            if (cb && cb->terminate()) break;
            if (cb) (*cb)(callback::Bounce::make(depth, throughput));

            hit_record rec;
            if (!scene.world->hit(curRay, epsilon, infinity, rec, rng)) {
                emitted += throughput * scene.background;

                if (cb) {
                    if (max(scene.background) > 0.0) 
                        (*cb)(callback::Emitted::make("background", scene.background));
                    (*cb)(callback::NoHitTerminal::make());
                }

                return emitted;
            }

            if (cb) (*cb)(callback::CandidateHit::make(rec));

            bool hitSurface = true;

            // take current medium into account
            if (medium) {
                // check if there is an internal scattering
                double distance = medium->sampleDistance(rng, rec.t);
                throughput *= medium->transmission(distance);

                // TODO introduce an event that describes medium transmission

                if ((distance + epsilon) < rec.t) {
                    ray scattered = ray(
                        curRay.at(distance),
                        medium->sampleScatterDirection(rng, curRay.direction())
                    );

                    // if the scattered ray is too close to the surface it is possible
                    // it will miss it, in that case ignore the medium scattering
                    hit_record tmp;
                    if (medium_obj->hit(scattered, epsilon, infinity, tmp, rng)) {
                        // ray scattered inside the medium
                        hitSurface = false;
                        curRay = scattered;

                        if (cb) {
                            (*cb)(callback::MediumHit::make(scattered.origin(), distance, rec.t));
                            (*cb)(callback::MediumScatter::make(scattered.direction()));
                        }
                    }
                }
            }

            if (hitSurface) {
                // only account for hit, when we actually hit the surface
                if (cb) (*cb)(callback::SurfaceHit::make(rec));

                scatter_record srec;
                color e = rec.mat_ptr->emitted(curRay, rec, rec.u, rec.v, rec.p);
                if (max(e) > 0.0)
                    (*cb)(callback::Emitted::make(rec.obj_ptr->name, e));

                emitted += e * throughput;

                if (!rec.mat_ptr->scatter(curRay, rec, srec, rng)) {
                    if (cb) (*cb)(callback::AbsorbedTerminal::make());

                    return emitted;
                }

                // check if we entered or exited a medium
                if (srec.is_refracted) {
                    // assumes mediums cannot overlap
                    if (medium) {
                        medium = nullptr;
                        medium_obj = nullptr;
                    }
                    else if (srec.medium_ptr) {
                        medium = srec.medium_ptr;
                        medium_obj = rec.obj_ptr;
                    }
                }

                if (srec.is_specular) {
                    throughput *= srec.attenuation;
                    curRay = srec.specular_ray;

                    if (cb) (*cb)(callback::SpecularScatter::make(curRay.direction(), srec.is_refracted));

                    continue;
                }

                ray scattered;
                double pdf_val;
                if (scene.lights->objects.empty()) {
                    // sample material directly
                    scattered = ray(rec.p, srec.pdf_ptr->generate(rng));
                    pdf_val = srec.pdf_ptr->value(scattered.direction());
                    if (cb)(*cb)(callback::PdfSample::make(srec.pdf_ptr->name(), pdf_val));
                }
                else {
                    // multiple importance sampling of light and material pdf
                    auto light_ptr = make_shared<hittable_pdf>(scene.lights, rec.p);
                    mixture_pdf mixed_pdf(light_ptr, srec.pdf_ptr);

                    scattered = ray(rec.p, mixed_pdf.generate(rng));
                    pdf_val = mixed_pdf.value(scattered.direction());
                    if (cb)(*cb)(callback::PdfSample::make(mixed_pdf.name(), pdf_val));
                }

                throughput *= srec.attenuation *
                    rec.mat_ptr->scattering_pdf(curRay, rec, scattered) / pdf_val;

                if (cb) (*cb)(callback::DiffuseScatter::make(scattered.direction()));

                curRay = scattered;
            }

            // Russian roulette
            if (depth > rroulette_depth) {
                double m = max(throughput);
                if (rng->random_double() > m) {
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

    color RenderPixel(unsigned i, unsigned j, callback::callback_ptr cb) {
        color pixel_color{ 0, 0, 0 };

        auto local_seed = (xor_rnd::wang_hash(j * image->width + i) * 336343633) | 1;
        auto local_rng = make_shared<xor_rnd>(local_seed);

        for (auto s = 0; s != samples_per_pixel; ++s) {
            auto u = (i + local_rng->random_double()) / (image->width - 1);
            auto v = (j + local_rng->random_double()) / (image->height - 1);
            ray r = cam->get_ray(u, v, local_rng);

            if (cb) (*cb)(callback::New::make(r, i, (image->height - 1) - j, s));

            pixel_color += ray_color(r, local_rng, cb);
            if (cb && cb->terminate()) break;
        }

        return pixel_color;
    }

public:
    pathtracer(shared_ptr<camera> c, shared_ptr<yocto::color_image> im, scene_desc sc, unsigned spp, unsigned md, unsigned rrd) : 
        cam(c), image(im), scene(sc), samples_per_pixel(spp), max_depth(md), rroulette_depth(rrd) {}

    virtual void Render(callback::callback_ptr cb) override {
        for (auto j = image->height - 1; j >= 0; --j) {
            std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
            for (auto i = 0; i != image->width; ++i) {
                color pixel_color = RenderPixel(i, j, cb);
                if (cb && cb->terminate()) return;

                yocto::set_pixel(*image, i, (image->height - 1) - j, convert(pixel_color, samples_per_pixel));
            }
        }

        std::cerr << std::endl;
    }

    virtual void DebugPixel(unsigned x, unsigned y, callback::callback_ptr cb) override {
        std::cerr << "DebugPixel(" << x << ", " << y << ")\n";

        // to render pixel (x, y)
        auto i = x;
        auto j = (image->height - 1) - y;
        RenderPixel(i, j, cb);
    }
};