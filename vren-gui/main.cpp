// riow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

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
#include "measured_mediums.h"

#include <iostream>
#include <functional>
#include <time.h>
#include <thread>

#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>

#include "pathtracer.h"
#include "tool/tool.h"

using namespace std;

void moriKnob(hittable_list& world) {
    auto obj = yocto::obj_model{};
    auto error = string{};
    if (!yocto::load_obj("models/mori-knob.obj", obj, error))
        yocto::print_fatal("failed to load model: " + error);
    for (auto shape : obj.shapes) {
        if (shape.name == "BasePlane_basePlane") {
            auto mat = make_shared<lambertian>(color(0.8));

            world.add(make_shared<model>(shape, mat));
        }
        else if (shape.name == "InnerSphere_innerSphere") {
            auto mat = make_shared<lambertian>(color(0.2));

            world.add(make_shared<model>(shape, mat));
        }
        else if (shape.name == "OuterSphere_outerSphere" || shape.name == "ObjBase_objBase") {
            // glass with ketchup
            vec3 sigma_s, sigma_a;
            GetMediumScatteringProperties("Ketchup", sigma_a, sigma_s);
            auto scale = 400.0f;
            auto ketchup = make_shared<HomogeneousMedium>(sigma_a * scale, sigma_s * scale);
            auto mat = make_shared<dielectric>(1.35, ketchup);

            world.add(make_shared<model>(shape, mat));
        }
        else {
            yocto::print_info("Ignoring shape: " + shape.name);
        }
    }
}

void simple_box(hittable_list& objects, shared_ptr<hittable> light, bool add_light) {

    // auto material_ground = make_shared<lambertian>(color(0.4));
    shared_ptr<texture> checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects.add(make_shared<plane>("floor", point3(0.0, -0.101, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    shared_ptr<material> mat;

    int type = 3;
    switch (type) {
        case 0: {
            // red diffuse
            mat = make_shared<lambertian>(color(0.5, 0.1, 0.1));
            break;
        }
        case 1: {
            // dark grey diffuse
            mat = make_shared<lambertian>(color(0.1));
            break;
        }
        case 2: {
            // glass with ketchup
            vec3 sigma_s, sigma_a;
            GetMediumScatteringProperties("Ketchup", sigma_a, sigma_s);
            auto scale = 100.0f;
            auto ketchup = make_shared<HomogeneousMedium>(sigma_a * scale, sigma_s * scale);
            mat = make_shared<dielectric>(1.35, ketchup);
            break;
        }
        case 3: {
            // passthrough with ketchup
            vec3 sigma_s, sigma_a;
            GetMediumScatteringProperties("Ketchup", sigma_a, sigma_s);
            auto scale = 100.0f;
            auto ketchup = make_shared<HomogeneousMedium>(sigma_a * scale, sigma_s * scale);
            mat = make_shared<passthrough>(ketchup);
            break;
        }
    }

    objects.add(make_shared<box>("base", point3(), vec3(1.0, 0.1, 1.0), mat));

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 15, 1.0), 0.5, material_light);
        objects.add(light);
    }

}

void glass_panels(hittable_list& objects, bool scattering_medium = false) {
    // white diffuse floor
    auto material_ground = make_shared<lambertian>(color(0.8));
    //shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(1.0, 1.0, 1.0));
    //auto material_ground = make_shared<lambertian>(checker);
    objects.add(make_shared<plane>("floor", point3(0.0, -0.005, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium) {
        vec3 sigma_s, sigma_a;
        GetMediumScatteringProperties("Ketchup", sigma_a, sigma_s);
        auto scale = 1.0f;
        medium = make_shared<HomogeneousMedium>(sigma_a * scale, sigma_s * scale);
    }
    else 
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);

    for (auto i : yocto::range(5)) {
        objects.add(make_shared<box>("panel1", point3(0.0, 0.5, 1.0 - i*0.5), vec3(1.0, 1.0, 0.1), tinted_glass));
    }
}

void dragon_scene(hittable_list& objects, bool scattering_medium = false) {
    auto material_ground = make_shared<lambertian>(color(0.6));
    //shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1), color(0.8));
    //auto material_ground = make_shared<lambertian>(checker);
    objects.add(make_shared<plane>("floor", point3(0.0, 0.1, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium) {
        vec3 sigma_s, sigma_a;
        GetMediumScatteringProperties("Ketchup", sigma_a, sigma_s);
        auto scale = 1.0f;
        medium = make_shared<HomogeneousMedium>(sigma_a * scale, sigma_s * scale);
    }
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);

    //auto m = make_shared<metal>(color(0.97, 0.96, 0.91));
    //auto m = make_shared<lambertian>(color(glass_color));

    auto frame = 
        yocto::translation_frame({ -0.5f, 0.5f, 0.0f }) *
        yocto::rotation_frame({ 0.0f, 1.0f, 0.0f }, yocto::radians(-45.0f)) *
        yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, yocto::radians(-37.5f)) *
        yocto::rotation_frame({ 0.0f, 0.0f, 1.0f }, yocto::radians(90.0f)) *
        yocto::rotation_frame(toYocto(unit_vector({ 1.0f, 0.0f, -1.0f })), yocto::radians(-2.0f)) *
        yocto::scaling_frame({ 1 / 100.0f, 1 / 100.0f, 1 / 100.0f });
    auto dragon = make_shared<model>("models/dragon_remeshed.ply", tinted_glass, frame);
    //auto dragon = make_shared<BVHModel>("models/dragon_remeshed.ply", tinted_glass, frame);
    objects.add(dragon);
}

void monk_scene(hittable_list& objects, bool scattering_medium = false) {
    //auto material_ground = make_shared<lambertian>(color(0.75));
    shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1), color(0.8));
    auto material_ground = make_shared<lambertian>(checker);
    objects.add(make_shared<plane>("floor", point3(0.0, -0.505, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium) {
        vec3 sigma_s, sigma_a;
        GetMediumScatteringProperties("Ketchup", sigma_a, sigma_s);
        auto scale = 1.0f;
        medium = make_shared<HomogeneousMedium>(sigma_a * scale, sigma_s * scale);
    }
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto monk_mat = make_shared<dielectric>(1.5, medium);
    //auto monk_mat = make_shared<diffuse_subsurface_scattering>(glass_color, medium);
    //auto monk_mat = make_shared<lambertian>(glass_color);

    auto frame = 
        yocto::translation_frame(toYocto(point3(0.0, -0.5, -0.5))) *
        yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, -yocto::pif / 2) *
        yocto::scaling_frame({ 0.01f, 0.01f, 0.01f });    
    auto monk = make_shared<model>("models/LuYu-obj/LuYu-obj.obj", monk_mat, frame);
    objects.add(monk);
}

shared_ptr<yocto::scene_model> init_scene(const hittable_list& world) {
    auto scene = make_shared<yocto::scene_model>();
    {
        // 0: sphere shape
        {
            auto shape = yocto::scene_shape{};
            // make shape with 8 steps in resolution and scale of 1
            auto quads = yocto::vector<yocto::vec4i>{};
            yocto::make_sphere(quads, shape.positions, shape.normals, shape.texcoords, 8, 1, 1);
            shape.triangles = quads_to_triangles(quads);
            scene->shapes.push_back(shape);
        }
        // 1: box shape
        {
            auto shape = yocto::scene_shape{};
            auto quads = yocto::vector<yocto::vec4i>{};
            yocto::make_cube(quads, shape.positions, shape.normals, shape.texcoords, 1);
            shape.triangles = quads_to_triangles(quads);
            scene->shapes.push_back(shape);
        }
        // 2: XZ plane shape
        {
            auto shape = yocto::scene_shape{};
            auto quads = yocto::vector<yocto::vec4i>{};
            yocto::make_floor(quads, shape.positions, shape.normals, shape.texcoords, { 32, 32 }, { 4, 4 }, { 1, 1 });
            shape.triangles = quads_to_triangles(quads);
            scene->shapes.push_back(shape);
        }
        scene->materials.push_back({}); // create a black material directly

        for (auto o : world.objects) {
            if (auto s = dynamic_pointer_cast<sphere>(o)) {
                if (s->radius > 99.0) continue; // ignore floor sphere

                auto instance = yocto::scene_instance{};
                instance.shape = 0;
                instance.material = 0;
                instance.frame =
                    yocto::translation_frame({ (float)s->center[0], (float)s->center[1], (float)s->center[2] }) *
                    yocto::scaling_frame({ (float)s->radius, (float)s->radius, (float)s->radius });
                scene->instances.push_back(instance);
            }
            else if (auto b = dynamic_pointer_cast<box>(o)) {
                auto bcenter = (b->bmax + b->bmin) / 2;
                auto bsize = (b->bmax - b->bmin) / 2;

                auto instance = yocto::scene_instance{};
                instance.shape = 1; // box shape
                instance.material = 0;
                instance.frame =
                    yocto::translation_frame({ (float)bcenter[0], (float)bcenter[1], (float)bcenter[2] }) *
                    yocto::scaling_frame({ (float)bsize[0], (float)bsize[1], (float)bsize[2] });
                scene->instances.push_back(instance);
            }
            else if (auto m = dynamic_pointer_cast<model>(o)) {
                scene->shapes.push_back(m->scene.shapes[0]);
                auto instance = m->scene.instances[0];
                instance.shape = scene->shapes.size() - 1;
                scene->instances.push_back(instance);
            }
            else if (auto p = dynamic_pointer_cast<plane>(o)) {
                // only supports XZ planes for now
                auto instance = yocto::scene_instance{};
                instance.shape = 2; // box shape
                instance.material = 0;
                instance.frame = yocto::translation_frame(toYocto(p->origin));
                scene->instances.push_back(instance);
            }
        }
    }

    //auto stats = yocto::scene_stats(*scene);
    //for (auto stat : stats) cerr << stat << endl;

    return scene;
}

void save_image(shared_ptr<yocto::color_image> image, string filename) {
    auto error = string{};
    if (!save_image(filename, *image, error)) {
        cerr << "Failed to save image: " << error << endl;
        return;
    }
}

void debug_pixel(shared_ptr<tracer> pt, unsigned x, unsigned y, unsigned spp, bool verbose = false) {
    auto cb = std::make_unique<callback::print_callback>(verbose);
    pt->DebugPixel(x, y, spp, cb.get());
}

void inspect_all(shared_ptr<tracer> pt, unsigned spp, bool verbose = false, int stopAtBug = -1) {
    //auto cb = std::make_shared<callback::validate_model>("models/LuYu-obj/LuYu-obj.obj_model");
    auto cb = std::make_unique<callback::validate_model>("models/dragon_remeshed.ply_model", stopAtBug, true);
    //auto cb = std::make_shared<callback::highlight_element>(1);
    pt->Render(spp, false, cb.get());
    cb->digest(cerr) << std::endl;
}

void window_debug(
        shared_ptr<tracer> pt, 
        const hittable_list& world, 
        const camera& cam, 
        Film& film, 
        unsigned x, unsigned y, unsigned spp) {
    // now display a window
    vec3 lookat = cam.getLookAt();
    vec3 lookfrom = cam.getLookFrom();

    auto s = init_scene(world);
    auto at = glm::vec3(lookat[0], lookat[1], lookat[2]);
    auto from = glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2]);
    auto w = tool::create_window(film, pt, s, at, from);

    w->debugPixel(x, y, spp);
    w->render();
}

void render(shared_ptr<tracer> pt, hittable_list& world, camera& cam, Film& film, int spp) {
    vec3 lookat = cam.getLookAt();
    vec3 lookfrom = cam.getLookFrom();

    auto s = init_scene(world);
    auto at = glm::vec3(lookat[0], lookat[1], lookat[2]);
    auto from = glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2]);
    auto w = tool::create_window(film, pt, s, at, from);

    //w->setCallback(std::make_shared<callback::validate_model>("models/dragon_remeshed.ply_model"));

    w->render(spp);
}

void offline_render(shared_ptr<tracer> pt, unsigned spp) {
    clock_t start = clock();
    auto cb = std::make_unique<callback::num_inters_callback>();
    pt->Render(spp, false, cb.get());
    clock_t stop = clock();
    double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
    cerr << "Rendering took " << timer_seconds << " seconds.\n" 
         << "Total intersections = " << cb->count << endl;
}

void offline_parallel_render(shared_ptr<tracer> pt, unsigned spp) {
    clock_t start = clock();
    pt->Render(spp);
    clock_t stop = clock();
    double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
    cerr << "Rendering took " << timer_seconds << " seconds.\n";
}

void debug_sss(
        shared_ptr<tracer> pt, 
        const hittable_list& world, 
        const camera& cam, 
        Film& film) {
    // render the scene offline while collecting the histogram
    // auto cb = make_shared<callback::num_medium_scatter_histo>(10, 20);
    std::shared_ptr<callback::callback> cb{};
    pt->Render(1, false, cb.get());

    vec3 lookat = cam.getLookAt();
    vec3 lookfrom = cam.getLookFrom();

    auto s = init_scene(world);
    auto at = glm::vec3(lookat[0], lookat[1], lookat[2]);
    auto from = glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2]);
    auto w = tool::create_window(film, pt, s, at, from);

    // quick test
    //w->showHisto("histogram", cb->getNormalizedBins());
    w->debugPixel(226, 262, 1024);
    w->updateCamera( glm::vec3{ -2.27297f, 0.15248f, 1.23364f }, at );
    w->render(0);
}

void test_envmap(std::string hdr_filename, std::shared_ptr<EnvMap> envmap) {
    // let's convert envmap to LDR image
    //auto hdr = EnvMap::loadImage("hdrs/christmas_photo_studio_04_1k.hdr");
    auto hdr = EnvMap::loadImage(hdr_filename);
    auto ldr = yocto::tonemap_image(hdr, 0.25f, true);

    // let's sample a bunch of points and plot them on the ldr image
    xor_rnd rng;
    for (auto i : yocto::range(10000)) {
        auto sample = envmap->pdf->generate(rng);
        auto ij = dir2ij(sample, hdr.width, hdr.height);
        yocto::set_pixel(ldr, ij.x, ij.y, { 1.0f, 0.0f, 0.0f, 1.0f });
    }

    // save the image to disk
    auto error = string{};
    if (!save_image("ldr_test.png", ldr, error)) {
        cerr << "Failed to save image: " << error << endl;
        return;
    }
}

int main()
{
    // Image

    const auto aspect_ratio = 1.0 / 1.0;
    const int image_width = 500;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int max_depth = 500;

    // World

    hittable_list world;
    auto light = shared_ptr<hittable>{};

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background { 0, 0, 0 };
    bool use_envmap = true;
    bool add_light = false;
    bool russian_roulette = true;

    switch (4) {
        case 0:
            simple_box(world, light, add_light);
            //lookfrom = point3(3, 2, 2);
            lookfrom = point3(3.68871, 1.71156, 3.11611);
            lookat = point3(0, 0, 0);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 1:
            monk_scene(world, true);
            lookfrom = point3(-8.49824, 3.01965, -2.37236);
            lookat = point3(0, 0.75, -0.75);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 2:
            glass_panels(world, false);
            lookfrom = point3(1.97006, 2.41049, 7.12357);
            lookat = point3(0, 0.5, 0);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;
        case 3:
            dragon_scene(world, false);
            // lookfrom = point3(-4.35952, 2.64187, 4.06531);
            //lookfrom = { 4.31991, 4.0518, -2.75208 };
            lookfrom = { 2.27155, 7.99803, 0.244723 };
            lookat = point3(-0.5, 0, -0.5);
            vfov = 20.0;
            aperture = 0.0;
            background = color(0.6, 0.6, 0.7);
            break;
        case 4:
            moriKnob(world);
            lookfrom = point3(-0.594562, 1.10331, -0.793232);
            lookat = point3(0, 0.1, 0);
            vfov = 20.0;
            aperture = 0.0;
            background = color(0.6, 0.6, 0.7);
            break;
    }

    // Camera
    vec3 vup{ 0, 1, 0 };

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture);
    auto film = Film(image_width, image_height);

    unique_ptr<EnvMap> envmap{};
    if (use_envmap) {
        //auto hdr_filename = "hdrs/christmas_photo_studio_04_1k.hdr";
        //auto hdr_filename = "hdrs/christmas_photo_studio_04_4k.exr";
        //auto hdr_filename = "hdrs/parched_canal_1k.exr";
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
    auto pt = make_shared<pathtracer>(cam, film, scene, max_depth, rr_depth);
    if (!russian_roulette)
        std::cerr << "RUSSIAN ROULETTE DISABLED\n";

    std::cerr << "hardware_concurrency = " << std::thread::hardware_concurrency() << std::endl;

    //debug_pixel(pt, 67, 0, 1, true);
    // inspect_all(pt, 1, false, -1);
    render(pt, world, cam, film, -1); // pass spp=0 to disable further rendering in the window
    //offline_render(pt, 128);
    // offline_parallel_render(pt, 128);
    // window_debug(pt, world, cam, image, 267, 161, 1);
    // debug_sss(pt, world, cam, image);

    //save_image(image, "output/dragon-glass-1.png"); 

    // compare to reference image
    bool save_ref = false;
    bool compare_ref = false;

    auto raw = RawData(film.width, film.height);
    film.GetRaw(raw);
    if (compare_ref) {
        auto ref = RawData("dragon-128spp.raw");
        std::cerr << "RMSE = " << raw.rmse(ref) << std::endl;
        yocto::vec2i coord;
        if (raw.findFirstDiff(ref, coord)) {
            std::cerr << "First diff found at (" << coord.x << ", " << coord.y << ")\n";
        }
    }
    if (save_ref) {
        //TODO I should add a timestamp to the reference name to avoid corrupting a previous one
        raw.saveToFile("dragon-128spp.raw");
    }
}
