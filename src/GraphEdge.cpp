/** \file GraphEdge.cpp
 * \brief Implementation of GraphEdge class
 */

#include "GraphEdge.h"
#include "Vehicle.h"



int GraphEdge::maxPassCount = 0;

void GraphEdge::add(Vehicle *v){
	v->setEdge(this);
	vehicles.insert(v);
	passCount++;
	if(maxPassCount < passCount)
		maxPassCount = passCount;
}


