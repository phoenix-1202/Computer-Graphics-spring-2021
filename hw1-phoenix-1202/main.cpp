#include <iostream>
#include <cstring>

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
                f == 'P' && (t == 5 || t == 6) && maxColor == 255 && endOfLine == '\n') {
            width = w;
            height = h;
            type = t;
            if (type == 6)
                pixelSize = 3;
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

    void doEffect(int effect) {
        switch (effect) {
            case 0:
                inversion();
                break;
            case 1:
                horizontalMirror();
                break;
            case 2:
                verticalMirror();
                break;
            case 3:
                rotateRight();
                break;
            case 4:
                rotateLeft();
                break;
            default:
                cerr << "Incorrect effect; please enter an integer from 0 to 4";
                exit(1);
        }
    }

    void inversion() {
        for (int i = 0; i < size; i++)
            data[i] = (unsigned char)(255 - (int)data[i]);
    }

    void horizontalMirror() {
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width / 2; j++) {
                int x = (i * width + j) * pixelSize;
                int y = (i * width + width - 1 - j) * pixelSize;
                for (int k = 0; k < pixelSize; k++)
                    swap(data[x + k], data[y + k]);
            }
    }

    void verticalMirror() {
        for (int j = 0; j < width; j++)
            for (int i = 0; i < height / 2; i++) {
                int x = (i * width + j) * pixelSize;
                int y = ((height - 1 - i) * width + j) * pixelSize;
                for (int k = 0; k < pixelSize; k++)
                    swap(data[x + k], data[y + k]);
            }
    }

    void rotateRight() {
        auto* newData = new (nothrow) unsigned char[size];
        if (newData == nullptr) {
            cerr << "Out of memory exception";
            exit(1);
        }
        int n = 0;
        for (int j = 0; j < width; j++)
            for (int i = height - 1; i >= 0; i--) {
                int x = (i * width + j) * pixelSize;
                for (int k = 0; k < pixelSize; k++) {
                    newData[n] = data[x + k];
                    n++;
                }
            }
        swap(height, width);
        memcpy(data, newData, size);
        delete[] newData;
    }

    void rotateLeft() {
        rotateRight();
        verticalMirror();
        horizontalMirror();
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
    int width, height, type, pixelSize = 1, size;
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Incorrect arguments count; please enter your image filename, new image filename and effect number (an integer from 0 to 4)";
        exit(1);
    }
    Image image(argv[1]);
    int effect;
    try {
        effect = stoi(argv[3]);
    } catch (const exception& e) {
        cerr << "Incorrect effect; please enter an int value";
        exit(1);
    }
    image.doEffect(effect);
    image.write(argv[2]);
    return 0;
}
