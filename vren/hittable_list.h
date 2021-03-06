#pragma once

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class hittable_list : public hittable {
private:
    int used_object;

public:
    hittable_list(): hittable("list") {}
    hittable_list(shared_ptr<hittable> object): hittable("list") { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;

    virtual double pdf_value(const vec3& o, const vec3& v) const override {
        auto weight = 1.0 / objects.size();
        auto sum = 0.0;

        for (const auto& object : objects)
            sum += weight * object->pdf_value(o, v);

        return sum;
    }

    virtual vec3 random(const vec3& o, rnd& rng) override {
        auto int_size = static_cast<int>(objects.size());
        used_object = rng.random_int(0, int_size - 1);
        return objects[used_object]->random(o, rng);
    }

    virtual std::string pdf_name() const override {
        return objects[used_object]->name;
    }

    std::vector<shared_ptr<hittable>> objects;
};

bool hittable_list::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
    hit_record temp_rec;
    bool hit_anything = false;
    auto closest_so_far = t_max;

    for (const auto& object : objects) {
        if (object->hit(r, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
            rec.obj_ptr = object.get();
        }
    }

    return hit_anything;
}