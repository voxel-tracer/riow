// riow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "constant_medium.h"
#include "camera.h"
#include "material.h"
#include "pdf.h"

#include <iostream>
#include <functional>
#include <time.h>

#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>

#include "pathtracer.h"
#include "tool/window.h"

using namespace std;

void three_spheres(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto material_center = make_shared<lambertian>(earth_texture);
    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    objects->add(make_shared<sphere>("center_ball", point3(0.0, 0.0, -1.0), 0.5, material_center));

    objects->add(make_shared<sphere>("left_ball", point3(-1.0, 0.0, -1.0), 0.5, material_left));

    objects->add(make_shared<sphere>("right_ball_inner", point3(1.0, 0.0, -1.0), -0.49, material_right));
    objects->add(make_shared<sphere>("right_ball_outer", point3(1.0, 0.0, -1.0), 0.5, material_right));
}

void multiple_glass_balls(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    // ground
    auto checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    {
        // red ball
        auto medium = make_shared<IsotropicScatteringMedium>(color(.8, .3, .3), .25, 2.0);
        auto glass = make_shared<dielectric>(1.35, medium);

        auto ball = make_shared<sphere>("red_ball", point3(-1.0, 0.1, -1.0), 0.5, glass);
        objects->add(ball);
        sampled->add(ball);
    }

    {
        // green ball
        auto medium = make_shared<IsotropicScatteringMedium>(color(.3, .8, .3), .25, 2.0);
        auto glass = make_shared<dielectric>(1.35, medium);

        auto ball = make_shared<sphere>("green_ball", point3(0.0, 0.1, -1.0), 0.5, glass);
        objects->add(ball);
        sampled->add(ball);
    }

    {
        // blue ball
        auto medium = make_shared<IsotropicScatteringMedium>(color(.3, .3, .8), .25, 2.0);
        auto glass = make_shared<dielectric>(1.35, medium);

        auto ball = make_shared<sphere>("blue_ball", point3(1.0, 0.1, -1.0), 0.5, glass);
        objects->add(ball);
        sampled->add(ball);
    }

    auto material_light = make_shared<diffuse_light>(color(20.0));
    auto light_sphere = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
    objects->add(light_sphere);
    sampled->add(light_sphere);
}

void two_glass_balls(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    // ground
    auto checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    //auto ketchup = make_shared<IsotropicScatteringMedium>(color(.98, .0061, .0033), 1.0, 9.0);
    //auto glass = make_shared<dielectric>(1.35, ketchup);
    color clr = { .98, .0061, .0033 };
    double ad = 0.005;
    double sd = 0.5;
    {
        // red ball without subsurface scattering
        auto medium = make_shared<NoScatterMedium>(clr, ad);
        auto glass = make_shared<dielectric>(1.35, medium);

        objects->add(make_shared<sphere>("glass_ball", point3(-0.5, 0.1, -1.0), 0.5, glass));
    }

    {
        // red ball with subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        auto glass = make_shared<dielectric>(1.35, medium);

        objects->add(make_shared<sphere>("sss_ball", point3(0.5, 0.1, -1.0), 0.5, glass));
    }

    //auto material_light = make_shared<diffuse_light>(color(20.0));
    //auto light_sphere = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.1, material_light);
    //objects->add(light_sphere);
    //sampled->add(light_sphere);
}

void two_mediums(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled, bool add_light) {
    // ground
    auto checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    color clr = { .98, .0061, .0033 };
    double ad = 0.005;
    double sd = 0.5;
    {
        // white diffuse scattering with red subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        color clr = { 1.0, 1.0, 1.0 };
        auto diffuse = make_shared<diffuse_subsurface_scattering>(0.5, clr, medium);

        objects->add(make_shared<sphere>("diffuse_sss_ball", point3(-0.5, 0.1, -1.0), 0.5, diffuse));
    }

    {
        // red ball with subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        auto glass = make_shared<dielectric>(1.35, medium);

        objects->add(make_shared<sphere>("sss_ball", point3(0.5, 0.1, -1.0), 0.5, glass));
    }

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        auto light_sphere = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
        objects->add(light_sphere);
        sampled->add(light_sphere);
    }
}

void glass_ball(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled, bool add_light) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    //auto medium = make_shared<NoScatterMedium>(color(0.8, 0.3, 0.3), 0.25);
    auto ketchup = make_shared<IsotropicScatteringMedium>(color(.98, .0061, .0033), 1.0, 9.0);
    auto glass = make_shared<dielectric>(1.35, ketchup);
    auto pure_glass = make_shared<dielectric>(1.5);
    auto ball = make_shared<sphere>("glass_ball", point3(0.0, 0.1, -1.0), 0.5, pure_glass);

    objects->add(ball);
    sampled->add(ball);

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        auto light_sphere = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
        objects->add(light_sphere);
        sampled->add(light_sphere);
    }
}

void three_spheres_with_medium(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    auto white = make_shared<lambertian>(color(0.73, 0.73, 0.73));
    shared_ptr<hittable> sphere1 = make_shared<sphere>("center_ball", point3(0.0, 0.0, -1.0), 0.5, white);
    objects->add(make_shared<constant_medium>(sphere1, 10.0, color(.99, .93, .93)));

    auto material_light = make_shared<diffuse_light>(color(10.0, 10.0, 10.0));
    auto light_sphere = make_shared<sphere>("light", point3(0.0, 2.0, 0.0), 0.1, material_light);
    objects->add(light_sphere);
    sampled->add(light_sphere);

    objects->add(make_shared<sphere>("left_ball", point3(-1.0, 0.0, -1.0), 0.5, material_left));

    objects->add(make_shared<sphere>("right_ball_inner", point3(1.0, 0.0, -1.0), -0.49, material_right));
    objects->add(make_shared<sphere>("right_ball_outer", point3(1.0, 0.0, -1.0), 0.5, material_right));
}

shared_ptr<tool::scene> init_scene(shared_ptr<hittable_list> world) {
    auto scene = yocto::scene_model{};
    {
        {
            auto shape = yocto::scene_shape{};
            // make shape with 8 steps in resolution and scale of 1
            auto quads = yocto::vector<yocto::vec4i>{};
            yocto::make_sphere(quads, shape.positions, shape.normals, shape.texcoords, 8, 1, 1);
            shape.triangles = quads_to_triangles(quads);
            scene.shapes.push_back(shape);
        }
        scene.materials.push_back({}); // create a black material directly

        for (auto o : world->objects) {
            // only supports spheres for now and constant_medium that uses sphere as a boundary
            if (auto cm = dynamic_pointer_cast<constant_medium>(o)) {
                o = cm->boundary;
            }
            if (auto s = dynamic_pointer_cast<sphere>(o)) {
                if (s->radius > 99.0) continue; // ignore floor sphere

                auto instance = yocto::scene_instance{};
                instance.shape = 0;
                instance.material = 0;
                instance.frame =
                    yocto::translation_frame({ (float)s->center[0], (float)s->center[1], (float)s->center[2] }) *
                    yocto::scaling_frame({ (float)s->radius, (float)s->radius, (float)s->radius });
                scene.instances.push_back(instance);
            }
        }
    }

    auto stats = yocto::scene_stats(scene);
    for (auto stat : stats) cerr << stat << endl;

    return make_shared<tool::scene>(scene);
}

void debug_pixel(shared_ptr<tracer> pt, unsigned x, unsigned y, bool verbose = false) {
//    auto cb = std::make_shared<callback::print_callback>(verbose);
    // auto cb = std::make_shared<callback::print_pdf_sample_cb>();
    auto cb = std::make_shared<callback::print_sample_callback>(11, true);
    pt->DebugPixel(x, y, cb);
}

void inspect_all(shared_ptr<tracer> pt, bool verbose = false, bool stopAtFirst = false) {
    auto cb = std::make_shared<callback::dbg_find_gothrough_diffuse>("floor", verbose, stopAtFirst);
    pt->Render(cb);
    cerr << "Found " << cb->getCount() << " buggy samples\n";
}

void window_debug(shared_ptr<tracer> pt, shared_ptr<hittable_list> world, shared_ptr<camera> cam, shared_ptr<yocto::color_image> image, unsigned x, unsigned y) {
    // now display a window
    vec3 lookat = cam->getLookAt();
    vec3 lookfrom = cam->getLookFrom();

    tool::window w{
        image, pt,
        glm::vec3(lookat[0], lookat[1], lookat[2]), // look_at
        glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2])  // look_from
    };
    w.set_scene(init_scene(world));
    w.debugPixel(x, y);
    w.render();
}

void render(shared_ptr<tracer> pt, shared_ptr<hittable_list> world, shared_ptr<camera> cam, shared_ptr<yocto::color_image> image) {
    //clock_t start = clock();
    //auto cb = std::make_shared<callback::num_inters_callback>();
    // pt->Render(cb);
    //for (auto i = 0; i != 16; ++i) {
    //    pt->RenderIteration(cb);
    //}
    //clock_t stop = clock();
    //double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
    //cerr << "Rendering took " << timer_seconds << " seconds. \nTotal intersections = " << cb->count << endl;

    // now display a window
    vec3 lookat = cam->getLookAt();
    vec3 lookfrom = cam->getLookFrom();

    tool::window w{
        image, pt,
        glm::vec3(lookat[0], lookat[1], lookat[2]), // look_at
        glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2])  // look_from
    };
    w.set_scene(init_scene(world));

    w.render();

    auto error = string{};
    if (!save_image("out.png", *image, error)) {
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
    const int samples_per_pixel = 16;
    const int max_depth = 500;

    // World

    auto world = make_shared<hittable_list>();
    auto lights = make_shared<hittable_list>();

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background { 0, 0, 0 };

    switch (6) {
        case 1:
            two_mediums(world, lights, true);
            lookfrom = point3(3.40, 2.75, 3.12);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;

        case 2:
            two_glass_balls(world, lights);
            lookfrom = point3(0.09, 3.44, 4.15);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;

        case 3:
            three_spheres_with_medium(world, lights);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.70, 0.80, 1.00);
            break;

        case 4:
            glass_ball(world, lights, true);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;

        case 5:
            glass_ball(world, lights, false);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;

        default:
        case 6:
            multiple_glass_balls(world, lights);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
    }

    // Camera
    vec3 vup{ 0, 1, 0 };

    auto cam = make_shared<camera>(lookfrom, lookat, vup, vfov, aspect_ratio, aperture);
    auto image = make_shared<yocto::color_image>(yocto::make_image(image_width, image_height, false));

    // Render
    scene_desc scene{
        background,
        world,
        lights
    };
    auto pt = make_shared<pathtracer>(cam, image, scene, samples_per_pixel, max_depth, 3);

    // debug_pixel(pt, 199, 41, true);
    // inspect_all(pt/*, true, true*/);
    render(pt, world, cam, image);
    // window_debug(pt, world, cam, image, 199, 41);
}
