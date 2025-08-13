#include "generateImage.h"


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning (disable : 4996)

int generateImage::getWidht() {
    return this->width;
}

int generateImage::getHeight() {
    return this->height;
}

void generateImage::saveBitmapImage(unsigned char** image, int height, int width, char* imageFileName)
{
    unsigned char padding = 0;
    int paddingSize = (4 - (width) % 4) % 4;

    int stride = width+paddingSize;

    FILE* imageFile = fopen(imageFileName, "wb");

    unsigned char* fileHeader = createBitmapFileHeader(height, stride);
    fwrite(fileHeader, 1, FILE_HEADER_SIZE, imageFile);

    unsigned char* infoHeader = createBitmapInfoHeader(height, width);
    fwrite(infoHeader, 1, INFO_HEADER_SIZE, imageFile);

    unsigned char* colorHeader = createBitmapColorHeader();
    fwrite(colorHeader, 1, COLOR_TABLE_SIZE, imageFile);

    /*
    Bitmap images in memory and in BMP files have opposite vertical order by default:
    BitBlt assumes the top row comes first (top-down),
    BMP files are bottom-up by default, unless explicitly stored as top-down.
    So i write data in reverse order to match what is displayed in the app.
    */
    int i;
    for (i = height - 1; i >= 0; i--) {
        fwrite(image[i], BYTES_PER_PIXEL, width, imageFile);
        fwrite(&padding, 1, paddingSize, imageFile);
    }

    fclose(imageFile);
}

unsigned char* generateImage::createBitmapFileHeader(int height, int stride)
{
    int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + COLOR_TABLE_SIZE + (stride * height);

    static unsigned char fileHeader[] = {
        0,0,     /// signature
        0,0,0,0, /// image file size in bytes
        0,0,0,0, /// reserved
        0,0,0,0, /// start of pixel array
    };

    fileHeader[0] = (unsigned char)('B');
    fileHeader[1] = (unsigned char)('M');
    fileHeader[2] = (unsigned char)(fileSize);
    fileHeader[3] = (unsigned char)(fileSize >> 8);
    fileHeader[4] = (unsigned char)(fileSize >> 16);
    fileHeader[5] = (unsigned char)(fileSize >> 24);
    fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE + COLOR_TABLE_SIZE);

    return fileHeader;
}

unsigned char* generateImage::createBitmapInfoHeader(int height, int width)
{
    static unsigned char infoHeader[] = {
        0,0,0,0, /// header size
        0,0,0,0, /// image width
        0,0,0,0, /// image height
        0,0,     /// number of color planes
        0,0,     /// bits per pixel
        0,0,0,0, /// compression
        0,0,0,0, /// image size
        0,0,0,0, /// horizontal resolution
        0,0,0,0, /// vertical resolution
        24,0,0,0, /// colors in color table
        0,0,0,0, /// important color count
    };

    infoHeader[0] = (unsigned char)(INFO_HEADER_SIZE);
    infoHeader[4] = (unsigned char)(width);
    infoHeader[5] = (unsigned char)(width >> 8);
    infoHeader[6] = (unsigned char)(width >> 16);
    infoHeader[7] = (unsigned char)(width >> 24);
    infoHeader[8] = (unsigned char)(height);
    infoHeader[9] = (unsigned char)(height >> 8);
    infoHeader[10] = (unsigned char)(height >> 16);
    infoHeader[11] = (unsigned char)(height >> 24);
    infoHeader[12] = (unsigned char)(1);
    infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL * 8);
    infoHeader[32] = (unsigned char)(24);

    return infoHeader;
}

unsigned char* generateImage::createBitmapColorHeader() {
    static unsigned char colorHeader[]{
        115,98, 0,  0,  //0  - deep water
        148,150,0,  0,  //1  - flat water
        45, 198,207,0,  //2  - beach (sand A?)
        29, 70, 92, 0,  //3  - clay
        25, 145,23, 0,  //4  - meadow (meadow A)
        4,  105,4,  0,  //5  - deep forest (meadow C)
        5,  130,5,  0,  //6  - forest (meadow B)
        37, 52, 52, 0,  //7  - swampland
        69, 137,140,0,  //8  - stony desert (desert E)
        0,  0,  0,  0,  //9  - border
        66, 66, 66, 0,  //10 - mountain
        92, 92, 92, 0,  //11 - this is in palette file idk why
        0,  126,200,0,  //12 - this is in palette file idk why
        255,255,255,0,  //13 - ice
        0,  0,  0,  0,  //14 - border again? this was in palette idk why
        115,115,115,0,  //15 - paving
        0,  115,109,0,  //16 - desert A
        0,  84, 110,0,  //17 - desert B
        62, 171,171,0,  //18 - desert C
        11, 94, 135,0,  //19 - desert D
        0,  255,255,0,  //20 - beach (sand B?)
        209,232,237,0,  //21 - snow
        17, 35, 140,0,  //22 - lava
        219,219,219,0   //23 - this is in palette file idk why
        //the colors have to be in this order, i guess because the map editor takes the index not the color values
    };
    return colorHeader;
}

void generateImage::createImage(char* imageFileName) {
    //char* imageFileName = (char*)"bitmapImage.bmp";
    saveBitmapImage(finalMap, height, width, imageFileName);
}

void generateImage::MakeBitmap(unsigned char*** map, HDC hdc)
{
    VOID* pvBits;          // pointer to DIB section 
    HBITMAP answer;
    BITMAPINFO bmi;
    HDC hdcMem;
    int x, y;
    int red, green, blue, alpha;

    // setup bitmap info   
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;         // four 8-bit components 
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = width * height * 4;

    hdcMem = CreateCompatibleDC(hdc);
    answer = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            /*red = (y * 255 / height);
            green = (x * 255 / width);
            blue = ((x + y) * 255 / (height + width));*/
            red = map[y][x][0];
            green = map[y][x][1];
            blue = map[y][x][2];
            alpha = 255;
            red = (red * alpha) >> 8;
            green = (green * alpha) >> 8;
            blue = (blue * alpha) >> 8;
            ((UINT32*)pvBits)[(height - y - 1) * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    }
    
    SelectObject(hdcMem, answer);
    BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

    DeleteDC(hdcMem);
    DeleteObject(answer);
}

void generateImage::createHeightMap(HDC hdc, unsigned int seed) {
    color lowHeiColor, midHeiColor, highHeiColor;
    lowHeiColor.b = 255;
    midHeiColor.g = 255;
    highHeiColor.r = 33; highHeiColor.g = 33; highHeiColor.b = 33;
    colors heiMapColor;
    heiMapColor.low = lowHeiColor; heiMapColor.med = midHeiColor; heiMapColor.high = highHeiColor;

    createMap(hdc, seed, heightMap, heightMapImage, waterLvl, mountainLvl, heiMapColor);
}

void generateImage::changeHeightMap(HDC hdc) {
    color lowHeiColor, midHeiColor, highHeiColor;
    lowHeiColor.b = 255;
    midHeiColor.g = 255;
    highHeiColor.r = 33; highHeiColor.g = 33; highHeiColor.b = 33;
    colors heiMapColor;
    heiMapColor.low = lowHeiColor; heiMapColor.med = midHeiColor; heiMapColor.high = highHeiColor;

    changeMap(hdc, heightMap, heightMapImage, waterLvl, mountainLvl, heiMapColor);
}

void generateImage::createTempMap(int minTemp, int maxTemp, HDC hdc, unsigned int seed) {
    color lowTempColor, midTempColor, highTempColor;
    lowTempColor.g = 255;
    midTempColor.r = 255;  midTempColor.g = 255;
    highTempColor.r = 255;
    colors tempMapColor;
    tempMapColor.low = lowTempColor; tempMapColor.med = midTempColor; tempMapColor.high = highTempColor;

    createMap(hdc, seed, tempMap, tempMapImage, coldLvl, warmLvl, tempMapColor);
}

void generateImage::changeTempMap(HDC hdc) {
    color lowTempColor, midTempColor, highTempColor;
    lowTempColor.g = 255;
    midTempColor.r = 255;  midTempColor.g = 255;
    highTempColor.r = 255;
    colors tempMapColor;
    tempMapColor.low = lowTempColor; tempMapColor.med = midTempColor; tempMapColor.high = highTempColor;

    changeMap(hdc, tempMap, tempMapImage, coldLvl, warmLvl, tempMapColor);
}

void generateImage::createHumidMap(int minHumid, int maxHumid, HDC hdc, unsigned int seed) {
    color lowHumidColor, midHumidColor, highHumidColor;
    lowHumidColor.r = 255; lowHumidColor.g = 255;
    midHumidColor.g = 255;
    highHumidColor.b = 255;
    colors humidMapColor;
    humidMapColor.low = lowHumidColor; humidMapColor.med = midHumidColor; humidMapColor.high = highHumidColor;

    createMap(hdc, seed, humidMap, humidMapImage, dryLvl, humidLvl, humidMapColor);
}

void generateImage::changeHumidMap(HDC hdc) {
    color lowHumidColor, midHumidColor, highHumidColor;
    lowHumidColor.r = 255; lowHumidColor.g = 255;
    midHumidColor.g = 255;
    highHumidColor.b = 255;
    colors humidMapColor;
    humidMapColor.low = lowHumidColor; humidMapColor.med = midHumidColor; humidMapColor.high = highHumidColor;

    changeMap(hdc, humidMap, humidMapImage, dryLvl, humidLvl, humidMapColor);
}

void generateImage::createDetailMap(unsigned int seedpart, int detailLvl) {
    siv::PerlinNoise::seed_type seed = seedpart;//rand() % 100000;
    siv::PerlinNoise perlin{ seed };

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            double noise = perlin.octave2D_01((j * 0.01), (i * 0.01), detailLvl, 1.5);
            const int f = round(noise * 255);

            detailMap[i][j] = f;
        }
    }
}

void generateImage::makeFinalMap(HDC hdc, bool genBeaches, bool genLava) {
    unsigned char*** image = new unsigned char** [height];
    for (int i = 0; i < height; i++) {
        image[i] = new unsigned char* [width];
        for (int j = 0; j < width; j++) image[i][j] = new unsigned char[3];
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            //height below sea lvl
            if (heightMap[i][j] < waterLvl) {
                //and low temperature means ice
                if (tempMap[i][j] < coldLvl) {
                    image[i][j][0] = 255;
                    image[i][j][1] = 255;
                    image[i][j][2] = 255;
                    finalMap[i][j] = (unsigned char)13;
                }
                else {//otherwise water
                    if (heightMap[i][j] < waterLvl / 2) {
                        //deep water
                        image[i][j][0] = 0;
                        image[i][j][1] = 98;
                        image[i][j][2] = 115;
                        finalMap[i][j] = (unsigned char)0;
                    }
                    else {
                        //flat water
                        image[i][j][0] = 0;
                        image[i][j][1] = 150;
                        image[i][j][2] = 148;
                        finalMap[i][j] = (unsigned char)1;
                    }
                }

            }
            if (heightMap[i][j] >= waterLvl) {
                //land
                if (tempMap[i][j] < coldLvl) {
                    
                    if (humidMap[i][j] < dryLvl) {
                        //stony Desert
                        image[i][j][0] = 140;
                        image[i][j][1] = 137;
                        image[i][j][2] = 69;
                        finalMap[i][j] = (unsigned char)8;
                    }
                    else {
                        //deep forest
                        image[i][j][0] = 4;
                        image[i][j][1] = 105;
                        image[i][j][2] = 4;
                        finalMap[i][j] = (unsigned char)5;
                    }
                    if (humidMap[i][j] > humidLvl) {
                        //snow
                        image[i][j][0] = 237;
                        image[i][j][1] = 232;
                        image[i][j][2] = 209;
                        finalMap[i][j] = (unsigned char)21;
                    }
                }
                else {
                    if (humidMap[i][j] < dryLvl) {
                        //other desert
                        image[i][j][0] = 109;
                        image[i][j][1] = 115;
                        image[i][j][2] = 0;
                        finalMap[i][j] = (unsigned char)16;
                    }
                    else {
                        //forest
                        image[i][j][0] = 5;
                        image[i][j][1] = 130;
                        image[i][j][2] = 5;
                        finalMap[i][j] = (unsigned char)6;
                    }
                    if (humidMap[i][j] > humidLvl) {
                        //forest / deep forest
                        if (detailMap[i][j] > 127) {
                            //forest
                            image[i][j][0] = 5;
                            image[i][j][1] = 130;
                            image[i][j][2] = 5;
                            finalMap[i][j] = (unsigned char)6;
                        }
                        else {
                            //deep forest
                            image[i][j][0] = 4;
                            image[i][j][1] = 105;
                            image[i][j][2] = 4;
                            finalMap[i][j] = (unsigned char)5;
                        }
                    }
                    
                }
                if (tempMap[i][j] > warmLvl) {
                    if (humidMap[i][j] < dryLvl) {
                        //desert
                        image[i][j][0] = 171;
                        image[i][j][1] = 171;
                        image[i][j][2] = 62;
                        finalMap[i][j] = (unsigned char)18;
                    }
                    else {
                        //temperate
                        image[i][j][0] = 23;
                        image[i][j][1] = 145;
                        image[i][j][2] = 25;
                        finalMap[i][j] = (unsigned char)4;
                    }
                    if (humidMap[i][j] > humidLvl) {
                        if (detailMap[i][j] > 150) {
                            //clay
                            image[i][j][0] = 92;
                            image[i][j][1] = 70;
                            image[i][j][2] = 29;
                            finalMap[i][j] = (unsigned char)3;
                        }
                        else {
                            //swamp
                            image[i][j][0] = 52;
                            image[i][j][1] = 52;
                            image[i][j][2] = 37;
                            finalMap[i][j] = (unsigned char)7;
                        }
                    }
                }
                if (genBeaches && heightMap[i][j] < waterLvl + 10 && tempMap[i][j] > coldLvl) {
                    //if theres a swamp dont make a beach
                    if (tempMap[i][j] > warmLvl && tempMap[i][j] > humidLvl) {
                        continue;
                    }
                    //beach
                    image[i][j][0] = 207;
                    image[i][j][1] = 198;
                    image[i][j][2] = 45;
                    finalMap[i][j] = (unsigned char)2;
                }
            }
            if (heightMap[i][j] > mountainLvl) {
                //mountains
                image[i][j][0] = 66;
                image[i][j][1] = 66;
                image[i][j][2] = 66;
                finalMap[i][j] = (unsigned char)(10);

                if (genLava && tempMap[i][j] > warmLvl && detailMap[i][j] > 170) {
                    //lava
                    image[i][j][0] = 140;
                    image[i][j][1] = 35;
                    image[i][j][2] = 17;
                    finalMap[i][j] = (unsigned char)(22);
                }
            }
        }
    }

    MakeBitmap(image, hdc);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) delete[] image[i][j];
        delete[] image[i];
    }
    delete[] image;
}

void generateImage::createMap(HDC hdc, unsigned int seedpart, unsigned int** map, HBITMAP mapImage, unsigned int lowLvl, unsigned int highLvl, colors mapColor) {

    siv::PerlinNoise::seed_type seed = seedpart;//rand() % 100000;
    siv::PerlinNoise perlin{ seed };

    unsigned char*** image = new unsigned char** [height];
    for (int i = 0; i < height; i++) {
        image[i] = new unsigned char* [width];
        for (int j = 0; j < width; j++) image[i][j] = new unsigned char[3];
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            double noise = perlin.octave2D_01((j * 0.01), (i * 0.01), 6);
            const int f = round(noise * 255);

            map[i][j] = f;

            if (f <= lowLvl) {
                image[i][j][0] = mapColor.low.r;
                image[i][j][1] = mapColor.low.g;
                image[i][j][2] = mapColor.low.b;
            }
            else {
                image[i][j][0] = mapColor.med.r;
                image[i][j][1] = mapColor.med.g;
                image[i][j][2] = mapColor.med.b;
            }
            if (f >= highLvl) {
                image[i][j][0] = mapColor.high.r;
                image[i][j][1] = mapColor.high.g;
                image[i][j][2] = mapColor.high.b;
            }
        }
    }

    MakeBitmap(image, hdc);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) delete[] image[i][j];
        delete[] image[i];
    }
    delete[] image;
}

void generateImage::changeMap(HDC hdc, unsigned int** map, HBITMAP mapImage, unsigned int lowLvl, unsigned int highLvl, colors mapColors) {

    unsigned char*** image = new unsigned char** [height];
    for (int i = 0; i < height; i++) {
        image[i] = new unsigned char* [width];
        for (int j = 0; j < width; j++) image[i][j] = new unsigned char[3];
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            const int f = map[i][j];

            if (f <= lowLvl) {
                image[i][j][0] = mapColors.low.r;
                image[i][j][1] = mapColors.low.g;
                image[i][j][2] = mapColors.low.b;
            }
            else {
                image[i][j][0] = mapColors.med.r;
                image[i][j][1] = mapColors.med.g;
                image[i][j][2] = mapColors.med.b;
            }
            if (f >= highLvl) {
                image[i][j][0] = mapColors.high.r;
                image[i][j][1] = mapColors.high.g;
                image[i][j][2] = mapColors.high.b;
            }
        }
    }

    MakeBitmap(image, hdc);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) delete[] image[i][j];
        delete[] image[i];
    }
    delete[] image;
}

generateImage::generateImage(int height, int width) : height(height), width(width) {
    this->heightMap = new unsigned int* [height];
    this->tempMap = new unsigned int* [height];
    this->humidMap = new unsigned int* [height];
    this->detailMap = new unsigned int* [height];
    this->finalMap = new unsigned char* [height];
    for (int i = 0; i < height; i++) {
        this->heightMap[i] = new unsigned int[width];
        this->tempMap[i] = new unsigned int[width];
        this->humidMap[i] = new unsigned int[width];
        this->detailMap[i] = new unsigned int[width];
        this->finalMap[i] = new unsigned char[width];
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            this->heightMap[i][j] = 0;
            this->tempMap[i][j] = 0;
            this->humidMap[i][j] = 0;
            this->detailMap[i][j] = 0;
            this->finalMap[i][j] = 0;
        }
    }
}

generateImage::~generateImage(){
    for (int i = 0; i < height; i++) {
        delete[] this->heightMap[i];
        delete[] this->tempMap[i];
        delete[] this->humidMap[i];
        delete[] this->detailMap[i];
        delete[] this->finalMap[i];
    }
    delete[] this->heightMap;
    delete[] this->tempMap;
    delete[] this->humidMap;
    delete[] this->detailMap;
    delete[] this->finalMap;
}
