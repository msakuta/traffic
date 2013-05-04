/** \file Graph.cpp
 * \brief Implementation of Graph class
 */

#include "Graph.h"
#include "GraphVertex.h"
#include "Vehicle.h"

extern "C"{
#include <clib/rseq.h>
}

const double Graph::minVertexDistance = 0.05;

Graph::Graph() : global_time(0){
	int n = 500;
	random_sequence rs;
	init_rseq(&rs, 342125);
	for(int i = 0; i < n; i++){
		double xy[2] = {drseq(&rs) * 2 - 1, drseq(&rs) * 2 - 1};
		bool tooNear = false;
		for(VertexList::iterator it = vertices.begin(); it != vertices.end(); ++it){
			if((*it)->measureDistance(xy) < minVertexDistance){
				tooNear = true;
				break;
			}
		}
		if(tooNear)
			continue;
		GraphVertex *v = new GraphVertex(xy[0], xy[1]);
		vertices.push_back(v);
	}

	n = vertices.size();
	int m = n * 250;
	for(int i = 0; i < m; i++){
		int s = rseq(&rs) % n, e = rseq(&rs) % n;
		if(s != e && vertices[s]->getEdges().size() < 4 && vertices[e]->getEdges().size() < 4)
			vertices[s]->connect(*this, vertices[e]);
	}
}

void Graph::update(double dt){
	static int invokes = 0;
	static random_sequence rs;
	if(invokes == 0)
		init_rseq(&rs, 87657444);
	const double genInterval = 0.1;

	if(fmod(global_time + dt, genInterval) < fmod(global_time, genInterval)){
		int starti = rseq(&rs) % vertices.size();
		int endi = rseq(&rs) % vertices.size();
		Vehicle *v = new Vehicle(vertices[endi]);
		if(v->findPath(this, vertices[starti])){
			vertices[starti]->add(v);
			vehicles.insert(v);
		}
		else
			delete v;
	}

	for(VehicleSet::iterator it = vehicles.begin(); it != vehicles.end();){
		VehicleSet::iterator next = it;
		++next;
		Vehicle *v = *it;
		if(!v->update(dt))
			vehicles.erase(it);
		it = next;
	}

	invokes++;
	global_time += dt;
}
