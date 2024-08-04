#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <iterator>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    // поля заголовка Bitmap File Header
    array<char, 2> caption{'B','M'}; 
    unsigned int header_and_data_size = 0;  // Расчитываемое: header_size + BitmapInfoHeader.data_byte_count
    array<char, 4> reserved{0, 0, 0, 0};
    unsigned int header_size = 54;

    bool IsValid() const {
        return (caption == array<char, 2>{'B','M'}
                && reserved == array<char, 4>{0, 0, 0, 0}
                && header_size == 54);
    }
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    unsigned int header_size = 40;
    int image_width = 0; // Расчитываемое
    int image_height = 0; // Расчитываемое
    uint16_t sheet_count = 1;
    uint16_t bit_per_pixel = 24;
    unsigned int compress_type = 0;
    unsigned int data_byte_count = 0; // Расчитываемое: stride * image_height
    int horizontal_resolution = 11811;
    int vertical_resolution = 11811;
    int color_count = 0;
    int main_color_count = 0x1000000;

    bool IsValid() const {
        return (header_size == 40
                && sheet_count == 1
                && bit_per_pixel == 24
                && compress_type == 0
                && horizontal_resolution == 11811
                && vertical_resolution == 11811
                && color_count == 0
                && main_color_count == 0x1000000);
    }
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool CheckHeaderValid(const BitmapFileHeader& file_header, const BitmapInfoHeader& info_header) {
    if (!file_header.IsValid() || !info_header.IsValid()) {
        return false;
    }
    if (file_header.header_and_data_size != file_header.header_size + info_header.data_byte_count) {
        return false;
    }
    return true;
}

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);

    const auto w = image.GetWidth();
    const auto stride(GetBMPStride(w));

    BitmapInfoHeader info_header;
    info_header.image_width = image.GetWidth();
    info_header.image_height = image.GetHeight();
    info_header.data_byte_count = stride * image.GetHeight();
    BitmapFileHeader file_header;
    file_header.header_and_data_size = file_header.header_size + info_header.data_byte_count;
    out.write((char*) &file_header, sizeof(file_header));
    out.write((char*) &info_header, sizeof(info_header));

    std::vector<char> buff(stride);
    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        
        for (int x = 0; x < w; ++x) {
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
        }
        out.write(buff.data(), stride);
    }
    return out.good();
}

// напишите эту функцию
Image LoadBMP(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать данные в двоичном формате
    ifstream ifs(file, ios::binary);
    
    // считываем заголовки файла
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
    try {
        ifs.read((char*) &file_header, sizeof(BitmapFileHeader));
        ifs.read((char*) &info_header, sizeof(BitmapInfoHeader));
    } catch (...) {
        return Image();
    }
    if (!CheckHeaderValid(file_header, info_header)) {
        return Image();
    }    

    const auto stride(GetBMPStride(info_header.image_width));
    Image result(info_header.image_width, info_header.image_height, Color::Black());
    std::vector<char> buff(stride);
    for (int y = info_header.image_height - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), stride);

        for (int x = 0; x < info_header.image_width; ++x) {
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
        }
    }
    return result;
}

}  // namespace img_lib