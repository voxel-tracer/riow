#pragma once

#include <memory>

#include "hittable.h"
#include "ray.h"
#include "rnd.h"

class box : public hittable {
public:
    box(std::string name, const point3& center, const vec3& size, std::shared_ptr<material> m) :
        hittable(name + "_box"), bmin(center - size / 2), bmax(center + size / 2), mat_ptr(m) {}

    virtual bool hit(const ray& r, const double t_min, const double t_max, hit_record& rec) const override {
        float tmin = -INFINITY;
        float tmax = INFINITY;
        int amin = -1;
        int amax = -1;

        for (int a = 0; a < 3; a++) {
            double invD = 1.0 / r.direction()[a];
            double t0 = (bmin[a] - r.origin()[a]) * invD;
            double t1 = (bmax[a] - r.origin()[a]) * invD;
            if (invD < 0) {
                // swap(t0, t1)
                double tmp = t0;
                t0 = t1;
                t1 = tmp;
            }
            if (t0 > tmin) {
                tmin = t0;
                amin = a;
            }
            if (t1 < tmax) {
                tmax = t1;
                amax = a;
            }
            if (tmax < tmin) return false;
        }

        // [tmin, tmax] are the smallest and biggest t values on the ray line
        if ((tmin >= t_max) || (tmax <= t_min) || (tmin <= t_min && tmax > t_max)) 
            return false;
        // at this point we are guaranteed to find an intersection with the box
        bool useMax = tmin <= t_min; // is the ray inside the box ?

        rec.t = useMax ? tmax : tmin;
        rec.p = r.at(rec.t);

        vec3 center = (bmax + bmin) / 2;
        vec3 norm{};
        norm[useMax ? amax : amin] = (rec.p - center)[useMax ? amax : amin];
        rec.set_face_normal(r, unit_vector(norm));

        rec.u = 0;
        rec.v = 0;

        rec.mat_ptr = mat_ptr;

        return true;

    }

    const vec3 bmin;
    const vec3 bmax;
    const std::shared_ptr<material> mat_ptr;
};