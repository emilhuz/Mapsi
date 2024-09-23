#include <vector>
#include <math.h>
#include <iostream>

#include "RenderText.h"


using namespace std;

unsigned char* make_img(char const* filename, int* x, int* y, int* comp, int req_comp);

class Pane2D {
public:
	std::vector<float> points_x, points_y;
	std::vector <float> screen_points_x, screen_points_y;

	float xLeft = 0, yBottom = 0;
	float w = 1, h = 1;
	float scaleFactor = 1;
	float zoomMultiplier = 1.5f;

	float xPoint = 0, yPoint = 0;
	float xScreen = 0, yScreen = 0;

	void mapToScreen_arrays(float* points, float* results, int numPoints);

	void mapToScreen(float x, float y);

	void mapFromScreen(float x, float y);

	void setDim(float screen_w, float screen_h);

	void zoom(float zoom_amount, float zoom_x, float zoom_y, bool reposition_points);

};



class TileTextureContainer {
private:
	void loadTexture(string fileName, int textureIndex, unsigned int* id);

public:
    Pane2D* paneRef;
    unsigned int* textureIDs;
    int numTextures;
    float* tilePositions;

    void loadTextures();

    void freeMem();

    void drawTextureAtInd(int ind);

    void drawTextures();
};






class StreetNameContainer {
public:
    int numNameTexturesMade;
    int* namesLoaded;
    unsigned int* nameTextures;
    vector <string> names;
    Renderer renderer;

    unsigned int* nameImgDimensions;

    float nameTextureScale = 1.0;
    int FONT_HEIGHT = 50;

    float winW, winH;

    void readNames(const char* namesPath);

    void makeNameTexture(int id);

    void drawName(int id);

    void freeMem();
};





void drawBoundTextureGL(float x, float y, float w, float h);