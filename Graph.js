
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
		if(this.checkTraffic(this.edge, this.pos))
			break;
		if(this.edge.length < this.pos + this.velocity * dt && 1 < this.path.length){
			var edges = this.path.back().edges;
			var next = this.path.back();
			var edge = edges[next.id];
//			assert(it != edges.end());
			if(edge !== undefined && this.checkTraffic(edge, this.pos - edge.length))
				break;
		}
		this.pos += this.velocity * dt;
	} while(0);

	if(this.edge.length < this.pos){
		this.pos -= this.edge.length;
		if(1 < this.path.length){
			var lastVertex = this.path.pop();
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

Vehicle.prototype.calcPos = function(){
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
	for(var i = 0; i < 2; i++)
		pos[i] = epos[i] * v.pos / v.edge.length + spos[i] * (v.edge.length - v.pos) / v.edge.length;
	return pos;
}

Vehicle.prototype.checkTraffic = function(edge, pos){
	var vehicleInterval = 15;
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
			if(pos < v.pos && v.pos < pos + vehicleInterval){
				jammed = true;
				break;
			}
		}
		else if(pos - vehicleInterval < v.pos && v.pos < pos){
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
	this.vertices = new Array(n);
	this.vehicles = [];
	this.vehicleIdGen = 1;
//	document.write(width + " " + height + ";");
	for(var i = 0; i < n; i++)
		this.vertices[i] = new GraphVertex(rng.next() * width, rng.next() * height);
	var m = n * 10;
	for(var i = 0; i < m; i++){
		var s = Math.floor(rng.next() * (n-1)), e = Math.floor(rng.next() * (n-1));
//		document.write(s + " " + e + ";");
		this.vertices[s].connect(this.vertices[e]);
	}
}

Graph.prototype.global_time = 0;

Graph.prototype.update = function(dt){
	var global_time = Graph.prototype.global_time;

	// Number of vehicles generated in a frame distributes in Poisson distribution.
	var numVehicles = poissonRandom(this.rng, 0.1);

	for(var n = 0; n < numVehicles; n++){
		for(var i = 0; i < 100; i++){
			var starti = Math.floor(this.rng.next() * (this.vertices.length-1));
			var endi = Math.floor(this.rng.next() * (this.vertices.length-1));
			var v = new Vehicle(this.vertices[endi]);
			if(v.findPath(this, this.vertices[starti])){
				// Assign the id only if addition of the vehicle is succeeded.
				v.id = this.vehicleIdGen++;
				this.vertices[starti].addVehicle(v);
				this.vehicles.push(v);
				v.onVehicleAdd();
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

