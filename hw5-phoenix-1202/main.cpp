#include <iostream>
#include <vector>
#include <algorithm>

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

    void multi_Otsu_tresholding(int delimiters) {
        calc_p_and_prefix();
        vector<int> f;
        for (int i = 0; i < delimiters; i++)
            f.push_back(i + 1);
        f.push_back(256);
        vector<int> positions(delimiters, 0);
        double maxi = 0;
        // find partition positions
        while (true) {
            double res = 0;
            int pos = -1;
            bool flag = true;
            for (int i = 0; i < delimiters + 1; i++) {
                double delta_p = (pos == -1) ? 0 : prefix_p[pos];
                double delta_fp = (pos == -1) ? 0 : prefix_fp[pos];
                if (prefix_p[f[i] - 1] - delta_p == 0) {
                    flag = false;
                    break;
                }
                res += pow(prefix_fp[f[i] - 1] - delta_fp, 2) / (prefix_p[f[i] - 1] - delta_p);
                pos = f[i] - 1;
            }
            if (flag) {
                if (res > maxi) {
                    maxi = res;
                    for (int i = 0; i < delimiters + 1; i++)
                        positions[i] = f[i];
                }
            }
            pos = delimiters - 1;
            while (pos != -1) {
                f[pos]++;
                if (f[pos] <= 255 - (delimiters - pos - 1)) break;
                pos--;
            }
            if (pos == -1)
                break;
            for (int i = pos + 1; i < delimiters; i++)
                f[i] = f[i - 1] + 1;
        }
        // write new colors
        for (int i = 0; i < size; i++) {
            unsigned char new_value = data[i];
            for (int j = delimiters; j >= 0; j--)
                if ((int) data[i] < positions[j])
                    new_value = (unsigned char) (j * 255 / delimiters);
                else break;
            data[i] = new_value;
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

    ~Image() {
        delete[] data;
    }

private:
    unsigned char* data;
    vector<double> prefix_p, prefix_fp;
    int width, height, type, pixelSize = 1, size;

    void calc_p_and_prefix() {
        vector<int> p(256, 0);
        prefix_p.assign(256, 0);
        prefix_fp.assign(256, 0);
        for (int i = 0; i < size; i++)
            p[(int) data[i]]++;
        for (int i = 0; i < 256; i++) {
            prefix_p[i] = prefix_p[max(0, i - 1)] + p[i];
            prefix_fp[i] = prefix_fp[max(0, i - 1)] + i * p[i];
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Incorrect arguments count; please enter your image filename, new image filename and effect number (an integer from 0 to 4)";
        exit(1);
    }
    Image image(argv[1]);
    int classes;
    try {
        classes = stoi(argv[3]);
        if (classes < 2) {
            cerr << "Incorrect classes count value; must be 2 or more";
            exit(1);
        }
    } catch (const exception& e) {
        cerr << "Incorrect effect; please enter an int value";
        exit(1);
    }
    image.multi_Otsu_tresholding(classes - 1);
    image.write(argv[2]);
    return 0;
}
