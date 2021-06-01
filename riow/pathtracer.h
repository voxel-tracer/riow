#pragma once
#include "rtweekend.h"
#include "tracer.h"
#include "color.h"

#include <yocto/yocto_parallel.h>


enum class render_state {
    SPECULAR, DIFFUSE, NOHIT, ABSORBED, MAXDEPTH
};

struct scene_desc {
    color background;
    shared_ptr<hittable> world;
    shared_ptr<hittable_list> lights;
};

inline yocto::vec3f toYocto(const vec3& v) {
    return { (float)v[0], (float)v[1], (float)v[2] };
}

struct callback_data {
    ray r;
    vec3 p;
    color albedo;
    render_state state;

    callback_data(render_state s, const color& a = { 0.0f, 0.0f, 0.0f }): 
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

class build_segments_callback : public render_callback {
public:

    virtual void operator ()(const callback_data& data) {
        switch (data.state) {
        case render_state::DIFFUSE:
        case render_state::SPECULAR:
        case render_state::ABSORBED:
        case render_state::NOHIT:
            segments.push_back(data.toSegment());
        default:
            // do nothing
            break;
        }
    }

    vector<tool::path_segment> segments;
};

class pathtracer: public tracer {
private:
    const shared_ptr<camera> cam;
    shared_ptr<yocto::color_image> image;
    const scene_desc scene;
    const unsigned samples_per_pixel;
    const unsigned max_depth;

    color ray_color(const ray& r, shared_ptr<rnd> rng, render_callback &callback) {
        color throughput = { 1, 1, 1 };
        color emitted = { 0, 0, 0 };
        ray curRay = r;

        for (auto depth = 0; depth < max_depth; ++depth) {
            hit_record rec;
            if (!scene.world->hit(curRay, 0.001, infinity, rec, rng)) {
                throughput = throughput * scene.background;
                callback({ curRay, {}, throughput, render_state::NOHIT });
                return throughput;
            }

            scatter_record srec;
            color e = rec.mat_ptr->emitted(curRay, rec, rec.u, rec.v, rec.p) * throughput;
            emitted += e;
            if (!rec.mat_ptr->scatter(curRay, rec, srec, rng)) {
                callback({ curRay, rec.p, e, render_state::ABSORBED });
                return emitted;
            }

            if (srec.is_specular) {
                throughput = throughput * srec.attenuation;
                callback({ curRay, rec.p, throughput, render_state::SPECULAR });
                curRay = srec.specular_ray;
                continue;
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

            throughput = throughput * srec.attenuation *
                rec.mat_ptr->scattering_pdf(curRay, rec, scattered) / pdf_val;
            callback({ curRay, rec.p, throughput, render_state::DIFFUSE });
            curRay = scattered;
        }

        // if we reach this point, we've exceeded the ray bounce limit, no more lights gathered
        callback({ render_state::MAXDEPTH });
        return color{ 0, 0, 0 };

    }

    color RenderPixel(unsigned i, unsigned j, render_callback &callback) {
        color pixel_color{ 0, 0, 0 };

        auto local_seed = (xor_rnd::wang_hash(j * image->width + i) * 336343633) | 1;
        auto local_rng = make_shared<xor_rnd>(local_seed);

        for (auto s = 0; s != samples_per_pixel; ++s) {
            auto u = (i + local_rng->random_double()) / (image->width - 1);
            auto v = (j + local_rng->random_double()) / (image->height - 1);
            ray r = cam->get_ray(u, v, local_rng);
            pixel_color += ray_color(r, local_rng, callback);
        }

        return pixel_color;
    }

public:
    pathtracer(shared_ptr<camera> c, shared_ptr<yocto::color_image> im, scene_desc sc, unsigned spp, unsigned md) : 
        cam(c), image(im), scene(sc), samples_per_pixel(spp), max_depth(md) {}

    virtual void Render() override {

        render_callback no_op;
        for (auto j = image->height - 1; j >= 0; --j) {
            std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
            for (auto i = 0; i != image->width; ++i) {
                color pixel_color = RenderPixel(i, j, no_op);

                yocto::set_pixel(*image, i, (image->height - 1) - j, convert(pixel_color, samples_per_pixel));
            }
        }

        //yocto::parallel_for(image->width, image->height, [this](unsigned i, unsigned j) {
        //    color pixel_color = RenderPixel(i, j, 
        //        [](const ray& r, const ray& s, render_state state) {});

        //    yocto::set_pixel(*image, i, (image->height - 1) - j, convert(pixel_color, samples_per_pixel));
        //});
    }

    virtual void DebugPixel(unsigned x, unsigned y, std::vector<tool::path_segment> &segments) override {
        std::cerr << "DebugPixel(" << x << ", " << y << ")\n";

        // to render pixel (x, y)
        auto i = x;
        auto j = (image->height - 1) - y;

        build_segments_callback bsc;
        RenderPixel(i, j, bsc);
        segments = bsc.segments;
    }
};