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
#include <assert.h>

#include <vector>
#include <map>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

class GraphEdge;
class Vehicle;
class Graph;

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
	void add(Vehicle *v);
};

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

class Vehicle{
public:
	typedef std::set<GraphVertex*> VertexSet;
	typedef std::map<GraphVertex*, GraphVertex*> VertexMap;
	typedef std::vector<GraphVertex*> Path;
	static const int stepStatCount = 20;
protected:
	const GraphVertex *dest;
	GraphEdge *edge;
	Path path;
	double pos; ///< [0,1)
	double velocity;
	static int stepStats[stepStatCount];
	bool findPathInt(Graph *, GraphVertex *root, VertexMap &prevMap, VertexSet &visited);
public:
	Vehicle(GraphVertex *dest) : dest(dest), edge(NULL), pos(0), velocity(0.1){}
	bool findPath(Graph *, GraphVertex *start);
	Path &getPath(){return path;}
	double getPos()const{return pos;}
	const GraphEdge *getEdge()const{return edge;}
	void setEdge(GraphEdge *edge){ this->edge = edge; }
	static const int *getStepStats(){return stepStats;}
	bool update(double dt){
		pos += velocity * dt;
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
};

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

		for(GraphVertex::EdgeMap::const_iterator it2 = (*it)->getEdges().begin(); it2 != (*it)->getEdges().end(); ++it2){
			double dpos[2];
			int passCount = it2->second->getPassCount();
			it2->first->getPos(dpos);

			glColor4f(GLfloat(passCount) / GraphEdge::getMaxPassCount(),0,1,1);

			glBegin(GL_LINES);
			glVertex2d(pos[0] * 200, pos[1] * 200);
			glVertex2d(dpos[0] * 200, dpos[1] * 200);
			glEnd();

			glRasterPos3d((pos[0] + dpos[0]) / 2. * 200, (pos[1] + dpos[1]) / 2. * 200., 0.);
			sprintf(buf, "%d",/* it2->second->getLength(),*/ passCount);
			putstring(buf);
		}
	}

	glColor4f(0,1,1,1);
	for(Graph::VehicleSet::const_iterator it2 = graph.getVehicles().begin(); it2 != graph.getVehicles().end(); ++it2){
		double spos[2];
		double epos[2];
		double pos[2];
		Vehicle *v = *it2;
		if(v->getPath().back() == v->getEdge()->getStart()){
			v->getEdge()->getEnd()->getPos(spos);
			v->getEdge()->getStart()->getPos(epos);
		}
		else{
			v->getEdge()->getStart()->getPos(spos);
			v->getEdge()->getEnd()->getPos(epos);
		}
		for(int i = 0; i < 2; i++)
			pos[i] = epos[i] * v->getPos() / v->getEdge()->getLength() + spos[i] * (v->getEdge()->getLength() - v->getPos()) / v->getEdge()->getLength();
		glBegin(GL_LINES);
		glVertex2d(pos[0] * 200 - 5, pos[1] * 200 - 5);
		glVertex2d(pos[0] * 200 + 5, pos[1] * 200 + 5);
		glVertex2d(pos[0] * 200 - 5, pos[1] * 200 + 5);
		glVertex2d(pos[0] * 200 + 5, pos[1] * 200 - 5);
		glEnd();
	}


	// Draw Vehicle's path length distribution chart.
	glColor4f(1,1,1,1);
	int maxStepCount = 0;
	const int *stepStats = Vehicle::getStepStats();
	for(int i = 0; i < Vehicle::stepStatCount; i++){
		if(maxStepCount < stepStats[i])
			maxStepCount = stepStats[i];
	}
	if(maxStepCount) for(int i = 0; i < Vehicle::stepStatCount; i++){
		glRasterPos2d(-200., -180 + i * 16);
		sprintf(buf, "%d:%d",/* it2->second->getLength(),*/ i, stepStats[i]);
		putstring(buf);
		glBegin(GL_LINES);
		glVertex2d(-200, -180 + i * 16);
		glVertex2d(-200 + stepStats[i] * 200 / maxStepCount, -180 + i * 16);
		glEnd();
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

