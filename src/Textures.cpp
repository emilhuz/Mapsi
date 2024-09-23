#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Pane2D.h"

#include <iostream>
#include <fstream>
#include <sstream>


void drawBoundTextureGL(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    // going clockwise from top left
    glTexCoord2f(0, 0);
    glVertex2f(x, y);

    glTexCoord2f(1, 0);
    glVertex2f(x + w, y);

    glTexCoord2f(1, 1);
    glVertex2f(x + w, y - h);

    glTexCoord2f(0, 1);
    glVertex2f(x, y - h);
    glEnd();
}


void TileTextureContainer::loadTexture(string fileName, int textureIndex, unsigned int* id) {
    int imgwidth, imgheight, imgnrChannels;

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, id);
    glBindTexture(GL_TEXTURE_2D, *id);

    string loc = "Data/Imgs/Tiles/" + fileName;

    unsigned char* imgData = make_img(loc.c_str(), &imgwidth, &imgheight, &imgnrChannels, 0);
    if (!imgData) {
        cout << "error loading image" << endl;
        return;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgwidth, imgheight, 0, GL_RGB, GL_UNSIGNED_BYTE, (const void*)imgData);

}

void TileTextureContainer::loadTextures() {
    std::ifstream inputFile("Data/Imgs/tile_pos.txt", std::ios::in | std::ios::binary);
    string imagename;
    float pos;
    vector<float> positions;
    vector<GLuint> currentTextureIDs;
    int num_rows = 0;

    while (!inputFile.eof()) {
        inputFile >> imagename;
        if (imagename.length() == 0) {
            cout << "got empty image name?" << endl;
            return;
        }
        for (int i = 0; i < 4; i++) {
            inputFile >> pos;
            positions.push_back(pos);
        }
        GLuint newID;
        loadTexture(imagename, num_rows, &newID);
        currentTextureIDs.push_back(newID);
        num_rows++;
    }

    numTextures = num_rows;
    textureIDs = (GLuint*)malloc(numTextures * sizeof(GLuint));

    for (int i = 0; i < numTextures; i++) {
        textureIDs[i] = currentTextureIDs[i];
    }
    tilePositions = (float*)malloc(numTextures * 4 * sizeof(float));

    for (int i = 0; i < numTextures * 4; i++) {
        tilePositions[i] = positions[i];
    }
}

void TileTextureContainer::freeMem() {
    free(tilePositions);
    free(textureIDs);
}

void TileTextureContainer::drawTextureAtInd(int ind) {
    GLuint id = textureIDs[ind];
    glBindTexture(GL_TEXTURE_2D, id);
    glBegin(GL_QUADS);

    float* texturePositionData = tilePositions + ind * 4;
    float mercX = texturePositionData[0], mercY = texturePositionData[1],
        tileWidth = texturePositionData[2], tileHeight = texturePositionData[3];

    paneRef->mapToScreen(mercX, mercY);
    float tileLeft = paneRef->xScreen, tileTop = paneRef->yScreen;

    paneRef->mapToScreen(mercX + tileWidth, mercY - tileHeight);
    float tileRight = paneRef->xScreen, tileBottom = paneRef->yScreen;

    float imgScreenW = tileRight - tileLeft, imgScreenH = tileTop - tileBottom;

    drawBoundTextureGL(tileLeft, tileTop, imgScreenW, imgScreenH);
}

void TileTextureContainer::drawTextures() {
    for (int i = 0; i < numTextures; i++) {
        drawTextureAtInd(i);
    }
}




/*
Functions for rendering street names
*/

void StreetNameContainer::readNames(const char* namesPath) {

    std::ifstream inputFile(namesPath);

    for (std::string line; getline(inputFile, line);) {
        names.push_back(line);
    }

    nameTextures = (GLuint*)calloc(names.size(), sizeof(GLuint));
    namesLoaded = (int*)malloc(names.size() * sizeof(int));
    nameImgDimensions = (unsigned int*)malloc(2 * names.size() * sizeof(unsigned int));

    int nameNum = 0;
    for (string name : names) {
        namesLoaded[nameNum] = 0;
        nameNum++;
    }
}


void StreetNameContainer::makeNameTexture(int id) {
    string name = names[id];
    unsigned char* nameImg;

    glEnable(GL_TEXTURE_2D);
    GLuint nameID;
    glGenTextures(1, &nameID);
    glBindTexture(GL_TEXTURE_2D, nameID);

    int nameImgW, nameImgH;
    renderer.getStringImage((char*)name.c_str(), name.length(), FONT_HEIGHT, 0, &nameImg, &nameImgW, &nameImgH, 0);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        nameImgW,
        nameImgH,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        (const void*)nameImg
    );
    nameTextures[id] = nameID;
    nameImgDimensions[2 * id] = nameImgW;
    nameImgDimensions[2 * id + 1] = nameImgH;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    free(nameImg);

    namesLoaded[id] = 1;
    numNameTexturesMade++;
}



void StreetNameContainer::drawName(int id) {
    glBindTexture(GL_TEXTURE_2D, nameTextures[id]);
    glBegin(GL_QUADS);

    float charW = nameTextureScale * nameImgDimensions[2 * id] / winW, charH = nameTextureScale * nameImgDimensions[2 * id + 1] / winH;

    drawBoundTextureGL(-1, 1, charW, charH);
}

void StreetNameContainer::freeMem() {
    free(nameTextures);
    free(namesLoaded);
    free(nameImgDimensions);
    names.~vector();
}
