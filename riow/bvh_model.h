#pragma once

#include <yocto/yocto_sceneio.h>
#include <iostream>

#include "hittable.h"
#include "bvh.h"

class BVHModel : public hittable {
private:
    std::shared_ptr<BVHAccel> bvh;
    std::shared_ptr<material> mat;

public:
    BVHModel(std::string filename, std::shared_ptr<material> mat, yocto::frame3f frame = {}) : 
            hittable(filename + "_bvhmodel"), mat(mat) {
        auto error = std::string{};
        if (!yocto::load_shape(filename, shape, error)) {
            throw std::runtime_error(error);
        }

        // remove existing normals, we don't want to be interpolating them for now
        shape.normals.clear();

        if (frame != yocto::identity3x4f) {
            std::cerr << "frame provided. Shape will be transformed to world coordinates\n";
            for (auto& p : shape.positions) p = yocto::transform_point(frame, p);
        }

        // Triangulate quads if necessary
        if (!shape.quads.empty()) {
            shape.triangles = yocto::quads_to_triangles(shape.quads);
            shape.quads.clear();
        }

        // Print shape stats
        auto stats = yocto::shape_stats(shape);
        for (auto stat : stats) std::cerr << stat << std::endl;

        bvh = BVHAccel::Create(shape, BVHAccel::SplitMethod::SAH);
    }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        yocto::ray3f r3f = {
            toYocto(r.origin()),
            toYocto(r.direction()),
            (float)t_min,
            (float)t_max
        };
        yocto::vec2f uv;
        float dist;
        int element;
        if (bvh->hit(r3f, uv, element, dist)) {
            rec.t = dist;

            // first compute geometric normal
            auto outward_normal = yocto::eval_normal(shape, element, uv);
            rec.set_face_normal(r, fromYocto(outward_normal));

            // then compute intersection point using uv coordinates to reduce floating point imprecision effects
            auto p = yocto::eval_position(shape, element, uv);
            //rec.p = fromYocto(yocto::offset_ray(p, outward_normal));
            rec.p = fromYocto(p);

            rec.u = uv.x;
            rec.v = uv.y;
            rec.element = element;
            rec.mat_ptr = mat;

            return true;
        }

        return false;
    }

    yocto::scene_shape shape;
};