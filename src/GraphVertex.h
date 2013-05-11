/** \file GraphVertex.h
 * \brief Definition of GraphVertex class
 */
#ifndef GRAPHVERTEX_H
#define GRAPHVERTEX_H

#include <cpplib/vec2.h>
#include <math.h>
#include <map>



extern const double vertexRadius;

class GraphEdge;
class Vehicle;
class Graph;

class GraphVertex{
public:
	typedef std::map<GraphVertex*, GraphEdge*> EdgeMap;
protected:
	EdgeMap edges;
	Vec2d pos;
public:
	GraphVertex(double x, double y){
		pos[0] = x, pos[1] = y;
	}
	const Vec2d &getPos()const{return pos;}
	const EdgeMap &getEdges()const{return edges;}
	double measureDistance(const GraphVertex &other)const{
		Vec2d endPos = other.getPos();
		return measureDistance(endPos);
	}
	double measureDistance(const Vec2d &endPos)const{
		Vec2d startPos = this->getPos();
		return (startPos - endPos).len();
	}
	bool connect(Graph &graph, GraphVertex *other);
	void add(Vehicle *v);
};


double calcPerp(double para[2], double perp[2], const double pos[2], const double dpos[2]);

#endif
