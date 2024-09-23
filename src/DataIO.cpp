#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include "LineUtils.h"
#include "DataIO.h"

using namespace std;


std::vector<Point> pathPoints(std::string pathStr);


int PathHandler::streetIndexForPath(int pathIndex) {
    if (pathIndex >= numPaths) {
        cout << "path index too high to belong to a street\n";
        return -1;
    }
    for (int numStr = 0; numStr < numStreets - 1; numStr++) {
        if (pathIndex >= numPathsBeforeStreet[numStr] && pathIndex < numPathsBeforeStreet[numStr + 1]) {
            return numStr;
        }
    }
    return numStreets - 1;
}

/*
Reads from file in binary format. Lengths need to be int32 and points float32.
First int in file is number of streets, followed by length data for each street: number of paths and length of each path
After that it reads all the coordinates x1 y1 x2 y2... as floats.
*/
void PathHandler::readPathsBinArrays(const char* fileName) {

    struct stat results;
    int file_size;
    if (stat(fileName, &results) == 0) {
        file_size = results.st_size;
    }
    else return; // error reading file size

    if (file_size < sizeof(int)) return;

    cout << "File size: " << file_size << endl;

    std::ifstream inputFile(fileName, std::ios::in | std::ios::binary);
    char* buffer = (char*)malloc(file_size);
    char* bufferStart = buffer;
    inputFile.read(buffer, file_size);
    inputFile.close();


    int totalpts = 0;
    int numpaths = 0;

    int numstreets = *(int*)buffer;
    numStreets = numstreets;
    printf("streets: %d\n", numstreets);
    buffer += sizeof(int);

    for (int i = 0; i < numstreets; i++) {
        int numPathsInCurrentStreet = *(int*)buffer;
        buffer += sizeof(int);
        numpaths += numPathsInCurrentStreet;
        pathsInStreet.push_back(numPathsInCurrentStreet);
        for (int currStreetPathNum = 0; currStreetPathNum < numPathsInCurrentStreet; currStreetPathNum++) {
            int pathSize = *(int*)buffer;
            pathSizes.push_back(pathSize);
            totalpts += pathSize;
            buffer += sizeof(int);
        }
    }

    totalPoints = totalpts;
    numPaths = numpaths;

    points = (Point*)malloc(totalpts * (int)sizeof(Point));
    memcpy((void*)points, (void*)buffer, totalpts * sizeof(Point));

    free(bufferStart);

    std::cout << "done reading file! lines: " << std::endl << numpaths << ", total points: " << totalpts << std::endl;
}

void PathHandler::readPathsString(const char* fileName) {
    std::ifstream myfile(fileName);

    int numline = 0;
    int nump = 0;
    int numstreets = 0;
    int maxlines = 20000000;//1000000000;
    for (std::string line; std::getline(myfile, line);) {
        std::vector<std::string> linePaths = splitString(line, "/");
        for (string pathStr : linePaths) {
            vector<Point> singlePathPoints = pathPoints(pathStr);
            nump += singlePathPoints.size();
            paths.push_back(singlePathPoints);
        }
        pathsInStreet.push_back(linePaths.size());
        numstreets++;

        numline++;
        if (numline % 1000 == 0) std::cout << numline << std::endl;
        if (numline >= maxlines) break;
    }
    numStreets = numstreets;
    std::cout << "done reading file! size: " << std::endl << paths.size() << ", total points: " << nump << std::endl;
}

void PathHandler::savePathsBin(const char* fileName) {
    std::ofstream outputFile(fileName, std::ios::out | std::ios::binary);
    int nump = paths.size();

    int totalpts = 0;
    for (int i = 0; i < nump; i++) {
        totalpts += paths.at(i).size();
    }
    int numbytes = (1 + numStreets) * sizeof(int) + nump * sizeof(int) + totalpts * 2 * sizeof(float);
    char* buffer = (char*)malloc(numbytes);

    int pathInd = 0;
    int* currPos = (int*)buffer;
    *currPos = numStreets;
    currPos++;

    for (int streetInd = 0; streetInd < numStreets; streetInd++) {
        int* numPathsStreet = (int*)currPos;
        int numPathsInStreet = pathsInStreet.at(streetInd);
        *currPos = numPathsInStreet;
        currPos++;

        for (int streetPathInd = 0; streetPathInd < numPathsInStreet; streetPathInd++) {
            int currSize = paths.at(pathInd + streetPathInd).size();
            *currPos = currSize;
            currPos++;
        }

        pathInd += numPathsInStreet;

    }

    float* coord = (float*)currPos;

    for (int pathInd = 0; pathInd < nump; pathInd++) {
        std::vector<Point> path = paths.at(pathInd);

        for (Point p : path) {
            *coord = p.x;
            coord++;
            *coord = p.y;
            coord++;
        }
    }

    outputFile.write(buffer, numbytes);

    outputFile.close();
    free(buffer);
};


void PathHandler::init_line_indices() {


    for (int i = 0; i < numPaths; i++) {
        totalIndices += 2 * (pathSizes[i] - 1);
    }
    line_indices = (unsigned int*)malloc(totalIndices * sizeof(unsigned int));
    pathStartIndices = (int*)malloc(numStreets * sizeof(int));
    street_index_sizes = (int*)malloc(numStreets * sizeof(int));
    numPathsBeforeStreet = (int*)malloc(numStreets * sizeof(int));
    int totalIndex = 0;

    

    int point_ind = 0;
    int ind_ind = 0;
    int pathInd = 0;
    int cumulPaths = 0;
    for (int numStr = 0; numStr < numStreets; numStr++) {
        int thisStrIndicesNum = 0;
        numPathsBeforeStreet[numStr] = cumulPaths;
        cumulPaths += pathsInStreet[numStr];
        for (int i = 0; i < pathsInStreet[numStr]; i++) {

            int psize = pathSizes[pathInd];
            pathInd++;
            for (int pind = 0; pind < psize - 1; pind++) {
                line_indices[ind_ind] = point_ind;
                line_indices[ind_ind + 1] = point_ind + 1;
                ind_ind += 2;
                point_ind++;
            }
            point_ind++;

            thisStrIndicesNum += 2 * (psize - 1);;
        }
        street_index_sizes[numStr] = thisStrIndicesNum;
        pathStartIndices[numStr] = totalIndex;
        totalIndex += thisStrIndicesNum;
    }

}

void PathHandler::freeMem() {
    paths.~vector();
    pathSizes.~vector();
    pathsInStreet.~vector();
    if (points != nullptr) {
        free(points);
    }
    if (numPathsBeforeStreet != nullptr) {
        free(numPathsBeforeStreet);
    }
    if (line_indices != nullptr) {
        free(line_indices);
    }
    if (pathStartIndices != nullptr) {
        free(pathStartIndices);
    }
    if (street_index_sizes != nullptr) {
        free(street_index_sizes);
    }
}

std::vector<Point> pathPoints(std::string pathStr) {
    std::vector<Point> path;
    std::vector<std::string> pointsstr = splitString(pathStr, ",");
    for (std::string point : pointsstr) {
        std::istringstream iss(point);
        Point p;
        iss >> p.x;
        iss >> p.y;
        path.push_back(p);
    }
    pointsstr.clear();
    pointsstr.~vector();
    return path;
};

