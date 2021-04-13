#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>

using namespace std;

struct Image {
public:
    explicit Image(const char* infile, int grad) {
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
            size = width * height * pixelSize;
            data = new (nothrow) double[size];
            result_data = new (nothrow) unsigned char[size];
            if (data == nullptr || result_data == nullptr) {
                cerr << "Cannot open image file: not enough memory";
                exit(1);
            }
            if (grad == 0) {
                if (fread(result_data, 1, size, file) != size) {
                    cerr << "Problems with reading the image file";
                    exit(1);
                }
                for (int i = 0; i < size; i++)
                    data[i] = (int) result_data[i];
            } else {
                for (int i = 0; i < height; i++)
                    for (int j = 0; j < width; j++)
                        data[i * width + j] = j * 255.0 / (width - 1);
            }
        } else {
            cerr << "Incorrect image format: must be P5 type with maxColorValue = 255";
            exit(1);
        }
        fclose(file);
    }

    void no_dithering() {
        for (int i = 0; i < size; i++) {
            double pixel = data[i];
            auto nearest_values = left_right(pixel);
            double left = anti_gamma_correction(nearest_values.first / 255.0) * 255;
            double right = anti_gamma_correction(nearest_values.second / 255.0) * 255;
            double mid = anti_gamma_correction(pixel / 255.0) * 255;
            result_data[i] = (unsigned char) (get_nearest(left, right, mid));
        }
    }

    void ordered_dithering() {
        vector<vector<double>> ord_matrix = { { 0, 32,  8, 40,  2, 34, 10, 42},
                                              {48, 16, 56, 24, 50, 18, 58, 26},
                                              {12, 44,  4, 36, 14, 46,  6, 38},
                                              {60, 28, 52, 20, 62, 30, 54, 22},
                                              { 3, 35, 11, 43,  1, 33,  9, 41},
                                              {51, 19, 59, 27, 49, 17, 57, 25},
                                              {15, 47,  7, 39, 13, 45,  5, 37},
                                              {63, 31, 55, 23, 61, 29, 53, 21}
                                            };
        ordered_dithering_general(ord_matrix, 0.5, 8);
    }

    void random_dithering() {
        unsigned seed = chrono::system_clock::now().time_since_epoch().count();
        mt19937 generator (seed);
        for (int i = 0; i < size; i++) {
            double pixel = data[i];
            auto nearest_values = left_right(pixel);
            double left = anti_gamma_correction(nearest_values.first / 255.0) * 255;
            double right = anti_gamma_correction(nearest_values.second / 255.0) * 255;
            double mid = anti_gamma_correction(pixel / 255.0) * 255;
            double delta = (double) generator() / mt19937::max() - 0.5;
            mid += delta * (right - left);
            result_data[i] = (unsigned char) (get_nearest(left, right, mid));
        }
    }

    void Floyd_Steinberg_dithering() {
        vector<vector<double>> errors = { {7.0/16, 0.0/16},
                                          {0.0/16, 3.0/16, 5.0/16, 1.0/16, 0.0/16},
                                          {0.0/16, 0.0/16, 0.0/16, 0.0/16, 0.0/16}
        };
        error_matrix_dithering_general(errors);
    }

    void Jarvis_Judice_Ninke_dithering() {
        vector<vector<double>> errors = { {7.0/48, 5.0/48},
                                          {3.0/48, 5.0/48, 7.0/48, 5.0/48, 3.0/48},
                                          {1.0/48, 3.0/48, 5.0/48, 3.0/48, 1.0/48}
        };
        error_matrix_dithering_general(errors);
    }

    void Sierra_3_dithering() {
        vector<vector<double>> errors = { {5.0/32, 3.0/32},
                                          {2.0/32, 4.0/32, 5.0/32, 4.0/32, 2.0/32},
                                          {0.0/32, 2.0/32, 3.0/32, 2.0/32, 0.0/32}
        };
        error_matrix_dithering_general(errors);
    }

    void Atkinson_dithering() {
        vector<vector<double>> errors = { {1.0/8, 1.0/8},
                                          {0.0/8, 1.0/8, 1.0/8, 1.0/8, 0.0/8},
                                          {0.0/8, 0.0/8, 1.0/8, 0.0/8, 0.0/8}
        };
        error_matrix_dithering_general(errors);
    }

    void halftone_dithering() {
        vector<vector<double>> halftone_matrix = {{ 7, 13, 11, 4},
                                                  {12, 16, 14, 8},
                                                  {10, 15,  6, 2},
                                                  { 5,  9,  3, 1}};
        ordered_dithering_general(halftone_matrix, -0.5, 4);
    }

    void dither(int dither, int b, double g) {
        bits = b;
        gamma = g;
        generate_palette();
        switch (dither) {
            case 1:
                ordered_dithering();
                break;
            case 2:
                random_dithering();
                break;
            case 3:
                Floyd_Steinberg_dithering();
                break;
            case 4:
                Jarvis_Judice_Ninke_dithering();
                break;
            case 5:
                Sierra_3_dithering();
                break;
            case 6:
                Atkinson_dithering();
                break;
            case 7:
                halftone_dithering();
                break;
            default:
                no_dithering();
                break;
        }
    }

    void write(const char* outfile) {
        FILE* file = fopen(outfile, "wb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }
        if (fprintf(file, "P5\n%d %d\n%d\n", width, height, 255) < 0 ||
            fwrite(result_data, 1, size, file) != size) {
            cerr << "Problems with writing image to outfile";
            exit(1);
        }
        fclose(file);
    }

    ~Image() {
        delete[] data;
        delete[] result_data;
    }

private:
    double* data;
    unsigned char* result_data;
    int width, height, pixelSize = 1, size, bits = 8;
    vector<double> palette;
    double gamma = 1;

    void generate_palette() {
        int cnt = 1 << bits;
        palette.resize(0);
        for (int i = 0; i < cnt; i++)
            palette.push_back(round(255.0 / (cnt - 1) * i));
    }

    pair<int, int> left_right(double x) {
        int l = 0;
        int r = palette.size();
        while (r - l > 1) {
            int m = (l + r) / 2;
            if (palette[m] > x)
                r = m;
            else
                l = m;
        }
        r = min(r, (int)palette.size() - 1);
        return make_pair(palette[l], palette[r]);
    }

    static double get_nearest(double left, double right, double mid) {
        return (fabs(mid - left) < fabs(right - mid)) ? left : right;
    }

    double anti_gamma_correction(double x) const {
        if (gamma != 0)
            return pow(x, gamma);
        if (x <= 0.04045)
            return x / 12.92;
        return pow((200 * x + 11) / 211, 2.4);
    }

    void ordered_dithering_general(vector<vector<double>>& matrix, double delta, int n) {
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                matrix[i][j] = (matrix[i][j] + delta) / n / n - 0.5;
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++) {
                double pixel = data[i * width + j];
                auto nearest_values = left_right(pixel);
                double left = anti_gamma_correction(nearest_values.first / 255.0) * 255;
                double right = anti_gamma_correction(nearest_values.second / 255.0) * 255;
                double mid = anti_gamma_correction(pixel / 255.0) * 255;
                mid += matrix[i % n][j % n] * (right - left);
                result_data[i * width + j] = (unsigned char) (get_nearest(left, right, mid));
            }
    }

    void error_matrix_dithering_general(vector<vector<double>>& errors) {
        vector<double> error_matrix[3];
        for (auto & i : error_matrix)
            i.assign(width, 0);
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                double pixel = data[i * width + j];
                auto nearest_values = left_right(pixel);
                double left = anti_gamma_correction(nearest_values.first / 255.0) * 255;
                double right = anti_gamma_correction(nearest_values.second / 255.0) * 255;
                double mid = anti_gamma_correction(pixel / 255.0) * 255;
                mid += error_matrix[0][j];
                double new_pixel = get_nearest(left, right, mid);
                double quant_error = mid - new_pixel;
                if (j < width - 1)
                    error_matrix[0][j + 1] += quant_error * errors[0][0];
                if (j < width - 2)
                    error_matrix[0][j + 2] += quant_error * errors[0][1];
                if (i < height - 1) {
                    if (j > 1)
                        error_matrix[1][j - 2] += quant_error * errors[1][0];
                    if (j > 0)
                        error_matrix[1][j - 1] += quant_error * errors[1][1];
                    error_matrix[1][j] += quant_error * errors[1][2];
                    if (j < width - 1)
                        error_matrix[1][j + 1] += quant_error * errors[1][3];
                    if (j < width - 2)
                        error_matrix[1][j + 2] += quant_error * errors[1][4];
                }
                if (i < height - 2) {
                    if (j > 1)
                        error_matrix[2][j - 2] += quant_error * errors[2][0];
                    if (j > 0)
                        error_matrix[2][j - 1] += quant_error * errors[2][1];
                    error_matrix[2][j] += quant_error * errors[2][2];
                    if (j < width - 1)
                        error_matrix[2][j + 1] += quant_error * errors[2][3];
                    if (j < width - 2)
                        error_matrix[2][j + 2] += quant_error * errors[2][4];
                }
                result_data[i * width + j] = (unsigned char) new_pixel;
            }
            error_matrix[0] = error_matrix[1];
            error_matrix[1] = error_matrix[2];
            error_matrix[2].clear();
            error_matrix[2].assign(width, 0);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 7) {
        cerr << "Incorrect arguments count; must be 6";
        exit(1);
    }

    /// Reading or creating the image file
    int grad;
    try {
        grad = stoi(argv[3]);
    } catch (const exception& e) {
        cerr << "Incorrect gradient value; please enter 0 or 1";
        exit(1);
    }
    Image image(argv[1], grad);

    /// Reading bits value and gamma-correction parameter
    int bits;
    try {
        bits = stoi(argv[5]);
        if (bits < 1 || bits > 8) {
            cerr << "Incorrect bits value; must be integer from 1 to 8";
            exit(1);
        }
    } catch (const exception& e) {
        cerr << "Incorrect bits value; must be integer";
        exit(1);
    }
    double gamma;
    try {
        gamma = stod(argv[6]);
    } catch (const exception& e) {
        cerr << "Incorrect gamma value; must be double";
        exit(1);
    }

    /// Start dithering
    int dither;
    try {
        dither = stoi(argv[4]);
        if (dither < 0 || dither > 7) {
            cerr << "Incorrect dither value; must be integer from 0 to 7";
            exit(1);
        }
    } catch (const exception& e) {
        cerr << "Incorrect dither value; must be integer from 0 to 7";
        exit(1);
    }
    image.dither(dither, bits, gamma);

    /// Write the result to outfile
    image.write(argv[2]);
    return 0;
}