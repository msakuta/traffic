function GraphVertex(x,y){
	this.x = x;
	this.y = y;
//	document.write(this.x + " " + this.y + ";");
	this.edges = new Array();
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
	this.edges.push(e);
	other.edges.push(e);
	return true;
}

function GraphEdge(start, end){
	this.start = start;
	this.end = end;
	this.passCount = 0;
	this.length = start.measureDistance(end);
}

function Graph(width, height){
	var rng = new Xor128(); // Create Random Number Generator
	var n = 100;
	this.vertices = new Array(n);
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
