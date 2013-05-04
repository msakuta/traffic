/** \file Vehicle.h
 * \brief Definition of Vehicle class
 */
#ifndef VEHICLE_H
#define VEHICLE_H

#include <GL/glut.h>
#include <GL/gl.h>
#define exit something_meanless


#include <vector>
#include <map>
#include <set>


class GraphVertex;
class GraphEdge;
class Vehicle;
class Graph;

class Vehicle{
public:
	typedef std::set<GraphVertex*> VertexSet;
	typedef std::map<GraphVertex*, GraphVertex*> VertexMap;
	typedef std::vector<GraphVertex*> Path;
	static const int stepStatCount = 20;
	static int movingSteps; ///< Step accumulator for moving frames for all Vehicles.
	static int jammedSteps; ///< Step accumulator for jammed frames for all Vehicles.
protected:
	const GraphVertex *dest;
	GraphEdge *edge;
	Path path;
	double pos; ///< [0,1)
	double velocity;
	GLfloat color[3];
	bool jammed; ///< Whether last frame is jammed
	static int stepStats[stepStatCount];
	bool findPathInt(Graph *, GraphVertex *root, VertexMap &prevMap, VertexSet &visited);
	bool checkTraffic(GraphEdge *, double pos);
public:
	Vehicle(GraphVertex *dest) : dest(dest), edge(NULL), pos(0), velocity(0.1), jammed(false){
		for(int i = 0; i < 3; i++)
			color[i] = (GLfloat)rand() / RAND_MAX;
	}
	bool findPath(Graph *, GraphVertex *start);
	Path &getPath(){return path;}
	double getPos()const{return pos;}
	const GraphEdge *getEdge()const{return edge;}
	void setEdge(GraphEdge *edge){ this->edge = edge; }
	static const int *getStepStats(){return stepStats;}
	bool update(double dt);
	void draw();
};

#endif
