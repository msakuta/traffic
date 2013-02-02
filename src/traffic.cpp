/** \file traffic.cpp
 * \brief The simple traffic simulator
 */

#include "GraphVertex.h"
#include "Vehicle.h"

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


const double vertexRadius = 5.;

class GraphEdge;
class Vehicle;
class Graph;



class Graph{
public:
	typedef std::set<Vehicle*> VehicleSet;
protected:
	std::vector<GraphVertex*> vertices;
	VehicleSet vehicles;
	double global_time;
public:
	Graph();
	const std::vector<GraphVertex*> &getVertices()const{return vertices;}
	const VehicleSet &getVehicles()const{return vehicles;}
	void update(double dt);
};


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

int GraphEdge::maxPassCount = 0;

inline void GraphEdge::add(Vehicle *v){
	v->setEdge(this);
	vehicles.insert(v);
	passCount++;
	if(maxPassCount < passCount)
		maxPassCount = passCount;
}



Graph::Graph() : global_time(0){
	int n = 100;
	random_sequence rs;
	init_rseq(&rs, 342125);
	for(int i = 0; i < n; i++){
		double x = drseq(&rs) * 2 - 1, y = drseq(&rs) * 2 - 1;
		GraphVertex *v = new GraphVertex(x, y);
		vertices.push_back(v);
	}

	int m = n * 10;
	for(int i = 0; i < m; i++){
		int s = rseq(&rs) % n, e = rseq(&rs) % n;
		vertices[s]->connect(vertices[e]);
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

static double gtime = 0.;
static int rollview = 0;
static double vscale = 1.;
static int pause = 0;
static int g_use_display_list = 1;

Graph graph;

static void register_lists(void);

#define CPIECES 32

void putstring(const char *s){
	while(*s)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s++);
}


/* indicators */
static void drawindics(double height, const double view_trans[16], double dt){
	int i;
	{
		glColor4ub(0,255,0,255);
		glPushMatrix();

		glLoadIdentity();
		{
			char buf[128];
			GLint vp[4];
			int w, h, m;
			double left, bottom;
			glGetIntegerv(GL_VIEWPORT, vp);
			w = vp[2], h = vp[3];
			m = w < h ? h : w;
			left = -(double)w / m;
			bottom = -(double)h / m;

/*			glRasterPos3d(left, -.2, -1.);
			sprintf(buf, "velo: %lg, %lg, %lg ", pl.velo[0], pl.velo[1], pl.velo[2]);
			putstring(buf);*/
			glRasterPos3d(left, bottom + 125. / m, -1.);
			sprintf(buf, "Display List: %s", g_use_display_list ? "On" : "Off");
			putstring(buf);
			glRasterPos3d(left, bottom + 100. / m, -1.);
			sprintf(buf, "Draw Time: %lg", dt);
			putstring(buf);
		}
		glPopMatrix();
	}
}

/// \brief Calculates parallel and perpendicular unit vectors against difference of given vectors.
/// \param para Buffer for returning vector parallel to difference of pos and dpos and have a unit length
/// \param perp Buffer for returning vector perpendicular to para and have a unit length
/// \param pos Input vector for the starting point
/// \param dpos Input vector for the destination point
/// \returns Distance of the given vectors
double calcPerp(double para[2], double perp[2], const double pos[2], const double dpos[2]){
	perp[0] = pos[1] - dpos[1];
	perp[1] = -(pos[0] - dpos[0]);
	double norm = sqrt(perp[0] * perp[0] + perp[1] * perp[1]);
	perp[0] /= norm;
	perp[1] /= norm;
	if(para){
		para[0] = -(pos[0] - dpos[0]) / norm;
		para[1] = -(pos[1] - dpos[1]) / norm;
	}
	return norm;
}

/// \brief Callback for drawing
void draw_func(double dt)
{
	
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glClearColor(0.0, 0.2, 0.1, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glScaled(0.005, 0.005, 1);

	char buf[128];

	// TODO: In this logic, we draw the road (edge) twice.
	const std::vector<GraphVertex*> &vertices = graph.getVertices();
	for(std::vector<GraphVertex*>::const_iterator it = vertices.begin(); it != vertices.end(); ++it){
		double pos[2];
		(*it)->getPos(pos);
		glColor4f(1,0,0,1);

		glPushMatrix();
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 16; i++)
			glVertex2d(pos[0] * 200 + vertexRadius * cos(i * M_2PI / 16.), pos[1] * 200 + vertexRadius * sin(i * M_2PI / 16.));
		glEnd();
		glPopMatrix();

		glRasterPos3d(pos[0] * 200, pos[1] * 200., 0.);
		sprintf(buf, "%d", &*it - &vertices.front());
		putstring(buf);

		for(GraphVertex::EdgeMap::const_iterator it2 = (*it)->getEdges().begin(); it2 != (*it)->getEdges().end(); ++it2){
			double dpos[2];
			int passCount = it2->second->getPassCount();
			it2->first->getPos(dpos);

			// Obtain vector perpendicular to the edige's direction.
			double para[2], perp[2];
			const double length = calcPerp(para, perp, pos, dpos);
			const double dashedLineLength = 0.07;
			const double lineHalfWidth = 0.003;
			const int dashedLines = int(length / dashedLineLength + 0.5);

			const double size = vertexRadius;

			glBegin(GL_QUADS);
			// Asphalt color
			glColor4f(0.5, 0.5, 0.5, 1);
			glVertex2d(pos[0] * 200 - perp[0] * size, pos[1] * 200 - perp[1] * size);
			glVertex2d(dpos[0] * 200 - perp[0] * size, dpos[1] * 200 - perp[1] * size);
			glVertex2d(pos[0] * 200 + perp[0] * size, pos[1] * 200 + perp[1] * size);
			glVertex2d(dpos[0] * 200 + perp[0] * size, dpos[1] * 200 + perp[1] * size);
			glEnd();

			glPushMatrix();
			glScaled(200, 200, 1);
			glColor4f(1, 1, 1, 1);
			glBegin(GL_QUADS);
			for(int j = 0; j < dashedLines; j++){
				glVertex2d(pos[0] + para[0] * j * dashedLineLength + perp[0] * lineHalfWidth, pos[1] + para[1] * j * dashedLineLength + perp[1] * lineHalfWidth);
				glVertex2d(pos[0] + para[0] * j * dashedLineLength - perp[0] * lineHalfWidth, pos[1] + para[1] * j * dashedLineLength - perp[1] * lineHalfWidth);
				glVertex2d(pos[0] + para[0] * (j + 0.5) * dashedLineLength - perp[0] * lineHalfWidth, pos[1] + para[1] * (j + 0.5) * dashedLineLength - perp[1] * lineHalfWidth);
				glVertex2d(pos[0] + para[0] * (j + 0.5) * dashedLineLength + perp[0] * lineHalfWidth, pos[1] + para[1] * (j + 0.5) * dashedLineLength + perp[1] * lineHalfWidth);
			}
			glEnd();
			glPopMatrix();

			// The edge color indicates traffic amount
			glColor4f(GLfloat(passCount) / GraphEdge::getMaxPassCount(),0,1,1);

			glBegin(GL_LINES);
			for(int k = -1; k <= 1; k++){
				glVertex2d(pos[0] * 200 + k * perp[0] * size, pos[1] * 200 + k * perp[1] * size);
				glVertex2d(dpos[0] * 200 + k * perp[0] * size, dpos[1] * 200 + k * perp[1] * size);
			}
			glEnd();

			glRasterPos3d((pos[0] + dpos[0]) / 2. * 200, (pos[1] + dpos[1]) / 2. * 200., 0.);
			sprintf(buf, "%d",/* it2->second->getLength(),*/ passCount);
			putstring(buf);
		}
	}

	glColor4f(0,1,1,1);
	for(Graph::VehicleSet::const_iterator it2 = graph.getVehicles().begin(); it2 != graph.getVehicles().end(); ++it2){
		Vehicle *v = *it2;
		v->draw();
	}


	// Draw Vehicle's path length distribution chart.
	glColor4f(1,1,1,1);
	int maxStepCount = 0;
	int stepSum = 0;
	int stepMoment = 0;
	const int *stepStats = Vehicle::getStepStats();
	for(int i = 0; i < Vehicle::stepStatCount; i++){
		if(maxStepCount < stepStats[i])
			maxStepCount = stepStats[i];
		stepSum += stepStats[i];
		stepMoment += stepStats[i] * i;
	}
	if(maxStepCount){
		for(int i = 0; i < Vehicle::stepStatCount; i++){
			glRasterPos2d(-200., -180 + i * 16);
			sprintf(buf, "%d:%d",/* it2->second->getLength(),*/ i, stepStats[i]);
			putstring(buf);
			glBegin(GL_LINES);
			glVertex2d(-200, -180 + i * 16);
			glVertex2d(-200 + stepStats[i] * 200 / maxStepCount, -180 + i * 16);
			glEnd();
		}
		glRasterPos2d(-200., -180 + Vehicle::stepStatCount * 16);
		sprintf(buf, "Avg: %lg", double(stepMoment) / stepSum);
		putstring(buf);
	}

	glFlush();
	glutSwapBuffers();

/*	Sleep(1);*/
}

#define EPSILON 1e-5



/// \brief Callback for updating screen
void display_func(void){
	static timemeas_t tm;
	static int init = 0;
	static double t = 0.;
	double dt = 0.;
	if(!init){
		init = 1;
		TimeMeasStart(&tm);
	}
	else{
		double t1;
		t1 = TimeMeasLap(&tm);
		dt = init ? t1 - t : 0.;

		if(!pause){
			graph.update(dt);
		}

		gtime = t = t1;
	}
	draw_func(dt);
/*	glViewport(dim[0], dim[1], dim[2], dim[3]);*/
}

void idle(void){
	glutPostRedisplay();
}

/// \brief Callback for window size change
void reshape_func(int w, int h)
{
	int m = w < h ? h : w;
	glViewport(0, 0, w, h);
}


/// \brief Callback for key input
static void key_func(unsigned char key, int x, int y){
	switch(key){
		case 'p': pause = !pause; break;

		case 'i': g_use_display_list = !g_use_display_list;
	}
}

int g_pressed = 0;

int g_prevX = -1;
int g_prevY = -1;
int g_prevZ = -1;

/// \brief Callback for mouse input
static void mouse_func(int button, int state, int x, int y){
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			g_prevX = x;
			g_prevY = y;
			g_pressed = 1;
		}
		else if (state == GLUT_UP) {
			g_pressed = 0;
		}
	}
	else if(button == GLUT_RIGHT_BUTTON){
		if(state == GLUT_DOWN) {
			g_prevY = y;
			g_pressed = 2;
		}
		else if(state == GLUT_UP) {
			g_pressed = 0;
		}
	}
}

/// \brief Callback for mouse motion
void motion_func(int x, int y){
	if (g_pressed != 0) {

		g_prevX = x;
		g_prevY = y;

		glutPostRedisplay();

	}

}


int main(int argc, char *argv[])
{

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutInitWindowSize(640,480);

	glutCreateWindow(2 <= argc ? argv[1] : "No File");

	glutDisplayFunc(display_func);

	glutReshapeFunc(reshape_func);

	glutMouseFunc(mouse_func);

	glutMotionFunc(motion_func);

	glutKeyboardFunc(key_func);
	glutIdleFunc(idle);

	register_lists();

	glutMainLoop();

	return 0;
}

static void register_lists(void){

}

