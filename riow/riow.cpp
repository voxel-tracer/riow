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

#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>

#include "tool/window.h"

using namespace std;

enum class render_state {
    SPECULAR, DIFFUSE, NOHIT, ABSORBED, MAXDEPTH
};

color ray_color(
        const ray& r, const color& background, const shared_ptr<hittable> world, 
        const shared_ptr<hittable_list> lights, shared_ptr<rnd> rng, int depth,
        std::function<void (const ray&, const ray&, render_state)> callback) {
    // If we've exceeded the ray bounce limit, no more lights gathered
    if (depth == 0) {
        callback(r, r, render_state::MAXDEPTH);
        return color{ 0, 0, 0 };
    }

    hit_record rec;
    if (!world->hit(r, 0.001, infinity, rec, rng)) {
        callback(r, r, render_state::NOHIT);
        return background;
    }

    scatter_record srec;
    color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
    if (!rec.mat_ptr->scatter(r, rec, srec, rng)) {
        callback(r, r, render_state::ABSORBED);
        return emitted;
    }

    if (srec.is_specular) {
        callback(r, srec.specular_ray, render_state::SPECULAR);
        return srec.attenuation * ray_color(srec.specular_ray, background, world, lights, rng, depth - 1, callback);
    }

    ray scattered;
    double pdf_val;
    if (lights->objects.empty()) {
        // sample material directly
        scattered = ray(rec.p, srec.pdf_ptr->generate(rng));
        pdf_val = srec.pdf_ptr->value(scattered.direction());
    } else {
        // multiple importance sampling of light and material pdf
        auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
        mixture_pdf mixed_pdf(light_ptr, srec.pdf_ptr);

        scattered = ray(rec.p, mixed_pdf.generate(rng));
        pdf_val = mixed_pdf.value(scattered.direction());
    }

    callback(r, scattered, render_state::DIFFUSE);
    return emitted +
        srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
               * ray_color(scattered, background, world, lights, rng, depth - 1, callback) / pdf_val;
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
    objects->add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), -0.45, material_left));
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
    objects->add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), -0.45, material_left));
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

yocto::vec4f convert(const color& pixel_color, unsigned spp, bool gamma_correct = true) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Replace NaN components with zero
    if (std::isnan(r)) r = 0.0;
    if (std::isnan(g)) g = 0.0;
    if (std::isnan(b)) b = 0.0;

    auto scale = 1.0 / spp;

    if (gamma_correct) {
        r = clamp(sqrt(scale * r), 0.0, 1.0);
        g = clamp(sqrt(scale * g), 0.0, 1.0);
        b = clamp(sqrt(scale * b), 0.0, 1.0);
    } else {
        r = scale * r;
        g = scale * g;
        b = scale * b;
    }

    return { 
        static_cast<float>(r), 
        static_cast<float>(g),
        static_cast<float>(b),
        1.0f 
    };
}

shared_ptr<tool::scene> init_scene() {
    auto scene = yocto::scene_model{};
    {
        auto shape = yocto::scene_shape{};
        // make shape with 32 steps in resolution and scale of 1
        auto quads = yocto::vector<yocto::vec4i>{};
        yocto::make_sphere(quads, shape.positions, shape.normals, shape.texcoords, 8, 1, 1);
        // yocto::make_monkey(quads, positions, 1);
        shape.triangles = quads_to_triangles(quads);
        scene.shapes.push_back(shape);
        scene.materials.push_back({}); // create a black material directly

        {
            auto instance = yocto::scene_instance{};
            instance.shape = 0;
            instance.material = 0;
            instance.frame =
                yocto::translation_frame({ 0.0f, 0.0f, -1.0f }) *
                yocto::scaling_frame({ 0.5f, 0.5f, 0.5f });
            scene.instances.push_back(instance);
        }
        {
            auto instance = yocto::scene_instance{};
            instance.shape = 0;
            instance.material = 0;
            instance.frame =
                yocto::translation_frame({ -1.0f, 0.0f, -1.0f }) *
                yocto::scaling_frame({ 0.5f, 0.5f, 0.5f });
            scene.instances.push_back(instance);
        }
        {
            auto instance = yocto::scene_instance{};
            instance.shape = 0;
            instance.material = 0;
            instance.frame =
                yocto::translation_frame({ 1.0f, 0.0f, -1.0f }) *
                yocto::scaling_frame({ 0.5f, 0.5f, 0.5f });
            scene.instances.push_back(instance);
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

    switch (2) {
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

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus);

    // Render
    auto image = yocto::make_image(image_width, image_height, false);

    // to render pixel (x, y)
    //  i = x;
    //  j = (image_height - 1) - y; => y = (image_height - 1) - j;

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
                pixel_color += ray_color(r, background, world, lights, local_rng, max_depth,
                    [](const ray& r, const ray& s, render_state state) {});
            }

            yocto::set_pixel(image, i, (image_height - 1) - j, convert(pixel_color, samples_per_pixel));
        }
    }

    auto error = string{};
    if (!save_image("out.png", image, error)) {
        cerr << "Failed to save image: " << error << endl;
        return -1;
    }

    // now display a window
    tool::window w{
        image,
        { 0.0f, 0.0f, -1.0f }, // look_at
        { 3.0f, 2.0f, 2.0f }  // look_from
    };
    w.set_scene(init_scene());

    w.render();
}
