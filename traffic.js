
var canvas;
var width;
var height;

var graph;

window.onload = function() {
	canvas = document.getElementById("scratch");
	if ( ! canvas || ! canvas.getContext ) {
		return false;
	}
	width = parseInt(canvas.style.width);
	height = parseInt(canvas.style.height);
	graph = new Graph(width, height);

	var loop = function() {
		draw();
		var timer = setTimeout(loop,100);
	};

	loop();
};

function resetTrans(ctx){
	ctx.setTransform(1,0,0,1,200,200);
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
	ctx.clearRect(0,0,width,height);

	ctx.font = "bold 16px Helvetica";
	ctx.textAlign = "center";
	ctx.textBaseline = "middle";

	// The first pass of GraphEdge traversal draws asphalt-colored, road-like graphics.
	ctx.strokeStyle = "#000";
	ctx.setTransform(1,0,0,1,0,0);
	for(var i = 0; i < graph.vertices.length; i++){
		var v = graph.vertices[i];
		var pos = v.getPos();
		for(var j in v.edges){
			var e = v.edges[j];
			if(e === undefined)
				continue;

			var dpos = e.start.getPos();

			// Color the road with traffic intensity
			ctx.fillStyle = roadColor(e.passCount / e.maxPassCount);

			// Obtain vector perpendicular to the edge's direction.
			var para = new Array(2);
			var perp = new Array(2);
			var length = calcPerp(para, perp, e.end.getPos(), dpos);

			var size = vertexRadius;

			ctx.beginPath();
			ctx.moveTo(pos[0] - perp[0] * size, pos[1]- perp[1] * size);
			ctx.lineTo(dpos[0] - perp[0] * size, dpos[1] - perp[1] * size);
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

		var angle = Math.atan2((spos[1] - epos[1]), spos[0] - epos[0]);

		ctx.strokeStyle = v.jammed ? "#f00" : "#000";
		ctx.fillStyle = "#" + numToHex(v.color[0]) + numToHex(v.color[1]) + numToHex(v.color[2]);

		// Reset the transformation to identity
		ctx.setTransform(1,0,0,1,0,0);

		// Construct transformation matrix
		ctx.translate(pos[0], pos[1]);
		ctx.rotate(angle);

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
	ctx.setTransform(1,0,0,1,0,0);

	// The third pass for graphs show the traffic signals.
	// This is placed after vehicles rendering because the signals are more important
	// and should not be obscured by vehicles.
	ctx.strokeStyle = "#000";
	for(var i = 0; i < graph.vertices.length; i++){
		var v = graph.vertices[i];

		if(v.countEdges() <= 2)
			continue;

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

			ctx.fillStyle = v.signals[j] === true ? "#f00" : "#0f0";
			ctx.beginPath();
			ctx.arc(v.x + (para[0] - perp[0] * 0.5) * vertexRadius,
				v.y + (para[1] - perp[1] * 0.5) * vertexRadius, 3, 0, Math.PI*2, false);
			ctx.stroke();
			ctx.fill();
		}
	}
}
