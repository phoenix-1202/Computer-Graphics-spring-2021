#include <iostream>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <cmath>

using namespace std;

const double eps = 1e-8;

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
            if (type == 6)
                pixelSize = 3;
            size = width * height * pixelSize;
            data = new(nothrow) unsigned char[size];
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

    explicit Image(const char* infile_1, const char* infile_2, const char* infile_3) {
        Image image1(infile_1);
        Image image2(infile_2);
        Image image3(infile_3);
        if (image1.type == 5 && image2.type == 5 && image3.type == 5) {
            if (image1.width == image2.width && image2.width == image3.width) {
                if (image1.height == image2.height && image2.height == image3.height) {
                    type = 6;
                    pixelSize = 3;
                    height = image1.height;
                    width = image1.width;
                    size = pixelSize * height * width;
                    data = new(nothrow) unsigned char[size];
                    if (data == nullptr) {
                        cerr << "Cannot open image file: not enough memory";
                        exit(1);
                    }
                    for (int i = 0; i < image1.size; i++) {
                        data[i * 3] = image1.data[i];
                        data[i * 3 + 1] = image2.data[i];
                        data[i * 3 + 2] = image3.data[i];
                    }
                }
                else {
                    cerr << "Three input images must have the same height";
                    exit(1);
                }
            }
            else {
                cerr << "Three input images must have the same width";
                exit(1);
            }
        } else {
            cerr << "Three input images must be in pgm format (P5)";
            exit(1);
        }
    }

    void write(const char* outfile) {
        FILE* file = fopen(outfile, "wb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }
        if (fprintf(file, "P%d\n%d %d\n%d\n", type, width, height, 255) < 0 ||
            fwrite(data, 1, size, file) != size) {
            cerr << "Problems with writing image to outfile";
            exit(1);
        }
        fclose(file);
    }

    void write(const char* outfile_1, const char* outfile_2, const char* outfile_3) {
        FILE* file_1 = fopen(outfile_1, "wb");
        FILE* file_2 = fopen(outfile_2, "wb");
        FILE* file_3 = fopen(outfile_3, "wb");
        if (file_1 == nullptr || file_2 == nullptr || file_3 == nullptr) {
            cerr << "Cannot open one of the image file: problems with file";
            exit(1);
        }
        if (fprintf(file_1, "P%d\n%d %d\n%d\n", 5, width, height, 255) < 0 ||
            fprintf(file_2, "P%d\n%d %d\n%d\n", 5, width, height, 255) < 0 ||
            fprintf(file_3, "P%d\n%d %d\n%d\n", 5, width, height, 255) < 0) {
            cerr << "Problems with writing image to one of the outfiles";
            exit(1);
        }
        for (int i = 0; i < size; i += 3) {
            if (fprintf(file_1, "%c", data[i]) < 0 ||
                fprintf(file_2, "%c", data[i + 1]) < 0 ||
                fprintf(file_3, "%c", data[i + 2]) < 0) {
                cerr << "Problems with writing image to one of the outfiles";
                exit(1);
            }
        }
        fclose(file_1);
        fclose(file_2);
        fclose(file_3);
    }

    /// Colorspace convert part

    void HSL_to_RGB() {
        for (int i = 0; i < size; i += 3) {
            double h = (double) data[i] / 255.0 * 360;
            double s = (double) data[i + 1] / 255.0;
            double l = (double) data[i + 2] / 255.0;
            double q;
            if (0.5 - l >= eps) q = l * (s + 1);
            else q = l + s - l * s;
            double p = 2 * l - q;
            double hk = h / 360;
            double tc[3];
            tc[0] = hk + 1.0 / 3;
            tc[1] = hk;
            tc[2] = hk - 1.0 / 3;
            for (double & c : tc) {
                if (c < 0) c += 1;
                if (c > 1) c -= 1;
            }
            double rgb[3];
            for (int j = 0; j < 3; j++) {
                if (1.0 / 6 - tc[j] >= eps) rgb[j] = p + (q - p) * 6 * tc[j];
                else if (1.0 / 2 - tc[j] >= eps) rgb[j] = q;
                else if (2.0 / 3 - tc[j] >= eps) rgb[j] = p + (q - p) * 6 * (2.0 / 3 - tc[j]);
                else rgb[j] = p;
            }
            data[i] = (unsigned char)(rgb[0] * 255);
            data[i + 1] = (unsigned char)(rgb[1] * 255);
            data[i + 2] = (unsigned char)(rgb[2] * 255);
        }
    }

    void HSV_to_RGB() {
        for (int i = 0; i < size; i += 3) {
            double h = (double) data[i] / 255 * 360;
            double s = (double) data[i + 1] / 255 * 100;
            double v = (double) data[i + 2] / 255 * 100;
            int hi = (int)(h / 60) % 6;
            double v_min = (100 - s) * v / 100;
            double a = (v - v_min) * ((int)h % 60) / 60;
            double v_inc = v_min + a;
            double v_dec = v - a;
            double r;
            double g;
            double b;
            switch (hi) {
                case 0:
                    r = v;
                    g = v_inc;
                    b = v_min;
                    break;
                case 1:
                    r = v_dec;
                    g = v;
                    b = v_min;
                    break;
                case 2:
                    r = v_min;
                    g = v;
                    b = v_inc;
                    break;
                case 3:
                    r = v_min;
                    g = v_dec;
                    b = v;
                    break;
                case 4:
                    r = v_inc;
                    g = v_min;
                    b = v;
                    break;
                default:
                    r = v;
                    g = v_min;
                    b = v_dec;
                    break;
            }
            data[i] = (unsigned char)(r * 255 / 100);
            data[i + 1] = (unsigned char)(g * 255 / 100);
            data[i + 2] = (unsigned char)(b * 255 / 100);
        }
    }

    void YCbCr_601_to_RGB() {
        YCbCr_to_RGB(0.299, 0.587, 0.114);
    }

    void YCbCr_709_to_RGB() {
        YCbCr_to_RGB(0.2126, 0.7152, 0.0722);
    }

    void YCoCg_to_RGB() {
        for (int i = 0; i < size; i += 3) {
            double y = (double) data[i] / 255;
            double co = (double) data[i + 1] / 255 - 0.5;
            double cg = (double) data[i + 2] / 255 - 0.5;
            double r = fmin(fmax(0, y + co - cg), 1);
            double g = fmin(fmax(0, y + cg), 1);
            double b = fmin(fmax(0, y - co - cg), 1);
            data[i] = (unsigned char) (r * 255);
            data[i + 1] = (unsigned char) (g * 255);
            data[i + 2] = (unsigned char) (b * 255);
        }
    }

    void CMY_to_RGB() {
        inversion();
    }

    void RGB_to_HSL() {
        for (int i = 0; i < size; i += 3) {
            double r = (double) data[i] / 255;
            double g = (double) data[i + 1] / 255;
            double b = (double) data[i + 2] / 255;
            double mini = fmin(r, fmin(g, b));
            double maxi = fmax(r, fmax(g, b));
            double h;
            if (fabs(maxi - r) < eps)
                h = 60 * (g - b) / (maxi - mini);
            else if (fabs(maxi - g) < eps)
                h = 60 * (b - r) / (maxi - mini) + 120;
            else
                h = 60 * (r - g) / (maxi - mini) + 240;
            if (h < 0) h += 360;
            double s;
            if (fabs(1 - fabs(1 - (maxi + mini))) < eps) s = 0;
            else s = (maxi - mini) / (1 - fabs(1 - (maxi + mini)));
            double l = 1.0 / 2 * (maxi + mini);
            data[i] = (unsigned char) (h / 360 * 255);
            data[i + 1] = (unsigned char) (s * 255);
            data[i + 2] = (unsigned char) (l * 255);
        }
    }

    void RGB_to_HSV() {
        for (int i = 0; i < size; i += 3) {
            double r = (double) data[i] / 255;
            double g = (double) data[i + 1] / 255;
            double b = (double) data[i + 2] / 255;
            double mini = fmin(r, fmin(g, b));
            double maxi = fmax(r, fmax(g, b));
            double h;
            if (fabs(maxi - r) < eps)
                h = 60 * (g - b) / (maxi - mini);
            else if (fabs(maxi - g) < eps)
                h = 60 * (b - r) / (maxi - mini) + 120;
            else
                h = 60 * (r - g) / (maxi - mini) + 240;
            if (h < 0) h += 360;
            double s;
            if (maxi < eps) s = 0;
            else s = 1 - mini / maxi;
            double v = maxi;
            data[i] = (unsigned char) (h / 360 * 255);
            data[i + 1] = (unsigned char) (s * 255);
            data[i + 2] = (unsigned char) (v * 255);
        }
    }

    void RGB_to_YCbCr_601() {
        RGB_to_YCbCr(0.299, 0.587, 0.114);
    }

    void RGB_to_YCbCr_709() {
        RGB_to_YCbCr(0.2126, 0.7152, 0.0722);
    }

    void RGB_to_YCoCg() {
        for (int i = 0; i < size; i += 3) {
            double r = (double) data[i] / 255;
            double g = (double) data[i + 1] / 255;
            double b = (double) data[i + 2] / 255;
            double y = r / 4 + g / 2 + b / 4;
            double co = r / 2 - b / 2;
            double cg = -r / 4 + g / 2 - b / 4;
            data[i] = (unsigned char) (y * 255);
            data[i + 1] = (unsigned char) ((co + 0.5) * 255);
            data[i + 2] = (unsigned char) ((cg + 0.5) * 255);
        }
    }

    void RGB_to_CMY() {
        inversion();
    }

    ~Image() {
        delete[] data;
    }

    int getType() const {
        return type;
    }

private:
    unsigned char* data;
    int width, height, type, pixelSize = 1, size;

    void inversion() {
        for (int i = 0; i < size; i++)
            data[i] = ~data[i];
    }

    void YCbCr_to_RGB(double kr, double kg, double kb) {
        for (int i = 0; i < size; i += 3) {
            auto y = (double) data[i] / 255 * 219 + 16;
            auto cb = (double) data[i + 1] / 255 * 224 + 16;
            auto cr = (double) data[i + 2] / 255 * 224 + 16;
            double yy = (y - 16) / 219;
            double pb = (cb - 128) / 224;
            double pr = (cr - 128) / 224;
            double r = yy + (2 - 2 * kr) * pr;
            double g = yy + (2 * kb - 2) * kb / kg * pb + (2 * kr - 2) * kr / kg * pr;
            double b = yy + (2 - 2 * kb) * pb;
            r = fmax(fmin(r, 1), 0) * 255;
            g = fmax(fmin(g, 1), 0) * 255;
            b = fmax(fmin(b, 1), 0) * 255;
            data[i] = (unsigned char) r;
            data[i + 1] = (unsigned char) g;
            data[i + 2] = (unsigned char) b;
        }
    }

    void RGB_to_YCbCr(double kr, double kg, double kb) {
        for (int i = 0; i < size; i += 3) {
            double r = (double) data[i] / 255;
            double g = (double) data[i + 1] / 255;
            double b = (double) data[i + 2] / 255;
            double yy = kr * r + kg * g + kb * b;
            double pb = 1.0 / 2 * (b - yy) / (1 - kb);
            double pr = 1.0 / 2 * (r - yy) / (1 - kr);
            double y = ((16 + yy * 219) - 16) / 219 * 255;
            double cb = ((128 + pb * 224) - 16) / 224 * 255;
            double cr = ((128 + pr * 224) - 16) / 224 * 255;
            data[i] = (unsigned char) y;
            data[i + 1] = (unsigned char) cb;
            data[i + 2] = (unsigned char) cr;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 11 && argc != 13 && argc != 15) {
        cerr << "Incorrect arguments count, must be 11, 13 or 15";
        exit(1);
    }
    unordered_map<string, int> m;
    for (int i = 1; i < argc; i++) {
        if (strlen(argv[i]) == 2 && argv[i][0] == '-')
            m[argv[i]] = 1;
    }
    string from_colorspace;
    string to_colorspace;
    vector<string> in_file_names;
    vector<string> out_file_names;
    int in_files_cnt;
    int out_files_cnt;
    if (m["-f"] == 1 && m["-t"] == 1 && m["-i"] == 1 && m["-o"] == 1) {
        int pos = 1;
        while (pos < argc) {
            if (strcmp(argv[pos], "-f") == 0) {
                pos++;
                from_colorspace = argv[pos];
                pos++;
            }
            else if (strcmp(argv[pos], "-t") == 0) {
                pos++;
                to_colorspace = argv[pos];
                pos++;
            }
            else if (strcmp(argv[pos], "-i") == 0) {
                pos++;
                if (strcmp(argv[pos], "1") == 0) {
                    in_files_cnt = 1;
                    in_file_names.emplace_back(argv[pos + 1]);
                    pos += 2;
                }
                else if (strcmp(argv[pos], "3") == 0) {
                    in_files_cnt = 3;
                    pos++;
                    int n = strlen(argv[pos]);
                    for (int i = 0; i < 3; i++) {
                        string new_file_name;
                        for (int j = 0; j < n; j++) {
                            if (j == n - 4) {
                                new_file_name += '_';
                                new_file_name += (char)(i + 1 + '0');
                            }
                            new_file_name += argv[pos][j];
                        }
                        in_file_names.push_back(new_file_name);
                    }
                    pos++;
                }
                else {
                    cerr << "Incorrect format of input: possible values of input arguments count is 1 and 3";
                    exit(1);
                }
            }
            else if (strcmp(argv[pos], "-o") == 0) {
                pos++;
                if (strcmp(argv[pos], "1") == 0) {
                    out_files_cnt = 1;
                    out_file_names.emplace_back(argv[pos + 1]);
                    pos += 2;
                }
                else if (strcmp(argv[pos], "3") == 0) {
                    out_files_cnt = 3;
                    pos++;
                    int n = strlen(argv[pos]);
                    for (int i = 0; i < 3; i++) {
                        string new_file_name;
                        for (int j = 0; j < n; j++) {
                            if (j == n - 4) {
                                new_file_name += '_';
                                new_file_name += (char)(i + 1 + '0');
                            }
                            new_file_name += argv[pos][j];
                        }
                        out_file_names.push_back(new_file_name);
                    }
                    pos++;
                }
                else {
                    cerr << "Incorrect format of input: possible values of output arguments count is 1 and 3";
                    exit(1);
                }
            }
            else {
                cerr << "Incorrect format of input: missed one of flags (-f, -t, -i or -o)";
                exit(1);
            }
        }

        /// Read the image
        Image* in_image;
        if (in_files_cnt == 3)
            in_image = new Image(in_file_names[0].c_str(), in_file_names[1].c_str(), in_file_names[2].c_str());
        else {
            in_image = new Image(in_file_names[0].c_str());
            if (in_image->getType() == 5) {
                cerr << "One image must be in ppm format (P6)";
                exit(1);
            }
        }

        /// Colorspace magic
        if (from_colorspace != to_colorspace) {
            if (from_colorspace != "RGB") {
                if (from_colorspace == "HSL") in_image->HSL_to_RGB();
                if (from_colorspace == "HSV") in_image->HSV_to_RGB();
                if (from_colorspace == "YCbCr.601") in_image->YCbCr_601_to_RGB();
                if (from_colorspace == "YCbCr.709") in_image->YCbCr_709_to_RGB();
                if (from_colorspace == "YCoCg") in_image->YCoCg_to_RGB();
                if (from_colorspace == "CMY") in_image->CMY_to_RGB();
            }
            if (to_colorspace != "RGB") {
                if (to_colorspace == "HSL") in_image->RGB_to_HSL();
                if (to_colorspace == "HSV") in_image->RGB_to_HSV();
                if (to_colorspace == "YCbCr.601") in_image->RGB_to_YCbCr_601();
                if (to_colorspace == "YCbCr.709") in_image->RGB_to_YCbCr_709();
                if (to_colorspace == "YCoCg") in_image->RGB_to_YCoCg();
                if (to_colorspace == "CMY") in_image->RGB_to_CMY();
            }
        }
        /// Write the result
        if (out_files_cnt == 1) {
            if (out_file_names[0][out_file_names[0].size() - 2] != 'p') {
                cerr << "One out file must be in ppm format (P6)";
                exit(1);
            }
            in_image->write(out_file_names[0].c_str());
        }
        else {
            if (out_file_names[0][out_file_names[0].size() - 2] != 'g' ||
                out_file_names[1][out_file_names[1].size() - 2] != 'g' ||
                out_file_names[2][out_file_names[2].size() - 2] != 'g') {
                cerr << "Three out files must be in pgm format (P5)";
                exit(1);
            }
            in_image->write(out_file_names[0].c_str(), out_file_names[1].c_str(), out_file_names[2].c_str());
        }
        delete in_image;
    } else {
        cerr << "Incorrect input; must be flags -f, -t, -i and -o";
        exit(1);
    }
    return 0;
}
