
#include <string>
#include <vector>
#include <iostream>

using namespace std;

vector<std::string> splitString(std::string s, std::string delim) {
    std::vector<int> indices;
    std::vector<std::string> result;
    int delLen = delim.length();
    int ind = 0;
    while (true) {
        int ind2 = s.find(delim, ind);
        if (ind2 == std::string::npos)
            break;
        indices.push_back(ind2);
        ind = ind2 + delLen;
    }
    if (indices.size() == 0) result.push_back(s);
    else {
        result.push_back(s.substr(0, indices[0]));
        for (size_t i = 1; i < indices.size(); i++) {
            int start = indices[i - 1] + delLen;
            result.push_back(s.substr(start, indices[i] - start));
        }
        int start = indices[indices.size() - 1] + delLen;
        result.push_back(s.substr(start, s.length() - start));
    }
    return result;
}





float sqDistPoints(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2, dy = y1 - y2;
    return dx * dx + dy * dy;
}

void coefLineThrough(float x1, float y1, float x2, float y2, float* abc_coefs) {
    
    if (x1 == x2) {
        abc_coefs[0] = 1;
        abc_coefs[1] = 0;
        abc_coefs[2] = -x1;
    }
    else {
        float k = (y2 - y1) / (x2 - x1);
        float l = y1 - k * x1;
        abc_coefs[0] = k;
        abc_coefs[1] = -1;
        abc_coefs[2] = l;
    }
}

void projToLine(float x, float y, float a, float b, float c, float* projPoint) {
    float par = b * x - a * y;
    projPoint[0] = b * par - a * c;
    projPoint[1] = -a * par - b * c;
    float sq = (a * a + b * b);
    projPoint[0] /= sq;
    projPoint[1] /= sq;
}

float sqDistPointToSegment(float x1, float y1, float x2, float y2, float x, float y) {
    float coef[3];
    coefLineThrough(x1, y1, x2, y2, coef);
    float projPoint[2];
    projToLine(x, y, coef[0], coef[1], coef[2], projPoint);

    float prx = projPoint[0], pry = projPoint[1];
    float dSeg = sqDistPoints(x1, y1, x2, y2), d1 = sqDistPoints(x1, y1, prx, pry), d2 = sqDistPoints(x2, y2, prx, pry);
    if (d1 <= dSeg && d2 <= dSeg) {
        return sqDistPoints(x, y, prx, pry);
    }
    else if (d2 < d1) {
        return sqDistPoints(x, y, x2, y2);
    }
    else if (d1 < d2) {
        return sqDistPoints(x, y, x1, y1);
    }
    return 1;
}

float sqDistPointToPath(float* coords, int numpoints, float x, float y) {
    if (numpoints < 2) return 0; // TODO return max
    float minDist = sqDistPointToSegment(*coords, *(coords + 1), *(coords + 2), *(coords + 3), x, y);
    for (int i = 0; i < numpoints-2; i++) {
        coords += 2;
        float dist = sqDistPointToSegment(*coords, *(coords + 1), *(coords + 2), *(coords + 3), x, y);
        if (dist < minDist) minDist = dist;
    }
    return minDist;
}

int indexOfClosestPath(float* points, std::vector<int> path_sizes, int numPaths, float x, float y, float* minSqDist) {


    float* path = points;
    float currentMinDist = sqDistPointToPath(points, path_sizes[0], x, y);
    int bestIndex = 0;
    for (int i = 1; i < numPaths; i++) {
        points += 2 * path_sizes[i - 1];
        float dist = sqDistPointToPath(points, path_sizes[i], x, y);
        if (dist < currentMinDist) {
            currentMinDist = dist;
            bestIndex = i;
        }
    }
    *minSqDist = currentMinDist;
    return bestIndex;
}