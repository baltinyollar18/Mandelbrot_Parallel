
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <tuple>
#include <time.h>
#include <cmath>
#include <complex>
#include <chrono>

#include <thread>

#include "mandelbrot-helpers.hpp"

using namespace std;

const double X_MIN = -2.0;
const double X_MAX = 1.0;
const double Y_MIN = -1.5;
const double Y_MAX = 1.5;


/**
 * Static work allocation
 *
 * @param[inout] image
 * @param[in] thread_id
 * @param[in] num_threads
 * @param[inout] pixels_inside
 *
*/


void worker_static(Image& image, int thread_id, int num_threads, int& pixels_inside) {
    int rows_per_thread = image.height / num_threads;
    int start_row = thread_id * rows_per_thread;
    int end_row = (thread_id + 1) * rows_per_thread;

    int color = (255*thread_id)/num_threads; // 0 means { 0, 0, 0 } -> black

 // static work distribution (see https://moped.par.univie.ac.at/test.html?taskcode=cpp10)
    for (int row = start_row; row < end_row; row++) {
        for (int col = 0; col < image.width; ++col) {
            double dx = X_MIN + (col / static_cast<double>(image.width)) * (X_MAX - X_MIN);
            double dy = Y_MIN + (row / static_cast<double>(image.height)) * (Y_MAX - Y_MIN);

            std::complex<double> c(dx, dy);
            std::array<int, 3>& pixel = image[row][col];

            if (mandelbrot_kernel(c, pixel, color)) {
                pixels_inside++;
            }
            
            image[row][col] = pixel;

        }
    }
}

/**
 * Dynamic work allocation
 *
 * @param[inout] image
 * @param[in] thread_id
 * @param[in] num_threads
 * @param[inout] idx
 * @param[inout] pixel_inside
 *
 * @return (pixels_inside could also be used as a return value)
*/
void worker_dynamic(Image& image, int thread_id, int num_threads, int& pixels_inside) {

    int color = (255*thread_id)/num_threads; // 0 means { 0, 0, 0 } -> black


    // dynamic - put your code here (see https://moped.par.univie.ac.at/test.html?taskcode=cpp11)
    for (int row = thread_id; row < image.height; row += num_threads) {
        for (int col = 0; col < image.width; ++col) {
            double dx = X_MIN + (col / static_cast<double>(image.width)) * (X_MAX - X_MIN);
            double dy = Y_MIN + (row / static_cast<double>(image.height)) * (Y_MAX - Y_MIN);

            std::complex<double> c(dx, dy);
            std::array<int, 3>& pixel = image[row][col];

            if (mandelbrot_kernel(c, pixel, color)) // the actual mandelbrot kernel, 0 means black color for mandelbrot pixels
                pixels_inside++;
            

            image[row][col] = pixel;
        }
    }
}


int main(int argc, char **argv)
{
    // arguments
    int num_threads = std::thread::hardware_concurrency();
    std::string work_allocation = "static";
    int print_level = 2; // 0 exec time only, 1 exec time and pixel count, 2 exec time, pixel cound and work allocation
    
    // height and width of the output image
    int width = 960, height = 720;

    parse_args(argc, argv, num_threads, work_allocation, height, width, print_level);

    double time;
    int pixels_inside = 0;

    // Generate Mandelbrot set in this image
    Image image(height, width);

    auto t1 = chrono::high_resolution_clock::now();

    auto start_time = std::chrono::high_resolution_clock::now();

    if (work_allocation == "static") {
        vector<thread> threads;
        for (int tid = 0; tid < num_threads; ++tid) {
            threads.push_back(thread(worker_static, ref(image), tid, num_threads, ref(pixels_inside)));
        }
        for (auto& t : threads) {
            t.join();
        }
    } else if (work_allocation == "dynamic") {
        vector<thread> threads;
        for (int tid = 0; tid < num_threads; ++tid) {
            threads.push_back(thread(worker_dynamic, ref(image), tid, num_threads, ref(pixels_inside)));
        }
        for (auto& t : threads) {
            t.join();
        }
    } else {
        cerr << "Invalid work allocation type. Use 'static' or 'dynamic'." << endl;
        return 1;
    }

    auto t2 = chrono::high_resolution_clock::now();

    // save image
    image.save_to_ppm("mandelbrot.ppm");

    if ( print_level >= 2) cout << "Work allocation: " << work_allocation << endl;
    if ( print_level >= 1 ) cout << "Total Mandelbrot pixels: " << pixels_inside << endl;
    cout << chrono::duration<double>(t2 - t1).count() << endl;

    return 0;
}
