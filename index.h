const char MAIN_page[] PROGMEM = R"=====(
	<!DOCTYPE html>
	<html>

	<head>
	<title id="AntName">IP rotator</title>
	<meta http-equiv="refresh" content="1800">
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
	<style type="text/css">
		html, body {
			font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana;
			background-color: #000;
		}
		body {
			width: 600px;
			margin: 0 auto;
		}
		.mouse{
			margin: auto;
			position: absolute;
			left: 0;
			top: 0;
			z-index: 2;
		}
		.form1{
			margin: auto;
			position: absolute;
			left: 0;
			top: 600px;
			z-index: 2;
		}
		.form2{
			margin: auto;
			position: absolute;
			left: 540px;
			top: 600px;
			z-index: 2;
		}
		.top{
			margin: auto;
			border: 0px solid #222222;
			position: absolute;
			left: 0;
			top: 0;
			z-index: 1;
		}
		.middle{
			position: absolute;
			left: 0;
			top: 0;
			z-index: 0;
		}
		.bot{
			position: absolute;
			left: 0;
			top: 0;
			z-index: -1;
		}
		.underbot{
			position: absolute;
			left: 0;
			top: 0;
			z-index: -2;
		}
		.second{
			border: 0px solid #222222;
			position: absolute;
			left: 0;
			top: 600px;
			width: 600px;
			z-index: 0;
		}
		a:hover {color: #fff;}
    a { color: #ccc; text-decoration: underline;}
	</style>
	<link href='http://fonts.googleapis.com/css?family=Roboto+Condensed:300italic,400italic,700italic,400,700,300&subset=latin-ext' rel='stylesheet' type='text/css'>
	</head>
	<body>

	<div style="position: relative;">
		<canvas class="mouse" id="Mouse" height="600" width="600"></canvas>
		<canvas class="top" id="Azimuth" width="600" height="600">Your browser does not support the HTML5 canvas tag.</canvas>
		<canvas class="middle" id="Static" width="600" height="600"></canvas>
		<canvas class="bot" id="Map" width="600" height="600"></canvas>
		<canvas class="underbot" id="DirLine" width="600" height="600"></canvas>
		<div class="second">
			<p style="font-size: 25px; color: #ccc; margin: 20 0 0 0; text-align: center;">
				<span style="color: #000; background: #666; padding: 4px 6px 4px 6px; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;">
					<span style="color: #fff;" id="AntName2"> </span> | POE
					<span style="color: #fff; font-weight: bold;" id="ADCValue">0</span> V |
					 raw <span style="font-weight: bold;" id="AZValue">0</span>&deg; |
					<a href="/set" onclick="window.open( this.href, this.href, 'width=700,height=1350,left=0,top=0,menubar=no,location=no,status=no' ); return false;"); return false;\">SETUP</a>

				</span>
				<br>
				<span style="color: #666; font-size: 73%" id="mac"> </span><span id="MapModeInfo" style="color: #666; font-size: 73%"></span><span id="OnlineStatus" style="color: #666; font-size: 73%"></span>
			</p>
		</div>
	</div>

	<script>
	var BoxSize = 600;
	var Target  = 0;
	var AzShift = 0;
	var AzRange = 0;
	var Azimuth = 0;
	var AzimuthTmp = 0;
	var AntRadiationAngle = 0;
	var Status = 4;
	var StatusTmp = 0;
	var MapSource = -1;
	var MapUrl = 0;
	var MapLocator = "JO60UC";
	var MapZoomKm = 5000;
	var OnlineTimeStamp = 0;
	var Elevation = 0; 
	
	var Xcenter = BoxSize/2;
	var Ycenter = BoxSize/2;


	// setInterval(function.reload(){ map();}, 600000); //mSeconds update rate
	setInterval(function() { map();}, 600000); //mSeconds update rate
	setInterval(function() { getData();}, 500); //mSeconds update rate
	setInterval(function() { CheckOnline();}, 2000); //mSeconds update rate
	getSet();
//	setTimeout(() => { map(); }, 1000);
	Static();
	StaticBot();
	var AZtarget;

	function CheckOnline() {
		if( new Date().getTime() - Number(OnlineTimeStamp) > 1500){
			document.getElementById("OnlineStatus").innerHTML = " | <span style='color: red;'>&#8226;</span> Offline";
		}else{
			document.getElementById("OnlineStatus").innerHTML = " | <span style='color: white;'>&#8226;</span> Connected";
		}
	}

	function updateMapModeInfo(){
		var info = document.getElementById("MapModeInfo");
		if(!info){ return; }
		if(Number(MapSource)===-1){
			info.innerHTML = "";
			return;
		}
		if(Number(MapSource)===1){
			info.innerHTML = " | map: locator " + String(MapLocator).toUpperCase() + " / " + String(MapZoomKm) + " km";
		}else{
			info.innerHTML = " | map: url";
		}
	}

	function getSet() {
	  var ihttp = new XMLHttpRequest();
	  ihttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				AzShift = this.responseText;
				// console.log ('AzShift ' + AzShift);
	    }
	  };
	  ihttp.open("GET", "readStart", true);
	  ihttp.send();

	  var jhttp = new XMLHttpRequest();
	  jhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				AzRange = this.responseText;
				// console.log ('AzRange ' + AzRange);
				AZ(Azimuth);
				Static();
				StaticBot();
	    }
	  };
	  jhttp.open("GET", "readMax", true);
	  jhttp.send();

	  var khttp = new XMLHttpRequest();
	  khttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				AntRadiationAngle = this.responseText;
				// console.log ('AzRange ' + AzRange);
	    }
	  };
	  khttp.open("GET", "readAnt", true);
	  khttp.send();

	  var lhttp = new XMLHttpRequest();
	  lhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      document.getElementById("AntName").innerHTML = this.responseText;
	      document.getElementById("AntName2").innerHTML = this.responseText;
				// AntName = this.responseText;
				// console.log ('AzRange ' + AzRange);
	    }
	  };
	  lhttp.open("GET", "readAntName", true);
	  lhttp.send();

	  var mhttp = new XMLHttpRequest();
	  mhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("AntName").innerHTML = this.responseText;
					MapUrl = this.responseText;
					// console.log ('MapUrl ' + MapUrl);
					updateMapModeInfo();
					map();
	    }
	  };
	  mhttp.open("GET", "readMapUrl", true);
	  mhttp.send();

	  var phttp = new XMLHttpRequest();
	  phttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				MapSource = this.responseText;
				updateMapModeInfo();
				map();
	    }
	  };
	  phttp.open("GET", "readMapSource", true);
	  phttp.send();

	  var qhttp = new XMLHttpRequest();
	  qhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				MapLocator = this.responseText;
				updateMapModeInfo();
				if (Number(MapSource) === 1) { map(); }
	    }
	  };
	  qhttp.open("GET", "readMapLocator", true);
	  qhttp.send();

	  var rhttp = new XMLHttpRequest();
	  rhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				MapZoomKm = this.responseText;
				updateMapModeInfo();
				if (Number(MapSource) === 1) { map(); }
	    }
	  };
	  rhttp.open("GET", "readMapZoomKm", true);
	  rhttp.send();

	  var nhttp = new XMLHttpRequest();
	  nhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      document.getElementById("mac").innerHTML = this.responseText;
				// MapUrl = this.responseText;
				// console.log ('MapUrl ' + MapUrl);
	    }
	  };
	  nhttp.open("GET", "readMAC", true);
	  nhttp.send();

	  var ohttp = new XMLHttpRequest();
	  ohttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				Elevation = this.responseText;
				// console.log ('Elevation ' + Elevation);
	    }
	  };
	  ohttp.open("GET", "readElevation", true);
	  ohttp.send();
	}

	function getData() {
	  var xhttp = new XMLHttpRequest();
	  xhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				if( Number(this.responseText)<11.5){

					document.getElementById("ADCValue").innerHTML = "<span style='color:red;'>"+ Math.round(this.responseText * 10) / 10 +"</span>";
				}else{
					document.getElementById("ADCValue").innerHTML = Math.round(this.responseText * 10) / 10;
				}
	    }
	  };
	  xhttp.open("GET", "readADC", true);
	  xhttp.send();

	  var yhttp = new XMLHttpRequest();
	  yhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      document.getElementById("AZValue").innerHTML = this.responseText;
				Azimuth = this.responseText;
				// if( Math.abs(Number(AzimuthTmp)-Number(Azimuth))>1 ){	// || Status != 4
				if( Number(AzimuthTmp) != Number(Azimuth) ){
					AZ(Azimuth);
					Static();
					StaticBot();
					AzimuthTmp=Azimuth;
				}
				OnlineTimeStamp = new Date().getTime();
				// console.log ('OnlineTimer ' + OnlineTimer);
	    }
	  };
	  yhttp.open("GET", "readAZ", true);
	  yhttp.send();

	  var zhttp = new XMLHttpRequest();
	  zhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StatValue").innerHTML = this.responseText;
				Status = this.responseText;
				if( Number(StatusTmp)!=Number(Status) ){
					AZ(Azimuth);
					Static();
					StaticBot();
					StatusTmp=Status;
				}
	    }
	  };
	  zhttp.open("GET", "readStat", true);
	  zhttp.send();
	}

	//---------------------------------------------------------------

	var mouse = document.getElementById("Mouse");
	var ctx = mouse.getContext("2d");

	//report the mouse position on click
	mouse.addEventListener("click", function (evt) {
	    var mousePos = getMousePos(mouse, evt);
	    // alert(mousePos.x + ',' + mousePos.y);
			AZtarget = Math.atan2(BoxSize/2 - Number(mousePos.y), Number(mousePos.x) - BoxSize/2) * 180 / Math.PI;
			AZtarget = AZtarget - 90;
			if(AZtarget<0){
				AZtarget = Math.abs(AZtarget);
			}else{
				AZtarget = 360 - AZtarget;
			}
			AZtarget = Math.round(AZtarget);
			// alert( AZtarget + '°');

			var http = new XMLHttpRequest();
			var url = '/';
			var params = "ROT="+AZtarget;
			http.open('POST', url, true);
			http.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
			http.onreadystatechange = function() {//Call a function when the state changes.
			    if(http.readyState == 4 && http.status == 200) {
			        alert(http.responseText);
			    }
			}
			http.send(params);

	}, false);

	//Get Mouse Position
	function getMousePos(mouse, evt) {
	    var rect = mouse.getBoundingClientRect();
			// return Target;
	    return {
	        x: evt.clientX - rect.left,
	        y: evt.clientY - rect.top
	    };
	}

	var EARTH_RADIUS_KM = 6371;
	var LocatorLandFillMode = true; // true = fill-like light land, false = outline only

var LAND_OUTLINES = [];
	var COUNTRY_BORDERS = [];
	var MapDatasetReady = false;
	var MapDatasetLoading = false;

	function ensureMapDataset(onReady){
		if(MapDatasetReady){
			onReady();
			return;
		}
		if(MapDatasetLoading){
			return;
		}
		MapDatasetLoading = true;
		var script = document.createElement('script');
		script.src = '/map50.js';
		script.onload = function(){
			MapDatasetLoading = false;
			MapDatasetReady = true;
			onReady();
		};
		script.onerror = function(){
			MapDatasetLoading = false;
		};
		document.head.appendChild(script);
	}


	function degToRad(deg){
		return deg * Math.PI / 180;
	}

	function normalizeLonDiffRad(lonDiff){
		while(lonDiff > Math.PI){ lonDiff -= 2 * Math.PI; }
		while(lonDiff < -Math.PI){ lonDiff += 2 * Math.PI; }
		return lonDiff;
	}

	function maidenheadToLatLon(locator){
		if(!locator){ return null; }
		var loc = String(locator).trim().toUpperCase();
		if(!/^[A-R]{2}[0-9]{2}[A-X]{2}$/.test(loc)){ return null; }
		var lon = (loc.charCodeAt(0) - 65) * 20 - 180;
		lon += (loc.charCodeAt(2) - 48) * 2;
		lon += (loc.charCodeAt(4) - 65) * (5/60);
		lon += 2.5/60;
		var lat = (loc.charCodeAt(1) - 65) * 10 - 90;
		lat += (loc.charCodeAt(3) - 48) * 1;
		lat += (loc.charCodeAt(5) - 65) * (2.5/60);
		lat += 1.25/60;
		return {lat: lat, lon: lon};
	}

	function projectAzimuthal(latDeg, lonDeg, centerLatDeg, centerLonDeg, mapRadiusPx, zoomKm){
		var phi1 = degToRad(centerLatDeg);
		var lam1 = degToRad(centerLonDeg);
		var phi = degToRad(latDeg);
		var lam = degToRad(lonDeg);
		var dLam = normalizeLonDiffRad(lam - lam1);

		var cosc = Math.sin(phi1) * Math.sin(phi) + Math.cos(phi1) * Math.cos(phi) * Math.cos(dLam);
		if(cosc > 1){ cosc = 1; }
		if(cosc < -1){ cosc = -1; }
		var c = Math.acos(cosc);
		var distKm = EARTH_RADIUS_KM * c;
		if(distKm > zoomKm){ return null; }

		var theta = 0;
		if(c > 1e-9){
			theta = Math.atan2(
				Math.sin(dLam) * Math.cos(phi),
				Math.cos(phi1) * Math.sin(phi) - Math.sin(phi1) * Math.cos(phi) * Math.cos(dLam)
			);
		}

		var r = (distKm / zoomKm) * mapRadiusPx;
		return {
			x: Xcenter + Math.sin(theta) * r,
			y: Ycenter - Math.cos(theta) * r
		};
	}

	function drawGeoLineCollection(ctx, lines, centerLat, centerLon, mapRadiusPx, zoomKm, strokeStyle, lineWidth){
		ctx.strokeStyle = strokeStyle;
		ctx.lineWidth = lineWidth;
		for(var i=0; i<lines.length; i++){
			var line = lines[i];
			var opened = false;
			var prev = null;
			for(var j=0; j<line.length; j++){
				var p = projectAzimuthal(line[j][0] / 100, line[j][1] / 100, centerLat, centerLon, mapRadiusPx, zoomKm);
				if(!p){
					if(opened){
						ctx.stroke();
						opened = false;
					}
					prev = null;
					continue;
				}
				if(!opened){
					ctx.beginPath();
					ctx.moveTo(p.x, p.y);
					opened = true;
				}else{
					// avoid connecting through clipped edge with unrealistically long segment
					if(prev && Math.hypot(p.x - prev.x, p.y - prev.y) > mapRadiusPx * 0.8){
						ctx.stroke();
						ctx.beginPath();
						ctx.moveTo(p.x, p.y);
					}else{
						ctx.lineTo(p.x, p.y);
					}
				}
				prev = p;
			}
			if(opened){
				ctx.stroke();
			}
		}
	}

	var locatorSvgImg = null;
	var locatorSvgLastKey = "";
	var locatorSvgLoadingKey = "";

	function projectGeoSegments(lines, centerLat, centerLon, mapRadiusPx, zoomKm){
		var allSegments = [];
		for(var i=0; i<lines.length; i++){
			var line = lines[i];
			var segments = [];
			var segment = [];
			var prev = null;
			for(var j=0; j<line.length; j++){
				var p = projectAzimuthal(line[j][0] / 100, line[j][1] / 100, centerLat, centerLon, mapRadiusPx, zoomKm);
				if(!p){
					if(segment.length > 1){
						segments.push(segment);
					}
					segment = [];
					prev = null;
					continue;
				}
				if(prev && Math.hypot(p.x - prev.x, p.y - prev.y) > mapRadiusPx * 0.8){
					if(segment.length > 1){
						segments.push(segment);
					}
					segment = [];
				}
				segment.push(p);
				prev = p;
			}
			if(segment.length > 1){
				segments.push(segment);
			}
			if(segments.length > 0){
				allSegments.push(segments);
			}
		}
		return allSegments;
	}

	function svgPathFromSegments(allSegments){
		var d = "";
		for(var i=0; i<allSegments.length; i++){
			var segments = allSegments[i];
			for(var j=0; j<segments.length; j++){
				var segment = segments[j];
				for(var k=0; k<segment.length; k++){
					var p = segment[k];
					var x = Math.round(p.x * 10) / 10;
					var y = Math.round(p.y * 10) / 10;
					if(k===0){
						d += "M" + x + " " + y;
					}else{
						d += " L" + x + " " + y;
					}
				}
			}
		}
		return d;
	}

	function isSegmentSafeClosed(segment, mapRadiusPx){
		if(!segment || segment.length < 4){
			return false;
		}
		var first = segment[0];
		var last = segment[segment.length - 1];
		var closureThreshold = Math.max(8, mapRadiusPx * 0.06);
		return Math.hypot(last.x - first.x, last.y - first.y) <= closureThreshold;
	}

	function pointToMapEdge(p, mapRadiusPx){
		var dx = p.x - Xcenter;
		var dy = p.y - Ycenter;
		var d = Math.hypot(dx, dy);
		if(d < 1e-6){
			return {x: Xcenter + mapRadiusPx, y: Ycenter};
		}
		var scale = mapRadiusPx / d;
		return {x: Xcenter + dx * scale, y: Ycenter + dy * scale};
	}

	function angleFromCenter(p){
		return Math.atan2(p.y - Ycenter, p.x - Xcenter);
	}

	function normalizeAngleDelta(a){
		while(a > Math.PI){ a -= 2 * Math.PI; }
		while(a < -Math.PI){ a += 2 * Math.PI; }
		return a;
	}

	function svgClosedPathFromSegments(allSegments, mapRadiusPx){
		var d = "";
		for(var i=0; i<allSegments.length; i++){
			var segments = allSegments[i];
			for(var j=0; j<segments.length; j++){
				var segment = segments[j];
				if(!isSegmentSafeClosed(segment, mapRadiusPx)){
					continue;
				}
				for(var k=0; k<segment.length; k++){
					var p = segment[k];
					var x = Math.round(p.x * 10) / 10;
					var y = Math.round(p.y * 10) / 10;
					if(k===0){
						d += "M" + x + " " + y;
					}else{
						d += " L" + x + " " + y;
					}
				}
				d += " Z";
			}
		}
		return d;
	}

	function buildLocatorSvg(centerLat, centerLon, mapRadiusPx, zoomKm){
		var landSegments = projectGeoSegments(LAND_OUTLINES, centerLat, centerLon, mapRadiusPx, zoomKm);
		var landPath = svgPathFromSegments(landSegments);
		var landFillPath = svgClosedPathFromSegments(landSegments, mapRadiusPx);
		var circleR = Math.round(mapRadiusPx * 10) / 10;

		var svg = "";
		svg += "<svg xmlns='http://www.w3.org/2000/svg' width='" + BoxSize + "' height='" + BoxSize + "' viewBox='0 0 " + BoxSize + " " + BoxSize + "'>";
		svg += "<rect width='100%' height='100%' fill='#000'/>";
		svg += "<defs><clipPath id='mapclip'><circle cx='" + Xcenter + "' cy='" + Ycenter + "' r='" + circleR + "'/></clipPath></defs>";
		svg += "<g clip-path='url(#mapclip)'>";
		svg += "<rect width='100%' height='100%' fill='#0b1320'/>";
		for(var ring = 1; ring <= 4; ring++){
			var rr = Math.round((circleR * ring / 4) * 10) / 10;
			svg += "<circle cx='" + Xcenter + "' cy='" + Ycenter + "' r='" + rr + "' fill='none' stroke='rgba(160,180,200,0.18)' stroke-width='1'/>";
		}
		if(LocatorLandFillMode && landFillPath.length>0){
			svg += "<path d='" + landFillPath + "' fill='rgba(208,232,200,0.78)' stroke='none' fill-rule='evenodd'/>";
		}else if(landPath.length>0){
			svg += "<path d='" + landPath + "' fill='none' stroke='rgba(238,249,231,0.95)' stroke-width='1.2' stroke-linecap='round' stroke-linejoin='round'/>";
		}
		svg += "</g></svg>";
		return svg;
	}

	function drawLocatorMapCanvas(centerLat, centerLon, mapRadiusPx, zoomKm){
		var c = document.getElementById('Map');
		if (!c.getContext) { return; }
		var ctx = c.getContext('2d');

		ctx.clearRect(0, 0, BoxSize, BoxSize);
		ctx.fillStyle = "#000";
		ctx.fillRect(0, 0, BoxSize, BoxSize);

		ctx.save();
		ctx.beginPath();
		ctx.arc(Xcenter, Ycenter, mapRadiusPx, 0, 2 * Math.PI);
		ctx.clip();

		ctx.fillStyle = "#0b1320";
		ctx.fillRect(0, 0, BoxSize, BoxSize);

		// subtle radial range rings for better readability in locator mode
		ctx.strokeStyle = "rgba(160, 180, 200, 0.18)";
		ctx.lineWidth = 1;
		for(var ring = 1; ring <= 4; ring++){
			ctx.beginPath();
			ctx.arc(Xcenter, Ycenter, mapRadiusPx * ring / 4, 0, 2 * Math.PI);
			ctx.stroke();
		}

		if(LocatorLandFillMode){
			var landSegments = projectGeoSegments(LAND_OUTLINES, centerLat, centerLon, mapRadiusPx, zoomKm);
			ctx.fillStyle = "rgba(208,232,200,0.78)";
			for(var i=0; i<landSegments.length; i++){
				var segments = landSegments[i];
				for(var j=0; j<segments.length; j++){
					var seg = segments[j];
					if(!seg || seg.length < 3){
						continue;
					}
					var closed = isSegmentSafeClosed(seg, mapRadiusPx);
					if(closed){
						ctx.beginPath();
						ctx.moveTo(seg[0].x, seg[0].y);
						for(var k=1; k<seg.length; k++){
							ctx.lineTo(seg[k].x, seg[k].y);
						}
						ctx.closePath();
						ctx.fill("evenodd");
						continue;
					}

					// Open segment clipped by the map edge: close it along the circle arc.
					var first = seg[0];
					var last = seg[seg.length - 1];
					var dFirst = Math.hypot(first.x - Xcenter, first.y - Ycenter);
					var dLast = Math.hypot(last.x - Xcenter, last.y - Ycenter);
					var edgeThreshold = mapRadiusPx * 0.82;
					if(dFirst < edgeThreshold || dLast < edgeThreshold){
						continue;
					}

					var firstEdge = pointToMapEdge(first, mapRadiusPx);
					var lastEdge = pointToMapEdge(last, mapRadiusPx);
					var aStart = angleFromCenter(firstEdge);
					var aEnd = angleFromCenter(lastEdge);
					var delta = normalizeAngleDelta(aStart - aEnd);

					ctx.beginPath();
					ctx.moveTo(first.x, first.y);
					for(var m=1; m<seg.length; m++){
						ctx.lineTo(seg[m].x, seg[m].y);
					}
					ctx.lineTo(lastEdge.x, lastEdge.y);
					ctx.arc(Xcenter, Ycenter, mapRadiusPx, aEnd, aStart, delta < 0);
					ctx.closePath();
					ctx.fill("evenodd");
				}
			}
		}else{
			drawGeoLineCollection(ctx, LAND_OUTLINES, centerLat, centerLon, mapRadiusPx, zoomKm, "rgba(238, 249, 231, 0.95)", 1.2);
		}
		ctx.restore();
	}

	function drawLocatorMap(){
		var mapRadiusPx = BoxSize / 2 * 0.9;
		var center = maidenheadToLatLon(MapLocator);
		if(!center){
			center = maidenheadToLatLon("JO60UC");
		}
		var zoomKm = Number(MapZoomKm);
		if(!(zoomKm >= 1000 && zoomKm <= 20000)){
			zoomKm = 5000;
		}

		var key = String(MapLocator).toUpperCase() + "|" + String(zoomKm);
		var c = document.getElementById('Map');
		if (!c.getContext) { return; }
		var ctx = c.getContext('2d');

		if(LocatorLandFillMode){
			drawLocatorMapCanvas(center.lat, center.lon, mapRadiusPx, zoomKm);
			return;
		}

		if(locatorSvgImg && locatorSvgLastKey === key && locatorSvgImg.complete){
			ctx.clearRect(0, 0, BoxSize, BoxSize);
			ctx.drawImage(locatorSvgImg, 0, 0, BoxSize, BoxSize);
			return;
		}
		if(locatorSvgLoadingKey === key){
			return;
		}

		var svg = buildLocatorSvg(center.lat, center.lon, mapRadiusPx, zoomKm);
		var img = new Image();
		locatorSvgLastKey = key;
		locatorSvgLoadingKey = key;
		img.onload = function(){
			locatorSvgImg = img;
			locatorSvgLoadingKey = "";
			ctx.clearRect(0, 0, BoxSize, BoxSize);
			ctx.drawImage(img, 0, 0, BoxSize, BoxSize);
		};
		img.onerror = function(){
			locatorSvgLoadingKey = "";
			// fallback for environments with stricter data-uri SVG policy
			drawLocatorMapCanvas(center.lat, center.lon, mapRadiusPx, zoomKm);
		};
		img.src = "data:image/svg+xml;charset=utf-8," + encodeURIComponent(svg);
	}


	function map(){
		if (Number(MapSource) === -1) {
			return;
		}
		if (Number(MapSource) === 1) {
			var m = document.getElementById('Map');
			if (m && m.getContext) {
				var mctx = m.getContext('2d');
				mctx.clearRect(0, 0, BoxSize, BoxSize);
				mctx.fillStyle = "#000";
				mctx.fillRect(0, 0, BoxSize, BoxSize);
			}
			ensureMapDataset(drawLocatorMap);
			return;
		}

		// If MapUrl is not ready yet, retry later
		if (!MapUrl || MapUrl === "0") {
			setTimeout(map, 1000);   // retry in 1 second
			return;
		}

		var c = document.getElementById('Map');
		if (c.getContext) {
			var ctx = c.getContext('2d');
			var img1 = new Image();

				// Draw the map once the image is fully loaded
				img1.onload = function () {
				if (Number(MapSource) !== 0) {
					return;
				}
				ctx.drawImage(img1, 0, 0, BoxSize, BoxSize);
				};

			// If loading fails (slow internet, temporary outage), retry later
			img1.onerror = function () {
			setTimeout(map, 2000); // retry in 2 seconds
			};

			// Cache-busting to avoid loading a broken or outdated cached image
			img1.src =
			String(MapUrl) +
			(String(MapUrl).includes("?") ? "&" : "?") +
			"t=" + Date.now();
		}
	}

	function mapOLD(){
		var ctx = document.getElementById('Map');
		if (ctx.getContext) {
				ctx = ctx.getContext('2d');
				var img1 = new Image();
				img1.onload = function () {
						ctx.drawImage(img1, 0, 0, BoxSize, BoxSize);
				};
				img1.src = String(MapUrl);
		}
	}

	function Xcoordinate(dir, r){
	  return Xcenter + Math.sin(dir * Math.PI / 180) * r;
	}
	function Ycoordinate(dir, r){
	  return Ycenter - Math.cos(dir * Math.PI / 180) * r;
	}

	function AZ(Azimuth){
	  var c = document.getElementById("Azimuth");
	  var az = c.getContext("2d");
		az.clearRect(0, 0, BoxSize, BoxSize);
	  az.beginPath();
		var AZtargetTmp = Number(AZtarget) + Number(AzShift);
		var AzimuthMapTmp = Azimuth;
		if(Number(AZtargetTmp) > 360){
			AZtargetTmp=Number(AZtargetTmp)-360;
		}
		if(Number(AzimuthMapTmp) > 360){
			AzimuthMapTmp=Number(AzimuthMapTmp)-360;
		}
			if( Math.abs(Number(AZtargetTmp) - Number(AzimuthMapTmp)) > Number(AntRadiationAngle)/2 ){	// || Status != 4
				az.moveTo(BoxSize/2, BoxSize/2);
			  az.lineTo(Xcoordinate(Number(AZtarget), BoxSize/2*0.9), Ycoordinate(Number(AZtarget), BoxSize/2*0.9));
			}
			// console.log ('Number(Azimuth) ' + Number(Azimuth) );

		  az.lineWidth = 5;
			if (Number(Azimuth) < 0 || Number(Azimuth) > Number(AzRange) ) {
				az.strokeStyle = '#c0c0c0';
			}else{
				if (Status != 4) {
					az.strokeStyle = "red";
				}else if (Azimuth > 359) {
					az.strokeStyle = "orange";
				}else{
					az.strokeStyle = '#00aa00';
				}
			}
		  az.moveTo(Xcoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.9), Ycoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.9));
		  az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.7), Ycoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.7));
		az.stroke();

		// cw/ccw arrow
		var Ofset = 0;
		az.beginPath();
			if (Status != 4) {
				az.fillStyle = "red";
				if(Status<4){
					Ofset = -6;
				}else{
					Ofset = 6;
				}
				az.moveTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)/2, BoxSize/2*0.85), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)/2, BoxSize/2*0.85));
			  az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)/2, BoxSize/2*0.75), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)/2, BoxSize/2*0.75));
				az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*1.3, BoxSize/2*0.8), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*1.3, BoxSize/2*0.8));

				az.moveTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*1.5, BoxSize/2*0.83), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*1.5, BoxSize/2*0.83));
			  az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*1.5, BoxSize/2*0.77), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*1.5, BoxSize/2*0.77));
				az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*2, BoxSize/2*0.8), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(Ofset)*2, BoxSize/2*0.8));
			}
		az.fill();

			// az.registerFont(path.join(__dirname, "fonts", "RobotoCondensed-Regular.ttf"), { family: "Roboto Condensed" });
			// az.font = "100px 'Roboto Condensed'";
			az.font = "bold 100px Arial";
			az.textAlign = 'center';
			az.textBaseline = 'middle';
			if(Elevation==0){
				var ShowAzimuth = Number(Azimuth) + Number(AzShift);
			}else{
				var ShowAzimuth = Number(Azimuth);
			}
			if(ShowAzimuth > 359){
				ShowAzimuth = Number(ShowAzimuth) - 360;
			}
			if (Number(Azimuth) < 0 || Number(Azimuth) > Number(AzRange) ) {
				az.font = "bold 30px Arial";
				az.fillStyle = '#c0c0c0';
				if (Azimuth < 0) {
					az.fillText("CCW endstop zone", Xcoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2), Ycoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2));
				}else{
					az.fillText("CW endstop zone", Xcoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2), Ycoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2));
				}
			}else{
				az.fillStyle = "black";
				az.fillText(ShowAzimuth+String.fromCharCode(176), Xcoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2)+3, Ycoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2)+3 );
				if (Status != 4) {
					az.fillStyle = "red";
				}else if (Azimuth > 359) {
					az.fillStyle = "orange";
				}else{
					az.fillStyle = "green";
				}
				az.fillText(ShowAzimuth+String.fromCharCode(176), Xcoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2), Ycoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2));
			}
		az.beginPath();
			az.moveTo(BoxSize/2, BoxSize/2);
			var CcwAngle = Number(AntRadiationAngle)/2;
			for(var i=0;i<17;i++){
				az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)-Number(CcwAngle)+Number(AntRadiationAngle)/16*i, Number(BoxSize)/2*0.9), Ycoordinate(Number(Azimuth) + Number(AzShift)-Number(CcwAngle)+Number(AntRadiationAngle)/16*i, Number(BoxSize)/2*0.9));
			}
			if (Number(Azimuth) < 0 || Number(Azimuth) > Number(AzRange) ) {
				az.fillStyle = "rgba(255, 255, 255, 0.10)";
			}else{
				if (Status != 4) {
					az.fillStyle = "rgba(255, 0, 0, 0.25)";
				}else if (Azimuth > 359) {
				 az.fillStyle = "rgba(255, 165, 0, 0.25)";
				}else{
					az.fillStyle = "rgba(255, 255, 255, 0.25)";
				}
			}
		az.fill();
	}

	function Static(){
	  var con = document.getElementById("Static");

	  // overlap
	  var overlap = con.getContext("2d");
		overlap.clearRect(0, 0, BoxSize, BoxSize);
		overlap.lineWidth = 8;
		overlap.strokeStyle = 'orange';
		var START = 270+Number(AzShift);
		var STOP  = 270+Number(AzShift)+Number(AzRange)-360;
		if(Number(AzRange)>360){
		  overlap.beginPath();
		  overlap.arc(Xcenter, Ycenter, BoxSize/2*0.9-8, Number(START) * Math.PI/180, Number(STOP) * Math.PI/180 );
		  overlap.stroke();
		}

	  // direction
	  var direction = con.getContext("2d");
	  direction.beginPath();
		if(Elevation==0){
			direction.arc(Xcenter, Ycenter, BoxSize/2*0.9, 0, 2 * Math.PI);
		}else{
			direction.arc(Xcenter, Ycenter, BoxSize/2*0.9, Math.PI, 2 * Math.PI);
		}
	  direction.lineWidth = 2;
	  direction.strokeStyle = '#c0c0c0';
	  direction.font = "20px Arial";
		direction.textAlign = 'center';
		direction.textBaseline = 'middle';
	  direction.fillStyle = "#808080";
		if(Elevation==0){
			for(var i=0;i<36;i++){
				if(i %9 === 0){
				}else{
					direction.moveTo(Xcoordinate(i*10, BoxSize/2*0.95), Ycoordinate(i*10, BoxSize/2*0.95));
					direction.lineTo(Xcoordinate(i*10, BoxSize/2*0.9), Ycoordinate(i*10, BoxSize/2*0.9));
				}
				if(i %3 === 0){
					if(i %9 === 0){
					}else{
						direction.fillText(i*10, Xcoordinate(i*10, BoxSize/2), Ycoordinate(i*10, BoxSize/2));
					}
				}
			}
		}else{
			for (var i = 0; i <= 18; i++) {
				direction.moveTo(Xcoordinate(i*10+270, BoxSize/2*0.95), Ycoordinate(i*10+270, BoxSize/2*0.95));
				direction.lineTo(Xcoordinate(i*10+270, BoxSize/2*0.9), Ycoordinate(i*10+270, BoxSize/2*0.9));
				if(i %3 === 0){
					if(i %9 === 0){
					}else{
						direction.fillText(i*10, Xcoordinate(i*10+270, BoxSize/2), Ycoordinate(i*10+270, BoxSize/2));
					}
				}
			}
		}
		if(Elevation==0){
		  	direction.fillStyle = "white";
			direction.fillText("N", BoxSize/2, BoxSize*0.025);
			direction.fillText("E", BoxSize*0.975, BoxSize/2);
			direction.fillText("S", BoxSize/2, BoxSize* 0.975);
			direction.fillText("W", BoxSize*0.025, BoxSize/2);
		}else{
			direction.fillStyle = "#404040";
			direction.font = "73px Arial";
			direction.textAlign = 'center';
			direction.textBaseline = 'middle';
			// direction.fillText("Elevation", BoxSize/2, BoxSize* 0.95);
		}
		direction.stroke();
		// darkzone if <360
		if(AzRange<360){
			direction.beginPath();
			direction.lineWidth = 1;
		  direction.strokeStyle = 'white';
			direction.moveTo(BoxSize/2, BoxSize/2);
			direction.lineTo(Xcoordinate(Number(AzShift), Number(BoxSize)/2*0.9), Ycoordinate(Number(AzShift), Number(BoxSize)/2*0.9));
			direction.moveTo(BoxSize/2, BoxSize/2);
			direction.lineTo(Xcoordinate(Number(AzShift) + Number(AzRange), Number(BoxSize)/2*0.9), Ycoordinate(Number(AzShift) + Number(AzRange), Number(BoxSize)/2*0.9));
			direction.stroke();

			if(Number(AzRange)+Number(AntRadiationAngle)<360){
				direction.beginPath();
					direction.fillStyle = "rgba(0, 0, 0, 0.5)";
					var DarkRange = 360-Number(AzRange)-Number(AntRadiationAngle);
					var Steps = (Number(DarkRange)-20)/10;
					direction.moveTo(BoxSize/2, BoxSize/2);
					direction.lineTo(Xcoordinate(Number(AzShift) + Number(AzRange) + Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9), Ycoordinate(Number(AzShift) + Number(AzRange) + Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9));
					for(var i=0;i<Steps;i++){
						direction.lineTo(Xcoordinate(Number(AzShift) + Number(AzRange) + Number(AntRadiationAngle)/2 + 10*(Number(i)+1), Number(BoxSize)/2*0.9), Ycoordinate(Number(AzShift) + Number(AzRange) + Number(AntRadiationAngle)/2 + 10*(Number(i)+1), Number(BoxSize)/2*0.9));
					}
					direction.lineTo(Xcoordinate(Number(AzShift)-Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9), Ycoordinate(Number(AzShift)-Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9));
				direction.fill();
			}
		}

	}

	function StaticBot(){
	  var con = document.getElementById("DirLine");
	  var dirline = con.getContext("2d");

	  // direction line under map
	  dirline.beginPath();
	  dirline.lineWidth = 1;
	  dirline.strokeStyle = '#606060';
	    for(var i=0;i<24;i++){
	  		if(i %2 === 0){
					dirline.moveTo(Xcoordinate(i*15, BoxSize/2*0.1), Ycoordinate(i*15, BoxSize/2*0.1));
					dirline.lineTo(Xcoordinate(i*15, BoxSize/2*0.9), Ycoordinate(i*15, BoxSize/2*0.9));
	  		}else{
					dirline.moveTo(Xcoordinate(i*15, BoxSize/2*0.3), Ycoordinate(i*15, BoxSize/2*0.3));
	  			dirline.lineTo(Xcoordinate(i*15, BoxSize/2*0.9), Ycoordinate(i*15, BoxSize/2*0.9));
	  		}
	    }
	  dirline.stroke();
	}

	</script>
	</body>
	</html>
)=====";
