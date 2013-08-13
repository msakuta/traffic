
function GraphVertex(x,y){
	this.x = x;
	this.y = y;
//	document.write(this.x + " " + this.y + ";");
	this.edges = new Array();
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

/// \brief Class representing a vehicle.
/// \param dest The destination GraphVertex.
function Vehicle(dest){
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
	var visited = [];
	visited[start.id] = true;

	var first = [];
	first[start.id] = start;

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
	var levelMap = [];
	for(var it = 0; it < prevMap.length; it++){
		var v = prevMap[it];
		if(v == undefined)
			continue;
		for(var it2 = 0; it2 < v.edges.length; it2++){
			if(it2 in visited)
				continue;
			var v2 = v.edges[it2];
			if(v2 == undefined)
				continue;
			visited[it2] = true;
			levelMap[it2] = v;
			if(it2 == this.dest.id){
				if(v == v2.start)
					this.path.push(v2.end);
				else
					this.path.push(v2.start);
				this.path.push(v); // We know the path came via v.
				return true;
			}
		}
	}
	if(levelMap.length != 0){
		if(this.findPathInt(g, start, levelMap, visited)){
			var v = levelMap[this.path.back()];
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
	this.pos += this.velocity * dt;
	if(this.edge.length < this.pos){
		this.pos -= this.edge.length;
		if(1 < this.path.length){
			var lastVertex = this.path.pop();
//			delete edge.this;
			this.edge = lastVertex;
//			edge.add(this);
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

Vehicle.prototype.onUpdate = function(dt){
	// Default does nothing
}

Vehicle.prototype.onVehicleAdd = function(v){
	// Default does nothing
}

Vehicle.prototype.onVehicleDelete = function(v){
	// Default does nothing
}




function Graph(width, height){
	this.rng = new Xor128(); // Create Random Number Generator
	var rng = this.rng;
	var n = 100;
	this.vertices = new Array(n);
	this.vehicles = [];
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
//	static int invokes = 0;
//	static random_sequence rs;
//	if(invokes == 0)
//		init_rseq(&rs, 87657444);
	var genInterval = 0.1;
	var global_time = Graph.prototype.global_time;
	
	if((global_time + dt) % genInterval < global_time % genInterval){
		for(var i = 0; i < 10; i++){
			var starti = Math.floor(this.rng.next() * (this.vertices.length-1));
			var endi = Math.floor(this.rng.next() * (this.vertices.length-1));
			var v = new Vehicle(this.vertices[endi]);
			if(v.findPath(this, this.vertices[starti])){
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
			this.vehicles.splice(i, 1);
		}
		else
			i++;
	}

//	invokes++;
	Graph.prototype.global_time += dt;
}
