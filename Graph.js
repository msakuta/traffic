
var vertexRadius = 10.;

function GraphVertex(x,y){
	this.x = x;
	this.y = y;
//	document.write(this.x + " " + this.y + ";");
	this.edges = {}; // Edges are a map of destination id to GraphEdge object.
	this.vehicles = [];
	this.id = GraphVertex.prototype.idGen++;
}

GraphVertex.prototype.idGen = 0;

GraphVertex.prototype.getPos = function(){
	return new Array(this.x, this.y);
}

GraphVertex.prototype.print = function(){
	return "(" + this.x + ", " + this.y + ")";
}

GraphVertex.prototype.measureDistance = function(other){
	return Math.sqrt((this.x - other.x) * (this.x - other.x) + (this.y - other.y) * (this.y - other.y));
}

GraphVertex.prototype.connect = function(other){
//	EdgeMap::iterator it = edges.find(other);
//	if(it != edges.end())
//		return false; // Already added

	var length = this.measureDistance(other);
	if(60.0 < length)
		return false; // Avoid adding long edges
	var e = new GraphEdge(this, other);
	this.edges[other.id] = e;
	other.edges[this.id] = e;
	return true;
}

/// \param v Vehicle to add
GraphVertex.prototype.addVehicle = function(v){
	var path = v.path;
	if(1 < path.length){
		this.edges[path[path.length - 2].id].addVehicle(v);
		path.pop();
	}
}

function GraphEdge(start, end){
	this.start = start;
	this.end = end;
	this.passCount = 0;
	this.length = start.measureDistance(end);
	this.vehicles = [];
}

/// \param v Vehicle to add
GraphEdge.prototype.addVehicle = function(v){
	v.edge = this;
	this.vehicles.push(v);
}

GraphEdge.prototype.deleteVehicle = function(v){
	for(var i = 0; i < this.vehicles.length; i++){
		if(this.vehicles[i] === v){
			this.vehicles.splice(i, 1);
			return true;
		}
	}
	return false;
}

/// \brief Class representing a vehicle.
/// \param dest The destination GraphVertex.
function Vehicle(dest){
	this.id = 0; // Initialize with invalid id
	this.dest = dest;
	this.edge = null;
	this.path = new Array();
	this.pos = 0;
	this.velocity = 10;
	this.color = new Array(3);
	this.jammed = false;
	for(var i = 0; i < 3; i++)
		this.color[i] = Math.random();
}

/// \brief Finds a path to the destination.
/// \param g The surrounding Graph.
/// \param start The starting GraphVertex for finding path.
Vehicle.prototype.findPath = function(g, start){
	var visited = {};
	visited[start.id] = true;

	// Unlike C++, we cannot have objects as indices for a map (dictionary).
	// So we must have IDs of each vertex as indices instead and be aware that
	// JavaScript hash indices are automatically converted to strings.
	var first = {};
	first[start.id] = null;

	if(this.findPathInt(g, start, first, visited)){
		if(this.path.length <= 1){
			this.path.length = 0;
			return false;
		}
//		path.push_back(start);
		// Make sure the path is reachable
		for(var i = 0; i < this.path.length-1; i++){
			var edges = this.path[i+1].edges;
//			assert(edges.find(path[i]) != edges.end());
		}
//		if(this.path.length < 10)
//			stepStats[path.size()]++;
		return true;
	}
	else
		return false;
	return true;
}

/// \brief An internal function to find a path to the destination.
/// \param g The surrounding Graph.
/// \param start The starting GraphVertex for finding path.
/// \param prevMap VertexMap
/// \param visited VertexSet
Vehicle.prototype.findPathInt = function(g, start, prevMap, visited){
	var levelMap = {};
	var hasLevelMap = false;
	for(var it in prevMap){
		var v = g.vertices[it];
		if(v === undefined)
			continue;
		for(var it2 in v.edges){
			if(it2 in visited)
				continue;
			var v2 = v.edges[it2];
			if(v2 === undefined)
				continue;
			visited[it2] = true;
			// Obtain GraphVertex object from vertex pool in Graph object.
			levelMap[it2] = v;
			hasLevelMap = true;
			if(it2 == this.dest.id){
				if(v === v2.start)
					this.path.push(v2.end);
				else
					this.path.push(v2.start);
				this.path.push(v); // We know the path came via v.
				return true;
			}
		}
	}
	if(hasLevelMap){
		if(this.findPathInt(g, start, levelMap, visited)){
			var v = levelMap[this.path.back().id];
//			assert(v);
			this.path.push(v);
			return true;
		}
	}
	return false;
}

Array.prototype.back = function(){
	return this[this.length-1];
}

Vehicle.prototype.update = function(dt){
	if(this.edge == null)
		return false;

	// Check collision with other vehicles
	do{
		if(this.checkTraffic(this.edge, this.pos)){
			this.jammed = true;
			break;
		}
		if(this.edge.length < this.pos + this.velocity * dt && 1 < this.path.length){
			var edges = this.path.back().edges;
			var next = this.path.back();
			var edge = edges[next.id];
//			assert(it != edges.end());
			if(edge !== undefined && this.checkTraffic(edge, this.pos - edge.length)){
				this.jammed = true;
				break;
			}
		}
		this.pos += this.velocity * dt;
		this.jammed = false;
	} while(0);

	if(this.edge.length < this.pos){
		this.pos -= this.edge.length;
		if(1 < this.path.length){
			var lastVertex = this.path.pop();
			this.edge.deleteVehicle(this);
			this.edge = lastVertex.edges[this.path.back().id];
			this.edge.addVehicle(this);
		}
		else{
//			edge->remove(this);
//			delete this;
			return false;
		}
	}
	this.onUpdate(dt);
	return true;
}

/// \brief Calculates parallel and perpendicular unit vectors against difference of given vectors.
/// \param para Buffer for returning vector parallel to difference of pos and dpos and have a unit length
/// \param perp Buffer for returning vector perpendicular to para and have a unit length
/// \param pos Input vector for the starting point
/// \param dpos Input vector for the destination point
/// \returns Distance of the given vectors
function calcPerp(para, perp, pos, dpos){
	perp[0] = pos[1] - dpos[1];
	perp[1] = -(pos[0] - dpos[0]);
	var norm = Math.sqrt(perp[0] * perp[0] + perp[1] * perp[1]);
	perp[0] /= norm;
	perp[1] /= norm;
	if(para !== null){
		para[0] = -(pos[0] - dpos[0]) / norm;
		para[1] = -(pos[1] - dpos[1]) / norm;
	}
	return norm;
}

/// \brief Returns position of this vehicle in world coordinates, optionally returning edge ends this Vehicle is on.
/// \param pSpos An array object for receiving starting position of the GraphEdge this Vehicle is on.
///              Can be undefined to ignore the result.
///              This parameter has a 'p' prefix to imply it's used like a pointer in C.
/// \param pEpos An array object for receiving ending position of the GraphEdge this Vehicle is on.
///              Can be undefined to ignore the result.
///
/// This function's main purpose is to calculate the world coordinates to display this Vehicle,
/// but obtaining positions of both ends of the edges this vehicle is currently running is not something
/// that we want separate function to do.  So this function can do both tasks at once.
Vehicle.prototype.calcPos = function(pSpos, pEpos){
	var v = this;
	var spos = null;
	var epos = null;
	if(v.path.back() == v.edge.start){
		spos = v.edge.end.getPos();
		epos = v.edge.start.getPos();
	}
	else{
		spos = v.edge.start.getPos();
		epos = v.edge.end.getPos();
	}
	var pos = new Array(2);

	var perp = new Array(2);
	calcPerp(null, perp, spos, epos);

	for(var i = 0; i < 2; i++)
		pos[i] = epos[i] * v.pos / v.edge.length + spos[i] * (v.edge.length - v.pos) / v.edge.length
			+ perp[i] * vertexRadius / 2.;
	
	// Store optional returning values into passed buffers.
	if(pSpos !== undefined)
		pSpos[0] = spos[0], pSpos[1] = spos[1];
	if(pEpos !== undefined)
		pEpos[0] = epos[0], pEpos[1] = epos[1];

	return pos;
}

Vehicle.prototype.vehicleInterval = 15;

Vehicle.prototype.checkTraffic = function(edge, pos){
	var vehicles = edge.vehicles;
	var jammed = false;
	for(var i = 0; i < vehicles.length; i++){
		var v = vehicles[i];
		if(v === this) // Skip checking itself
			continue;
		// If we are going on opposite direction, ignore it
		// But the velocity is by definition always approaches destination in
		// positive side, so we must look the path to find the actual direction on the edge.
//		if(v.velocity * this.velocity < 0)
//			continue;
		if(v.path.back() !== this.path.back())
			continue;
		if(0 < this.velocity){
			if(pos < v.pos && v.pos < pos + this.vehicleInterval){
				jammed = true;
				break;
			}
		}
		else if(pos - this.vehicleInterval < v.pos && v.pos < pos){
			jammed = true;
			break;
		}
	}
	return jammed;
}

Vehicle.prototype.onUpdate = function(dt){
	// Default does nothing
}

Vehicle.prototype.onVehicleAdd = function(v){
	// Default does nothing
}

Vehicle.prototype.onVehicleDelete = function(v){
	// Default does nothing
}




/// A pseudo-random number generator distributed in Poisson distribution.
/// It uses Knuth's algorithm, which is not optimal when lambda gets
/// so high.  We probably should use an approximation.
function poissonRandom(rng,lambda){
	var L = Math.exp(-lambda);
	var k = 0;
	var p = 1;
	do{
		k++;
		p *= rng.next();
	}while(L < p);
	return k - 1;
}


function Graph(width, height){
	this.rng = new Xor128(); // Create Random Number Generator
	var rng = this.rng;
	var n = 100;
	var minDist = vertexRadius * 3; // Minimum distance between vertices, initialized by 3 times the radius of vertices.
	this.vertices = new Array(n);
	this.vehicles = [];
	this.vehicleIdGen = 1;
//	document.write(width + " " + height + ";");
	for(var i = 0; i < n; i++){
		var x, y;
		
		// Repeat generating random positions until a position that are not close to
		// any of the vertices is obtained.
		// Be careful not to set too high value for n which could consume room for a vertex,
		// in which case an infinite loop could occur.
		do{
			x = rng.next() * width;
			y = rng.next() * height;
		} while(function(t){
			for(var j = 0; j < i; j++){
				var dx = t.vertices[j].x - x;
				var dy = t.vertices[j].y - y;
				if(dx * dx + dy * dy < minDist * minDist)
					return true;
			}
			return false;
		}(this));
		
		// Use the position to create a GraphVertex.
		this.vertices[i] = new GraphVertex(x, y);
	}
	for(var i = 0; i < n; i++){
		// If average number of edges a vertex has is 2, most vertices connect to far ones,
		// which is ideal for simulating traffic.
		// But we cannot set the parameter too high because  it would get cluttered and
		// harder to grasp what's going on.
		// The numbers of edges are assumed Poisson distributed.
		var numEdges = poissonRandom(rng, 2);
		for(var j = 0; j < numEdges; j++){
			// Try to find close connections to make a network.
			for(var tries = 0; tries < 500; tries++){
				var e = Math.floor(rng.next() * (n-1));
				if(this.vertices[i].connect(this.vertices[e]))
					break;
			}
		}
	}
}

Graph.prototype.global_time = 0;

Graph.prototype.update = function(dt){
	var global_time = Graph.prototype.global_time;

	// Number of vehicles generated in a frame distributes in Poisson distribution.
	var numVehicles = poissonRandom(this.rng, 0.5);

	// A local function to find if the given vehicle's starting point is too crowded to start the vehicle.
	function isCrowded(v){
		// Get the first edge this vehicle is going to pass.
		var path = v.path;
		var lastVertex = path.back();
		if(lastVertex === undefined)
			return false;
		var beforeLastVertex = path[path.length-2];
		if(beforeLastVertex === undefined)
			return false;
		var edge = lastVertex.edges[beforeLastVertex.id];
		if(edge === undefined)
			return false;

		// If this edge has too many vehicles that another vehicle has no room to enter,
		// give up creating the vehicle.
		// This would prevent number of vehicles from increasing forever even if
		// the road network is complex.
		return edge.length / Vehicle.prototype.vehicleInterval <= edge.vehicles.length;
	}

	for(var n = 0; n < numVehicles; n++){
		for(var i = 0; i < 100; i++){
			var starti = Math.floor(this.rng.next() * (this.vertices.length-1));
			var endi = Math.floor(this.rng.next() * (this.vertices.length-1));
			var v = new Vehicle(this.vertices[endi]);

			if(v.findPath(this, this.vertices[starti]) && !isCrowded(v)){
				// Assign the id only if addition of the vehicle is succeeded.
				v.id = this.vehicleIdGen++;
				this.vertices[starti].addVehicle(v);
				this.vehicles.push(v);
				v.onVehicleAdd();
				break;
			}
//		else
//			delete v;
		}
	}

	for(var i = 0; i < this.vehicles.length;){
		var v = this.vehicles[i];
		if(!v.update(dt)){
			v.onVehicleDelete();
			v.edge.deleteVehicle(v);
			this.vehicles.splice(i, 1);
		}
		else
			i++;
	}

//	invokes++;
	Graph.prototype.global_time += dt;
}

