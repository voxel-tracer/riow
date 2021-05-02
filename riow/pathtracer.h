#pragma once
#include "rtweekend.h"
#include "tracer.h"
#include "color.h"


enum class render_state {
    SPECULAR, DIFFUSE, NOHIT, ABSORBED, MAXDEPTH
};

struct scene_desc {
    color background;
    shared_ptr<hittable> world;
    shared_ptr<hittable_list> lights;
};

typedef std::function<void(const ray&, const ray&, render_state)> render_callback;

inline yocto::vec3f toYocto(const vec3& v) {
    return { (float)v[0], (float)v[1], (float)v[2] };
}

class pathtracer: public tracer {
private:
    const shared_ptr<camera> cam;
    shared_ptr<yocto::color_image> image;
    const scene_desc scene;
    const unsigned samples_per_pixel;
    const unsigned max_depth;

    color ray_color(const ray& r, shared_ptr<rnd> rng, int depth,
        render_callback callback) {
        // If we've exceeded the ray bounce limit, no more lights gathered
        if (depth == 0) {
            callback(r, r, render_state::MAXDEPTH);
            return color{ 0, 0, 0 };
        }

        hit_record rec;
        if (!scene.world->hit(r, 0.001, infinity, rec, rng)) {
            callback(r, r, render_state::NOHIT);
            return scene.background;
        }

        scatter_record srec;
        color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
        if (!rec.mat_ptr->scatter(r, rec, srec, rng)) {
            callback(r, r, render_state::ABSORBED);
            return emitted;
        }

        if (srec.is_specular) {
            callback(r, srec.specular_ray, render_state::SPECULAR);
            return srec.attenuation * ray_color(srec.specular_ray, rng, depth - 1, callback);
        }

        ray scattered;
        double pdf_val;
        if (scene.lights->objects.empty()) {
            // sample material directly
            scattered = ray(rec.p, srec.pdf_ptr->generate(rng));
            pdf_val = srec.pdf_ptr->value(scattered.direction());
        }
        else {
            // multiple importance sampling of light and material pdf
            auto light_ptr = make_shared<hittable_pdf>(scene.lights, rec.p);
            mixture_pdf mixed_pdf(light_ptr, srec.pdf_ptr);

            scattered = ray(rec.p, mixed_pdf.generate(rng));
            pdf_val = mixed_pdf.value(scattered.direction());
        }

        callback(r, scattered, render_state::DIFFUSE);
        return emitted +
            srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
            * ray_color(scattered, rng, depth - 1, callback) / pdf_val;
    }

    color RenderPixel(unsigned i, unsigned j, render_callback callback) {
        color pixel_color{ 0, 0, 0 };

        auto local_seed = (xor_rnd::wang_hash(j * image->width + i) * 336343633) | 1;
        auto local_rng = make_shared<xor_rnd>(local_seed);

        for (auto s = 0; s != samples_per_pixel; ++s) {
            auto u = (i + local_rng->random_double()) / (image->width - 1);
            auto v = (j + local_rng->random_double()) / (image->height - 1);
            ray r = cam->get_ray(u, v, local_rng);
            pixel_color += ray_color(r, local_rng, max_depth, callback);
        }

        return pixel_color;
    }

public:
    pathtracer(shared_ptr<camera> c, shared_ptr<yocto::color_image> im, scene_desc sc, unsigned spp, unsigned md) : 
        cam(c), image(im), scene(sc), samples_per_pixel(spp), max_depth(md) {}

    virtual void Render() override {

        for (auto j = image->height - 1; j >= 0; --j) {
            std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
            for (auto i = 0; i != image->width; ++i) {
                color pixel_color = RenderPixel(i, j, 
                    [](const ray& r, const ray& s, render_state state) {});

                yocto::set_pixel(*image, i, (image->height - 1) - j, convert(pixel_color, samples_per_pixel));
            }
        }
    }

    virtual void DebugPixel(unsigned x, unsigned y, std::vector<yocto::vec3f> &paths) override {
        std::cerr << "DebugPixel(" << x << ", " << y << ")\n";

        // to render pixel (x, y)
        auto i = x;
        auto j = (image->height - 1) - y;

        RenderPixel(i, j,
            [&paths](const ray& r, const ray& s, render_state state) {
                switch (state) {
                case render_state::DIFFUSE:
                case render_state::SPECULAR:
                    paths.push_back(toYocto(r.origin()));
                    paths.push_back(toYocto(s.origin()));
                    break;
                case render_state::NOHIT:
                    paths.push_back(toYocto(r.origin()));
                    paths.push_back(toYocto(r.at(10))); // infinite ray
                    break;
                default:
                    // do nothing
                    break;
                }
            });
    }
};