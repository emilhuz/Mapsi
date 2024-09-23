#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>


using namespace std;

typedef struct { float x; float y; } Point;

class PathHandler {
private:
	std::vector<std::vector<Point>> paths;
	
public:
	int totalPoints;
	int numPaths;
	int numStreets;

	Point* points = nullptr;

	vector<int> pathSizes;
	vector<int> pathsInStreet;
	int* numPathsBeforeStreet = nullptr;

	int totalIndices;
	unsigned int* line_indices = nullptr;
	int* pathStartIndices = nullptr;
	int* street_index_sizes = nullptr;

	int streetIndexForPath(int pathIndex);

	void readPathsBinArrays(const char* fileName);

	void readPathsString(const char* fileName);

	void savePathsBin(const char* fileName);

	void init_line_indices();

	void freeMem();
};