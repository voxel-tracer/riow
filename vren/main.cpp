#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "plane.h"
#include "box.h"
#include "camera.h"
#include "material.h"
#include "pdf.h"
#include "callbacks.h"
#include "model.h"

#include <iostream>
#include <functional>
#include <time.h>
#include <thread>

#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_cli.h>

#include "pathtracer.h"

using namespace std;

struct app_params {
    int samples = 128;
    int bounces = 500;
    int resolution = 500;
    string output = "out";
    string reference = "";
    bool infinite = false;
    bool embree = false;
    bool save_reference = false;
    bool sss = false;
};

void parse_cli(app_params& params, int argc, const char** argv) {
    auto cli = yocto::make_cli("vren", "High Quality Renderer");
    yocto::add_option(cli, 
        "samples", params.samples, "Number of Samples.", { 1, numeric_limits<int>::max() });
    yocto::add_option(cli, 
        "bounces", params.bounces, "Number of Bounces.", { 1, numeric_limits<int>::max() });
    yocto::add_option(cli, "resolution", params.resolution, "Image Resolution.", { 1, 4096 });
    yocto::add_option(cli, "output", params.output, "Output Filename.");
    yocto::add_option(cli, "reference", params.reference, "Reference image filename.");
    yocto::add_option(cli, "infinite", params.infinite, "Render forever.");
    yocto::add_option(cli, "embree", params.embree, "Use Embree.");
    yocto::add_option(cli, "save_ref", params.save_reference, "Save Reference as reference.raw");
    yocto::add_option(cli, "sss", params.sss, "Use Subsufrace Scatteting material.");
    yocto::parse_cli(cli, argc, argv);
}

void dragon_scene(hittable_list& objects, bool embree, bool scattering_medium = false) {
    auto material_ground = make_shared<lambertian>(color(0.6));
    objects.add(make_shared<plane>("floor", point3(0.0, 0.1, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.25, 0.025);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);

    auto frame =
        yocto::translation_frame({ -0.5f, 0.5f, 0.0f }) *
        yocto::rotation_frame({ 0.0f, 1.0f, 0.0f }, yocto::radians(-45.0f)) *
        yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, yocto::radians(-37.5f)) *
        yocto::rotation_frame({ 0.0f, 0.0f, 1.0f }, yocto::radians(90.0f)) *
        yocto::rotation_frame(toYocto(unit_vector({ 1.0f, 0.0f, -1.0f })), yocto::radians(-2.0f)) *
        yocto::scaling_frame({ 1 / 100.0f, 1 / 100.0f, 1 / 100.0f });
    auto dragon = make_shared<model>("models/dragon_remeshed.ply", tinted_glass, frame, embree);
    objects.add(dragon);
}

void save_image(const yocto::color_image& image, string filename) {
    auto error = string{};
    if (!save_image(filename, image, error))
        yocto::print_fatal("Failed to save image: " + error);
}

void single_pass(shared_ptr<tracer> pt, const app_params& params, int pass) {
    yocto::print_progress_begin("Rendering Pass " + to_string(pass));
    pt->RenderParallel(params.samples);
    yocto::print_progress_end();

}

void parallel_render(tracer& pt, const app_params& params, int pass) {
    yocto::print_progress_begin("Rendering Pass " + to_string(pass), params.samples);
    for (auto i : yocto::range(params.samples)) {
        pt.RenderParallel(1);
        yocto::print_progress_next();
    }
}

int main(int argc, const char* argv[]) {
#ifndef NDEBUG
    yocto::print_info("WARNING! Running in DEBUG mode");
#endif // !NDEBUG


    // command line paramers
    app_params params{};
    parse_cli(params, argc, argv);
    
    yocto::print_info("hardware_concurrency = " + to_string(thread::hardware_concurrency()));

    // Image

    const auto aspect_ratio = 1.0 / 1.0;
    const int image_width = params.resolution;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const unsigned max_depth = params.bounces;

    // World

    hittable_list world;

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background{ 0, 0, 0 };
    bool use_envmap = true;
    bool add_light = false;
    bool russian_roulette = true;

    dragon_scene(world, params.embree, params.sss);
    lookfrom = { 2.27155, 7.99803, 0.244723 };
    lookat = point3(-0.5, 0, -0.5);
    vfov = 20.0;
    aperture = 0.0;
    background = color(0.6, 0.6, 0.7);

    // Camera
    vec3 vup{ 0, 1, 0 };

    camera cam{ lookfrom, lookat, vup, vfov, aspect_ratio, aperture };
    auto image = yocto::make_image(image_width, image_height, false);

    unique_ptr<EnvMap> envmap = nullptr;
    if (use_envmap) {
        auto hdr_filename = "hdrs/large_corridor_4k.exr";
        envmap = make_unique<EnvMap>(hdr_filename);
    }

    // Render
    scene_desc scene{
        background,
        world,
        envmap.get()
    };
    unsigned rr_depth = russian_roulette ? 3 : max_depth;
    pathtracer pt{ cam, image, scene, max_depth, rr_depth };
    if (!russian_roulette)
        yocto::print_info("WARNING! Russian Roulette is disabled");

    if (params.infinite) {
        int pass = 0;
        while (true) {
            ++pass;
            parallel_render(pt, params, pass);
            save_image(image, params.output + to_string(pass) + ".png");
        }
    }
    else {
        parallel_render(pt, params, 1);
        //single_pass(pt, params, 1);
        save_image(image, params.output + ".png");
    }

    if (params.save_reference) {
        RawData raw(image.width, image.height);
        pt.getRawData(raw);
        raw.saveToFile("reference.raw");
    }

    if (!params.reference.empty()) {
        yocto::print_info("comparing with reference image");
        RawData raw(image.width, image.height);
        pt.getRawData(raw);

        auto ref = make_shared<RawData>(params.reference);
        yocto::print_info("RMSE = " + std::to_string(raw.rmse(ref)));
    }
}
