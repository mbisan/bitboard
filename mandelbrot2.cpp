#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

#include <string.h>

using namespace std;

// struct Complex {
// double r, im, rsq, imsq;

// Complex(double r, double im) : r(r), im(im) {
//     rsq = r*r;
//     imsq = im*im;
// }

// Complex operator+(Complex other) {
//     return {r+other.r, im+other.im};
// }

// Complex operator+(double other) {
//     return {r+other, im};
// }

// Complex operator*(Complex other) {
//     return {r*other.r - im*other.im, r*other.im + im*other.r};
// }

// Complex operator*(double other) {
//     return {r*other, im*other};
// }

// bool mandelbrotRecursion() {
//     // updates locally
//     rsq = r*r;
//     imsq = im*im;
//     if (rsq + imsq > 4.0) return true;
//     r = rsq - imsq + 1.0;
//     im = (-2.0)*(r*im);

//     return false;
// }

// };

// template<typename T>
// class smartPointer {
// public:

// T *bytesArray;

// smartPointer(const unsigned n) {
//     bytesArray = (T*) malloc(n * sizeof(T)); // uninitialized memory
// }

// ~smartPointer() {
//     free(bytesArray);
// }

// };

int main(int argc, char* argv[])
{
    const unsigned max_y = max(0, (argc > 1) ? atoi(argv[1]) : 0); 
    const unsigned max_x = (max_y + 7) / 8;
    const unsigned max_iterations = 50;
    const double limit = 2.0;
    const double limit_sq = 4.0;

    FILE* out = (argc == 3) ? fopen(argv[2], "wb") : stdout;

    unsigned char buffer[max_x*max_y] = {0};
    memset(buffer, 0xff, max_x*max_y);

    double xvalues[max_x*8];

    for (unsigned x = 0; x < max_x*8; x++)
    {
        xvalues[x] = (2.0 * x) / max_y - 1.5; // range from -1,5 to 0.5
    }

//#pragma omp parallel for
    for (unsigned y = 0; y < max_y; ++y)
    {
        const double curr_im = (2.0 * y) / max_y - 1.0; // range from -i to i
        for (unsigned x = 0; x < max_x; ++x)
        {
            double curr_r_values[8];
            memcpy(curr_r_values, &xvalues[8*x], 8*sizeof(double));
            for (unsigned bit = 0; bit < 8; bit++) {
                const double curr_r = curr_r_values[bit];
                
                double r = 0.0;
                double im = 0.0; 
                double rsq, imsq;

                // max iterations + 1 since the condition is checked before the recursion is applied, which gives the same result
                for (unsigned i = 0; i < max_iterations; ++i) 
                {
                    rsq = r*r;
                    imsq = im*im;
                    if (rsq + imsq > 4.0) {
                        buffer[y*max_x + x] ^= 0x80 >> bit;
                        break;
                    }
                    r = rsq - imsq + curr_r;
                    im = 2.0 * r*im + curr_im;
                }                
            }
        }
    }

    fprintf(out, "P4\n%u %u\n", max_x*8, max_y);
    fwrite(buffer, max_x*max_y, 1, out);

    if (out != stdout)
    {
        fclose(out);
    }

    return 0;
}
    