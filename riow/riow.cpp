// riow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "constant_medium.h"
#include "camera.h"
#include "material.h"
#include "box.h"
#include "aarect.h"
#include "pdf.h"

#include <iostream>
#include <functional>
#include <time.h>

#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>

#include "pathtracer.h"
#include "tool/window.h"

using namespace std;

hittable_list earth() {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);

    return hittable_list(globe);
}

hittable_list diffuse_spheres() {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto white = color(.75, .75, .75);
    auto material_white = make_shared<lambertian>(white);

    hittable_list world;
    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_white));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_white));

    auto material_light = make_shared<diffuse_light>(color(10.0, 10.0, 10.0));
    world.add(make_shared<sphere>(point3(0.0, 1.5, 1.75), 0.1, material_light));

    return world;
}

void three_spheres(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto material_center = make_shared<lambertian>(earth_texture);
    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    objects->add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));

    objects->add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));

    objects->add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));

    objects->add(make_shared<sphere>(point3(1.0, 0.0, -1.0), -0.49, material_right));
    objects->add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
}

void three_spheres_with_medium(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    objects->add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));

    auto white = make_shared<lambertian>(color(0.73, 0.73, 0.73));
    shared_ptr<hittable> sphere1 = make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, white);
    objects->add(make_shared<constant_medium>(sphere1, 10.0, color(.73, .73, .73)));

    auto material_light = make_shared<diffuse_light>(color(10.0, 10.0, 10.0));
    auto light_sphere = make_shared<sphere>(point3(0.0, 0.0, -1.75), 0.1, material_light);
    objects->add(light_sphere);
    sampled->add(light_sphere);

    objects->add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));

    objects->add(make_shared<sphere>(point3(1.0, 0.0, -1.0), -0.45, material_right));
    objects->add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
}

hittable_list four_spheres() {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto material_center = make_shared<lambertian>(earth_texture);
    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);
    auto material_light = make_shared<diffuse_light>(color(10.0, 10.0, 10.0));

    hittable_list world;
    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));

    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));

    world.add(make_shared<sphere>(point3(-1.0, 0.0, -0.5), 0.5, material_left));

    world.add(make_shared<sphere>(point3(1.0, 0.25, -1.0), 0.5, material_right));
    world.add(make_shared<sphere>(point3(1.0, 0.25, -1.0), -0.45, material_right));

    world.add(make_shared<sphere>(point3(0.0, 2.0, 0.0), 0.25, material_light));

    return world;
}

void cornell_box(shared_ptr<hittable_list> objects, shared_ptr<hittable_list> sampled) {
    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    objects->add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    objects->add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));

    auto light_quad = make_shared<xz_rect>(213, 343, 227, 332, 554, light);
    objects->add(make_shared<flip_face>(light_quad)); // flip_face doesn't support hittable sampling
    sampled->add(light_quad);
    
    objects->add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    objects->add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    objects->add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

    //shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);
    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    objects->add(box1);

    //auto glass = make_shared<dielectric>(1.5);
    //objects.add(make_shared<sphere>(point3(190, 90, 190), 90, glass));

    shared_ptr<hittable> sphere1 = make_shared<sphere>(point3(190, 90, 190), 90, white);
    objects->add(make_shared<constant_medium>(sphere1, 10.0, color(.73, .73, .73)));
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

int main()
{
    // Image

    const auto aspect_ratio = 1.0 / 1.0;
    const int image_width = 500;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 1;
    const int max_depth = 50;

    // World

    auto world = make_shared<hittable_list>();
    auto lights = make_shared<hittable_list>();

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background { 0, 0, 0 };

    switch (3) {
        case 1:
            //world = earth();
            background = color(0.70, 0.80, 1.00);
            lookfrom = point3(13, 2, 3);
            lookat = point3(0, 0, 0);
            vfov = 20.0;
            break;

        case 2:
            three_spheres(world, lights);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.70, 0.80, 1.00);
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
            //world = four_spheres();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 30.0;
            aperture = 0.1;
            background = color(0.1, 0.1, 0.1);
            break;

        case 5:
            //world = diffuse_spheres();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.1, 0.1, 0.1);
            break;

        default:
        case 6:
            cornell_box(world, lights);
            lookfrom = point3(278, 278, -800);
            lookat = point3(278, 278, 0);
            break;
    }

    // Camera
    vec3 vup{ 0, 1, 0 };
    auto dist_to_focus = (lookfrom - lookat).length();

    auto cam = make_shared<camera>(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus);
    auto image = make_shared<yocto::color_image>(yocto::make_image(image_width, image_height, false));

    // Render
    scene_desc scene{
        background,
        world,
        lights
    };
    shared_ptr<tracer> pt = make_shared<pathtracer>(cam, image, scene, samples_per_pixel, max_depth);

    if (false)
    {
        vector<tool::path_segment> segments;
        pt->DebugPixel(262, 211, segments);
    } else {
        clock_t start = clock();
        num_inters_callback cb;
        pt->Render(cb);
        clock_t stop = clock();
        double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
        cerr << "Rendering took " << timer_seconds << " seconds. \nTotal intersections = " << cb.count << endl;
    }

    auto error = string{};
    if (!save_image("out.png", *image, error)) {
        cerr << "Failed to save image: " << error << endl;
        return -1;
    }

    // now display a window
    tool::window w{
        *image,
        pt,
        { 0.0f, 0.0f, -1.0f }, // look_at
        { 3.0f, 2.0f, 2.0f }  // look_from
    };
    w.set_scene(init_scene(world));

    w.render();
}
