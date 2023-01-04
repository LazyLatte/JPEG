#include "jpeg.h"
#include <vector>
#include <iostream>
#include <map>
#include <cmath>
#include <assert.h>
#include <algorithm>
#include <fstream>
#define PI 3.14159
using namespace std;

uint8_t mode444[3] = {0x11, 0x11, 0x11};
uint8_t mode422[3] = {0x21, 0x11, 0x11};
uint8_t mode420[3] = {0x22, 0x11, 0x11};
map<string, uint8_t*> sampling_ratio = {
    {"444", mode444},
    {"422", mode422},
    {"420", mode420}
};

const unsigned q_table[2][8][8] = {
    {{16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 67, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}},

    {{17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}}
};

const int z_table[8][8][2] = {
    {{0, 0}, {0, 1}, {1, 0}, {2, 0}, {1, 1}, {0, 2}, {0, 3}, {1, 2}},
    {{2, 1}, {3, 0}, {4, 0}, {3, 1}, {2, 2}, {1, 3}, {0, 4}, {0, 5}},
    {{1, 4}, {2, 3}, {3, 2}, {4, 1}, {5, 0}, {6, 0}, {5, 1}, {4, 2}},
    {{3, 3}, {2, 4}, {1, 5}, {0, 6}, {0, 7}, {1, 6}, {2, 5}, {3, 4}},
    {{4, 3}, {5, 2}, {6, 1}, {7, 0}, {7, 1}, {6, 2}, {5, 3}, {4, 4}},
    {{3, 5}, {2, 6}, {1, 7}, {2, 7}, {3, 6}, {4, 5}, {5, 4}, {6, 3}},
    {{7, 2}, {7, 3}, {6, 4}, {5, 5}, {4, 6}, {3, 7}, {4, 7}, {5, 6}},
    {{6, 5}, {7, 4}, {7, 5}, {6, 6}, {5, 7}, {6, 7}, {7, 6}, {7, 7}}
};

// (category) code
const std::string DC_Htable[2][12] = {
    {"00", "010", "011", "100", "101", "110", "1110", "11110", "111110", "1111110", "11111110", "111111110"},   // luminance
    {"00", "01", "10", "110", "1110", "11110", "111110", "1111110", "11111110", "111111110", "1111111110", "11111111110"}   // chrominance
}; 

// bit length 1~16
const uint8_t DC_codeLength_num[2][16] = { 
    {0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // luminance
    {0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}    // chrominance
};  
// (category) for lum & chrom
const uint8_t DC_symbol[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b}; 

// (numOfZero, category) code
const std::string AC_Htable[2][16][11] = {
    // luminance
    {{"1010", "00", "01", "100", "1011", "11010", "1111000", "11111000", "1111110110", "1111111110000010", "1111111110000011"},
    {"", "1100", "11011", "1111001", "111110110", "11111110110", "1111111110000100", "1111111110000101", "1111111110000110", "1111111110000111", "1111111110001000"},
    {"", "11100", "11111001", "1111110111", "111111110100", "1111111110001001", "1111111110001010", "1111111110001011", "1111111110001100", "1111111110001101", "1111111110001110"},
    {"", "111010", "111110111", "111111110101", "1111111110001111", "1111111110010000", "1111111110010001", "1111111110010010", "1111111110010011", "1111111110010100", "1111111110010101"},
    {"", "111011", "1111111000", "1111111110010110", "1111111110010111", "1111111110011000", "1111111110011001", "1111111110011010", "1111111110011011", "1111111110011100", "1111111110011101"},
    {"", "1111010", "11111110111", "1111111110011110", "1111111110011111", "1111111110100000", "1111111110100001", "1111111110100010", "1111111110100011", "1111111110100100", "1111111110100101"},
    {"", "1111011", "111111110110", "1111111110100110", "1111111110100111", "1111111110101000", "1111111110101001", "1111111110101010", "1111111110101011", "1111111110101100", "1111111110101101"},
    {"", "11111010", "111111110111", "1111111110101110", "1111111110101111", "1111111110110000", "1111111110110001", "1111111110110010", "1111111110110011", "1111111110110100", "1111111110110101"},
    {"", "111111000", "111111111000000", "1111111110110110", "1111111110110111", "1111111110111000", "1111111110111001", "1111111110111010", "1111111110111011", "1111111110111100", "1111111110111101"},
    {"", "111111001", "1111111110111110", "1111111110111111", "1111111111000000", "1111111111000001", "1111111111000010", "1111111111000011", "1111111111000100", "1111111111000101", "1111111111000110"},
    {"", "111111010", "1111111111000111", "1111111111001000", "1111111111001001", "1111111111001010", "1111111111001011", "1111111111001100", "1111111111001101", "1111111111001110", "1111111111001111"},
    {"", "1111111001", "1111111111010000", "1111111111010001", "1111111111010010", "1111111111010011", "1111111111010100", "1111111111010101", "1111111111010110", "1111111111010111", "1111111111011000"},
    {"", "1111111010", "1111111111011001", "1111111111011010", "1111111111011011", "1111111111011100", "1111111111011101", "1111111111011110", "1111111111011111", "1111111111100000", "1111111111100001"},
    {"", "11111111000", "1111111111100010", "1111111111100011", "1111111111100100", "1111111111100101", "1111111111100110", "1111111111100111", "1111111111101000", "1111111111101001", "1111111111101010"},
    {"", "1111111111101011", "1111111111101100", "1111111111101101", "1111111111101110", "1111111111101111", "1111111111110000", "1111111111110001", "1111111111110010", "1111111111110011", "1111111111110100"},
    {"11111111001", "1111111111110101", "1111111111110110", "1111111111110111", "1111111111111000", "1111111111111001", "1111111111111010", "1111111111111011", "1111111111111100", "1111111111111101", "1111111111111110"}},
    // chrominance
    {{"00", "01", "100", "1010", "11000", "11001", "111000", "1111000", "111110100", "1111110110", "111111110100"},
    {"", "1011", "111001", "11110110", "111110101", "11111110110", "111111110101", "1111111110001000", "1111111110001001", "1111111110001010", "1111111110001011"},
    {"", "11010", "11110111", "1111110111", "111111110110", "111111111000010", "1111111110001100", "1111111110001101", "1111111110001110", "1111111110001111", "1111111110010000"},
    {"", "11011", "11111000", "1111111000", "111111110111", "1111111110010001", "1111111110010010", "1111111110010011", "1111111110010100", "1111111110010101", "1111111110010110"},
    {"", "111010", "111110110", "1111111110010111", "1111111110011000", "1111111110011001", "1111111110011010", "1111111110011011", "1111111110011100", "1111111110011101", "1111111110011110"},
    {"", "111011", "1111111001", "1111111110011111", "1111111110100000", "1111111110100001", "1111111110100010", "1111111110100011", "1111111110100100", "1111111110100101", "1111111110100110"},
    {"", "1111001", "11111110111", "1111111110100111", "1111111110101000", "1111111110101001", "1111111110101010", "1111111110101011", "1111111110101100", "1111111110101101", "1111111110101110"},
    {"", "1111010", "11111111000", "1111111110101111", "1111111110110000", "1111111110110001", "1111111110110010", "1111111110110011", "1111111110110100", "1111111110110101", "1111111110110110"},
    {"", "11111001", "1111111110110111", "1111111110111000", "1111111110111001", "1111111110111010", "1111111110111011", "1111111110111100", "1111111110111101", "1111111110111110", "1111111110111111"},
    {"", "111110111", "1111111111000000", "1111111111000001", "1111111111000010", "1111111111000011", "1111111111000100", "1111111111000101", "1111111111000110", "1111111111000111", "1111111111001000"},

    {"", "111111000", "1111111111001001", "1111111111001010", "1111111111001011", "1111111111001100", "1111111111001101", "1111111111001110", "1111111111001111", "1111111111010000", "1111111111010001"},
    {"", "111111001", "1111111111010010", "1111111111010011", "1111111111010100", "1111111111010101", "1111111111010110", "1111111111010111", "1111111111011000", "1111111111011001", "1111111111011010"},
    {"", "111111010", "1111111111011011", "1111111111011100", "1111111111011101", "1111111111011110", "1111111111011111", "1111111111100000", "1111111111100001", "1111111111100010", "1111111111100011"},
    {"", "11111111001", "1111111111100100", "1111111111100101", "1111111111100110", "1111111111100111", "1111111111101000", "1111111111101001", "1111111111101010", "1111111111101011", "1111111111101100"},
    {"", "11111111100000", "1111111111101101", "1111111111101110", "1111111111101111", "1111111111110000", "1111111111110001", "1111111111110010", "1111111111110011", "1111111111110100", "1111111111110101"},
    {"1111111010", "111111111000011", "1111111111110110", "1111111111110111", "1111111111111000", "1111111111111001", "1111111111111010", "1111111111111011", "1111111111111100", "1111111111111101", "1111111111111110"}}
};  
// bit length 1~16
const uint8_t AC_codeLength_num[2][16] = {
    {0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d},   // luminance
    {0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77}    // chrominance
};  
// (numOfZero, category)
const uint8_t AC_symbol[2][162] = {
    // luminance
    {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    },
    // chrominance
    {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    }
};  

JPEG::JPEG(){
    this->precision = 8;
    this->useDefaultHuffmanTable = true;
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            this->Qtable[0][i][j] = q_table[0][i][j];
            this->Qtable[1][i][j] = q_table[1][i][j];
        }
    }
}
JPEG::JPEG(const int lum_Qtable[8][8], const int chrom_Qtable[8][8], bool useDefaultHuffmanTable){
    this->precision = 8;
    this->useDefaultHuffmanTable = useDefaultHuffmanTable;
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            this->Qtable[0][i][j] = lum_Qtable[i][j];
            this->Qtable[1][i][j] = chrom_Qtable[i][j];
        }
    }
}

vector<vector<vector<unsigned char>>> JPEG::image_1D_to_2D(vector<unsigned char> image_1D, unsigned height, unsigned width, unsigned channel){
    vector<vector<vector<unsigned char>>> image_2D(channel, vector<vector<unsigned char>>(height, vector<unsigned char>(width)));
    for(int ch=0; ch<channel; ch++){
        for(int i=0; i<height; i++){
            for(int j=0; j<width; j++){
                image_2D[ch][i][j] = image_1D[channel*(i*width+j) + ch];
            }
        }
    }

    return image_2D;
}
void JPEG::RGB_to_YCbCr(vector<vector<vector<unsigned char>>>& image, unsigned height, unsigned width){
    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            int R = image[0][i][j];
            int G = image[1][i][j];
            int B = image[2][i][j];
            image[0][i][j] = (unsigned char)(0.257f*R + 0.564f*G + 0.098f*B + 16);
            image[1][i][j] = (unsigned char)(-0.148f*R - 0.291f*G + 0.439f*B + 128);
            image[2][i][j] = (unsigned char)(0.439f*R - 0.368f*G + 0.071f*B + 128);
        }
    }
}




vector<vector<float>> JPEG::leftShift(vector<vector<unsigned char>> block){
    vector<vector<float>> result(8, vector<float>(8));
    int shiftVal = 1<<(this->precision-1);
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            result[i][j] = (float)((int)block[i][j] - shiftVal);
        }
    }
    return result;
}

vector<float> JPEG::DCT_1D(vector<float> x){
    vector<float> y(8);

    for(int i=0; i<8; i++){
        y[i] = 0.0;
        for(int j=0; j<8; j++){
            float theta = ((2*j+1)*i*PI) / (2 * 8);
            y[i]+=(x[j]*cos(theta));
        }
        y[i] *= sqrt(2.0f / 8);
    }
    y[0] *= 0.70710678;
    return y;
}
void JPEG::DCT_2D_8x8(vector<vector<float>>& block){
    vector<vector<float>> dct_val(8, vector<float>(8));
    for(int i=0; i<8; i++){
        vector<float> row = DCT_1D(block[i]);
        for(int j=0; j<8; j++){
            dct_val[j][i] = row[j];
        }
    }

    for(int i=0; i<8; i++){
        vector<float> row = DCT_1D(dct_val[i]);
        for(int j=0; j<8; j++){
            block[j][i] = row[j];
        }
    }
}
vector<vector<int>> JPEG::quantization(vector<vector<float>> dct_8x8, bool Qtable_id){
    vector<vector<int>> Q(8, vector<int>(8));
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            if(dct_8x8[i][j]>=0){
                Q[i][j] = (int)floor(dct_8x8[i][j] / this->Qtable[Qtable_id][i][j]);
            }else{
                Q[i][j] = (int)floor(abs(dct_8x8[i][j]) / this->Qtable[Qtable_id][i][j])*-1;
            }

        }
    }
    return Q;
}
//------------------------------------------------
int get_value_category(int val){
    int i;
    val = abs(val);
    for(i=31; i>0; i--){
        if(val & (1 << (i-1))) break ; 
    }
    return i;
}

string get_value_code(int val, int category){
    string value_code="";
    if(val<0) val+=((1 << category) - 1);
    for(int i=0; i<category; i++){
        bool lsb = val & 1;
        value_code += lsb ? '1' : '0';
        val >>= 1;
    }
    reverse(value_code.begin(), value_code.end());
    return value_code;
}


string get_diffDC_code(int DiffDC, bool Htable_id){
    int category;
    string length_code;
    string value_code;

    category = (DiffDC == 0) ? 0 : get_value_category(DiffDC);
    length_code = DC_Htable[Htable_id][category];
    value_code = get_value_code(DiffDC, category);
    return length_code+value_code;
}
string encodeDC(int DC, int& pred_DC, bool Htable_id){
    int DiffDC = DC-pred_DC;
    pred_DC = DC;
    return get_diffDC_code(DiffDC, Htable_id);
}
bool checkIsSpecialCode(int numOfZero, int AC_val){
    if(AC_val == 0){
        if(numOfZero == 0 || numOfZero == 15){
            return true;
        }else{
            cout << "Invalid AC_RLE : (none-0, 15-value ,0)" << endl;
            return false;
        }
    }else{
        return false;
    }
}
string get_AC_RLE_code(pair<int, int> zeroNum_val_pair, bool Htable_id){
    int category;
    string length_code;
    string value_code;

    int numOfZero = zeroNum_val_pair.first;
    int AC_val = zeroNum_val_pair.second;

    bool isSpecialCode = checkIsSpecialCode(numOfZero, AC_val);
    category = isSpecialCode ? 0 : get_value_category(AC_val);
    length_code = AC_Htable[Htable_id][numOfZero][category];
    value_code = get_value_code(AC_val, category);
    return length_code+value_code;
}

vector<pair<int, int>> RLE(vector<int> AC_seq){
    int consecutive_zero = 0;
    vector<pair<int, int>> AC_RLE_seq;
    // (zero_num, val)
    for(auto iter = AC_seq.begin(); iter!=AC_seq.end(); iter++){
        int AC = *iter;
        if(AC!=0){
            pair<int, int> p;
            pair<int, int> ZRL = make_pair(15, 0); //zero run length
            while(consecutive_zero>=16){
                AC_RLE_seq.push_back(ZRL); 
                consecutive_zero-=16;
            }

            p = make_pair(consecutive_zero, AC);
            AC_RLE_seq.push_back(p);
            consecutive_zero = 0;

        }else{
            consecutive_zero++;
        } 
    }
    if(consecutive_zero>0) AC_RLE_seq.push_back(make_pair(0, 0));
    return AC_RLE_seq;
}

vector<int> zigzag(vector<vector<int>> quantized_8x8){
    vector<int> AC_seq;
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            if(i==0 && j==0) continue;

            int r, c;
            r = z_table[i][j][0];
            c = z_table[i][j][1];
            AC_seq.push_back(quantized_8x8[r][c]);
        }
    }
    return AC_seq;
}


string encodeAC(vector<vector<int>> quantized_8x8, bool Htable_id){
    vector<int> AC_zigzag_seq;
    vector<pair<int, int>> AC_RLE_seq;  // (zero_num, val)
    string AC_code = "";
    AC_zigzag_seq = zigzag(quantized_8x8);
    AC_RLE_seq = RLE(AC_zigzag_seq);

    for(auto iter=AC_RLE_seq.begin(); iter!=AC_RLE_seq.end(); iter++){
        pair<int, int> AC_RLE = *iter;
        string code = get_AC_RLE_code(AC_RLE, Htable_id);
        AC_code+=code;
    }
    return AC_code;
}

string JPEG::encodeBlock(vector<vector<unsigned char>> block, int& pred_DC, bool tableID){
    vector<vector<float>> D(8, vector<float>(8));
    vector<vector<int>> Q(8, vector<int>(8));
    D = this->leftShift(block);
    this->DCT_2D_8x8(D);
    Q = this->quantization(D, tableID);
    return encodeDC(Q[0][0], pred_DC, tableID) + encodeAC(Q, tableID);
}
string JPEG::encodeImage(vector<vector<vector<unsigned char>>> image, unsigned height, unsigned width, unsigned channel){
    string ecs = "";
    int max_H = *max_element(this->H, this->H+3);
    int max_V = *max_element(this->V, this->V+3);
    int MCU_H = max_V*8, MCU_W = max_H*8;

    int predDC[3] = {0, 0, 0};
    vector<vector<unsigned char>> block(8, vector<unsigned char>(8));

    for(int mcu_i=0; mcu_i<height; mcu_i+=MCU_H){
        for(int mcu_j=0; mcu_j<width; mcu_j+=MCU_W){
            // encode MCU (interleaved)
            for(int ch=0; ch<channel; ch++){
                int numOfBlock_x = this->H[ch];
                int numOfBlock_y = this->V[ch];
                int x_scaling = max_H / this->H[ch];
                int y_scaling = max_V / this->V[ch];
                
                for(int blockY_id=0; blockY_id < numOfBlock_y; blockY_id++){
                    for(int blockX_id=0; blockX_id < numOfBlock_x; blockX_id++){
                        // encode DataUnit (8x8 block)
                        for(int delta_i=0; delta_i<8; delta_i++){
                            for(int delta_j=0; delta_j<8; delta_j++){
                                int pos_i = mcu_i + (8 * blockY_id + delta_i) * y_scaling;
                                int pos_j = mcu_j + (8 * blockX_id + delta_j) * x_scaling;
                                block[delta_i][delta_j] = image[ch][pos_i][pos_j];
                            }
                        }
                        ecs += encodeBlock(block, predDC[ch], ch!=0);
                    }
                }
            }
        }
    }
    return ecs;
}
void JPEG::compressImage(vector<unsigned char> raw_image, unsigned height, unsigned width, ColorType type, string mode){
    assert(raw_image.size() == height*width*type);
    assert(!mode.compare("444") || !mode.compare("422") || !mode.compare("420"));
    assert((type==GREY && !mode.compare("444")) || type == RGB);
    //-----init------
    this->channel = type;
    this->height = height;
    this->width = width;
    this->type = type;
    this->sampling_mode = mode;

    uint8_t* triple = sampling_ratio[mode];
    for(int i=0; i<3; i++){
        this->H[i] = int(triple[i] >> 4);
        this->V[i] = int(triple[i] & 0b00001111);
    }
    //------------------
    vector<vector<vector<unsigned char>>> image = this->image_1D_to_2D(raw_image, height, width, type);
    if(type == RGB){
        cout << "RGB to YCbCr..." <<endl;
        this->RGB_to_YCbCr(image, height, width);
    }
    //-----compress-----
    cout<<"Start compression..." <<endl;
    this->ecs = this->encodeImage(image, height, width, type);
    cout<<"End compression..." <<endl;
}
//------------------------------------------------------------------------

void JPEG::write_jpeg(){
    int tableNum =  (this->type==GREY) ? 1 : 2;
    cout << "Start writing file..." << endl;
    cout<<this->ecs.size()<<endl;
    ofstream out;
    out.open("out.jfif", ios::out | ios::binary);

    //-----SOI-----//
    out << uint8_t(0xff) << uint8_t(0xd8);
    //-----APP0-----//
    out << uint8_t(0xff) <<uint8_t(0xe0); //marker
    out << uint8_t(0x00) << uint8_t(0x10); //length=16
    out << uint8_t(0x4A) << uint8_t(0x46) << uint8_t(0x49) << uint8_t(0x46) << uint8_t(0x00); // JFIF
    out << uint8_t(0x01) << uint8_t(0x01); //version
    out << uint8_t(0x01); //density units
    out << uint8_t(0x00) << uint8_t(0x48); //x-density
    out << uint8_t(0x00) << uint8_t(0x48); //y-sensity
    out << uint8_t(0x00); // x-thumbnail
    out << uint8_t(0x00); // y-thumbnail

    //-----DQT-----//
    for(int table_id=0; table_id<tableNum; table_id++){
        out << uint8_t(0xff) << uint8_t(0xdb);
        out << uint8_t(0x00) << uint8_t(0x43); //length = 67 Byte
        out << uint8_t(table_id); // table_id
        for(int i=0; i<8; i++){
            for(int j=0; j<8; j++){
                int r, c;
                r = z_table[i][j][0];
                c = z_table[i][j][1];
                out << uint8_t(q_table[table_id][r][c]);
                
            }
        }
    }

    //-----SOF-----//
    out << uint8_t(0xff) << uint8_t(0xc0);
    out << uint8_t(0x00) << uint8_t(3*this->channel + 8); // 1 or 3 channel length
    out << uint8_t(this->precision); // precision
    out << uint8_t(this->height>>8) << uint8_t(this->height); // height
    out << uint8_t(this->width>>8) << uint8_t(this->width);  // width
    out << uint8_t(this->channel); // #components

    for(int c_id = 0; c_id<this->channel; c_id++){
        out << uint8_t(c_id); // component i identifier
        out << sampling_ratio[this->sampling_mode][c_id]; // (horizontal sampling factor, vertical sampling factor)
        out << ((c_id==0) ? uint8_t(0x00) : uint8_t(0x01)); //component's Q_table 
    }


    //-----DHT-----//
    // DC lum & chrom
    for(int table_id=0; table_id<tableNum; table_id++){
        out << uint8_t(0xff) << uint8_t(0xc4);
        out << uint8_t(0x00) << uint8_t(0x1F); // length = 31 bytes
        out << ((table_id==0) ? uint8_t(0x00) : uint8_t(0x01)); // (Huffman type (DC = 0), Huffman identifier (0~3))

        for(int i=0; i<16; i++) out << DC_codeLength_num[table_id][i]; // # code word with bit_length = (1~16)
        for(int i=0; i<12; i++) out << DC_symbol[i]; // symbol (12 category)
    }

    // AC lum & chrom
    for(int table_id=0; table_id<tableNum; table_id++){
        out << uint8_t(0xff) << uint8_t(0xc4);
        out << uint8_t(0x00) << uint8_t(0xb5); // length = 181 bytes
        out << ((table_id==0) ? uint8_t(0x10) : uint8_t(0x11)); // (Huffman type (AC = 1), Huffman identifier (0~3))

        for(int i=0; i<16; i++) out << AC_codeLength_num[table_id][i]; // # code word with bit_length = (1~16)
        for(int i=0; i<162; i++) out << AC_symbol[table_id][i]; // symbol {162 (numOfZero, category)}
    }

    //-----SOS-----//
    out << uint8_t(0xff) << uint8_t(0xda);
    out << uint8_t(0x00) << uint8_t(2*this->channel + 6); // length
    out << uint8_t(this->channel); // #component per scan
    for(int c_id = 0; c_id < this->channel; c_id++){
        out << uint8_t(c_id); // component i identifier
        out << ((c_id==0) ? uint8_t(0x00) : uint8_t(0x11)); // (component i DC huffman identifier (0~1) , component i AC huffman identifier (0~1))
    }
    out << uint8_t(0x00); // ss
    out << uint8_t(0x3F); // se
    out << uint8_t(0x00); // (Ah, AI)

    //-----ECS-----//

    unsigned char buffer = 0;
    unsigned count = 0;
    for(int i=0; i<this->ecs.size(); i++){
        int bit = this->ecs[i] - '0';

        buffer<<=1;
        buffer |= bit;
        count++;
        if(count == 8){
            out << buffer;
            if(buffer==0xff) out<<uint8_t(0x00);
            buffer = 0;
            count = 0;
        }
    }
    if(count > 0){

        unsigned stuff_bits = 8-count;
        for(unsigned i=0; i<stuff_bits; i++){
            buffer<<=1;
            buffer |= 1;
        }
        out<<buffer;
    }

    //-----EOI-----//
    out << uint8_t(0xff) << uint8_t(0xd9);
    out.close();
    cout << "End writing file..." << endl;
}
