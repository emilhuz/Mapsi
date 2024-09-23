// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/stat.h>
#include <math.h>

#include "Pane2D.h"
#include "LineUtils.h"
#include "RenderText.h"
#include "DataIO.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H

unsigned char* make_img(char const* filename, int* x, int* y, int* comp, int req_comp) {
    return stbi_load(filename, x, y, comp, req_comp);
}

// Consts
const GLuint WIDTH = 500, HEIGHT = 300;
const GLuint WINDOWX = 200, WINDOWY = 50;
const int GUI_FONT_HEIGHT = 20;
const float distToHighlightPx = 10;
const char* fontPath = "Data/fonts/arial.ttf";
const char* streetDataPath = "Data/streets.bin";
const char* namesPath = "Data/names.txt";
const char* viewConfigPath = "Data/viewConfig.txt";

using namespace std;

Pane2D pane;

GLFWwindow* mainWindow = nullptr;
int winW, winH;
int oldW = 1, oldH = 1;
bool needs_redraw = true;

Renderer renderer;


Point* screenPoints;


PathHandler paths;
TileTextureContainer tiles;
StreetNameContainer nameTextures;

void initPosition() {
    std::ifstream myfile(viewConfigPath);
    myfile >> pane.xLeft;
    myfile >> pane.yBottom;
    myfile >> pane.scaleFactor;
}

void makeTexture(char* text, GLuint id, int color, int *w, int *h) {
    unsigned char* img;
    renderer.getStringImage(text, strlen(text), GUI_FONT_HEIGHT, 0, &img, w, h, color);

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, *w, *h, 0, GL_RGB, GL_UNSIGNED_BYTE, (const void*)img);
    free(img);
}

class Button {
public:
    int wPix, hPix;
    float wGL, hGL;
    float xScreen, yScreen;
    float xGL, yGL;
    bool showing;
    void (*listener) ();

    GLuint textureID;

    void initButton(char* buttonText, int color, void (*clickListener) (), bool isShowing) {
        listener = clickListener;
        showing = isShowing;

        glGenTextures(1, &textureID);
        makeTexture(buttonText, textureID, color, &wPix, &hPix);
    }

    void setPosition(float xScr, float yScr, float windowW, float windowH) {
        xScreen = xScr; yScreen = yScr;
        
        xGL = -1 + 2 * xScr / windowW;
        yGL = 1 - 2 * yScr / windowH;
        wGL = wPix / windowW*2;
        hGL = hPix / windowH*2;
    }

    void draw() {
        if (showing) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            drawBoundTextureGL(xGL, yGL, wGL, hGL);
        }
    }

    void freeMem() {
        glDeleteTextures(1, &textureID);
    }
};

vector<Button> buttons;
const int NUM_BUTTONS = 3;

void freeButtons() {
    for (int buttInd = 0; buttInd < NUM_BUTTONS; buttInd++) {
        buttons[buttInd].freeMem();
    }
    buttons.~vector();
}


int whichHighlight = -1;

using namespace std::chrono;
steady_clock::time_point tStart, tEnd;

class QuizManager {
public:
    bool quizActive = false;
    std::vector<int> quizPathIndices;
    int numPathsGuessed;
    int currentQuizPathIndex;
    int numGuessAttempts = 0;

    GLuint infoTexture;
    int infoW, infoH;
    float infoXGL, infoYGL, infoWGL, infoHGL;
    bool infoShowing = false;

    void startQuiz() {
        infoShowing = false;
        buttons[2].showing = false;
        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(quizPathIndices), std::end(quizPathIndices), rng);
        quizActive = true;
        numPathsGuessed = 0; numGuessAttempts = 0;
        currentQuizPathIndex = quizPathIndices[0];
        if (nameTextures.namesLoaded[currentQuizPathIndex] == 0) {
            nameTextures.makeNameTexture(currentQuizPathIndex);
            nameTextures.namesLoaded[currentQuizPathIndex] = 1;
        }
        tStart = high_resolution_clock::now();
        buttons[0].showing = false; buttons[1].showing = true; needs_redraw = true;
    }

    void showInfo(string info) {
        makeTexture((char*)info.c_str(), infoTexture, 0x0, &infoW, &infoH);
        infoShowing = true;
        buttons[2].showing = true;
        adjustInfo();
    }

    void adjustInfo() {
        infoWGL = 1.0f*infoW / winW * 2;
        infoXGL = -infoWGL / 2;
        infoHGL = 1.0f*infoH / winH * 2;
        infoYGL = infoHGL/2;
        buttons[2].setPosition(winW / 2 - buttons[2].wPix / 2, winH / 2 + infoH*(0.5f+0.2f), winW, winH);
    }

    void hideInfo() {
        infoShowing = false;
        buttons[2].showing = false;
        needs_redraw = true;
    }

    void endQuiz() {
        quizActive = false;
        tEnd = high_resolution_clock::now();
        float amtSec = duration_cast<microseconds>(tEnd - tStart).count() / 1000000.0f;
        buttons[0].showing = true; buttons[1].showing = false; needs_redraw = true;
        string info = to_string(numPathsGuessed) + "/" + to_string(paths.numStreets) + " correct guesses (" + to_string(numGuessAttempts) + " attempts, " + to_string(amtSec/numPathsGuessed) + " sec per guess";
        showInfo(info);
    }

    void freeMem() {
        glDeleteTextures(1, &infoTexture);
        quizPathIndices.~vector();

    }
};

QuizManager quiz;



void initGUI() {    
    buttons.push_back(Button());     buttons.push_back(Button());      buttons.push_back(Button());

    buttons[0].initButton((char*)"Start quiz", 0x00AA00, []() {quiz.startQuiz();}, true);
    buttons[1].initButton((char*)"Stop quiz", 0xAA0000, []() {quiz.endQuiz();}, false);
    buttons[2].initButton((char*)"Ok", 0x0000CC, []() {quiz.hideInfo();}, false);
}


double mousePosX, mousePosY;



void mouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (action != GLFW_RELEASE) return;

    int prevHighlight = whichHighlight;
    whichHighlight = -1;

    for (int buttInd = 0; buttInd < NUM_BUTTONS; buttInd++) {
        Button button = buttons[buttInd];
        if (!button.showing) {
            continue;
        }
        if (mousePosX >= button.xScreen && mousePosX <= button.xScreen+button.wPix
                && mousePosY >= button.yScreen && mousePosY <= button.yScreen + button.hPix) {
            button.listener();
            return;
        }
    }

    pane.mapFromScreen((float)(2 * mousePosX / winW - 1), (float)(2 * (winH - mousePosY) / winH - 1));

    float minDistSq;
    int whichPathClosest = indexOfClosestPath((float*)paths.points, paths.pathSizes, paths.numPaths, pane.xPoint, pane.yPoint, &minDistSq);

    if (sqrt(minDistSq) * winW / pane.w <= distToHighlightPx) {
        int potentialHighlight = paths.streetIndexForPath(whichPathClosest);
        if (potentialHighlight >= 0) {
            whichHighlight = potentialHighlight;
            needs_redraw = true;
            if (!quiz.quizActive && nameTextures.namesLoaded[whichHighlight] == 0) {
                nameTextures.makeNameTexture(whichHighlight);
            }
            if (quiz.quizActive) {
                quiz.numGuessAttempts++;
                if (whichHighlight == quiz.currentQuizPathIndex)
                    quiz.numPathsGuessed++;

                if (quiz.numPathsGuessed == paths.numStreets) {
                    quiz.endQuiz();
                    buttons[0].showing = true;
                    buttons[1].showing = false;
                }
                else {
                    quiz.currentQuizPathIndex = quiz.quizPathIndices[quiz.numPathsGuessed];
                    if (nameTextures.namesLoaded[quiz.currentQuizPathIndex] == 0) {
                        nameTextures.makeNameTexture(quiz.currentQuizPathIndex);
                        nameTextures.namesLoaded[quiz.currentQuizPathIndex] = 1;
                    }
                }
            }
        }
    }
    else if (prevHighlight >= 0) {
        needs_redraw = true;
    }
}


void mouseMove(GLFWwindow* window, double x, double y)
{
    mousePosX = x; mousePosY = y;
}

void mouseScroll(GLFWwindow* window, double x, double y)
{
    needs_redraw = true;
    double xnorm = 2 * mousePosX / winW - 1, ynorm = 2 * (winH - mousePosY) / winH - 1;
    pane.mapFromScreen((float)xnorm, (float)ynorm);
    pane.zoom((float)y, pane.xPoint, pane.yPoint, false);
}


void adjustButtons() {
    buttons[0].setPosition(winW - buttons[0].wPix, 0, winW, winH);
    buttons[1].setPosition(winW - buttons[1].wPix, 0, winW, winH);
    if (quiz.infoShowing) {
        quiz.adjustInfo();
    }
}

bool draw_axes = true;

void display(GLuint vertexBuffer) {
    if (mainWindow != nullptr) {
        int w = 1, h = 1;
        glfwGetWindowSize(mainWindow, &w, &h);
        if (w != oldW || h != oldH) {
            oldW = w; oldH = h;
            pane.w = w / pane.scaleFactor;
            pane.h = h / pane.scaleFactor;
            winW = w; winH = h;
            nameTextures.winW = 1.0f*winW; nameTextures.winH = 1.0f*winH;
            glViewport(0, 0, w, h);
            needs_redraw = true;
            adjustButtons();
        }
        else {
            if (!needs_redraw)
                return;
        }
    }

    glClearColor(0.2f, 0.3f, 0.3f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1, 1, 1);

    tiles.drawTextures();

    glColor3f(1, 1, 1);
    pane.mapToScreen_arrays((float*)paths.points, (float*)screenPoints, paths.totalPoints);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, paths.totalPoints * sizeof(Point), screenPoints, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(
        0,
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    glLineWidth(3);
    glDrawElements(
        GL_LINES,           // mode
        paths.totalIndices, // count
        GL_UNSIGNED_INT,    // type
        (void*)0            // element array buffer offset
    );
    glLineWidth(1);
    glColor3f(1, 0, 0);
    glDrawElements(
        GL_LINES,           // mode
        paths.totalIndices, // count
        GL_UNSIGNED_INT,    // type
        (void*)0            // element array buffer offset
    );

    if (whichHighlight >= 0) {
        // give the highlighted street width 5 and draw it red
        glColor3f(1, 0, 0);
        glLineWidth(5);
        glDrawElements(
            GL_LINES,
            paths.street_index_sizes[whichHighlight],
            GL_UNSIGNED_INT,
            (void*)(paths.pathStartIndices[whichHighlight] * sizeof(GLuint))
        );
        glLineWidth(1);
        glColor3f(1, 1, 1);
        if (!quiz.quizActive) {
            nameTextures.drawName(whichHighlight);
        }
    }
    if (quiz.quizActive) {
        glColor3f(1, 1, 1);
        nameTextures.drawName(quiz.currentQuizPathIndex);
    }

    if (draw_axes) {
        glBegin(GL_LINES);
        glLineWidth(1);
        glColor3f(0, 0, 0);
        pane.mapToScreen(0, 0);
        glVertex2f(-1, pane.yScreen); glVertex2f(1, pane.yScreen); // x axis
        glVertex2f(pane.xScreen, -1); glVertex2f(pane.xScreen, 1); // y axis
        glEnd();
    }

    glColor3f(1, 1, 1);
    for (int buttInd = 0; buttInd < NUM_BUTTONS; buttInd++) {
        Button button = buttons[buttInd];
        if (!button.showing) {
            continue;
        }
        button.draw();
    }

    if (quiz.infoShowing) {
        glBindTexture(GL_TEXTURE_2D, quiz.infoTexture);
        drawBoundTextureGL(quiz.infoXGL, quiz.infoYGL, quiz.infoWGL, quiz.infoHGL);
    }

    glFlush();
}


void bufferLineIndices() {
    GLuint index_buff_ob_id;
    glGenBuffers(1, &index_buff_ob_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buff_ob_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, paths.totalIndices * sizeof(unsigned int), paths.line_indices, GL_STATIC_DRAW);
}

int main()
{
    initPosition();
    std::cout << pane.scaleFactor << " " << pane.xLeft << " " << pane.yBottom << std::endl;
    tiles.paneRef = &pane;

    paths.readPathsBinArrays(streetDataPath);

    screenPoints = (Point*)malloc(paths.totalPoints * (int)sizeof(Point));


    if (!glfwInit()) {
        std::cout << "GLFW Init failed" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Mapsi", NULL, NULL);
    glfwSetWindowPos(window, WINDOWX, WINDOWY);
    mainWindow = window;
    glfwMakeContextCurrent(window);

    if (!glewInit()) {
        cout << "GLEW Init failed" << endl;
    }

    renderer.initTheFT(fontPath);
    nameTextures.readNames(namesPath);
    nameTextures.renderer = renderer;

    tiles.loadTextures();
    initGUI();


    paths.init_line_indices();
    bufferLineIndices();
    for (int i = 0; i < paths.numStreets; i++) {
        quiz.quizPathIndices.push_back(i);
    }
    cout << paths.totalIndices << " indices" << endl;

    glfwSetScrollCallback(window, mouseScroll); // for zooming in and out
    glfwSetMouseButtonCallback(window, mouseButton); // for clicking
    glfwSetCursorPosCallback(window, mouseMove); // for tracking position

    GLuint vertexBuffer;
    // Generate buffer for line segments
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glGenTextures(1, &quiz.infoTexture);

    display(vertexBuffer);
    glfwSwapBuffers(window);

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        display(vertexBuffer);

        if (needs_redraw) { // needs_redraw is true if mouse has been scrolled, window resized or streets or buttons clicked
            /* Swap front and back buffers */
            glfwSwapBuffers(window);
        }
        needs_redraw = false;

        /* Poll for and process events */
        glfwPollEvents();
    }

    if (quiz.quizActive) {
        quiz.endQuiz();
    }

    for (int i = 0; i < paths.numPaths; i++) {
        if (nameTextures.namesLoaded[i] != 0) {
            glDeleteTextures(1, nameTextures.nameTextures + i);
        }
    }
    glDeleteTextures(tiles.numTextures, tiles.textureIDs);
    glDeleteBuffers(1, &vertexBuffer);
    glfwTerminate();
    
    std::cout << "Exiting..." << std::endl;

    free(screenPoints);
    paths.freeMem();
    tiles.freeMem();
    nameTextures.freeMem();
    renderer.freeMem();
    quiz.freeMem();
    freeButtons();

    return 0;
}
