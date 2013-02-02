/** \file GraphVertex.cpp
 * \brief Implementation of GraphVertex class
 */

#include "GraphVertex.h"
#include "GraphEdge.h"
#include "Vehicle.h"



const double vertexRadius = 5.;


bool GraphVertex::connect(GraphVertex *other){
	EdgeMap::iterator it = edges.find(other);
	if(it != edges.end())
		return false; // Already added

	double length = measureDistance(*other);
	if(0.6 < length)
		return false; // Avoid adding long edges
	GraphEdge *e = new GraphEdge(this, other);
	edges[other] = e;
	other->edges[this] = e;
	return true;
}

void GraphVertex::add(Vehicle *v){
	Vehicle::Path &path = v->getPath();
	if(1 < path.size()){
		edges[path[path.size() - 2]]->add(v);
		path.pop_back();
	}
}
