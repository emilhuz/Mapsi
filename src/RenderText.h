#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>

typedef struct {
	unsigned char* pixels;
	unsigned int contentWidth, height;
	unsigned int above, below;
	unsigned int leftMargin, totalWidth;

} charImg;

class Renderer {
public:
	std::map<int, charImg> renderedChars;
	FT_Library ft;
	FT_Face face;
	int getStringImage(char* str, int lenStr, int fontHeight, int padding, unsigned char** result, int* imgw, int* imgh, int backgroundColorCode);
	void initTheFT(const char* fontPath);
	void freeMem();
};