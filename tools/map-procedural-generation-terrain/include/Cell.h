#ifndef _CELL_H_
#define _CELL_H_

#include "Point2.h"
#include <vector>

struct cellBoundingBox {
	double xmin;
	double ymin;
	double width;
	double height;

	cellBoundingBox(double _xmin, double _ymin, double _xmax, double _ymax) :
		xmin(_xmin), ymin(_ymin), width(_xmax - _xmin), height(_ymax - _ymin) {};
};

struct Cell;
struct Site {
	Point2 p;
	Cell* cell;

	Site() {};
	Site(Point2 _p, Cell* _cell) : p(_p), cell(_cell) {};
};

struct HalfEdge;
struct Cell {
	Site site;
	std::vector<HalfEdge*> halfEdges;
	bool closeMe;

	Cell() : closeMe(false) {};
	Cell(Point2 _site) : site(_site, this), closeMe(false) {};

	std::vector<Cell*> getNeighbors();
	cellBoundingBox getBoundingBox();

	// Return whether a point is inside, on, or outside the cell:
	//   -1: point is outside the perimeter of the cell
	//    0: point is on the perimeter of the cell
	//    1: point is inside the perimeter of the cell
	int pointIntersection(double x, double y);

	static bool edgesCCW(HalfEdge* a, HalfEdge* b);
};

#endif