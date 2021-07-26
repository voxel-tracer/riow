#pragma once

#include <vector>
#include <memory>
#include <exception>
#include <iostream>

#include <yocto/yocto_shape.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_bvh.h>
#include <yocto/yocto_geometry.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>

#include "hittable.h"
#include "rnd.h"

class model : public hittable {
public:
    model(std::string filename, const point3& center, std::shared_ptr<material> mat, int subdiv_lvl = 0, float scale = 1.0f) :
            hittable(filename + "_model"), mat_ptr(mat) {
        auto error = std::string{};
        auto shape = yocto::scene_shape{};
        if (!yocto::load_shape(filename, shape, error)) {
            throw std::runtime_error(error);
        }

        if (subdiv_lvl > 0) {
            auto [squads, spositions] = yocto::subdivide_catmullclark(shape.quads, shape.positions, subdiv_lvl);
            shape.triangles = yocto::quads_to_triangles(squads);
            shape.positions = spositions;
        }
        else {
            shape.triangles = yocto::quads_to_triangles(shape.quads);
        }
        shape.quads.clear();

        scene.shapes.push_back(shape);
        scene.materials.push_back({}); // create a black material directly

        auto instance = yocto::scene_instance{};
        instance.shape = 0;
        instance.material = 0;
        instance.frame = yocto::translation_frame(toYocto(center))
            * yocto::scaling_frame({ scale, scale, scale });
        scene.instances.push_back(instance);

        auto stats = yocto::scene_stats(scene);
        for (auto stat : stats) std::cerr << stat << std::endl;

        bvh = yocto::make_bvh(scene, true);
    }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, std::shared_ptr<rnd> rng) const override {
        auto ray = yocto::ray3f{ toYocto(r.origin()), toYocto(r.direction()) };
        auto isec = yocto::intersect_bvh(bvh, scene, ray);
        if (!isec.hit)
            return false;

        rec.t = isec.distance;
        rec.p = r.at(rec.t);

        auto normal = yocto::eval_normal(scene, scene.instances[isec.instance], isec.element, isec.uv);
        vec3 outward_normal = fromYocto(normal);
        rec.set_face_normal(r, outward_normal);

        rec.u = isec.uv.x;
        rec.v = isec.uv.y;
        rec.mat_ptr = mat_ptr;

        return true;
    }

    yocto::scene_model scene;
    yocto::bvh_scene bvh;
    std::shared_ptr<material> mat_ptr;
};