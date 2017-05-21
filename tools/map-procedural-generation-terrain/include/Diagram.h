#ifndef _DIAGRAM_H_
#define _DIAGRAM_H_

#include "../src/MemoryPool/C-11/MemoryPool.h"
//#include "../src/MemoryPool/C-98/MemoryPool.h" //You will need to use this version instead of the one above if your compiler doesn't handle C++11's noexcept operator
#include "Edge.h"
#include "Cell.h"
#include <set>

struct BoundingBox;
class Diagram {
public:
	std::vector<Cell*> cells;
	std::vector<Edge*> edges;
	std::vector<Point2*> vertices;

	~Diagram();
	void printDiagram();
private:
	friend class VoronoiDiagramGenerator;

	std::set<Cell*> tmpCells;
	std::set<Edge*> tmpEdges;
	std::set<Point2*> tmpVertices;

	MemoryPool<Cell> cellPool;
	MemoryPool<Edge> edgePool;
	MemoryPool<HalfEdge> halfEdgePool;
	MemoryPool<Point2> vertexPool;

	Point2* createVertex(double x, double y);
	Cell* createCell(Point2 site);
	Edge* createEdge(Site* lSite, Site* rSite, Point2* vertA, Point2* vertB);
	Edge* createBorderEdge(Site* lSite, Point2* vertA, Point2* vertB);

	bool connectEdge(Edge* edge, BoundingBox bbox);
	bool clipEdge(Edge* edge, BoundingBox bbox);
	void clipEdges(BoundingBox bbox);
	void closeCells(BoundingBox bbox);
	void finalize();

	template <typename T>
	void freeVecMemory(T& t) {
		T tmp;
		t.swap(tmp);
	}
};

#endif