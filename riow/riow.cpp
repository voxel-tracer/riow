// riow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "plane.h"
#include "box.h"
#include "constant_medium.h"
#include "camera.h"
#include "material.h"
#include "pdf.h"
#include "callbacks.h"
#include "model.h"

#include <iostream>
#include <functional>
#include <time.h>

#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>

#include "pathtracer.h"
#include "tool/tool.h"

using namespace std;

void three_spheres(shared_ptr<hittable_list> objects) {
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

void multiple_glass_balls(shared_ptr<hittable_list> objects, shared_ptr<hittable>& light, bool add_light) {
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
    }

    {
        // green ball
        auto medium = make_shared<IsotropicScatteringMedium>(color(.3, .8, .3), .25, 2.0);
        auto glass = make_shared<dielectric>(1.35, medium);

        auto ball = make_shared<sphere>("green_ball", point3(0.0, 0.1, -1.0), 0.5, glass);
        objects->add(ball);
    }

    {
        // blue ball
        auto medium = make_shared<IsotropicScatteringMedium>(color(.3, .3, .8), .25, 2.0);
        auto glass = make_shared<dielectric>(1.35, medium);

        auto ball = make_shared<sphere>("blue_ball", point3(1.0, 0.1, -1.0), 0.5, glass);
        objects->add(ball);
    }

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
        objects->add(light);
    }
}

void two_glass_balls(shared_ptr<hittable_list> objects, shared_ptr<hittable> light, bool add_light) {
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

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.1, material_light);
        objects->add(light);
    }
}

void two_mediums(shared_ptr<hittable_list> objects, shared_ptr<hittable> light, bool add_light) {
    // ground
    //auto checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
    //auto material_ground = make_shared<lambertian>(checker);
    auto material_ground = make_shared<lambertian>(color(0.4));
    objects->add(make_shared<plane>("floor", point3(0.0, -0.501, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    color clr = { .98, .0061, .0033 };
    double ad = 0.005;
    double sd = 0.5;
    {
        // white diffuse scattering with red subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        auto diffuse = make_shared<diffuse_subsurface_scattering>(clr, medium);

        objects->add(make_shared<sphere>("diffuse_sss_ball", point3(-0.5, 0.0, -1.0), 0.5, diffuse));
    }

    {
        // red ball with subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        auto glass = make_shared<dielectric>(1.35, medium);

        objects->add(make_shared<sphere>("sss_ball", point3(0.5, 0.0, -1.0), 0.5, glass));
    }

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
        objects->add(light);
    }
}

void dielectric_and_sss(shared_ptr<hittable_list> objects, shared_ptr<hittable> light, bool add_light) {
    // ground
    //auto checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
    //auto material_ground = make_shared<lambertian>(checker);
    auto material_ground = make_shared<lambertian>(color(1.0));
    objects->add(make_shared<plane>("floor", point3(0.0, -0.501, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    color clr = { .98, .0061, .0033 };
    double ad = 0.005;
    double sd = 0.5;
    {
        // white diffuse scattering with red subsurface scattering
        auto medium = make_shared<NoScatterMedium>(clr, ad);
        color clr = { 1.0, 1.0, 1.0 };
        auto glass = make_shared<dielectric>(1.35, medium);

        objects->add(make_shared<sphere>("dielectric_ball", point3(-0.5, 0.0, -1.0), 0.5, glass));
    }

    {
        // red ball with subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        auto glass = make_shared<dielectric>(1.35, medium);

        objects->add(make_shared<sphere>("sss_ball", point3(0.5, 0.0, -1.0), 0.5, glass));
    }

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
        objects->add(light);
    }
}

void diffuse_and_sss(shared_ptr<hittable_list> objects, shared_ptr<hittable> light, bool add_light) {
    // ground
    //auto checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
    //auto material_ground = make_shared<lambertian>(checker);
    auto material_ground = make_shared<lambertian>(color(0.4));
    objects->add(make_shared<plane>("floor", point3(0.0, -0.501, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    color clr = { .98, .0061, .0033 };
    double ad = 0.005;
    double sd = 0.5;
    {
        // white diffuse scattering with red subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(clr, ad, sd);
        auto diffuse = make_shared<diffuse_subsurface_scattering>(clr, medium);

        objects->add(make_shared<sphere>("diffuse_sss_ball", point3(-0.5, 0.0, -1.0), 0.5, diffuse));
    }

    {
        // white diffuse ball
        auto white_diffuse = make_shared<lambertian>(color(1.0, 1.0, 1.0));

        objects->add(make_shared<sphere>("diffuse_ball", point3(0.5, 0.0, -1.0), 0.5, white_diffuse));
    }

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 1.5, 1.0), 0.5, material_light);
        objects->add(light);
    }
}

void simple_box(shared_ptr<hittable_list> objects, shared_ptr<hittable> light, bool add_light) {

    // auto material_ground = make_shared<lambertian>(color(0.4));
    shared_ptr<texture> checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<plane>("floor", point3(0.0, -0.101, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

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
        // red glass
        auto ketchup = make_shared<IsotropicScatteringMedium>(color(.98, .0061, .0033), 1.0, 9.0);
        mat = make_shared<dielectric>(1.35, ketchup);
        break;
    }
    case 3: {
        // pure glass
        mat = make_shared<dielectric>(1.2);
        break;
    }
    case 4: {
        // white diffuse scattering with red subsurface scattering
        color clr(.98, .0061, .0033);
        auto medium = make_shared<IsotropicScatteringMedium>(clr, 0.005, 0.5);
        mat = make_shared<diffuse_subsurface_scattering>(clr, medium);
        break;
    }
    case 5: {
        // red ball with subsurface scattering
        auto medium = make_shared<IsotropicScatteringMedium>(color(.98, .0061, .0033), 0.005, 0.5);
        mat = make_shared<dielectric>(1.5, medium);
    }
    }

    objects->add(make_shared<box>("base", point3(), vec3(1.0, 0.1, 1.0), mat));

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(20.0));
        light = make_shared<sphere>("light", point3(0.0, 15, 1.0), 0.5, material_light);
        objects->add(light);
    }

}

void glass_panels(shared_ptr<hittable_list> objects, bool scattering_medium = false) {
    // white diffuse floor
    auto material_ground = make_shared<lambertian>(color(0.8));
    //shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1, 0.1, 0.1), color(1.0, 1.0, 1.0));
    //auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<plane>("floor", point3(0.0, -0.005, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.1, 0.05);
    else 
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);

    for (auto i : yocto::range(5)) {
        objects->add(make_shared<box>("panel1", point3(0.0, 0.5, 1.0 - i*0.5), vec3(1.0, 1.0, 0.1), tinted_glass));
    }
}

void glass_ball(shared_ptr<hittable_list> objects, bool scattering_medium = false) {
    //auto material_ground = make_shared<lambertian>(color(0.8));
    shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1), color(0.8));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<plane>("floor", point3(0.0, -0.505, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.1, 0.05);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);
    auto ball = make_shared<sphere>("glass_ball", point3(0.0, 0.0, -1.0), 0.5, tinted_glass);

    objects->add(ball);
}

void monkey_debug(shared_ptr<hittable_list> objects) {
    // no floor
    // passthrough material
    auto m = make_shared<passthrough>();
    auto monkey = make_shared<model>("models/monkey-lowpoly.obj", m, 0);
    monkey->buildBvh();
    objects->add(monkey);
}

void monkey_scene(shared_ptr<hittable_list> objects, bool scattering_medium = false) {
    auto material_ground = make_shared<lambertian>(color(0.1));
    //shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1), color(0.8));
    //auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<plane>("floor", point3(0.0, -1.005, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.1, 0.025);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);

    //auto m = make_shared<metal>(color(0.97, 0.96, 0.91));
    auto m = make_shared<lambertian>(color(glass_color));
    auto monkey = make_shared<model>("models/monkey-lowpoly.obj", tinted_glass, 0);
    monkey->buildBvh();
    objects->add(monkey);
}

void dragon_debug(shared_ptr<hittable_list> objects) {
    // no floor

    // passthrough material
    //auto m = make_shared<passthrough>();

    // complex material
    bool scattering_medium = true;
    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);
    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.1, 0.025);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto m = make_shared<dielectric>(1.5, medium);

    auto dragon = make_shared<model>("models/dragon_remeshed.ply", m, 0);
    dragon->scene.instances[0].frame =
        yocto::translation_frame({ -0.5f, 0.0f, 0.0f }) *
        yocto::rotation_frame({ 0.0f, 1.0f, 0.0f }, yocto::radians(180.0f)) *
        yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, yocto::radians(-35.0f)) *
        yocto::rotation_frame({ 0.0f, 0.0f, 1.0f }, yocto::radians(90.0f)) *
        yocto::scaling_frame({ 1 / 100.0f, 1 / 100.0f, 1 / 100.0f });
    dragon->buildBvh();
    objects->add(dragon);
}

void dragon_scene(shared_ptr<hittable_list> objects, bool scattering_medium = false) {
    auto material_ground = make_shared<lambertian>(color(0.6));
    //shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1), color(0.8));
    //auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<plane>("floor", point3(0.0, -0.4505, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.1, 0.025);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto tinted_glass = make_shared<dielectric>(1.5, medium);

    //auto m = make_shared<metal>(color(0.97, 0.96, 0.91));
    auto m = make_shared<lambertian>(color(glass_color));
    auto dragon = make_shared<model>("models/dragon_remeshed.ply", tinted_glass, 0);
    dragon->scene.instances[0].frame =
        yocto::translation_frame({ -0.5f, 0.0f, 0.0f }) *
        yocto::rotation_frame({ 0.0f, 1.0f, 0.0f }, yocto::radians(180.0f)) *
        yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, yocto::radians(-35.0f)) *
        yocto::rotation_frame({ 0.0f, 0.0f, 1.0f }, yocto::radians(90.0f)) *
        yocto::scaling_frame({ 1 / 100.0f, 1 / 100.0f, 1 / 100.0f });
    dragon->buildBvh();
    objects->add(dragon);
}

void monk_scene(shared_ptr<hittable_list> objects, bool scattering_medium = false) {
    //auto material_ground = make_shared<lambertian>(color(0.75));
    shared_ptr<texture> checker = make_shared<checker_texture>(color(0.1), color(0.8));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<plane>("floor", point3(0.0, -0.505, 0.0), vec3(0.0, 1.0, 0.0), material_ground));

    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);

    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.05, 0.025);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto monk_mat = make_shared<dielectric>(1.5, medium);
    //auto monk_mat = make_shared<diffuse_subsurface_scattering>(glass_color, medium);
    //auto monk_mat = make_shared<lambertian>(glass_color);

    auto monk = make_shared<model>("models/LuYu-obj/LuYu-obj.obj", monk_mat, 0);
    monk->scene.instances[0].frame = yocto::translation_frame(toYocto(point3(0.0, -0.5, -0.5)))
        * yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, -yocto::pif / 2)
        * yocto::scaling_frame({ 0.01f, 0.01f, 0.01f });
    monk->buildBvh();
    objects->add(monk);
}

void monk_debug(shared_ptr<hittable_list> objects) {
    // no floor
    // passthrough material
    //auto m = make_shared<passthrough>();

    // complex material
    bool scattering_medium = false;
    float c = 1.0;  // this allows us to adjust the filter color without changing the hue
    color glass_color(0.27 * c, 0.49 * c, 0.42 * c);
    shared_ptr<Medium> medium{};
    if (scattering_medium)
        medium = make_shared<IsotropicScatteringMedium>(glass_color, 0.1, 0.025);
    else
        medium = make_shared<NoScatterMedium>(glass_color, 0.25);
    auto m = make_shared<dielectric>(1.5, medium);

    auto monk = make_shared<model>("models/LuYu-obj/LuYu-obj.obj", m, 0);
    monk->scene.instances[0].frame = yocto::translation_frame(toYocto(point3(0.0, -0.5, -0.5)))
        * yocto::rotation_frame({ 1.0f, 0.0f, 0.0f }, -yocto::pif / 2)
        * yocto::scaling_frame({ 0.01f, 0.01f, 0.01f });
    monk->buildBvh();
    objects->add(monk);
}

void metal_ball(shared_ptr<hittable_list> objects) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);
    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    //auto medium = make_shared<NoScatterMedium>(color(0.8, 0.3, 0.3), 0.25);
    auto met = make_shared<metal>(color(0.8, 0.6, 0.2), 0.1);
    auto ball = make_shared<sphere>("glass_ball", point3(0.0, 0.1, -1.0), 0.5, met);
    objects->add(ball);
}

void three_spheres_with_medium(shared_ptr<hittable_list> objects, shared_ptr<hittable> &light, bool add_light) {
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    auto material_ground = make_shared<lambertian>(checker);

    auto material_right = make_shared<dielectric>(1.5);
    auto material_left = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    objects->add(make_shared<sphere>("floor", point3(0.0, -100.5, -1.0), 100.0, material_ground));

    auto white = make_shared<lambertian>(color(0.73, 0.73, 0.73));
    shared_ptr<hittable> sphere1 = make_shared<sphere>("center_ball", point3(0.0, 0.0, -1.0), 0.5, white);
    objects->add(make_shared<constant_medium>(sphere1, 10.0, color(.99, .93, .93)));

    if (add_light) {
        auto material_light = make_shared<diffuse_light>(color(10.0, 10.0, 10.0));
        light = make_shared<sphere>("light", point3(0.0, 2.0, 0.0), 0.1, material_light);
        objects->add(light);
    }

    objects->add(make_shared<sphere>("left_ball", point3(-1.0, 0.0, -1.0), 0.5, material_left));

    objects->add(make_shared<sphere>("right_ball_inner", point3(1.0, 0.0, -1.0), -0.49, material_right));
    objects->add(make_shared<sphere>("right_ball_outer", point3(1.0, 0.0, -1.0), 0.5, material_right));
}

shared_ptr<yocto::scene_model> init_scene(shared_ptr<hittable_list> world) {
    auto scene = make_shared<yocto::scene_model>();
    {
        // sphere shape
        {
            auto shape = yocto::scene_shape{};
            // make shape with 8 steps in resolution and scale of 1
            auto quads = yocto::vector<yocto::vec4i>{};
            yocto::make_sphere(quads, shape.positions, shape.normals, shape.texcoords, 8, 1, 1);
            shape.triangles = quads_to_triangles(quads);
            scene->shapes.push_back(shape);
        }
        // box shape
        {
            auto shape = yocto::scene_shape{};
            // make shape with 8 steps in resolution and scale of 1
            auto quads = yocto::vector<yocto::vec4i>{};
            yocto::make_cube(quads, shape.positions, shape.normals, shape.texcoords, 1);
            shape.triangles = quads_to_triangles(quads);
            scene->shapes.push_back(shape);
        }
        scene->materials.push_back({}); // create a black material directly

        for (auto o : world->objects) {
            // only supports spheres for now and constant_medium that uses sphere as a boundary
            if (auto cm = dynamic_pointer_cast<constant_medium>(o)) {
                o = cm->boundary;
            }
            else if (auto s = dynamic_pointer_cast<sphere>(o)) {
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
    auto cb = std::make_shared<callback::print_callback>(verbose);
    pt->DebugPixel(x, y, spp, cb);
}

void inspect_all(shared_ptr<tracer> pt, unsigned spp, bool verbose = false, bool stopAtFirst = false) {
    //auto cb = std::make_shared<callback::validate_model>("models/LuYu-obj/LuYu-obj.obj_model");
    auto cb = std::make_shared<callback::validate_model>("models/dragon_remeshed.ply_model", stopAtFirst);
    //auto cb = std::make_shared<callback::highlight_element>(1);
    pt->Render(spp, cb);
    cb->digest(cerr) << std::endl;
}

void window_debug(shared_ptr<tracer> pt, shared_ptr<hittable_list> world, shared_ptr<camera> cam, shared_ptr<yocto::color_image> image, unsigned x, unsigned y, unsigned spp) {
    // now display a window
    vec3 lookat = cam->getLookAt();
    vec3 lookfrom = cam->getLookFrom();

    auto s = init_scene(world);
    auto at = glm::vec3(lookat[0], lookat[1], lookat[2]);
    auto from = glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2]);
    auto w = tool::create_window(image, pt, s, at, from);

    w->debugPixel(x, y, spp);
    w->render();
}

void render(shared_ptr<tracer> pt, shared_ptr<hittable_list> world, shared_ptr<camera> cam, shared_ptr<yocto::color_image> image, int spp) {
    vec3 lookat = cam->getLookAt();
    vec3 lookfrom = cam->getLookFrom();

    auto s = init_scene(world);
    auto at = glm::vec3(lookat[0], lookat[1], lookat[2]);
    auto from = glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2]);
    auto w = tool::create_window(image, pt, s, at, from);

    //w->setCallback(std::make_shared<callback::validate_model>("models/dragon_remeshed.ply_model"));

    w->render(spp);
}

void offline_render(shared_ptr<tracer> pt, unsigned spp) {
    clock_t start = clock();
    auto cb = std::make_shared<callback::num_inters_callback>();
    pt->Render(spp, cb);
    clock_t stop = clock();
    double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
    cerr << "Rendering took " << timer_seconds << " seconds.\n" 
         << "Total intersections = " << cb->count << endl;
}

void offline_parallel_render(shared_ptr<tracer> pt, unsigned spp) {
    clock_t start = clock();
    pt->RenderParallel(spp);
    clock_t stop = clock();
    double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
    cerr << "Rendering took " << timer_seconds << " seconds.\n";
}

void debug_sss(
        shared_ptr<tracer> pt, 
        shared_ptr<hittable_list> world, 
        shared_ptr<camera> cam, 
        shared_ptr<yocto::color_image> image) {
    // render the scene offline while collecting the histogram
    // auto cb = make_shared<callback::num_medium_scatter_histo>(10, 20);
    std::shared_ptr<callback::callback> cb{};
    pt->Render(1, cb);

    vec3 lookat = cam->getLookAt();
    vec3 lookfrom = cam->getLookFrom();

    auto s = init_scene(world);
    auto at = glm::vec3(lookat[0], lookat[1], lookat[2]);
    auto from = glm::vec3(lookfrom[0], lookfrom[1], lookfrom[2]);
    auto w = tool::create_window(image, pt, s, at, from);

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
    auto rng = std::make_shared<xor_rnd>();
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

    auto world = make_shared<hittable_list>();
    auto light = shared_ptr<hittable>{};

    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    color background { 0, 0, 0 };
    bool use_envmap = true;
    bool add_light = false;
    bool russian_roulette = true;

    switch (15) {
        case 0:
            two_mediums(world, light, add_light);
            lookfrom = point3(3.40, 2.75, 3.12);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;
        case 1:
            diffuse_and_sss(world, light, add_light);
            lookfrom = point3(3.40, 2.75, 3.12);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;
        case 2:
            dielectric_and_sss(world, light, add_light);
            lookfrom = point3(3.40, 2.75, 3.12);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;
        case 3:
            two_glass_balls(world, light, add_light);
            lookfrom = point3(0.09, 3.44, 4.15);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;
        case 4:
            three_spheres_with_medium(world, light, add_light);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.70, 0.80, 1.00);
            break;
        case 5:
            glass_ball(world, true);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 6:
            simple_box(world, light, add_light);
            //lookfrom = point3(3, 2, 2);
            lookfrom = point3(3.68871, 1.71156, 3.11611);
            lookat = point3(0, 0, 0);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 7:
            three_spheres(world);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 8:
            metal_ball(world);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 9:
            multiple_glass_balls(world, light, add_light);
            lookfrom = point3(3, 2, 2);
            lookat = point3(0, 0, -1);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 10:
            monkey_scene(world, true);
            lookfrom = point3(2.95012, 3.84593, 6.67006);
            lookat = point3(0, 0.05, 0);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 11:
            monk_scene(world, true);
            lookfrom = point3(-8.49824, 3.01965, -2.37236);
            lookat = point3(0, 0.75, -0.75);
            vfov = 20.0;
            aperture = 0.1;
            background = color(0.6, 0.6, 0.7);
            break;
        case 12:
            glass_panels(world, false);
            lookfrom = point3(1.97006, 2.41049, 7.12357);
            lookat = point3(0, 0.5, 0);
            vfov = 20.0;
            aperture = 0.1;
            background = color(1.0, 1.0, 1.0);
            break;
        case 13:
            monkey_debug(world);
            lookfrom = point3(2.95012, 3.84593, 6.67006);
            lookat = point3(0, 0.05, 0);
            vfov = 20.0;
            aperture = 0.0;
            background = color(1.0, 1.0, 1.0);
            use_envmap = false;
            russian_roulette = false;
            break;
        case 14:
            monk_debug(world);
            lookfrom = point3(-8.49824, 3.01965, -2.37236);
            lookat = point3(0, 0.75, -0.75);
            vfov = 20.0;
            aperture = 0.0;
            background = color(1.0, 1.0, 1.0);
            use_envmap = false;
            russian_roulette = false;
            break;
        case 15:
            dragon_scene(world, false);
            lookfrom = point3(-4.35952, 2.64187, 4.06531);
            lookat = point3(0, 0.05, 0);
            vfov = 20.0;
            aperture = 0.0;
            background = color(0.6, 0.6, 0.7);
            russian_roulette = false; //TODO remove, just needed for debugging
            break;
        case 16:
            dragon_debug(world);
            lookfrom = point3(-4.35952, 2.64187, 4.06531);
            lookat = point3(0, 0.05, 0);
            vfov = 20.0;
            aperture = 0.0;
            background = color(1.0, 1.0, 1.0);
            use_envmap = false;
            russian_roulette = false;
            break;
    }

    // Camera
    vec3 vup{ 0, 1, 0 };

    auto cam = make_shared<camera>(lookfrom, lookat, vup, vfov, aspect_ratio, aperture);
    auto image = make_shared<yocto::color_image>(yocto::make_image(image_width, image_height, false));

    shared_ptr<EnvMap> envmap = nullptr;
    if (use_envmap) {
        //auto hdr_filename = "hdrs/christmas_photo_studio_04_1k.hdr";
        //auto hdr_filename = "hdrs/christmas_photo_studio_04_4k.exr";
        //auto hdr_filename = "hdrs/parched_canal_1k.exr";
        auto hdr_filename = "hdrs/large_corridor_4k.exr";
        envmap = make_shared<EnvMap>(hdr_filename);
    }

    // Render
    scene_desc scene{
        background,
        world,
        light,
        envmap
    };
    unsigned rr_depth = russian_roulette ? 3 : max_depth;
    auto pt = make_shared<pathtracer>(cam, image, scene, max_depth, rr_depth);
    if (!russian_roulette)
        std::cerr << "RUSSIAN ROULETTE DISABLED\n";

    debug_pixel(pt, 166, 257, 1, true);
    //inspect_all(pt, 1, false, false);
    //render(pt, world, cam, image, 0); // pass spp=0 to disable further rendering in the window
    // offline_render(pt);
    // offline_parallel_render(pt);
    window_debug(pt, world, cam, image, 166, 257, 1);
    //debug_sss(pt, world, cam, image);

    //save_image(image, "output/dragon-glass-1.png"); 

    // compare to reference image
    bool save_ref = false;
    bool compare_ref = false;

    auto raw = make_shared<RawData>(image->width, image->height);
    pt->getRawData(raw);
    if (compare_ref) {
        auto ref = make_shared<RawData>("reference.raw");
        std::cerr << "RMSE = " << raw->rmse(ref) << std::endl;
    }
    if (save_ref) {
        //TODO I should add a timestamp to the reference name to avoid corrupting a previous one
        raw->saveToFile("reference-dummy.raw");
    }
}
