/** \file Graph.h
 * \brief Definition of Graph class
 */
#ifndef GRAPH_H
#define GRAPH_H

#include <set>
#include <vector>

class GraphVertex;
class GraphEdge;
class Vehicle;

class Graph{
public:
	typedef std::set<Vehicle*> VehicleSet;
	typedef std::vector<GraphVertex*> VertexList;
protected:
	VertexList vertices;
	VehicleSet vehicles;
	double global_time;
public:
	Graph();
	const VertexList &getVertices()const{return vertices;}
	const VehicleSet &getVehicles()const{return vehicles;}
	void update(double dt);
};

#endif
