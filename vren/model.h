#pragma once

#include <vector>
#include <memory>
#include <exception>
#include <iostream>
#include <time.h>

#include <yocto/yocto_shape.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_bvh.h>
#include <yocto/yocto_geometry.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_cli.h>

#include "hittable.h"
#include "rnd.h"

class model : public hittable {
private:
    void buildBvh(bool embree) {
        yocto::print_progress_begin("Build BVH");
        bvh = yocto::make_bvh(scene, true, embree);
        yocto::print_progress_end();
    }

public:
    model(std::string filename, std::shared_ptr<material> mat, yocto::frame3f frame = {}, bool embree = true, 
            int subdivisions = 0, bool catmullclark = true) :
            hittable(filename + "_model"), mat_ptr(mat) {
        auto error = std::string{};
        auto shape = yocto::scene_shape{};
        if (!yocto::load_shape(filename, shape, error)) {
            throw std::runtime_error(error);
        }

        // remove existing normals, we don't want to be interpolating them for now
        shape.normals.clear();

        if (frame != yocto::identity3x4f) {
            yocto::print_info("frame provided. Shape will be transformed to world coordinates");
            for (auto& p : shape.positions) p = yocto::transform_point(frame, p);
        }

        if (subdivisions > 0) {
            shape = yocto::subdivide_shape(shape, subdivisions, catmullclark);
        }

        if (!shape.quads.empty())
            shape.triangles = yocto::quads_to_triangles(shape.quads);
        shape.quads.clear();

        scene.shapes.push_back(shape);
        scene.materials.push_back({}); // create a black material directly

        auto instance = yocto::scene_instance{};
        instance.shape = 0;
        instance.material = 0;
        scene.instances.push_back(instance);

        buildBvh(embree);
    }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        auto ray = yocto::ray3f{ 
            toYocto(r.origin()), 
            toYocto(r.direction()),
            (float)t_min,
            (float)t_max
        };
        auto isec = yocto::intersect_bvh(bvh, scene, ray);
        if (!isec.hit)
            return false;

        const yocto::scene_instance& instance = scene.instances[isec.instance];

        rec.t = isec.distance;

        // first compute geometric normal
        auto outward_normal = yocto::eval_normal(scene, instance, isec.element, isec.uv);
        rec.set_face_normal(r, fromYocto(outward_normal));

        // then compute intersection point using uv coordinates to reduce floating point imprecision effects
        auto p = yocto::eval_position(scene, instance, isec.element, isec.uv);
        //rec.p = fromYocto(yocto::offset_ray(p, outward_normal));
        rec.p = fromYocto(p);

        rec.u = isec.uv.x;
        rec.v = isec.uv.y;
        rec.element = isec.element;
        rec.mat_ptr = mat_ptr;

        return true;
    }

    yocto::scene_model scene;
    yocto::bvh_scene bvh;
    std::shared_ptr<material> mat_ptr;
};