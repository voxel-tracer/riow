#pragma once

#include "rtweekend.h"

class Medium {
public:
    virtual color transmittance(double distance) const = 0;
};

class NoScatterMedium : public Medium {
public:
    NoScatterMedium(color c, double d) : aColor(c), aDistance(d) {}

    virtual color transmittance(double distance) const override {
        return pow(aColor, distance / aDistance);
    }

    color aColor; // absorption color
    double aDistance; // absorption at distance
};