#ifndef BMP_H
#define BMP_H

#include <stdio.h>
#include <stdlib.h>

#include <cstring>
class BMPGenerator {
private:
    int width, height;
    char* data;

public:
    //传入所需BMP文件weight和height
    BMPGenerator(int const& width, int const& height);
    ~BMPGenerator();
    //设置图片对应像素位置的RGB颜色值
    void setPixel(int const& x, int const& y, char const& R, char const& G, char const& B);
    //文件操作输出BMP文件
    void writeBMP();
    //辅助函数，用于将数据对齐到最近的4字节整数倍位置
    int alignTo4(int const& x);
};

#endif // BMP_H
