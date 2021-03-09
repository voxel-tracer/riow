// riow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

using namespace std;

int main()
{
    // Image

    const int image_width = 256;
    const int image_height = 256;

    // Render

    cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (auto j = image_height - 1; j >= 0; --j) {
        cerr << "\rScanlines remaining: " << j << ' ' << flush;
        for (auto i = 0; i != image_width; ++i) {
            auto r = double(i) / (image_width - 1);
            auto g = double(j) / (image_height - 1);
            auto b = 0.25;

            int ir = static_cast<int>(255.99 * r);
            int ig = static_cast<int>(255.99 * g);
            int ib = static_cast<int>(255.99 * b);

            cout << ir << ' ' << ig << ' ' << ib << endl;
        }
    }
}
