#include "bmp.h"

#include <QDebug>
#include <cassert>
#include <fstream>
#include <iostream>

BMPGenerator::BMPGenerator(int const& _width, int const& _height) { //对象初始化
    width = _width;
    height = _height;
    int size = _width * _height * 3 + 30;
    data = new char[size];
    memset(data, 0, size * sizeof(char));
    for (int i = 0; i < size; i++) data[i] = 0;
}

BMPGenerator::~BMPGenerator() { //销毁对象
    delete[] data;
}

void BMPGenerator::setPixel(int const& x, int const& y, char const& R, char const& G, char const& B) { //设置指定区域的颜色
    assert(x >= 0 && x < width && y >= 0 && y < height);
    int offset = (y * width + x) * 3; //计算二维坐标在数组内映射
    data[offset + 0] = R;
    data[offset + 1] = G;
    data[offset + 2] = B;
}

int BMPGenerator::alignTo4(int const& x) { //BMP规范 向之后的4的整数倍字节对齐
    return (x % 4 == 0) ? x : x - x % 4 + 4;
}

void BMPGenerator::writeBMP() { //输出BMP文件
    int bitmap_size = height * alignTo4(width * 3);

    char* bitmap = new char[bitmap_size];
    memset(bitmap, 0, bitmap_size * sizeof(char)); //置零
    for (int y = 0; y < height; y++) {
        //按照BMP规范重排像素顺序，自下而上、从左向右存储
        for (int x = 0; x < width; x++) {
            for (int color = 0; color < 3; color++) {
                bitmap[y * alignTo4(width * 3) + x * 3 + color] = data[((height - 1 - y) * width + x) * 3 + (2 - color)];
            }
        }
    }

    char filetag[] = {'B', 'M'}; //前2Bytes制定文件类型
    int header[] = {
        //文件头
        0,    // 文件大小
        0,    // 保留字节
        0x36, // 像素数据起点
        //信息头
        0x28,          // 信息头大小
        width, height, // 像素数目
        0x180001,      // 前2bytes位深（0x18）24 bits 后2bytes色彩平面（0x1） 1 color plane
        0,             // 无压缩
        0,             // 压缩后数据大小（ignored）
        0,             // x轴显示密度（ignored）
        0,             // y轴显示密度（ignored）
        0, 0,          // 24bits无需color pallete
    };
    // 更新文件头中的文件大小
    header[0] = sizeof(filetag) + sizeof(header) + bitmap_size * sizeof(char);

    FILE* fp = fopen("./1.bmp", "wb"); //!important 打开二进制文件必须使用wb修饰符 否则遇到0x0A会自动补充0x0D 出现莫名其妙的多余字节的诡异事件！！！
    fwrite(&filetag, sizeof(filetag), 1, fp);
    fwrite(&header, sizeof(header), 1, fp);
    fwrite(bitmap, bitmap_size * sizeof(char), 1, fp);
    fclose(fp);
    //    上面的代码等价于以下的输出流实现
    //    std::ofstream fp;
    //    fp.open("./1.bmp", std::ios::binary);
    //    fp.write((char*)&filetag, sizeof(filetag));
    //    fp.write((char*)&header, sizeof(header));
    //    fp.write((char*)bitmap, bitmap_size * sizeof(char));
    //    fp.close();

    delete[] bitmap;
}
