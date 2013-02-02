/** \file GraphVertex.h
 * \brief Definition of GraphVertex class
 */
#ifndef GRAPHVERTEX_H
#define GRAPHVERTEX_H

#include <GL/glut.h>
#include <GL/gl.h>
#define exit something_meanless


#include <math.h>


#include <vector>
#include <map>
#include <set>



extern const double vertexRadius;

class GraphEdge;
class Vehicle;
class Graph;

class GraphVertex{
public:
	typedef std::map<GraphVertex*, GraphEdge*> EdgeMap;
protected:
	std::map<GraphVertex*, GraphEdge*> edges;
	double pos[2];
public:
	GraphVertex(double x, double y){
		pos[0] = x, pos[1] = y;
	}
	void getPos(double pos[2])const{pos[0] = this->pos[0]; pos[1] = this->pos[1];}
	const EdgeMap &getEdges()const{return edges;}
	double measureDistance(const GraphVertex &other)const{
		double startPos[2], endPos[2];
		this->getPos(startPos);
		other.getPos(endPos);
		return sqrt((startPos[0] - endPos[0]) * (startPos[0] - endPos[0]) + (startPos[1] - endPos[1]) * (startPos[1] - endPos[1]));
	}
	bool connect(GraphVertex *other);
	void add(Vehicle *v);
};

class GraphEdge{
	typedef std::set<Vehicle*> VehicleSet;
	GraphVertex *start;
	GraphVertex *end;
	VehicleSet vehicles;
	double length;
	mutable int passCount;
	static int maxPassCount;
public:
	GraphEdge(GraphVertex *start, GraphVertex *end) : start(start), end(end), passCount(0){
		length = start->measureDistance(*end);
	}
	GraphVertex *getStart()const{return start;}
	GraphVertex *getEnd()const{return end;}
	double getLength()const{return length;}
	void add(Vehicle *v);
	void remove(Vehicle *v){
		vehicles.erase(v);
	}
	int getPassCount()const{return passCount;}
	static int getMaxPassCount(){return maxPassCount;}
};

double calcPerp(double para[2], double perp[2], const double pos[2], const double dpos[2]);

#endif
