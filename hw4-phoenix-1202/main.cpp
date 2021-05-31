#include <iostream>
#include <cmath>
#include <vector>
#include <functional>

using namespace std;

double B = 0, C = 0.5;

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
            f == 'P' && (t == 5 || t == 6) && maxColor == 255 && endOfLine == '\n') {
            width = w;
            height = h;
            type = t;
            pixelSize = (t == 6) ? 3 : 1;
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
        newData = new (nothrow) unsigned char[newHeight * newWidth * pixelSize];
        if (newData == nullptr) {
            cerr << "Not enough memory for write the result of effect";
            exit(1);
        }
        fill(newData, newData + newHeight * newWidth * pixelSize, 0);
    }

    void set_new_params(double g, double dx, double dy) {
        gamma = g;
        di = dy;
        dj = dx;
    }

    void nearest_neighbor() {
        double scale_height = (double)(newHeight) / height;
        double scale_width = (double)(newWidth) / width;
        for (int i = 0; i < newHeight; i++) {
            for (int j = 0; j < newWidth; j++) {
                int ii = (int) ((double)i / scale_height);
                int jj = (int) ((double)j / scale_width);
                for (int k = 0; k < pixelSize; k++) {
                    if (incorrect(i + (int) di, newHeight) || incorrect(j + (int) dj, newWidth))
                        continue;
                    double x = ((double) data[(ii * width + jj) * pixelSize + k]) / 255;
                    newData[(int) ((i + di) * newWidth + (j + dj)) * pixelSize + k] = (int) (anti_gamma_correction(x) * 255.0);
                }
            }
        }
    }

    void do_algorithm(int a) {
        switch (a) {
            case 0:
                nearest_neighbor();
                break;
            case 1:
                kernel_resize(0, 1, linear, true);
                break;
            case 2:
                kernel_resize(2, 3, L3, false);
                break;
            case 3:
                kernel_resize(1, 2, Mitchell_Netravali, false);
                break;
            default:
                cerr << "Incorrect argument value; must be an integer from 0 to 4";
                exit(1);
        }
    }

    void write(const char* outfile) {
        FILE* file = fopen(outfile, "wb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }
        int newSize = newHeight * newWidth * pixelSize;
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
    int width, height, type, pixelSize, size, newHeight, newWidth;
    double gamma, di, dj;

    static double L3(double x) {
        if (x == 0)
            return 1;
        return 3 * sin(M_PI * x) * sin(M_PI * x / 3) / pow(M_PI * x, 2);
    }

    static double linear(double x) {
        return (x > 0) ? (1 - x) : (x + 1);
    }

    static double Mitchell_Netravali(double x) {
        double xx = fabs(x);
        if (xx < 1)
            return 1.0/2 * (4 - 3 * B - 2 * C) * pow(xx, 3) + (-3 + 2 * B + C) * pow(xx, 2) + 1 - B / 3;
        if (xx < 2)
            return -(B / 6 + C) * pow(xx, 3) + (B + 5 * C) * pow(xx, 2) - (2 * B + 8 * C) * xx + (4 / 3 * B + 4 * C);
        return 0;
    }

    double anti_gamma_correction(double x) const {
        if (gamma == 1)
            return x;
        if (gamma != 0)
            return pow(x, gamma);
        if (x <= 0.04045)
            return x / 12.92;
        return pow((200 * x + 11) / 211, 2.4);
    }

    static bool incorrect(int x, int b) {
        return x < 0 || x >= b;
    }

    void kernel_resize(int d1_start, int d2_start, const function<double(double)>& F, bool flag) {
        vector<double> buffer(height * newWidth * pixelSize, 0);
        /// first resize: j coordinate
        double scale_width = (double)(newWidth) / width;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < newWidth; j++) {
                double jj = (double) j / scale_width;
                int d1 = d1_start, d2 = d2_start;
                if (width > newWidth) {
                    d1 = (int) (d1 / scale_width);
                    d2 = (int) (d2 / scale_width);
                }
                for (int k = 0; k < pixelSize; k++) {
                    double res = 0;
                    double sum = 0;
                    for (int id = (int) jj - d1; id <= (int) jj + d2; id++) {
                        double r;
                        if (flag)
                            r = (d1 == d1_start) ? F(jj - (double) id) : F((jj - (double) id) * scale_width / (d2 - d1)) * (d2 - d1);
                        else
                            r = (d1 == d1_start) ? F(jj - (double) id) : F((jj - (double) id) * scale_width);
                        sum += r;
                        double x = data[(i * width + min(max(0, id), width - 1)) * pixelSize + k];
                        res += anti_gamma_correction(x / 255.0) * 255 * r;
                    }
                    buffer[(i * newWidth + j) * pixelSize + k] = res / sum;
                }
            }
        }
        /// second resize: i coordinate
        double scale_height = (double)(newHeight) / height;
        for (int i = 0; i < newHeight; i++) {
            for (int j = 0; j < newWidth; j++) {
                if (incorrect(i + (int) di, newHeight) || incorrect(j + (int) dj, newWidth))
                    continue;
                double ii = (double) i / scale_height;
                int d1 = d1_start, d2 = d2_start;
                if (height > newHeight) {
                    d1 = (int) (d1 / scale_height);
                    d2 = (int) (d2 / scale_height);
                }
                for (int k = 0; k < pixelSize; k++) {
                    double res = 0;
                    double sum = 0;
                    for (int id = (int) ii - d1; id <= (int) ii + d2; id++) {
                        double r;
                        if (flag)
                            r = (d1 == d1_start) ? F(ii - (double) id) : F((ii - (double) id) * scale_height / (d2 - d1)) * (d2 - d1);
                        else
                            r = (d1 == d1_start) ? F(ii - (double) id) : F((ii - (double) id) * scale_height);
                        sum += r;
                        res += buffer[(min(max(0, id), height - 1) * newWidth + j) * pixelSize + k] * r;
                    }
                    newData[(int)((i + di) * newWidth + j + dj) * pixelSize + k] = (unsigned char) fmin(255, fmax(0, res / sum));
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 9 && argc != 11) {
        cerr << "Incorrect arguments count; expected 8, 9 or 10";
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

    /// dx (dj) and dy (di) offset
    double dx;
    double dy;
    try {
        dx = stod(argv[5]);
        dy = stod(argv[6]);
    } catch (const exception& e) {
        cerr << "Incorrect di or dj; please enter two numbers";
        exit(1);
    }

    /// gamma correction coefficient
    double gamma;
    try {
        gamma = stod(argv[7]);
    } catch (const exception& e) {
        cerr << "Incorrect gamma value; please enter a positive number";
        exit(1);
    }

    image.set_new_params(gamma, dx, dy);

    /// Algorithm
    int algorithm;
    try {
        algorithm = stoi(argv[8]);
    } catch (const exception& e) {
        cerr << "Incorrect algorithm value; please enter an int value";
        exit(1);
    }

    /// B and C for BC-splines
    try {
        if (argc == 11) {
            B = stod(argv[9]);
            C = stod(argv[10]);
        }
    } catch (const exception& e) {
        cerr << "Incorrect B or C; please enter two numbers";
        exit(1);
    }

    /// Doing the algorithm and writing the result
    image.do_algorithm(algorithm);
    image.write(argv[2]);
    return 0;
}
