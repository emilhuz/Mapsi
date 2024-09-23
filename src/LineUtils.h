
#include <string>
#include <vector>
#include <string>
#include <iostream>


using namespace std;

void printMat(float* m, int rows, int cols);

void printMat4(glm::mat4 m, int rows, int cols);

string formatMemorySize(int size);

vector<string> splitString(std::string s, std::string delim);

void printIntArr(int* ptr, int len);




float sqDistPoints(float x1, float y1, float x2, float y2);

void coefLineThrough(float x1, float y1, float x2, float y2, float* abc_coefs);

void projToLine(float x, float y, float a, float b, float c, float* projPoint);

float distPointToSegment(float x1, float y1, float x2, float y2, float x, float y);

float sqDistPointToPath(float* coords, int numpoints, float x, float y);

int indexOfClosestPath(float* points, std::vector<int> path_sizes, int numPaths, float x, float y, float* minSqDist);