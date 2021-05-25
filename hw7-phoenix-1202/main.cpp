#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include "zlib/zlib.h"

using namespace std;

int parse_dec_from_hex(unsigned char* arr, int begin, int end) {
    stringstream ss;
    ss << "0x";
    for (int i = begin; i < end; i++) {
        if ((int) arr[i] < 16)
            ss << "0";
        ss << hex << (int) arr[i];
    }
    int res = stoi(ss.str(), nullptr, 16);
    ss.clear();
    ss.str("");
    return res;
}

struct PNGChunk {
    string type;
    int dataSize = 0;
    unsigned char* data;

    PNGChunk(FILE* file) {
        auto buffer = new (nothrow) unsigned char[4];
        if (buffer == nullptr) {
            cerr << "Not enough memory for read the .png image";
            exit(1);
        }
        fread(buffer, 1, 4, file);
        dataSize = parse_dec_from_hex(buffer, 0, 4);
        delete[] buffer;

        buffer = new (nothrow) unsigned char[4];
        if (buffer == nullptr) {
            cerr << "Not enough memory for read the .png image";
            exit(1);
        }
        fread(buffer, 1, 4, file);
        for (int i = 0; i < 4; i++)
            type += (char)buffer[i];
        delete[] buffer;

        data = new (nothrow) unsigned char[dataSize];
        if (data == nullptr) {
            cerr << "Not enough memory for read the .png image";
            exit(1);
        }
        fread(data, 1, dataSize, file);

        // skip check_sum
        buffer = new (nothrow) unsigned char[4];
        if (buffer == nullptr) {
            cerr << "Not enough memory for read the .png image";
            exit(1);
        }
        fread(buffer, 1, 4, file);
        delete[] buffer;
    }

    ~PNGChunk() {
        delete[] data;
    }
};

struct PNGImage {
public:
    explicit PNGImage(const char* infile) {
        FILE* file = fopen(infile, "rb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }

        auto start_png = new (nothrow) unsigned char[8];
        if (start_png == nullptr) {
            cerr << "Not enough memory for read the .png image";
            exit(1);
        }
        fread(start_png, 1, 8, file);
        stringstream ss;
        for (int i = 0; i < 8; i++)
            ss << hex << (int) start_png[i];
        if (ss.str() != "89504e47da1aa") {
            cerr << "Error! Input file is not in .png format";
            exit(1);
        }
        delete[] start_png;

        while (true) {
            PNGChunk chunk(file);
            if (chunk.type == "IHDR")
                parse_IHDR(chunk);
            else if (chunk.type == "IDAT")
                parse_one_IDAT(chunk);
            else if (chunk.type == "IEND") {
                parse_all_IDAT_Data();
                break;
            }
        }

        fclose(file);
    }

    void parse_IHDR(const PNGChunk& chunk) {
        width = parse_dec_from_hex(chunk.data, 0, 4);
        height = parse_dec_from_hex(chunk.data, 4, 8);

        int bitDepth = (int)chunk.data[8];
        int colorType = (int)chunk.data[9];
        int compressionMethod = (int)chunk.data[10];
        int filterMethod = (int)chunk.data[11];
        int interlaceMethod = (int)chunk.data[12];

        if (bitDepth != 8) {
            cerr << "Unsupported bit depth value, expected 8";
            exit(1);
        }

        if (colorType != 0 && colorType != 2) {
            cerr << "Unsupported color type value, expected 0 or 2";
            exit(1);
        }
        pixelSize = (colorType == 2) ? 3 : 1;
        type = (pixelSize == 3) ? 6 : 5;

        if (compressionMethod != 0) {
            cerr << "Unsupported compression method value, expected 0";
            exit(1);
        }

        if (filterMethod != 0) {
            cerr << "Unsupported filter method value, expected 0";
            exit(1);
        }

        if (interlaceMethod != 0) {
            cerr << "Unsupported interlace method value, expected 0";
            exit(1);
        }
    }

    void parse_one_IDAT(const PNGChunk& chunk) {
        auto buffer = new (nothrow) unsigned char[size + chunk.dataSize];
        if (buffer == nullptr) {
            cerr << "Not enough memory for work with .png image";
            exit(1);
        }
        memcpy(buffer, idatData, size);
        memcpy(buffer + size, chunk.data, chunk.dataSize);
        if (size != 0)
            delete[] idatData;
        size += chunk.dataSize;
        idatData = new (nothrow) unsigned char[size];
        if (idatData == nullptr) {
            cerr << "Not enough memory for work with .png image";
            exit(1);
        }
        memcpy(idatData, buffer, size);
        delete[] buffer;
    }


    void parse_all_IDAT_Data() {
        int m_width = (pixelSize * width + 1);
        auto rawData = new (nothrow) unsigned char[m_width * height]; // data with filter column
        if (rawData == nullptr) {
            cerr << "Not enough memory for work with .png image";
            exit(1);
        }

        z_stream inf;
        inf.zalloc = Z_NULL;
        inf.zfree = Z_NULL;
        inf.opaque = Z_NULL;

        inf.avail_in = size; // size of input
        inf.next_in = (Bytef *) idatData; // input char array casted to Bytef *
        inf.avail_out = m_width * height; // size of output
        inf.next_out = (Bytef *) rawData; // output char array casted to Bytef *

        inflateInit(&inf);
        inflate(&inf, Z_NO_FLUSH);
        inflateEnd(&inf);

        data = new (nothrow) unsigned char[pixelSize * height * width];
        if (data == nullptr) {
            cerr << "Not enough memory for work with .png image";
            exit(1);
        }
        int k = 0;
        int delta;
        int dist_u, dist_l, dist_ul;
        int upper, left, upper_left;

        for (int i = 0; i < height; i++) {
            int filter = (int) rawData[i * m_width];
            for (int j = 1; j < m_width; j++) {
                switch (filter) {
                    case 0: // raw value
                        delta = 0;
                        break;
                    case 1: // + left
                        delta = (j <= pixelSize) ? 0 : data[i * width * pixelSize + j - pixelSize - 1];
                        break;
                    case 2: // + up
                        delta = (i == 0) ? 0 : data[(i - 1) * width * pixelSize + j - 1];
                        break;
                    case 3: // + (left + up) / 2
                        delta = 0;
                        if (i != 0)
                            delta += data[(i - 1) * width * pixelSize + j - 1];
                        if (j > pixelSize)
                            delta += data[i * width * pixelSize + j - pixelSize - 1];
                        delta /= 2;
                        break;
                    case 4: // + left | upper | upper_left according to dist between them and (left + upper - upper_left)
                        upper = (i == 0) ? 0 : data[(i - 1) * width * pixelSize + j - 1];
                        left = (j <= pixelSize) ? 0 : data[i * width * pixelSize + j - pixelSize - 1];
                        upper_left = (i == 0 || j <= pixelSize) ? 0 : data[(i - 1) * width * pixelSize + j - pixelSize - 1];

                        delta = upper + left - upper_left;
                        dist_u = abs(delta - upper);
                        dist_l = abs(delta - left);
                        dist_ul = abs(delta - upper_left);

                        delta = upper_left;
                        if (min(dist_u, min(dist_l, dist_ul)) == dist_u)
                            delta = upper;
                        else if (min(dist_u, min(dist_l, dist_ul)) == dist_l)
                            delta = left;
                        break;
                    default:
                        break;
                }
                data[k++] = (unsigned char) ((delta + (int) rawData[i * m_width + j]) % 256);
            }
        }
        delete[] rawData;
    }

    void write_to_pnm(const char* outfile) {
        FILE* file = fopen(outfile, "wb");
        if (file == nullptr) {
            cerr << "Cannot open the image file: problems with file";
            exit(1);
        }
        if (fprintf(file, "P%d\n%d %d\n%d\n", type, width, height, 255) < 0 ||
            fwrite(data, 1, width * height * pixelSize, file) != width * height * pixelSize) {
            cerr << "Problems with writing image to outfile";
            exit(1);
        }
        fclose(file);
    }

    ~PNGImage() {
        delete[] data;
        delete[] idatData;
    }

private:
    unsigned char* data;
    unsigned char* idatData;
    int width = 0, height = 0, type, pixelSize = 1, size = 0;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Incorrect arguments count; must be 2 files: <input>.png and <output>.pnm";
        exit(1);
    }
    PNGImage image(argv[1]);
    image.write_to_pnm(argv[2]);
    return 0;
}