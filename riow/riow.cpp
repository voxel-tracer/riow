// riow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "constant_medium.h"
#include "camera.h"
#include "material.h"

#include <iostream>

using namespace std;

color ray_color(const ray& r, const color& background, const hittable_list& world, int depth) {
    // If we've exceeded the ray bounce limit, no more lights gathered
    if (depth == 0)
        return color{ 0, 0, 0 };

    hit_record rec;
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    ray scattered;
    color attenuation;
    color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted;

    return emitted + attenuation * ray_color(scattered, background, world, depth - 1);
}

hittable_list earth() {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);

    return hittable_list(globe);
}

hittable_list three_spheres() {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto material_center = make_shared<lambertian>(earth_texture);
    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    hittable_list world;
    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), -0.45, material_left));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
    return world;
}

hittable_list three_spheres_with_medium() {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    hittable_list world;
    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));

    auto white = make_shared<lambertian>(color(0.73, 0.73, 0.73));
    shared_ptr<hittable> sphere1 = make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, white);
    world.add(make_shared<constant_medium>(sphere1, 10.0, color(1, 1, 1)));

    auto material_light = make_shared<diffuse_light>(color(10.0, 10.0, 10.0));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.75), 0.1, material_light));

    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), -0.45, material_left));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
    return world;
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

int main()
{
    // Image

    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 1024;
    const int max_depth = 50;

    // World

    hittable_list world;

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background = { 0, 0, 0 };

    switch (3) {
        case 1:
            world = earth();
            background = color(0.70, 0.80, 1.00);
            lookfrom = point3(13, 2, 3);
            lookat = point3(0, 0, 0);
            vfov = 20.0;
            break;

        case 2:
            world = three_spheres();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.70, 0.80, 1.00);
            break;

        case 3:
            world = three_spheres_with_medium();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.70, 0.80, 1.00);
            break;

        default:
        case 4:
            world = four_spheres();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 30.0;
            aperture = 0.1;
            background = color(0.1, 0.1, 0.1);
            break;
    }

    // Camera
    vec3 vup{ 0, 1, 0 };
    auto dist_to_focus = (lookfrom - lookat).length();

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus);

    // Render

    cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (auto j = image_height - 1; j >= 0; --j) {
        cerr << "\rScanlines remaining: " << j << ' ' << flush;
        for (auto i = 0; i != image_width; ++i) {
            color pixel_color{ 0, 0, 0 };
            for (auto s = 0; s != samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width - 1);
                auto v = (j + random_double()) / (image_height - 1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, background, world, max_depth);
            }

            write_color(cout, pixel_color, samples_per_pixel);
        }
    }

    cerr << "\nDone.\n";
}
