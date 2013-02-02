/** \file GraphEdge.h
 * \brief Definition of GraphEdge class
 */
#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H
#include "GraphVertex.h"
	
#include <set>

class Vehicle;

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


#endif
