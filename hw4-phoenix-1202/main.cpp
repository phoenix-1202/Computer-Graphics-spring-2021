#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

#define _USE_MATH_DEFINES

using namespace std;

struct Image {
public:
    explicit Image(const char* infile) {
        FILE* file = fopen(infile, "rb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }
        char f, endOfLine;
        int w, h, maxColor, t;
        if (fscanf(file, "%c%d%d%d%d%c", &f, &t, &w, &h, &maxColor, &endOfLine) == 6 &&
            f == 'P' && t == 5 && maxColor == 255 && endOfLine == '\n') {
            width = w;
            height = h;
            type = t;
            size = width * height * pixelSize;
            data = new (nothrow) unsigned char[size];
            if (data == nullptr) {
                cerr << "Cannot open image file: not enough memory";
                exit(1);
            }
            if (fread(data, 1, size, file) != size) {
                cerr << "Problems with reading the image file";
                exit(1);
            }
        } else {
            cerr << "Incorrect image format: must be P5 or P6 type with maxColorValue = 255";
            exit(1);
        }
        fclose(file);
    }

    void set_new_sizes(int w, int h) {
        newHeight = h;
        newWidth = w;
        newData = new (nothrow) unsigned char[newHeight * newWidth];
        if (newData == nullptr) {
            cerr << "Not enough memory for write the result of effect";
            exit(1);
        }
    }

    void set_new_BC(double b, double c) {
        B = b;
        C = c;
    }

    void nearest_neighbor() {
        double scale_height = (double)(newHeight) / height;
        double scale_width = (double)(newWidth) / width;
        for (int i = 0; i < newHeight; i++) {
            for (int j = 0; j < newWidth; j++) {
                int ii = (int) ((double)i / scale_height);
                int jj = (int) ((double)j / scale_width);
                newData[i * newWidth + j] = data[ii * width + jj];
            }
        }
    }

    void bilinear() {
        double scale_height = (double)(newHeight) / height;
        double scale_width = (double)(newWidth) / width;
        for (int i = 0; i < newHeight; i++) {
            for (int j = 0; j < newWidth; j++) {
                double ii = (double) i / scale_height;
                double jj = (double) j / scale_width;
                int i1 = fmin(height - 1, fmax(0, floor(ii)));
                int i2 = fmin(height - 1, fmax(0, floor(ii) + 1));
                double weight_i2 = fabs(ii - i1);
                double weight_i1 = 1 - weight_i2;
                int j1 = fmin(width - 1, fmax(0, floor(jj)));
                int j2 = fmin(width - 1, fmax(0, floor(jj) + 1));
                double weight_j2 = fabs(jj - j1);
                double weight_j1 = 1 - weight_j2;
                double res_j1 = weight_i1 * data[i1 * width + j1] + weight_i2 * data[i2 * width + j1];
                double res_j2 = weight_i1 * data[i1 * width + j2] + weight_i2 * data[i2 * width + j2];
                double res = weight_j1 * res_j1 + weight_j2 * res_j2;
                newData[i * newWidth + j] = (unsigned char) fmin(255, fmax(0, res));
            }
        }
    }

    void Lanczos3() {
        vector<double> buffer(height * newWidth, 0);
        /// first resize: j coordinate
        double scale_width = (double)(newWidth) / width;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < newWidth; j++) {
                double jj = (double) j / scale_width;
                double res = 0;
                for (int id = (int) jj - 2; id < (int) jj + 4; id++)
                    res += data[i * width + min(max(0, id), width - 1)] * L3(jj - (double) id);
                buffer[i * newWidth + j] = res;
            }
        }
        /// second resize: i coordinate
        double scale_height = (double)(newHeight) / height;
        for (int i = 0; i < newHeight; i++) {
            for (int j = 0; j < newWidth; j++) {
                double ii = (double) i / scale_height;
                double res = 0;
                for (int id = (int) ii - 2; id < (int) ii + 4; id++)
                    res += buffer[min(max(0, id), height - 1) * newWidth + j] * L3(ii - (double) id);
                newData[i * newWidth + j] = (unsigned char) fmin(255, fmax(0, res));
            }
        }
    }

    void BC_splines() {
        vector<double> buffer(height * newWidth, 0);
        vector<double> p(4, 0);
        /// first resize: j coordinate
        double scale_width = (double)(newWidth) / width;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < newWidth; j++) {
                double jj = (double) j / scale_width;
                int j1 = fmin(width - 1, fmax(0, floor(jj)));
                int j2 = fmin(width - 1, fmax(0, floor(jj) + 1));
                for (int id = j1 - 1; id < j2 + 2; id++)
                    p[id - j1 + 1] = data[i * width + min(max(id, 0), width - 1)];
                buffer[i * newWidth + j] = Mitchell_Netravali_filter(p, jj - j1);
            }
        }
        /// second resize: i coordinate
        double scale_height = (double)(newHeight) / height;
        for (int i = 0; i < newHeight; i++) {
            for (int j = 0; j < newWidth; j++) {
                double ii = (double) i / scale_height;
                int i1 = fmin(height - 1, fmax(0, floor(ii)));
                int i2 = fmin(height - 1, fmax(0, floor(ii) + 1));
                for (int id = i1 - 1; id < i2 + 2; id++)
                    p[id - i1 + 1] = buffer[min(max(id, 0), height - 1) * newWidth + j];
                double res = Mitchell_Netravali_filter(p, ii - i1);
                newData[i * newWidth + j] = (unsigned char) round(fmin(255, fmax(0, res)));
            }
        }
    }

    void do_algorithm(int a) {
        switch (a) {
            case 1:
                bilinear();
                break;
            case 2:
                Lanczos3();
                break;
            case 3:
                BC_splines();
                break;
            default:
                nearest_neighbor();
                break;
        }
    }

    void write(const char* outfile) {
        FILE* file = fopen(outfile, "wb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }
        int newSize = newHeight * newWidth;
        if (fprintf(file, "P%d\n%d %d\n%d\n", type, newWidth, newHeight, 255) < 0 ||
            fwrite(newData, 1, newSize, file) != newSize) {
            cerr << "Problems with writing image to outfile";
            exit(1);
        }
        fclose(file);
    }

    ~Image() {
        delete[] data;
        delete[] newData;
    }

private:
    unsigned char* data;
    unsigned char* newData;
    int width, height, type, pixelSize = 1, size, newHeight, newWidth;
    double B, C;

    double Mitchell_Netravali_filter(vector<double> &p, double d) const {
        return ((-1/6 * B - C) * (p[0] - p[3]) + (-3/2 * B - C + 2) * (p[1] - p[2])) * pow(d, 3)
             + ((1/2 * B + 2 * C) * p[0] + (2 * B + C - 3) * p[1] + (-5/2 * B - 2 * C + 3) * p[2] - C * p[3]) * d * d
             + (-1/2 * B - C) * (p[0] - p[2]) * d
             + 1/6 * B * p[0] + (-1/3 * B + 1) * p[1] + 1/6 * B * p[2];
    }

    static double L3(double x) {
        if (x == 0)
            return 1;
        return 3 * sin(M_PI * x) * sin(M_PI * x / 3) / pow(M_PI * x, 2);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 9 && argc != 11) {
        cerr << "Incorrect arguments count; expected 8 or 10";
        exit(1);
    }
    Image image(argv[1]);

    /// New sizes
    int new_width;
    int new_height;
    try {
        new_width = stoi(argv[3]);
        new_height = stoi(argv[4]);
    } catch (const exception& e) {
        cerr << "Incorrect height or width; please enter two int values";
        exit(1);
    }
    image.set_new_sizes(new_width, new_height);

    /// dx and dy are not necessary for my half-done algorithm
    double dx;
    double dy;
    try {
        dx = stod(argv[5]);
        dy = stod(argv[6]);
    } catch (const exception& e) {
        cerr << "Incorrect dx or dy; please enter two numbers";
        exit(1);
    }

    /// gamma also is not necessary for my half-done algorithm, it always equals 1
    try {
        double _ = stod(argv[7]);
    } catch (const exception& e) {
        cerr << "Incorrect gamma value; please enter a positive number";
        exit(1);
    }

    /// Algorithm
    int algorithm;
    try {
        algorithm = stoi(argv[8]);
    } catch (const exception& e) {
        cerr << "Incorrect algorithm value; please enter an int value";
        exit(1);
    }

    /// B and C for BC-splines
    double b = 0;
    double c = 0.5;
    try {
        if (argc == 11) {
            b = stod(argv[9]);
            c = stod(argv[10]);
        }
    } catch (const exception& e) {
        cerr << "Incorrect B or C; please enter two numbers";
        exit(1);
    }
    image.set_new_BC(b, c);

    /// Doing the algorithm and writing the result
    image.do_algorithm(algorithm);
    image.write(argv[2]);
    return 0;
}
