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

using namespace std;

color ray_color(const ray& r, const color& background, const hittable_list& world, shared_ptr<hittable> lights, shared_ptr<rnd> rng, int depth) {
    // If we've exceeded the ray bounce limit, no more lights gathered
    if (depth == 0)
        return color{ 0, 0, 0 };

    hit_record rec;
    if (!world.hit(r, 0.001, infinity, rec, rng))
        return background;

    scatter_record srec;
    color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
    if (!rec.mat_ptr->scatter(r, rec, srec, rng))
        return emitted;

    if (srec.is_specular) {
        return srec.attenuation * ray_color(srec.specular_ray, background, world, lights, rng, depth - 1);
    }

    // multiple importance sampling of light and material pdf
    auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
    mixture_pdf mixed_pdf(light_ptr, srec.pdf_ptr);

    ray scattered = ray(rec.p, mixed_pdf.generate(rng));
    auto pdf_val = mixed_pdf.value(scattered.direction());

    return emitted + 
        srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
               * ray_color(scattered, background, world, lights, rng, depth - 1) / pdf_val;
}

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

/*
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
*/

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

hittable_list cornell_box() {
    hittable_list objects;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
    objects.add(make_shared<flip_face>(make_shared<xz_rect>(213, 343, 227, 332, 554, light)));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

    //shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);
    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    objects.add(box1);

    //auto glass = make_shared<dielectric>(1.5);
    //objects.add(make_shared<sphere>(point3(190, 90, 190), 90, glass));

    shared_ptr<hittable> sphere1 = make_shared<sphere>(point3(190, 90, 190), 90, white);
    objects.add(make_shared<constant_medium>(sphere1, 10.0, color(.73, .73, .73)));
    //objects.add(make_shared<sphere>(point3(190, 90, 190), 90, white));

    //objects.add(make_shared<sphere>(point3(390, 90, 190), 90, white));

    return objects;
}

int main()
{
    // Image

    const auto aspect_ratio = 1.0 / 1.0;
    const int image_width = 500;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 1024;
    const int max_depth = 50;

    // World

    hittable_list world;
    
    auto lights = make_shared<hittable_list>();
    lights->add(make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>()));
    //lights->add(make_shared<sphere>(point3(190, 90, 190), 90, shared_ptr<material>()));

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background { 0, 0, 0 };

    switch (0) {
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
/*
        case 3:
            world = three_spheres_with_medium();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.70, 0.80, 1.00);
            break;
*/
        case 4:
            world = four_spheres();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 30.0;
            aperture = 0.1;
            background = color(0.1, 0.1, 0.1);
            break;

        case 5:
            world = diffuse_spheres();
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.1, 0.1, 0.1);
            break;

        default:
        case 6:
            world = cornell_box();
            lookfrom = point3(278, 278, -800);
            lookat = point3(278, 278, 0);
            break;
    }

    // Camera
    vec3 vup{ 0, 1, 0 };
    auto dist_to_focus = (lookfrom - lookat).length();

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus);

    // Render

    // to render pixel (x, y)
    //  i = x;
    //  j = (image_height - 1) - y;

    cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (auto j = image_height - 1; j >= 0; --j) {
        cerr << "\rScanlines remaining: " << j << ' ' << flush;
        for (auto i = 0; i != image_width; ++i) {
            color pixel_color{ 0, 0, 0 };

            auto local_seed = (xor_rnd::wang_hash(j * image_width + i) * 336343633) | 1;
            auto local_rng = make_shared<xor_rnd>(local_seed);

            for (auto s = 0; s != samples_per_pixel; ++s) {
                auto u = (i + local_rng->random_double()) / (image_width - 1);
                auto v = (j + local_rng->random_double()) / (image_height - 1);
                ray r = cam.get_ray(u, v, local_rng);
                pixel_color += ray_color(r, background, world, lights, local_rng, max_depth);
            }

            write_color(cout, pixel_color, samples_per_pixel);
        }
    }

    cerr << "\nDone.\n";
}
