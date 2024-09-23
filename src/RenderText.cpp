
#include <iostream>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "RenderText.h"

# define RENDER_STR_OK 0


int pasteImg(unsigned char* backgroundImg, unsigned char* cutout, int wBackground, int hBackground, int wCutout, int hCutout, int x, int y) {
	if (x < 0 || y < 0) {
		printf("negative position! background: %d %d, img width: %d %d, pos: %d %d\n", wBackground, hBackground, wCutout, hCutout, x, y);
		return -1;
	}
	if (x + wCutout > wBackground || y + hCutout > hBackground) {
		printf("image overflow! background: %d %d, img width: %d %d, pos: %d %d\n", wBackground, hBackground, wCutout, hCutout, x, y);

		return -1;
	}
	// xpos and ypos refer to the position of the pixel in the small image (cutout)
	for (int xpos = 0; xpos < wCutout; xpos++) {
		for (int ypos = 0; ypos < hCutout; ypos++) {
			unsigned char copyVal = cutout[ypos * wCutout + xpos];
			int copyTo = (y + ypos) * wBackground + x + xpos;
			backgroundImg[copyTo] = copyVal;
		}
	}
	return RENDER_STR_OK;
}

unsigned char* pad4(unsigned char* buffer, int w, int h, int* neww, int* newh) {
	int newW = w / 4 * 4 + 4, newH = h / 4 * 4 + 4;
	unsigned char* newBuffer = (unsigned char*)malloc(newW * newH);
	int numPix = 0;
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			numPix = i * w + j;
			newBuffer[i * newW + j] = buffer[i * w + j];
		}
	}
	for (int i = 0; i < newH; i++) {
		for (int j = w; j < newW; j++) {
			newBuffer[i * newW + j] = 0;
		}
	}
	for (int i = h; i < newH; i++) {
		for (int j = 0; j < w; j++) {
			newBuffer[i * newW + j] = 0;
		}
	}
	*neww = newW; *newh = newH;
	return newBuffer;
}

unsigned char* makeRGB(unsigned char* buffer, int w, int h, int backgroundColorCode) {
	int r = (backgroundColorCode >> 16) & 0xFF,
		g = (backgroundColorCode >> 8) & 0xFF,
		b = (backgroundColorCode) & 0xFF;
	unsigned char* newBuffer = (unsigned char*)malloc(w * h * 3);
	int numPix = 0;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			if (backgroundColorCode == 0) {
				newBuffer[3 * numPix] = newBuffer[3 * numPix + 1] = newBuffer[3 * numPix + 2] = buffer[numPix];
			}
			else {
				char oldVal = buffer[numPix];
				if (oldVal == 0) {
					newBuffer[3 * numPix] = r;
					newBuffer[3 * numPix + 1] = g;
					newBuffer[3 * numPix + 2] = b;
				}
			}
			numPix += 1;
		}
	}
	return newBuffer;
}

using namespace std;


unsigned int strToCharCode(char* s, int lenStr) {
	int x = 0;
	int fact = 1;
	for (int i = lenStr - 1; i >= 0; i--) {
		unsigned int ch = (unsigned int)s[i] & 0xff;

		x += ((unsigned int)(ch & 0xff)) * fact;

		fact *= 256;
	}

	unsigned int fromLowByte = x % 64,
		fromHighByte = (x / 256) % 32;
	unsigned int result = fromHighByte * 64 + fromLowByte;
	return result;
}

int countLeadingOnes(char c) {
	int count = 0;
	for (; count < 8; count++) {
		int bit = (c >> (7 - count)) & 0x1;
		if (!bit) break;
	}
	return count;
}

vector<int> getCharcodes(char* str, int lenStr) {
	vector<int> codes;
	for (int i = 0; i < lenStr; i++) {
		if (((str[i] >> 7) & 0x1) == 0) {
			codes.push_back(str[i]);
			continue;
		} else {
			int numOnes = countLeadingOnes(str[i]);
			
			if (i + numOnes >= lenStr) break;
			int code = strToCharCode(str + i, numOnes);
			codes.push_back(code);
			i += numOnes - 1;
		}
	}
	return codes;
}

int Renderer::getStringImage(char* str, int lenStr, int fontHeight, int padding, unsigned char** result, int* imgw, int* imgh, int backgroundColorCode) {
	vector<int> codes = getCharcodes(str, lenStr);
	lenStr = codes.size();

	charImg* charImgs = (charImg*)malloc(lenStr * sizeof(charImg));
	if (FT_Set_Pixel_Sizes(face, 0, fontHeight)) {
		std::cout << "ERROR::FREETYPE: Failed to set pixel sizes" << std::endl;
		free(charImgs);
		return -1;
	}
	for (int i = 0; i < lenStr; i++) {
		auto existingCharImg = renderedChars.find(codes[i]);
		if (existingCharImg != renderedChars.end()) {
			charImgs[i] = existingCharImg->second;
			continue;
		}
		unsigned long c = FT_Get_Char_Index(face, (FT_ULong)codes[i]);

		if (FT_Load_Glyph(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			free(charImgs);
			return -1;
		}

		FT_Bitmap bitmap = face->glyph->bitmap;
		unsigned char* bufferCopy = (unsigned char*)malloc(bitmap.width * bitmap.rows);
		memcpy(bufferCopy, bitmap.buffer, bitmap.width * bitmap.rows);

		FT_Glyph_Metrics metr = face->glyph->metrics;
		unsigned int contentWidth = metr.width / 64,
			totalWidth = metr.horiAdvance / 64;
		if (totalWidth < contentWidth) {
			totalWidth = contentWidth;
		}
		unsigned int height = metr.height / 64;
		auto newCharImg = charImg{ bufferCopy, contentWidth, height, (unsigned int)metr.horiBearingY / 64, height - metr.horiBearingY / 64, (totalWidth - contentWidth) / 2, totalWidth };
		renderedChars.insert({ codes[i], newCharImg });
		charImgs[i] = newCharImg;
	}
	unsigned int width = 0, maxAbove = 0, maxBelow = 0;

	for (int i = 0; i < lenStr; i++) {
		charImg img = charImgs[i];
		width += img.totalWidth;
		if (img.above > maxAbove) {
			maxAbove = img.above;
		}
		if (img.below > maxBelow) {
			maxBelow = img.below;
		}
	}
	unsigned int height = maxAbove + maxBelow;
	if (width * height < 0 || width * height > 1000000) {
		return -1;
	}
	unsigned char* pixels = (unsigned char*)malloc(width * height);
	for (int i = 0; i < width * height; i++) {
		pixels[i] = 0;
	}

	int currentX = 0;



	for (int i = 0; i < lenStr; i++) {
		charImg img = charImgs[i];
		int pasteStatus = pasteImg(pixels, img.pixels, width, height, img.contentWidth, img.height, currentX + img.leftMargin, (int)maxAbove - (int)img.above);
		if (pasteStatus != RENDER_STR_OK) {
			printf("Error pasting image!\n");
		}
		currentX += img.totalWidth;
	}
	unsigned char* newPixels;
	if (width % 4 != 0 || height % 4 != 0) {
		int neww, newh;
		newPixels = pad4(pixels, width, height, &neww, &newh);
		free(pixels);
		pixels = newPixels;
		width = neww; height = newh;
	}

	newPixels = makeRGB(pixels, width, height, backgroundColorCode);
	free(pixels);
	*imgw = width;
	*imgh = height;

	*result = newPixels;

	return RENDER_STR_OK;
}

void Renderer::initTheFT(const char* fontPath) {
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return;
	}

	if (FT_New_Face(ft, fontPath, 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}
}

void Renderer::freeMem() {
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}