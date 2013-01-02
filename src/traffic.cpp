/** \file traffic.cpp
 * \brief The simple traffic simulator
 */

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

#include <vector>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

class GraphEdge;
class Vehicle;

class GraphVertex{
public:
	typedef std::map<GraphVertex*, GraphEdge*> EdgeMap;
protected:
	std::map<GraphVertex*, GraphEdge*> edges;
	double pos[2];
public:
	GraphVertex(double x, double y){
		pos[0] = x, pos[1] = y;
	}
	void getPos(double pos[2])const{pos[0] = this->pos[0]; pos[1] = this->pos[1];}
	const EdgeMap &getEdges()const{return edges;}
	double measureDistance(const GraphVertex &other)const{
		double startPos[2], endPos[2];
		this->getPos(startPos);
		other.getPos(endPos);
		return sqrt((startPos[0] - endPos[0]) * (startPos[0] - endPos[0]) + (startPos[1] - endPos[1]) * (startPos[1] - endPos[1]));
	}
	bool connect(GraphVertex *other);
};

class GraphEdge{
	GraphVertex *start;
	GraphVertex *end;
	std::vector<Vehicle*> vehicles;
	double length;
public:
	GraphEdge(GraphVertex *start, GraphVertex *end) : start(start), end(end){
		length = start->measureDistance(*end);
	}
	double getLength()const{return length;}
};

class Vehicle{
	const GraphVertex *dest;
	double velocity;
};

class Graph{
	std::vector<GraphVertex*> vertices;
public:
	Graph();
	const std::vector<GraphVertex*> &getVertices()const{return vertices;}
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

Graph::Graph(){
	int n = 100;
	random_sequence rs;
	init_rseq(&rs, 342125);
	for(int i = 0; i < n; i++){
		double x = drseq(&rs) * 2 - 1, y = drseq(&rs) * 2 - 1;
		GraphVertex *v = new GraphVertex(x, y);
		vertices.push_back(v);
	}

	int m = n * 3;
	for(int i = 0; i < m; i++){
		int s = rseq(&rs) % n, e = rseq(&rs) % n;
		vertices[s]->connect(vertices[e]);
	}
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


/// \brief Callback for drawing
void draw_func(double dt)
{
	
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glClearColor(0.2, 0.3, 0.4, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glScaled(0.005, 0.005, 1);

	char buf[128];

	const std::vector<GraphVertex*> &vertices = graph.getVertices();
	for(std::vector<GraphVertex*>::const_iterator it = vertices.begin(); it != vertices.end(); ++it){
		double pos[2];
		(*it)->getPos(pos);
		glColor4f(1,0,0,1);
		glBegin(GL_LINE_LOOP);
		glVertex2d(pos[0] * 200 - 5, pos[1] * 200 - 5);
		glVertex2d(pos[0] * 200 - 5, pos[1] * 200 + 5);
		glVertex2d(pos[0] * 200 + 5, pos[1] * 200 + 5);
		glVertex2d(pos[0] * 200 + 5, pos[1] * 200 - 5);
		glEnd();

		glRasterPos3d(pos[0] * 200, pos[1] * 200., 0.);
		sprintf(buf, "%d", &*it - &vertices.front());
		putstring(buf);

		glColor4f(1,0,1,1);
		for(GraphVertex::EdgeMap::const_iterator it2 = (*it)->getEdges().begin(); it2 != (*it)->getEdges().end(); ++it2){
			double dpos[2];
			it2->first->getPos(dpos);
			glBegin(GL_LINES);
			glVertex2d(pos[0] * 200, pos[1] * 200);
			glVertex2d(dpos[0] * 200, dpos[1] * 200);
			glEnd();

			glRasterPos3d((pos[0] + dpos[0]) / 2. * 200, (pos[1] + dpos[1]) / 2. * 200., 0.);
			sprintf(buf, "%lg", it2->second->getLength());
			putstring(buf);
		}
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
		int i;
		double t1;
		t1 = TimeMeasLap(&tm);
		dt = init ? t1 - t : 0.;

		if(!pause){
			// ...
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

