
var canvas;
var width;
var height;


var vertexRadius = 10.;
var graph;

function Graph(){
	this.rng = new Xor128(); // Create Random Number Generator
	var rng = this.rng;
	var n = 30;
	var minDist = vertexRadius * 6; // Minimum distance between vertices, initialized by 3 times the radius of vertices.
	this.vertices = new Array(n);
	this.vehicles = [];
	this.vehicleIdGen = 1;
	this.vehicleFreq = 0.05;

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

		// Randomize interval of traffic signals to make the simulation feel a bit more realistic.
		this.vertices[i].signalRemaining =
		this.vertices[i].signalInterval = rng.next() * 5 + 5;
	}
	for(var i = 0; i < n; i++){
		// If average number of edges a vertex has is 2, most vertices connect to far ones,
		// which is ideal for simulating traffic.
		// But we cannot set the parameter too high because  it would get cluttered and
		// harder to grasp what's going on.
		// The numbers of edges are assumed Poisson distributed.
		var cumdist = [0.0, 0.1, 0.8, 0.9, 1.0];
		var picker = rng.next();
		var numEdges;
		for(numEdges = 0; numEdges < cumdist.length; numEdges++){
			if(picker < cumdist[numEdges])
				break;
		}
		this.vertices[i].numEdges = numEdges;
	}
	for(var i = 0; i < n; i++){
		var candidates = [];
		var accum = 0;
		for(var k = 0; k < n; k++){
			if(i === k)
				continue;
			var factor = 1. / this.vertices[i].measureDistance(this.vertices[k]);
			factor *= factor;
			candidates.push([k, factor]);
			accum += factor;
		}
		// Try to find close connections to make a network.
		for(var tries = 0; 0 < candidates.length; tries++){
			if(this.vertices[i].numEdges <= this.vertices[i].countEdges())
				break;
			var r = rng.next() * (accum);
			for(var k = 0; k < candidates.length; k++){
				if(r < candidates[k][1])
					break;
				r -= candidates[k][1];
			}
			if(candidates.length <= k)
				continue;
			accum -= candidates[k][1];
			var e = candidates[k][0];
			candidates.splice(k,1);
			if(this.vertices[e].numEdges <= this.vertices[e].countEdges())
				continue;
			this.vertices[i].connect(this.vertices[e]);
		}
		this.vertices[i].setSignals();
	}

	var margin = 50;

	// Apply relaxation with Hooke's law
	for(var i = 0; i < 100; i++){
		for(var j = 0; j < n; j++){
			var pull = [0, 0];
			for(var k = 0; k < this.vertices.length; k++){
				if(j === k)
					continue;
				var dx = this.vertices[k].x - this.vertices[j].x;
				var dy = this.vertices[k].y - this.vertices[j].y;
				var r = Math.sqrt(dx * dx + dy * dy);
				// Connected vertices gather, non-connected vertices repel
				if(k in this.vertices[j].edges){
					pull[0] += dx * ((r - 100) / 100);
					pull[1] += dy * ((r - 100) / 100);
				}
				else if(r < 150){
					pull[0] -= dx / (0.1 + r / 150);
					pull[1] -= dy / (0.1 + r / 150);
				}
			}
			this.vertices[j].x += pull[0] * 0.01;
			this.vertices[j].y += pull[1] * 0.01;
			if(this.vertices[j].x < margin)
				this.vertices[j].x = margin;
			if(width - margin < this.vertices[j].x)
				this.vertices[j].x = width - margin;
			if(this.vertices[j].y < margin)
				this.vertices[j].y = margin;
			if(height - margin < this.vertices[j].y)
				this.vertices[j].y = height - margin;
		}
	}

	// Recalculate distance after relaxation
	for(var i = 0; i < this.vertices.length; i++){
		for(var j in this.vertices[i].edges)
			this.vertices[i].edges[j].recalcLength();
	}

};


Graph.prototype.update = function(dt){
	var global_time = Graph.prototype.global_time;

	// Number of vehicles generated in a frame distributes in Poisson distribution.
	var numVehicles = poissonRandom(this.rng, this.vehicleFreq);

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
		return 0 < edge.vehicles.length;
	}

	for(var n = 0; n < numVehicles; n++){
		for(var i = 0; i < 100; i++){
			var starti = Math.floor(this.rng.next() * (this.vertices.length-1));
			var endi = Math.floor(this.rng.next() * (this.vertices.length-1));
			var v = new Vehicle(this.vertices[endi]);

			if(v.findPath(this, this.vertices[starti]) && !isCrowded(v)){
				// Assign the id only if addition of the vehicle is succeeded.
				v.id = this.vehicleIdGen++;
				v.maxVelocity = this.rng.next() * 10 + 10;
				v.acceleration = this.rng.next() * 3 + 6;
				this.vertices[starti].addVehicle(v);
				this.vehicles.push(v);
				v.onVehicleAdd();
				break;
			}
//		else
//			delete v;
		}
	}

	for(var i = 0; i < this.vertices.length; i++){
		this.vertices[i].update(dt);
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

}

var magnification = 1.;
var mouseCenter = [0,0];
var lastMouseCenter = [0,0];
var mouseDragging = false;
var trans = [1,0,0,1,0,0];

var drawCountElement = null;


function GraphVertex(x,y){
	this.x = x;
	this.y = y;
//	document.write(this.x + " " + this.y + ";");
	this.edges = {}; // Edges are a map of destination id to GraphEdge object.
	this.signals = {};
	this.vehicles = [];
	this.id = GraphVertex.prototype.idGen++;
	this.passCount = 0;
	this.signalInterval = 5;
	this.signalRemaining = 5;
}

GraphVertex.prototype.idGen = 0;

GraphVertex.prototype.maxPassCount = 1; // Initialize with 0 for avoiding zero division

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
	if(other === this || other.id in this.edges || this.id in other.edges)
		return false; // Already added

	var length = this.measureDistance(other);
//	if(120.0 < length)
//		return false; // Avoid adding long edges
	var e = new GraphEdge(this, other);
	this.edges[other.id] = e;
//	e = new GraphEdge(other, this);
	other.edges[this.id] = e;

	return true;
}

GraphVertex.prototype.setSignals = function(){
	var edgeCount = 0;
	for(var i in this.edges)
		edgeCount++;
	if(edgeCount !== 2)
		return;
	for(var i in this.edges){
		this.signals[i] = true;
	}
}

/// \brief Returns count of edges this vertex has.
///
/// This should really be more efficient implementation.
GraphVertex.prototype.countEdges = function(){
	var ret = 0;
	for(var e in this.edges)
		ret++;
	return ret;
}

/// \param v Vehicle to add
GraphVertex.prototype.addVehicle = function(v){
	var path = v.path;
	if(1 < path.length){
		this.edges[path[path.length - 2].id].addVehicle(v);
		path.pop();
	}
	this.passVehicle(v);
}

/// \brief Called every frame
GraphVertex.prototype.update = function(dt){
	// Alternate signals
	for(var i in this.edges){
		var edge = this.edges[i];
		var other = graph.vertices[i];
		var otherEdge = graph.vertices[i].edges[this.id];
		var blocked = edge.vehicles.length !== 0 || otherEdge.vehicles.length !== 0;
		if(this.signals[i] !== undefined)
			this.signals[i] = blocked;
		if(other.signals[this.id] !== undefined)
			other.signals[this.id] = blocked;
	}
/*	if(this.signalRemaining < dt){
		for(var i in this.signals){
			this.signals[i] = !this.signals[i];
		}
		this.signalRemaining = this.signalInterval;
	}
	else
		this.signalRemaining -= dt;*/
}

/// \brief Count up vehicle passes for GraphVertex
/// \param v Ignored
GraphVertex.prototype.passVehicle = function(v){
	this.passCount++;
	if(GraphVertex.prototype.maxPassCount < this.passCount)
		GraphVertex.prototype.maxPassCount = this.passCount;
}


function GraphEdge(start, end){
	this.start = start;
	this.end = end;
	this.passCount = 0;
	this.recalcLength();
	this.vehicles = [];
}

GraphEdge.prototype.maxPassCount = 1; // Initialize with 0 for avoiding zero division

GraphEdge.prototype.recalcLength = function(){
	this.length = this.start.measureDistance(this.end);
}

/// \param v Vehicle to add
GraphEdge.prototype.addVehicle = function(v){
	v.edge = this;
	this.vehicles.push(v);
	this.passCount++;
	if(GraphEdge.prototype.maxPassCount < this.passCount)
		GraphEdge.prototype.maxPassCount = this.passCount;
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
	this.velocity = 0;
	this.maxVelocity = 10;
	this.acceleration = 3;
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
			if(parseInt(it2) === this.dest.id){
				this.path.push(this.dest);
/*				if(v === v2.start)
					this.path.push(v2.end);
				else
					this.path.push(v2.start);*/
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
	if(this.edge === null)
		return false;

	var direction = this.edge.end === this.path.back() ? 1 : -1;
	var destpos = this.edge.end === this.path.back() ? this.edge.length : 0;

	// Check collision with other vehicles
	do{
		var nextVelocity = Math.min(this.maxVelocity, Math.max(-this.maxVelocity, this.velocity + direction * this.acceleration * dt));

		// Check if traffic signal is red
		var headPos = this.pos + Vehicle.prototype.vehicleInterval * direction * 0.5;
		if(0 < -direction * (headPos + nextVelocity * dt - (destpos - direction * vertexRadius))){
/*			var lastVertex = this.path.back();
			if(2 < lastVertex.countEdges() && lastVertex.signals[this.edge.start.id]){
				this.velocity = 0;
				break;
			}*/
			if(1 < this.path.length){
				var destVertex = this.path[this.path.length-1];
				var nextVertex = this.path[this.path.length-2];
				if(destVertex.signals[nextVertex.id]){
					this.velocity = 0;
					break;
				}
			}
		}

		if(this.checkTraffic(this.edge, this.pos + direction * this.vehicleInterval)){
			this.velocity = 0;
			this.jammed = true;
			break;
		}

		// If this Vehicle is partially entering the next GraphEdge, check on that edge, too.
		if(this.edge.length < this.pos + this.vehicleInterval + nextVelocity * dt && 1 < this.path.length){
			var edges = this.path.back().edges;
			var next = this.path[this.path.length-2];
			var edge = edges[next.id];
//			assert(it != edges.end());
			if(edge !== undefined && this.checkTraffic(edge, this.pos - this.edge.length)){
				this.velocity = 0;
				this.jammed = true;
				break;
			}
		}
		this.velocity = nextVelocity;
		this.jammed = false;
	} while(0);

	if(this.jammed)
		;
	else if(Math.abs(destpos - this.pos) < Math.abs(this.velocity * dt)){
		var extraRun = Math.abs(this.velocity * dt) - Math.abs(destpos - this.pos);
		var lastVertex = this.path.back();
		// Count up passes for vertices
		lastVertex.passVehicle(this);

		if(1 < this.path.length){
			this.path.pop();
			this.edge.deleteVehicle(this);
			this.edge = lastVertex.edges[this.path.back().id];
			var newdirection = this.edge.end === this.path.back() ? 1 : -1;
			this.pos = newdirection < 0 ? this.edge.length - extraRun : extraRun;
			this.velocity = newdirection * Math.abs(this.velocity);

			this.edge.addVehicle(this);
		}
		else{
			return false;
		}
	}
	else
		this.pos += this.velocity * dt;

	this.onUpdate(dt);
	return true;
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
	var spos = v.edge.start.getPos();
	var epos = v.edge.end.getPos();
	var pos = new Array(2);

	var perp = new Array(2);
	calcPerp(null, perp, spos, epos);

	for(var i = 0; i < 2; i++)
		pos[i] = epos[i] * v.pos / v.edge.length + spos[i] * (v.edge.length - v.pos) / v.edge.length;

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
		// Now that GraphEdge objects are created for each direction, we do not need to check
		// the direction (it's always the right direction, in sacrifice of memory).
//		if(v.path.back() !== this.path.back())
//			continue;
		if(pos < v.pos && v.pos < pos + this.vehicleInterval){
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

/// Vector 2D addition
function vecadd(v1,v2){
	return [v1[0] + v2[0], v1[1] + v2[1]];
}

/// Vector 2D scale
function vecscale(v1,s2){
	return [v1[0] * s2, v1[1] * s2];
}

/// Vector 2D distance
function vecdist(v1,v2){
	var dx = v1[0] - v2[0], dy = v1[1] - v2[1];
	return Math.sqrt(dx * dx + dy * dy);
}

/// \brief Calculates product of matrices
///
/// Note that this function assumes arguments augmented matrices, see http://en.wikipedia.org/wiki/Augmented_matrix
/// The least significant suffix is rows.
/// To put it simply, array of coefficients as the same order as parameters to canvas.setTransform().
function matmp(a,b){
	var ret = new Array(6);
	for(var i = 0; i < 3; i++){
		for(var j = 0; j < 2; j++){
			var val = 0;
			for(var k = 0; k < 2; k++)
				val += a[k * 2 + j] * b[i * 2 + k];
			if(i === 2)
				val += a[2 * 2 + j];
			ret[i * 2 + j] = val;
		}
	}
	return ret;
}

window.onload = function() {
	canvas = document.getElementById("scratch");
	if ( ! canvas || ! canvas.getContext ) {
		return false;
	}
	width = parseInt(canvas.style.width);
	height = parseInt(canvas.style.height);
	graph = new Graph(width, height);

	var edit = document.getElementById("freqEdit");
	if(edit !== undefined)
		edit.value = graph.vehicleFreq;
	updateFreq();

	var zoomElement = document.getElementById("zoom");
	var transElement = document.getElementById("trans");
	var mouseElement = document.getElementById("mouse");
	drawCountElement = document.getElementById("drawcount");

	function magnify(f){
		// Prepare the transformation matrix for zooming
		trans = matmp([f, 0, 0, f, (1 - f) * mouseCenter[0], (1 - f) * mouseCenter[1]], trans);

		var result = magnification * f;
		if(result < 1){
			// When fully zoomed out, reset the matrix to identity.
			magnification = 1.;
			trans = [1, 0, 0, 1, 0, 0];
		}
		else
			magnification = result;
		zoomElement.innerHTML = magnification.toString();
		transElement.innerHTML = trans.toString();
	}

	// For Google Chrome
	function MouseWheelListenerFunc(e){
		magnify(0 < e.wheelDelta ? 1.2 : 1. / 1.2);

		// Cancel scrolling by the mouse wheel
		e.preventDefault();
	}

	// For FireFox
	function MouseScrollFunc(e){
		magnify(e.detail < 0 ? 1.2 : 1. / 1.2);

		// Cancel scrolling by the mouse wheel
		e.preventDefault();
	}

	if(canvas.addEventListener){
		canvas.addEventListener("mousewheel" , MouseWheelListenerFunc);
		canvas.addEventListener("DOMMouseScroll" , MouseScrollFunc);
	}

	// It's tricky to obtain client coordinates of a HTML element.
	function getOffsetRect(elem){
		var box = elem.getBoundingClientRect();
		var body = document.body;
		var docElem = document.documentElement;

		var clientTop = docElem.clientTop || body.clientTop || 0
		var clientLeft = docElem.clientLeft || body.clientLeft || 0

		var top  = box.top - clientTop
		var left = box.left - clientLeft

		return { top: Math.round(top), left: Math.round(left) }
	}

	canvas.onmousemove = function (e){

		// For older InternetExplorerS
		if (!e)	e = window.event;

		var r = getOffsetRect(canvas);

		mouseCenter[0] = e.clientX - r.left;
		mouseCenter[1] = e.clientY - r.top;

		if(mouseDragging){
			var nextx = trans[4] + mouseCenter[0] - lastMouseCenter[0];
			var nexty = trans[5] + mouseCenter[1] - lastMouseCenter[1];
			if(0 <= -nextx && -nextx < width * (trans[0] - 1))
				trans[4] += mouseCenter[0] - lastMouseCenter[0];
			if(0 <= -nexty && -nexty < height * (trans[3] - 1))
				trans[5] += mouseCenter[1] - lastMouseCenter[1];

			lastMouseCenter[0] = mouseCenter[0];
			lastMouseCenter[1] = mouseCenter[1];
		}
		e.preventDefault();
	};

	canvas.onmousedown = function(e){
		mouseDragging = true;
		mouseElement.innerHTML = "true";

		var r = getOffsetRect(canvas);

		lastMouseCenter[0] = e.clientX - r.left;
		lastMouseCenter[1] = e.clientY - r.top;
	};

	canvas.onmouseup = function(e){
		mouseDragging = false;
		mouseElement.innerHTML = "false";
	};

	var loop = function() {
		draw();
		var timer = setTimeout(loop,100);
	};

	loop();
};

function resetTrans(ctx){
	ctx.setTransform(1,0,0,1,200,200);
}

function updateFreq(){
	var text = document.getElementById("freq");
	text.innerHTML = graph.vehicleFreq;
}

function submitFreq(){
	var edit = document.getElementById("freqEdit");
	if(edit === undefined)
		return;
	var val = parseFloat(edit.value);
	if(isNaN(val))
		return;
	graph.vehicleFreq = val;
	updateFreq();
}

function draw() {
	// A local function to convert a color channel intensity into hexadecimal notation
	function numToHex(d){
		var hex = Math.floor(d * 255).toString(16);

		while(hex.length < 2)
			hex = "0" + hex;

		return hex;
	}

	// A local function to determine road color for showing traffic intensity.
	function roadColor(f){
		return "#" + numToHex((1. + f) / 2.) + "7f7f";
	}

	graph.update(0.1);

	var ctx = canvas.getContext('2d');
	ctx.setTransform(1,0,0,1,0,0);
	ctx.clearRect(0,0,width,height);

	function transform(){
		ctx.setTransform(trans[0], trans[1], trans[2], trans[3], trans[4], trans[5]);
	}

	function hitCheck(pos,radius){
		var x = trans[0] * pos[0] + trans[4];
		var y = trans[3] * pos[1] + trans[5];
		var tr = radius * trans[0]; // Transformed Radius
		return 0 <= x + tr && x - tr < width && 0 <= y + tr && y - tr < height;
	}

	var drawCounts = {}, totalCounts = {};
	for(var i = 0; i < 2; i++){
		var counts = [drawCounts, totalCounts][i];
		counts.edge = counts.vertex = counts.vehicle = counts.signal = 0;
	}

	ctx.font = "bold 16px Helvetica";
	ctx.textAlign = "center";
	ctx.textBaseline = "middle";

	// The first pass of GraphEdge traversal draws asphalt-colored, road-like graphics.
	ctx.strokeStyle = "#000";
	transform();
	for(var i = 0; i < graph.vertices.length; i++){
		var v = graph.vertices[i];
		var pos = v.getPos();
		for(var j in v.edges){
			var e = v.edges[j];
			if(e === undefined)
				continue;

			totalCounts.edge++;

			var dpos = e.end.getPos();

			if(!hitCheck(vecscale(vecadd(pos, dpos), 0.5), vecdist(pos, dpos) / 2. + vertexRadius))
				continue;

			drawCounts.edge++;

			// Color the road with traffic intensity
//			ctx.fillStyle = roadColor(e.passCount / e.maxPassCount);
			ctx.fillStyle = e.vehicles.length !== 0 ? "#ff7f7f" : "#7f7f7f";

			// Obtain vector perpendicular to the edge's direction.
			var para = new Array(2);
			var perp = new Array(2);
			var length = calcPerp(para, perp, pos, dpos);

			var size = vertexRadius;

			ctx.beginPath();
			ctx.moveTo(pos[0], pos[1]);
			ctx.lineTo(dpos[0], dpos[1]);
			ctx.lineTo(dpos[0] + perp[0] * size, dpos[1] + perp[1] * size);
			ctx.lineTo(pos[0] + perp[0] * size, pos[1] + perp[1] * size);
			ctx.stroke();
			ctx.fill();

			ctx.beginPath();
			ctx.moveTo(e.start.x, e.start.y);
			ctx.lineTo(e.end.x, e.end.y);
			ctx.stroke();
		}
	}

	// The second pass draws vertex circles and ids
	for(var i = 0; i < graph.vertices.length; i++){
		var v = graph.vertices[i];

		totalCounts.vertex++;

		if(!hitCheck(v.getPos(), vertexRadius * trans[0]))
			continue;

		drawCounts.vertex++;

		// Color the crossing with traffic intensity
		ctx.fillStyle = roadColor(v.passCount / v.maxPassCount);
		ctx.beginPath();
		ctx.arc(v.x, v.y, vertexRadius, 0, Math.PI*2, false);
		ctx.stroke();
		ctx.fill();

		ctx.fillStyle = "#000";
		ctx.fillText(v.id, v.x, v.y);
	}

	ctx.font = "bold 12px Helvetica";

	for(var i = 0; i < graph.vehicles.length; i++){
		var v = graph.vehicles[i];
		var spos = new Array(2);
		var epos = new Array(2);
		var pos = v.calcPos(spos, epos);

		totalCounts.vehicle++;

		if(!hitCheck(pos, 7 * trans[0]))
			continue;

		drawCounts.vehicle++;

		var angle = Math.atan2((spos[1] - epos[1]), spos[0] - epos[0]);

		ctx.strokeStyle = v.jammed ? "#f00" : "#000";
		ctx.fillStyle = "#" + numToHex(v.color[0]) + numToHex(v.color[1]) + numToHex(v.color[2]);

		// Reset the transformation to identity
		transform();

		// Construct transformation matrix
		ctx.translate(pos[0], pos[1]);
		ctx.rotate(angle + (v.velocity < 0 ? Math.PI : 0));

		// Actually draw the vehicle's graphic
		ctx.beginPath();
		ctx.moveTo(-6, -3);
		ctx.lineTo(-6,  3);
		ctx.lineTo( 6,  4);
		ctx.lineTo( 6, -4);
		ctx.closePath();
		ctx.fill();
		ctx.stroke();
	}

	// Reset the transformation for the next drawing
	transform();

	// The third pass for graphs show the traffic signals.
	// This is placed after vehicles rendering because the signals are more important
	// and should not be obscured by vehicles.
	ctx.strokeStyle = "#000";
	for(var i = 0; i < graph.vertices.length; i++){
		var v = graph.vertices[i];

//		if(v.countEdges() <= 2)
//			continue;

		for(var j in v.edges){
			var e = v.edges[j];
			if(e === undefined)
				continue;

			var tpos = v.getPos();
			var opos = graph.vertices[j].getPos();

			// Obtain vector perpendicular to the edge's direction.
			var para = new Array(2);
			var perp = new Array(2);
			var length = calcPerp(para, perp, tpos, opos);
			var x = v.x + (para[0] - perp[0] * 0.5) * vertexRadius;
			var y = v.y + (para[1] - perp[1] * 0.5) * vertexRadius;

			totalCounts.signal++;

			if(!hitCheck([x, y], 3))
				continue;

			drawCounts.signal++;

			if(v.signals[j] !== undefined){
				ctx.fillStyle = v.signals[j] === true ? "#f00" : "#0f0";
				ctx.beginPath();
				ctx.arc(x, y, 3, 0, Math.PI*2, false);
				ctx.stroke();
				ctx.fill();
			}
		}
	}

	drawCountElement.innerHTML = "Edges: " + drawCounts.edge + " / " + totalCounts.edge
		+ ", Vertices: " + drawCounts.vertex + " / " + totalCounts.vertex
		+ ", Vehicles: " + drawCounts.vehicle + " / " + totalCounts.vehicle
		+ ", Signals: " + drawCounts.signal + " / " + totalCounts.signal;
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
