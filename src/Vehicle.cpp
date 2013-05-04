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
int Vehicle::movingSteps = 0;
int Vehicle::jammedSteps = 0;


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
			const GraphVertex::EdgeMap &edges = path[i+1]->getEdges();
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

bool Vehicle::checkTraffic(GraphEdge *edge, double pos){
	static const double vehicleInterval = 0.07;
	const GraphEdge::VehicleSet &vehicles =  edge->getVehicles();
	bool jammed = false;
	for(GraphEdge::VehicleSet::const_iterator it = vehicles.begin(); it != vehicles.end(); ++it){
		Vehicle *v = *it;
		// If we are going on opposite direction, ignore it
		if(v->velocity * this->velocity < 0)
			continue;
		if(0 < this->velocity){
			if(this->pos < v->pos && v->pos < this->pos + vehicleInterval){
				jammed = true;
				break;
			}
		}
		else if(this->pos - vehicleInterval < v->pos && v->pos < this->pos){
			jammed = true;
			break;
		}
	}
	return jammed;
}

bool Vehicle::update(double dt){
	do{
		if(checkTraffic(edge, pos)){
			jammed = true;
			jammedSteps++;
			break;
		}
		else{
			jammed = false;
			movingSteps++;
		}
		if(edge->getLength() < pos + velocity * dt && 1 < path.size()){
			GraphVertex::EdgeMap &edges = const_cast<GraphVertex::EdgeMap&>(path.back()->getEdges());
			GraphVertex *next = *(path.rbegin()+1);
			GraphVertex::EdgeMap::iterator it = edges.find(next);
			assert(it != edges.end());
			if(checkTraffic(it->second, pos - edge->getLength()))
				break;
		}
		pos += velocity * dt;
	} while(0);
	if(edge->getLength() < pos){
		pos -= edge->getLength();
		if(1 < path.size()){
			GraphVertex *lastVertex = path.back();
			path.pop_back();
			GraphVertex::EdgeMap::const_iterator it = lastVertex->getEdges().find(path.back());
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
	Vec2d spos;
	Vec2d epos;
	Vec2d pos;
	if(getPath().back() == getEdge()->getStart()){
		spos = getEdge()->getEnd()->getPos();
		epos = getEdge()->getStart()->getPos();
	}
	else{
		spos = getEdge()->getStart()->getPos();
		epos = getEdge()->getEnd()->getPos();
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
		else{
			if(jammed)
				glColor4f(1,0,0,1);
			else
				glColor4f(0,0,0,1);
		}
		glBegin(i == 0 ? GL_QUADS : GL_LINE_LOOP);
		glVertex2d(-5, -2);
		glVertex2d(-5,  2);
		glVertex2d( 5,  2.5);
		glVertex2d( 5, -2.5);
		glEnd();
	}
	glPopMatrix();

}
