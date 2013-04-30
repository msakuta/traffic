/** \file Vehicle.cpp
 * \brief Implementation of Vehicle class
 */

#include "Vehicle.h"
#include "GraphVertex.h"
#include "GraphEdge.h"

#include <GL/glut.h>
#include <GL/gl.h>
#define exit something_meanless
extern "C"{
#include <clib/rseq.h>
#include <clib/timemeas.h>
}

#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include <vector>
#include <map>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#ifndef M_2PI
#define M_2PI (2. * M_PI)
#endif


int Vehicle::stepStats[Vehicle::stepStatCount] = {0};


bool Vehicle::findPath(Graph *g, GraphVertex *start){
	VertexSet visited;
	visited.insert(start);

	VertexMap first;
	first[start] = NULL;

	if(findPathInt(g, start, first, visited)){
		if(path.size() <= 1){
			path.clear();
			return false;
		}
//		path.push_back(start);
		// Make sure the path is reachable
		for(int i = 0; i < path.size()-1; i++){
			auto edges = path[i+1]->getEdges();
			assert(edges.find(path[i]) != edges.end());
		}
		if(path.size() < 10)
			stepStats[path.size()]++;
		return true;
	}
	else
		return false;
}

bool Vehicle::findPathInt(Graph *g, GraphVertex *start, VertexMap &prevMap, VertexSet &visited){
	VertexMap levelMap;
	for(VertexMap::iterator it = prevMap.begin(); it != prevMap.end(); ++it){
		GraphVertex *v = it->first;
		for(GraphVertex::EdgeMap::const_iterator it2 = v->getEdges().begin(); it2 != v->getEdges().end(); ++it2){
			if(visited.find(it2->first) != visited.end())
				continue;
			visited.insert(it2->first);
			levelMap[it2->first] = v;
			if(it2->first == dest){
				path.push_back(it2->first);
				path.push_back(v); // We know the path came via v.
				return true;
			}
		}
	}
	if(!levelMap.empty()){
		if(findPathInt(g, start, levelMap, visited)){
			GraphVertex *v = levelMap[path.back()];
			assert(v);
			path.push_back(v);
			return true;
		}
	}
	return false;
}

bool Vehicle::update(double dt){
	pos += velocity * dt;
	if(edge->getLength() < pos){
		pos -= edge->getLength();
		if(1 < path.size()){
			GraphVertex *lastVertex = path.back();
			path.pop_back();
			auto it = lastVertex->getEdges().find(path.back());
			assert(it != lastVertex->getEdges().end());
			edge->remove(this);
			edge = it->second;
			edge->add(this);
		}
		else{
			edge->remove(this);
			delete this;
			return false;
		}
	}
	return true;
}


void Vehicle::draw(){
	double spos[2];
	double epos[2];
	double pos[2];
	if(getPath().back() == getEdge()->getStart()){
		getEdge()->getEnd()->getPos(spos);
		getEdge()->getStart()->getPos(epos);
	}
	else{
		getEdge()->getStart()->getPos(spos);
		getEdge()->getEnd()->getPos(epos);
	}

	double perp[2];
	calcPerp(NULL, perp, spos, epos);

	for(int i = 0; i < 2; i++)
		pos[i] = epos[i] * getPos() / getEdge()->getLength() + spos[i] * (getEdge()->getLength() - getPos()) / getEdge()->getLength()
			+ perp[i] * vertexRadius / 2. / 200.;
	glPushMatrix();
	glTranslated(pos[0] * 200, pos[1] * 200, 0);
	double angle = atan2((spos[1] - epos[1]), spos[0] - epos[0]);
	glRotated(angle * 360 / M_2PI, 0, 0, 1);
	for(int i = 0; i < 2; i++){
		if(i == 0)
			glColor3fv(color);
		else
			glColor4f(0,0,0,1);
		glBegin(i == 0 ? GL_QUADS : GL_LINE_LOOP);
		glVertex2d(-5, -2);
		glVertex2d(-5,  2);
		glVertex2d( 5,  2.5);
		glVertex2d( 5, -2.5);
		glEnd();
	}
	glPopMatrix();

}
