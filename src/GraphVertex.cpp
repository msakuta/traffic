/** \file GraphVertex.cpp
 * \brief Implementation of GraphVertex class
 */

#include "GraphVertex.h"
#include "GraphEdge.h"
#include "Vehicle.h"
#include "Graph.h"


const double vertexRadius = 2.5;
const double maxEdgeLength = 0.2;


bool GraphVertex::connect(Graph &graph, GraphVertex *other){
	EdgeMap::iterator it = edges.find(other);
	if(it != edges.end())
		return false; // Already added

	double length = measureDistance(*other);
	if(maxEdgeLength < length)
		return false; // Avoid adding long edges

	double startPos0[2], endPos0[2], dir0[2];
	this->getPos(startPos0);
	other->getPos(endPos0);
	for(int j = 0; j < 2; j++)
		dir0[j] = endPos0[j] - startPos0[j];
	const Graph::VertexList &vertices = graph.getVertices();
	for(Graph::VertexList::const_iterator it = vertices.begin(); it != vertices.end(); ++it){
		if(*it == this)
			continue;
		for(EdgeMap::iterator it2 = (*it)->edges.begin(); it2 != (*it)->edges.end(); ++it2){
			if(it2->second->isIntersecting(startPos0, dir0))
				return false;
		}
	}
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
