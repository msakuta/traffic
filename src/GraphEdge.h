/** \file GraphEdge.h
 * \brief Definition of GraphEdge class
 */
#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H
#include "GraphVertex.h"
	
#include <set>

class Vehicle;

class GraphEdge{
public:
	typedef std::set<Vehicle*> VehicleSet;
protected:
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
	const VehicleSet &getVehicles()const{return vehicles;}
	void add(Vehicle *v);
	void remove(Vehicle *v){
		vehicles.erase(v);
	}
	int getPassCount()const{return passCount;}
	static int getMaxPassCount(){return maxPassCount;}
	bool isIntersecting(const double start[2], const double dir0[2])const;
	static bool intersectTest(const double start0[2], const double dir0[2], const double start1[2], const double dir1[2]);
};


#endif
