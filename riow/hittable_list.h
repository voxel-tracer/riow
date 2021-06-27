#pragma once

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class hittable_list : public hittable {
public:
    hittable_list(): hittable("list") {}
    hittable_list(shared_ptr<hittable> object): hittable("list") { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override;
    virtual double pdf_value(const vec3& o, const vec3& v) const override;
    virtual vec3 random(const vec3& o, shared_ptr<rnd> rng) const override;

    std::vector<shared_ptr<hittable>> objects;
};

bool hittable_list::hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const {
    hit_record temp_rec;
    bool hit_anything = false;
    auto closest_so_far = t_max;

    for (const auto& object : objects) {
        if (object->hit(r, t_min, closest_so_far, temp_rec, rng)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
            rec.obj_ptr = object;
        }
    }

    return hit_anything;
}

double hittable_list::pdf_value(const point3& o, const vec3& v) const {
    auto weight = 1.0 / objects.size();
    auto sum = 0.0;

    for (const auto& object : objects)
        sum += weight * object->pdf_value(o, v);

    return sum;
}


vec3 hittable_list::random(const vec3& o, shared_ptr<rnd> rng) const {
    auto int_size = static_cast<int>(objects.size());
    return objects[rng->random_int(0, int_size - 1)]->random(o, rng);
}
