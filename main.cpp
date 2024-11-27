/*
Multithreaded Gaussian Blur
---------------------------
A parallel implementation of Gaussian blur using pthreads to process the image
in 4 concurrent segments. Supports 24-bit BMP files.

Usage: ./blur <file_name>.bmp <blur_radius>
Outputs: output.bmp
*/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

// http://www.dragonwins.com/domains/getteched/bmp/bmpfileformat.htm
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint32_t reserved;
    uint32_t bfOffBits;        // offset of the start of the Pixel Data section relative to the start of the file
    uint32_t biSize;           // Header Size - Must be at least 40
    uint32_t biWidth;          // Image width in pixels
    uint32_t biHeight;         // Image height in pixels
    uint16_t biPlanes;         // Must be 1
    uint16_t biBitCount;       // Bits per pixel - 1, 4, 8, 16, 24, or 32
    uint32_t biCompression;    // Compression type (0 = uncompressed)
    uint32_t biSizeImage;      // Image Size - may be zero for uncompressed images
    uint32_t biXPelsPerMeter;  // Preferred resolution in pixels per meter
    uint32_t biYPelsPerMeter;  // Preferred resolution in pixels per meter
    uint32_t biClrUsed;        // Number Color Map entries that are actually used
    uint32_t biClrImportant;   // Number of significant colors
};
#pragma pack(pop)

struct Pixel {
    uint8_t red, green, blue;
};

struct BlurParams {
    BMPHeader header;
    Pixel *image;
    Pixel *blurred_image;
    vector<vector<double>> kernel;
    int start;
    int end;
};

// check if it ends in .bmp
bool is_valid_file(string &filename);

bool read_bmp_file(ifstream &file, BMPHeader &header);

void load_image(ifstream &file, BMPHeader &header, Pixel *image);

void save_image(ofstream &file, BMPHeader &header, Pixel *image);

double gaussian(int x, int y, double sigma);

vector<vector<double>> gen_gaussian_kernel(int kernel_size);

void *apply_blur(void *params);

// first argument is usually executing "./blur"
int main(int argc, char *argv[]) {
    if (argc <= 2) {
        cerr << "Error: Make sure to specify the BMP file you would like to blur along with the radius of the blur.\n";
        cerr << "\t Usage: ./main <file_name>.bmp <blur_radius>\n";
        return 1;
    }

    string filename = argv[1];
    if (!is_valid_file(filename)) {
        cerr << "Error: The file specified does not end with \".bmp\"\n";
        return 1;
    }

    int radius = atoi(argv[2]);

    ifstream file(filename, ios::binary);

    if (!file) {
        cerr << "Error: Unable to open file " << filename << '\n';
        return 1;
    }

    BMPHeader header;
    if (!read_bmp_file(file, header)) {
        return 1;
    }

    int width = header.biWidth, height = header.biHeight;

    // allocate enough memory for the image
    Pixel *image = (Pixel *)malloc(sizeof(Pixel) * width * height);

    load_image(file, header, image);
    file.close();

    auto kernel = gen_gaussian_kernel(radius);

    Pixel *blurred_image = (Pixel *)malloc(sizeof(Pixel) * width * height);

    pthread_t thread1, thread2, thread3, thread4; /*thread identifier*/
    pthread_attr_t attr;                          /*set of thread attributes */
    pthread_attr_init(&attr);                     /* set the default attributes of the thread */

    BlurParams params1 = {header, image, blurred_image, kernel, 0, width * height / 4};
    BlurParams params2 = {header, image, blurred_image, kernel, width * height / 4, width * height / 2};
    BlurParams params3 = {header, image, blurred_image, kernel, width * height / 2, 3 * width * height / 4};
    BlurParams params4 = {header, image, blurred_image, kernel, 3 * width * height / 4, width * height};

    pthread_create(&thread1, &attr, apply_blur, &params1);
    pthread_create(&thread2, &attr, apply_blur, &params2);
    pthread_create(&thread3, &attr, apply_blur, &params3);
    pthread_create(&thread4, &attr, apply_blur, &params4);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);

    ofstream output_file("output.bmp");
    save_image(output_file, header, blurred_image);
    output_file.close();

    free(image);
    free(blurred_image);

    return 0;
}

void *apply_blur(void *params) {
    BlurParams *blur_params = (BlurParams *)params;
    BMPHeader header = blur_params->header;
    Pixel *image = blur_params->image;
    Pixel *blurred_image = blur_params->blurred_image;
    vector<vector<double>> kernel = blur_params->kernel;

    int kernel_size = kernel.size();
    int radius = kernel_size / 2;

    int width = header.biWidth;
    int height = header.biHeight;

    for (int i = blur_params->start; i < blur_params->end; i++) {
        double red = 0, green = 0, blue = 0;

        for (int r = -radius; r <= radius; r++) {
            for (int c = -radius; c <= radius; c++) {
                int x = i % width, y = i / width;
                if (x + c < 0 || x + c >= width || y + r < 0 || y + r >= height) {
                    continue;
                }

                Pixel *sample = image + (y + r) * width + (x + c);

                double weight = kernel[r + radius][c + radius];

                red += sample->red * weight;
                green += sample->green * weight;
                blue += sample->blue * weight;
            }
        }

        Pixel *blurred_pixel = blurred_image + i;
        blurred_pixel->red = red;
        blurred_pixel->green = green;
        blurred_pixel->blue = blue;
    }

    pthread_exit(0);
}

// https://en.wikipedia.org/wiki/Gaussian_function
double gaussian(int x, int y, double sigma) {
    return (1.0 / (2.0 * M_PI * sigma * sigma)) * exp(-(x * x + y * y) / (2 * sigma * sigma));
}

// https://en.wikipedia.org/wiki/Gaussian_function
vector<vector<double>> gen_gaussian_kernel(int radius) {
    int kernel_size = 2 * radius + 1;
    vector<vector<double>> kernel(kernel_size, vector<double>(kernel_size, 0.0));

    // nvidia uses sigma = radius / 3.0
    // https://stackoverflow.com/questions/17841098/gaussian-blur-standard-deviation-radius-and-kernel-size
    // https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-40-incremental-computation-gaussian
    double sigma = radius / 3.0;

    double sum = 0;

    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            double value = gaussian(i, j, sigma);
            kernel[i + radius][j + radius] = value;
            sum += value;
        }
    }

    // normalize
    for (int i = 0; i < kernel_size; i++) {
        for (int j = 0; j < kernel_size; j++) {
            kernel[i][j] /= sum;
        }
    }

    return kernel;
}

void save_image(ofstream &file, BMPHeader &header, Pixel *image) {
    file.write(reinterpret_cast<char *>(&header), sizeof(BMPHeader));

    int row_size = header.biWidth * sizeof(Pixel);
    int padding = (4 - (row_size % 4)) % 4;

    for (int y = 0; y < (int)header.biHeight; y++) {
        file.write((char *)(image + y * header.biWidth), row_size);
        char padding_bytes[3] = {0};  // BMP padding is zeroed
        file.write(padding_bytes, padding);
    }
}

void load_image(ifstream &file, BMPHeader &header, Pixel *image) {
    file.seekg(header.bfOffBits, ios::beg);

    // https://en.wikipedia.org/wiki/BMP_file_format#Pixel_storage
    // http://www.dragonwins.com/domains/getteched/bmp/bmpfileformat.htm#The%20Pixel%20Data
    int row_size = header.biWidth * 3;  // 3 is the number of bytes in each pixel for the 24 bit count

    // we do another % 4 to make sure we don't include 4 - 0 = 4 since its
    // already divisible by 4
    int padding = (4 - (row_size % 4)) % 4;

    for (int y = 0; y < (int)header.biHeight; y++) {
        file.read((char *)(image + y * header.biWidth), row_size);
        file.seekg(padding, ios::cur);
    }
}

bool read_bmp_file(ifstream &file, BMPHeader &header) {
    // Read the BMP header
    file.read(reinterpret_cast<char *>(&header), sizeof(BMPHeader));

    if (!file) {
        cerr << "Error: Unable to read BMP header.\n";
        return false;
    }

    // Validate BMP file type
    // BM in little-endian
    if (header.bfType != 0x4D42) {
        cerr << "Error: Not a valid BMP file.\n";
        return false;
    }

    if (header.biBitCount != 24) {
        cerr << "We only support 24-bit BMP files!\n";
        return false;
    }

    return true;
}

bool is_valid_file(string &filename) {
    const string suffix = ".bmp";

    if (filename.size() < suffix.size()) {
        return false;
    }
    return filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
}