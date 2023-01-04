
#include <vector>
#include <iostream>
#ifndef JPEG_H
#define JPEG_H

enum ColorType {GREY = 1, RGB = 3};  

class JPEG{
    private:
        unsigned height;
        unsigned width;
        unsigned channel;
        unsigned precision;
        ColorType type;
        std::string ecs;
        std::string sampling_mode;
        int H[3];
        int V[3];
        
        int Qtable[2][8][8];
        bool useDefaultHuffmanTable;
        
        std::vector<std::vector<std::vector<unsigned char>>> image_1D_to_2D(std::vector<unsigned char> image_1D, unsigned height, unsigned width, unsigned channel);
        void RGB_to_YCbCr(std::vector<std::vector<std::vector<unsigned char>>>& image, unsigned height, unsigned width);
        std::vector<std::vector<float>> leftShift(std::vector<std::vector<unsigned char>> block);
        std::vector<float> DCT_1D(std::vector<float> x);
        void DCT_2D_8x8(std::vector<std::vector<float>>& block);
        std::vector<std::vector<int>> quantization(std::vector<std::vector<float>> image_8x8, bool Qtable_id);

        std::string encodeBlock(std::vector<std::vector<unsigned char>> block, int& pred_DC, bool tableID);
        std::string encodeImage(std::vector<std::vector<std::vector<unsigned char>>> image, unsigned height, unsigned width, unsigned channel);
    public:
        JPEG();
        JPEG(const int lum_Qtable[8][8], const int chrom_Qtable[8][8], bool useDefaultHuffmanTable);
        void compressImage(std::vector<unsigned char> image, unsigned height, unsigned width, ColorType type, std::string sampling_mode="444");
        void write_jpeg();

};

#endif /*JPEG_H*/