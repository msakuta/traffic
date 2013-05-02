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


bool GraphEdge::isIntersecting(const double startPos0[2], const double dir0[2])const{
	double startPos[2], endPos[2], dir[2];
	getStart()->getPos(startPos);
	getEnd()->getPos(endPos);
	for(int j = 0; j < 2; j++)
		dir[j] = endPos[j] - startPos[j];
	return intersectTest(startPos0, dir0, startPos, dir);
}

bool GraphEdge::intersectTest(const double start0[2], const double dir0[2], const double start1[2], const double dir1[2]){
	double dirvp = (dir1[0] * dir0[1] - dir1[1] * dir0[0]);
	if(dirvp == 0.)
		return false;
	double startDiff[2];
	for(int j = 0; j < 2; j++)
		startDiff[j] = start1[j] - start0[j];
	double t1 = -(startDiff[0] * dir0[1] - startDiff[1] * dir0[0]) / dirvp;
	if(t1 < 0 || 1 <= t1)
		return false;
	double t0 = -(startDiff[0] * dir1[1] - startDiff[1] * dir1[0]) / dirvp;
	if(t0 < 0 || 1 <= t0)
		return false;
	return true;
}
