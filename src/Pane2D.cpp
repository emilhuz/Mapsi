#include <vector>
#include <math.h>
#include <iostream>
#include "Pane2D.h"

using namespace std;

void Pane2D::mapToScreen_arrays(float* points, float* results, int numPoints) {
	float* p0 = points;
	for (int i_map = 0; i_map < numPoints; i_map++) {
		*(results++) = 2 * (*(points++) - xLeft) / w - 1;
		*(results++) = 2 * (*(points++) - yBottom) / h - 1;
	}

}

void Pane2D::mapToScreen(float x, float y) {
	xScreen = 2 * (x - xLeft) / w - 1;
	yScreen = 2 * (y - yBottom) / h - 1;
}

void Pane2D::mapFromScreen(float x, float y) {
	xPoint = xLeft + (x + 1) * w / 2;
	yPoint = yBottom + (y + 1) * h / 2;
}

void Pane2D::setDim(float screen_w, float screen_h) {
	w = screen_w / scaleFactor;
	h = screen_h / scaleFactor;
}

void Pane2D::zoom(float zoom_amount, float zoom_x, float zoom_y, bool reposition_points) {
	float ratioX = (zoom_x - xLeft) / w, ratioY = (zoom_y - yBottom) / h;
	double zoomFactor = pow((double)zoomMultiplier, (double)zoom_amount);
	double newScaling = scaleFactor * zoomFactor;
	double neww = w / zoomFactor, newh = h / zoomFactor;
	double sumMargX = w - neww, sumMargY = h - newh;
	xLeft += (float)sumMargX * ratioX;
	yBottom += (float)sumMargY * ratioY;
	scaleFactor = (float)newScaling;
	w = (float)neww; h = (float)newh;
}

