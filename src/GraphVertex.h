/** \file GraphVertex.h
 * \brief Definition of GraphVertex class
 */
#ifndef GRAPHVERTEX_H
#define GRAPHVERTEX_H


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
	double pos[2];
public:
	GraphVertex(double x, double y){
		pos[0] = x, pos[1] = y;
	}
	void getPos(double pos[2])const{pos[0] = this->pos[0]; pos[1] = this->pos[1];}
	const EdgeMap &getEdges()const{return edges;}
	double measureDistance(const GraphVertex &other)const{
		double endPos[2];
		other.getPos(endPos);
		return measureDistance(endPos);
	}
	double measureDistance(const double endPos[2])const{
		double startPos[2];
		this->getPos(startPos);
		return sqrt((startPos[0] - endPos[0]) * (startPos[0] - endPos[0]) + (startPos[1] - endPos[1]) * (startPos[1] - endPos[1]));
	}
	bool connect(Graph &graph, GraphVertex *other);
	void add(Vehicle *v);
};


double calcPerp(double para[2], double perp[2], const double pos[2], const double dpos[2]);

#endif
