#pragma once
#include <vector>
#include <stdio.h>
#include <cmath>
#include <windows.h>
#include "PerlinNoise.hpp"

class generateImage
{
private:
	struct color {
		int r = 0;
		int g = 0;
		int b = 0;
	};
	struct colors {
		color low;
		color med;
		color high;
	};
	const int BYTES_PER_PIXEL = 1;
	const int FILE_HEADER_SIZE = 14;
	const int INFO_HEADER_SIZE = 40;
	const int COLOR_TABLE_SIZE = 96;
	const int height;
	const int width;
	HBITMAP heightMapImage = NULL;
	unsigned int** heightMap;
	HBITMAP tempMapImage = NULL;
	unsigned int** tempMap;
	HBITMAP humidMapImage = NULL;
	unsigned int** humidMap;
	HBITMAP finalMapImage = NULL;
	unsigned int** detailMap;
	unsigned char** finalMap;
	unsigned char* createBitmapFileHeader(int height, int stride);
	unsigned char* createBitmapInfoHeader(int height, int width);
	unsigned char* createBitmapColorHeader();
	void saveBitmapImage(unsigned char** image, int height, int width, char* imageFileName);
public:
	unsigned int waterLvl = 100;
	unsigned int mountainLvl = 200;
	unsigned int coldLvl = 100;
	unsigned int warmLvl = 200;
	unsigned int humidLvl = 200;
	unsigned int dryLvl = 100;
	generateImage(int height, int width);
	~generateImage();
	int getWidht();
	int getHeight();
	void createImage(char* imageFileName);
	//void createBitmap();
	void createHeightMap(HDC hdc, unsigned int seed);
	void createTempMap(int minTemp, int maxTemp, HDC hdc, unsigned int seed);
	void createHumidMap(int minHumid, int maxHumid, HDC hdc, unsigned int seed);
	void createDetailMap(unsigned int seedpart, int detailLvl);
	void MakeBitmap(unsigned char***, HDC hdc);
	void changeHeightMap(HDC hdc);
	void changeTempMap(HDC hdc);
	void changeHumidMap(HDC hdc);
	void makeFinalMap(HDC hdc, bool genBeaches, bool genLava);
	void createMap(HDC hdc, unsigned int seed, unsigned int** map, HBITMAP mapImage, unsigned int lowLvl, unsigned int highLvl, colors mapColors);
	void changeMap(HDC hdc, unsigned int** map, HBITMAP mapImage, unsigned int lowLvl, unsigned int highLvl, colors mapColors);
};

