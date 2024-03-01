#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

#include <string.h>
#include <time.h>

using namespace std;

int main(int argc, char* argv[])
{
    clock_t start, end;
    start = clock();

    const unsigned max_y = max(0, (argc > 1) ? atoi(argv[1]) : 0); 
    const unsigned max_x = (max_y + 7) / 8;
    const unsigned max_iterations = 50;
    const double limit = 2.0;
    const double limit_sq = 4.0;

    FILE* out = (argc == 3) ? fopen(argv[2], "wb") : stdout;

    unsigned char *buffer = (unsigned char*) malloc(max_x*max_y);

    double *xvalues = (double *) malloc(max_x*8*sizeof(double));

    for (unsigned x = 0; x < max_x*8; x++)
    {
        xvalues[x] = (2.0 * x) / max_y - 1.5; // range from -1,5 to 0.5
    }

#pragma omp parallel for
    for (unsigned y = 0; y < max_y; ++y)
    {
        const double curr_im = (2.0 * y) / max_y - 1.0; // range from -i to i
        unsigned char *buffer_line = buffer + y*max_x;

        double curr_r[8];
        double r[8] = {0.0};
        double im[8] = {0.0};
        double rsq[8];
        double imsq[8];

        for (unsigned x = 0; x < max_x; ++x)
        {
            memcpy(curr_r, &xvalues[8*x], 8*sizeof(double));
            memset(r, 0.0, 8*sizeof(double));
            memset(im, 0.0, 8*sizeof(double));

            unsigned char temp = 0xff;
            for (unsigned i=0; i<max_iterations && temp; i++) {
                unsigned j = 0;
                for (unsigned char bit = 0x80; bit; bit >>= 1) {
                    if (temp & bit) {
                        rsq[j] = r[j]*r[j];
                        imsq[j] = im[j]*im[j];
                        if (rsq[j] + imsq[j] > 4.0) {
                            temp ^= bit;
                            continue;
                        }
                        im[j] = 2.0 * r[j]*im[j] + curr_im;
                        r[j] = rsq[j] - imsq[j] + curr_r[j];
                    }
                    j++;
                }
            }
            buffer_line[x] = temp;
        }
    }

    fprintf(out, "P4\n%u %u\n", max_x*8, max_y);
    fwrite(buffer, max_x*max_y, 1, out);

    if (out != stdout)
    {
        fclose(out);
    }

    free(xvalues);
    free(buffer);

    end = clock();
    printf("%f\n", (double) (end-start)/CLOCKS_PER_SEC);

    return 0;
}
    