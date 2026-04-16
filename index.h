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
				<span style="color: #666; font-size: 73%" id="mac"> </span><span id="MapModeInfo" style="color: #666; font-size: 73%"></span><span id="NtpStatus" style="color: #666; font-size: 73%"></span><span id="OnlineStatus" style="color: #666; font-size: 73%"></span>
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
	var MapTheme = 1;
	var GraylineDarkness = 44;
	var GraylineEpoch = 0;
	var GraylineNtpOk = false;
	var GraylineMinuteKey = "";
	var AzimuthPulseStart = 0;
	var OnlineTimeStamp = 0;
	var Elevation = 0; 
	
	var Xcenter = BoxSize/2;
	var Ycenter = BoxSize/2;


	// setInterval(function.reload(){ map();}, 600000); //mSeconds update rate
	setInterval(function() { map();}, 600000); //mSeconds update rate
	setInterval(function() { getData();}, 500); //mSeconds update rate
	setInterval(function() { CheckOnline();}, 2000); //mSeconds update rate
	setInterval(function() { getGraylineInfo();}, 30000); // update UTC/NTP status for grayline
	setInterval(function() { if (AzimuthPulseStart > 0) { AZ(Azimuth); } }, 40); // short pulse animation after azimuth change
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

	function updateNtpStatus(){
		var info = document.getElementById("NtpStatus");
		if(!info){ return; }
		if(Number(MapSource)!==1){
			info.innerHTML = "";
			return;
		}
		if(GraylineNtpOk){
			info.innerHTML = "";
		}else{
			info.innerHTML = " | NTP no answer";
		}
	}

	function getGraylineInfo() {
	  var ghttp = new XMLHttpRequest();
	  ghttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				var parts = String(this.responseText).split("|");
				var oldMinuteKey = GraylineMinuteKey;
				GraylineNtpOk = (parts[0] === "1");
				GraylineEpoch = GraylineNtpOk ? Number(parts[1] || 0) : 0;
				GraylineMinuteKey = GraylineNtpOk ? String(Math.floor(GraylineEpoch / 60)) : "";
				updateNtpStatus();
				if (Number(MapSource) === 1 && oldMinuteKey !== GraylineMinuteKey) {
					map();
				}
	    }
	  };
	  ghttp.open("GET", "readGraylineInfo", true);
	  ghttp.send();
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
				updateNtpStatus();
				if (Number(MapSource) === 1) { getGraylineInfo(); }
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

	  var thttp = new XMLHttpRequest();
	  thttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				MapTheme = Number(this.responseText);
				if (Number(MapSource) === 1) { map(); }
	    }
	  };
	  thttp.open("GET", "readMapTheme", true);
	  thttp.send();

	  var shttp = new XMLHttpRequest();
	  shttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				GraylineDarkness = Number(this.responseText);
				if (Number(MapSource) === 1) { map(); }
	    }
	  };
	  shttp.open("GET", "readGraylineDarkness", true);
	  shttp.send();

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

		getGraylineInfo();
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
					AzimuthPulseStart = new Date().getTime();
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
	var LocatorLandFillMode = false; // true = fill-like light land, false = outline only

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

	function getMapThemeStyle(){
		if(Number(MapTheme)===1){
			return {
				mapBg: "#081018",
				ring: "rgba(120, 164, 130, 0.18)",
				landFill: "rgba(113,145,118,0.78)",
				landStroke: "rgba(184,215,168,0.92)",
				grayline: "0,10,6"
			};
		}
		if(Number(MapTheme)===2){
			return {
				mapBg: "#15161b",
				ring: "rgba(156, 166, 188, 0.18)",
				landFill: "rgba(198,191,173,0.78)",
				landStroke: "rgba(241,233,214,0.94)",
				grayline: "10,9,14"
			};
		}
		if(Number(MapTheme)===3){
			return {
				mapBg: "#140d06",
				ring: "rgba(205, 140, 64, 0.18)",
				landFill: "rgba(163,117,67,0.78)",
				landStroke: "rgba(255,210,145,0.94)",
				grayline: "20,10,0"
			};
		}
		if(Number(MapTheme)===4){
			return {
				mapBg: "#071109",
				ring: "rgba(88, 182, 104, 0.18)",
				landFill: "rgba(68,136,82,0.78)",
				landStroke: "rgba(169,255,176,0.94)",
				grayline: "0,12,0"
			};
		}
		if(Number(MapTheme)===5){
			return {
				mapBg: "#1a0906",
				ring: "rgba(255, 86, 38, 0.22)",
				landFill: "rgba(211,76,32,0.80)",
				landStroke: "rgba(255,194,92,0.96)",
				grayline: "20,4,0"
			};
		}
		if(Number(MapTheme)===6){
			return {
				mapBg: "#06111b",
				ring: "rgba(90, 208, 255, 0.22)",
				landFill: "rgba(48,123,187,0.78)",
				landStroke: "rgba(191,248,255,0.96)",
				grayline: "0,6,14"
			};
		}
		if(Number(MapTheme)===7){
			return {
				mapBg: "#120819",
				ring: "rgba(255, 80, 182, 0.20)",
				landFill: "rgba(123,54,178,0.80)",
				landStroke: "rgba(255,146,228,0.96)",
				grayline: "8,0,16"
			};
		}
		return {
			mapBg: "#0b1320",
			ring: "rgba(160, 180, 200, 0.18)",
			landFill: "rgba(208,232,200,0.78)",
			landStroke: "rgba(238,249,231,0.95)",
			grayline: "0,0,0"
		};
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
	var locatorFillCacheCanvas = null;
	var locatorFillCacheKey = "";
	var LAND_META = null;

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

	function projectAzimuthalRaw(latDeg, lonDeg, centerLatDeg, centerLonDeg, mapRadiusPx, zoomKm){
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

	function buildCircleClipPolygon(mapRadiusPx, segments){
		var poly = [];
		var n = segments || 96;
		for(var i=0; i<n; i++){
			var a = (2 * Math.PI * i) / n;
			poly.push({
				x: Xcenter + Math.cos(a) * mapRadiusPx,
				y: Ycenter - Math.sin(a) * mapRadiusPx
			});
		}
		return poly;
	}

	function cross2(ax, ay, bx, by){
		return ax * by - ay * bx;
	}

	function polygonSignedArea(poly){
		var s = 0;
		for(var i=0; i<poly.length; i++){
			var p = poly[i];
			var q = poly[(i + 1) % poly.length];
			s += p.x * q.y - q.x * p.y;
		}
		return s * 0.5;
	}

	function isInsideClipEdge(p, a, b, clipOrientation){
		var eps = 1e-7;
		var side = cross2(b.x - a.x, b.y - a.y, p.x - a.x, p.y - a.y);
		return (clipOrientation * side) >= -eps;
	}

	function lineIntersectionWithClipEdge(p1, p2, a, b){
		var dx = p2.x - p1.x;
		var dy = p2.y - p1.y;
		var ex = b.x - a.x;
		var ey = b.y - a.y;
		var denom = cross2(dx, dy, ex, ey);
		if(Math.abs(denom) < 1e-12){
			return {x: p2.x, y: p2.y};
		}
		var t = cross2(a.x - p1.x, a.y - p1.y, ex, ey) / denom;
		return {
			x: p1.x + t * dx,
			y: p1.y + t * dy
		};
	}

	function clipPolygonWithConvex(subject, clipPoly){
		var output = subject.slice();
		var clipOrientation = polygonSignedArea(clipPoly) >= 0 ? 1 : -1;
		for(var i=0; i<clipPoly.length; i++){
			var a = clipPoly[i];
			var b = clipPoly[(i + 1) % clipPoly.length];
			var input = output.slice();
			output = [];
			if(input.length === 0){
				break;
			}
			var s = input[input.length - 1];
			for(var j=0; j<input.length; j++){
				var e = input[j];
				var eInside = isInsideClipEdge(e, a, b, clipOrientation);
				var sInside = isInsideClipEdge(s, a, b, clipOrientation);

				if(eInside){
					if(!sInside){
						output.push(lineIntersectionWithClipEdge(s, e, a, b));
					}
					output.push(e);
				}else if(sInside){
					output.push(lineIntersectionWithClipEdge(s, e, a, b));
				}
				s = e;
			}
		}
		return output;
	}

	function buildClippedLandPolygons(centerLat, centerLon, mapRadiusPx, zoomKm){
		var clipPoly = buildCircleClipPolygon(mapRadiusPx, 96);
		var out = [];
		for(var i=0; i<LAND_OUTLINES.length; i++){
			var line = LAND_OUTLINES[i];
			if(!line || line.length < 3){
				continue;
			}
			var poly = [];
			for(var j=0; j<line.length; j++){
				poly.push(projectAzimuthalRaw(line[j][0] / 100, line[j][1] / 100, centerLat, centerLon, mapRadiusPx, zoomKm));
			}
			var clipped = clipPolygonWithConvex(poly, clipPoly);
			if(clipped.length >= 3){
				out.push(clipped);
			}
		}
		return out;
	}

	function svgClosedPathFromSegments(allSegments){
		var d = "";
		for(var i=0; i<allSegments.length; i++){
			var segments = allSegments[i];
			for(var j=0; j<segments.length; j++){
				var segment = segments[j];
				if(!segment || segment.length < 3){
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
		var theme = getMapThemeStyle();
		var landSegments = projectGeoSegments(LAND_OUTLINES, centerLat, centerLon, mapRadiusPx, zoomKm);
		var landPath = svgPathFromSegments(landSegments);
		var landFillPath = svgClosedPathFromSegments(landSegments);
		var circleR = Math.round(mapRadiusPx * 10) / 10;

		var svg = "";
		svg += "<svg xmlns='http://www.w3.org/2000/svg' width='" + BoxSize + "' height='" + BoxSize + "' viewBox='0 0 " + BoxSize + " " + BoxSize + "'>";
		svg += "<rect width='100%' height='100%' fill='#000'/>";
		svg += "<defs><clipPath id='mapclip'><circle cx='" + Xcenter + "' cy='" + Ycenter + "' r='" + circleR + "'/></clipPath></defs>";
		svg += "<g clip-path='url(#mapclip)'>";
		svg += "<rect width='100%' height='100%' fill='" + theme.mapBg + "'/>";
		for(var ring = 1; ring <= 4; ring++){
			var rr = Math.round((circleR * ring / 4) * 10) / 10;
			svg += "<circle cx='" + Xcenter + "' cy='" + Ycenter + "' r='" + rr + "' fill='none' stroke='" + theme.ring + "' stroke-width='1'/>";
		}
		if(LocatorLandFillMode && landFillPath.length>0){
			svg += "<path d='" + landFillPath + "' fill='" + theme.landFill + "' stroke='none' fill-rule='evenodd'/>";
		}else if(landPath.length>0){
			svg += "<path d='" + landPath + "' fill='none' stroke='" + theme.landStroke + "' stroke-width='1.2' stroke-linecap='round' stroke-linejoin='round'/>";
		}
		svg += "</g></svg>";
		return svg;
	}

	function ensureLandMeta(){
		if(LAND_META){ return; }
		LAND_META = [];
		for(var i=0; i<LAND_OUTLINES.length; i++){
			var line = LAND_OUTLINES[i];
			if(!line || line.length < 3){
				continue;
			}
			var latMin = 999999;
			var latMax = -999999;
			for(var j=0; j<line.length; j++){
				var lat = line[j][0];
				if(lat < latMin){ latMin = lat; }
				if(lat > latMax){ latMax = lat; }
			}
			LAND_META.push({
				ring: line,
				latMin: latMin,
				latMax: latMax
			});
		}
	}

	function normalizeLonAroundRef(lonDeg, refLonDeg){
		var lon = lonDeg;
		while(lon - refLonDeg > 180){ lon -= 360; }
		while(lon - refLonDeg < -180){ lon += 360; }
		return lon;
	}

	function pointInRingLatLon(latDeg, lonDeg, ring){
		var inside = false;
		for(var i=0, j=ring.length - 1; i<ring.length; j=i++){
			var yi = ring[i][0] / 100;
			var yj = ring[j][0] / 100;
			var xi = normalizeLonAroundRef(ring[i][1] / 100, lonDeg);
			var xj = normalizeLonAroundRef(ring[j][1] / 100, lonDeg);
			var intersects = ((yi > latDeg) !== (yj > latDeg)) &&
				(lonDeg < ((xj - xi) * (latDeg - yi) / ((yj - yi) || 1e-12) + xi));
			if(intersects){
				inside = !inside;
			}
		}
		return inside;
	}

	function isLandLatLon(latDeg, lonDeg){
		ensureLandMeta();
		var inside = false;
		var latScaled = Math.round(latDeg * 100);
		for(var i=0; i<LAND_META.length; i++){
			var meta = LAND_META[i];
			if(latScaled < meta.latMin || latScaled > meta.latMax){
				continue;
			}
			if(pointInRingLatLon(latDeg, lonDeg, meta.ring)){
				inside = !inside;
			}
		}
		return inside;
	}

	function inverseProjectAzimuthal(x, y, centerLatDeg, centerLonDeg, mapRadiusPx, zoomKm){
		var dx = x - Xcenter;
		var dy = Ycenter - y;
		var rho = Math.hypot(dx, dy);
		if(rho > mapRadiusPx){
			return null;
		}
		if(rho < 1e-9){
			return {lat: centerLatDeg, lon: centerLonDeg};
		}

		var phi1 = degToRad(centerLatDeg);
		var lam1 = degToRad(centerLonDeg);
		var c = (rho / mapRadiusPx) * (zoomKm / EARTH_RADIUS_KM);
		var sinC = Math.sin(c);
		var cosC = Math.cos(c);

		var phi = Math.asin(cosC * Math.sin(phi1) + (dy * sinC * Math.cos(phi1) / rho));
		var lam = lam1 + Math.atan2(
			dx * sinC,
			rho * Math.cos(phi1) * cosC - dy * Math.sin(phi1) * sinC
		);

		var lat = phi * 180 / Math.PI;
		var lon = lam * 180 / Math.PI;
		while(lon > 180){ lon -= 360; }
		while(lon < -180){ lon += 360; }
		return {lat: lat, lon: lon};
	}

	function normalizeDegrees(angle){
		var a = angle;
		while(a > 180){ a -= 360; }
		while(a < -180){ a += 360; }
		return a;
	}

	function julianDayFromUnix(epochSec){
		return (epochSec / 86400) + 2440587.5;
	}

	function getSubsolarPoint(epochSec){
		var jd = julianDayFromUnix(epochSec);
		var n = jd - 2451545.0;
		var L = (280.460 + 0.9856474 * n) % 360;
		if(L < 0){ L += 360; }
		var g = (357.528 + 0.9856003 * n) % 360;
		if(g < 0){ g += 360; }
		var gRad = degToRad(g);
		var lambda = L + 1.915 * Math.sin(gRad) + 0.020 * Math.sin(2 * gRad);
		var lambdaRad = degToRad(lambda);
		var epsilon = 23.439 - 0.0000004 * n;
		var epsilonRad = degToRad(epsilon);
		var declRad = Math.asin(Math.sin(epsilonRad) * Math.sin(lambdaRad));
		var raRad = Math.atan2(Math.cos(epsilonRad) * Math.sin(lambdaRad), Math.cos(lambdaRad));
		var raDeg = raRad * 180 / Math.PI;
		var gmst = (280.46061837 + 360.98564736629 * (jd - 2451545.0)) % 360;
		if(gmst < 0){ gmst += 360; }
		return {
			lat: declRad * 180 / Math.PI,
			lon: normalizeDegrees(raDeg - gmst)
		};
	}

	function solarAltitudeDeg(latDeg, lonDeg, subsolar){
		var latRad = degToRad(latDeg);
		var subLatRad = degToRad(subsolar.lat);
		var dLonRad = degToRad(normalizeDegrees(lonDeg - subsolar.lon));
		var sinAlt = Math.sin(latRad) * Math.sin(subLatRad) +
			Math.cos(latRad) * Math.cos(subLatRad) * Math.cos(dLonRad);
		if(sinAlt > 1){ sinAlt = 1; }
		if(sinAlt < -1){ sinAlt = -1; }
		return Math.asin(sinAlt) * 180 / Math.PI;
	}

	var graylineCacheCanvas = null;
	var graylineCacheKey = "";

	function drawSubsolarPoint(ctx, subsolar, centerLat, centerLon, mapRadiusPx, zoomKm){
		if(!subsolar){
			return;
		}
		var p = projectAzimuthal(subsolar.lat, subsolar.lon, centerLat, centerLon, mapRadiusPx, zoomKm);
		if(!p){
			return;
		}
		ctx.save();
		ctx.beginPath();
		ctx.arc(Xcenter, Ycenter, mapRadiusPx, 0, 2 * Math.PI);
		ctx.clip();
		ctx.shadowColor = "rgba(255, 220, 90, 0.80)";
		ctx.shadowBlur = 12;
		ctx.fillStyle = "#ffd84d";
		ctx.beginPath();
		ctx.arc(p.x, p.y, 4.5, 0, 2 * Math.PI);
		ctx.fill();
		ctx.shadowBlur = 0;
		ctx.fillStyle = "rgba(255, 248, 210, 0.95)";
		ctx.beginPath();
		ctx.arc(p.x, p.y, 1.8, 0, 2 * Math.PI);
		ctx.fill();
		ctx.restore();
	}

	function drawGrayline(ctx, centerLat, centerLon, mapRadiusPx, zoomKm){
		if(!GraylineNtpOk || GraylineEpoch <= 0){
			return;
		}
		var theme = getMapThemeStyle();
		var subsolar = getSubsolarPoint(Math.floor(GraylineEpoch / 60) * 60);
		if(!graylineCacheCanvas){
			graylineCacheCanvas = document.createElement('canvas');
			graylineCacheCanvas.width = BoxSize;
			graylineCacheCanvas.height = BoxSize;
		}
		var minuteEpoch = Math.floor(GraylineEpoch / 60) * 60;
		var cacheKey = String(centerLat) + "|" + String(centerLon) + "|" + String(zoomKm) + "|" + String(minuteEpoch);
		if(graylineCacheKey !== cacheKey){
			var gctx = graylineCacheCanvas.getContext('2d');
			gctx.clearRect(0, 0, BoxSize, BoxSize);
			var step = 3;
			var maxAlpha = Math.max(0, Math.min(100, Number(GraylineDarkness))) / 100;
			for(var py=0; py<BoxSize; py+=step){
				for(var px=0; px<BoxSize; px+=step){
					var sample = inverseProjectAzimuthal(px + step * 0.5, py + step * 0.5, centerLat, centerLon, mapRadiusPx, zoomKm);
					if(!sample){
						continue;
					}
					var altDeg = solarAltitudeDeg(sample.lat, sample.lon, subsolar);
					if(altDeg >= 2){
						continue;
					}
					var alpha = 0;
					if(altDeg <= -10){
						alpha = maxAlpha;
					}else{
						alpha = maxAlpha * ((2 - altDeg) / 12);
					}
					gctx.fillStyle = "rgba(" + theme.grayline + "," + alpha.toFixed(3) + ")";
					gctx.fillRect(px, py, step, step);
				}
			}
			graylineCacheKey = cacheKey;
		}
		ctx.save();
		ctx.beginPath();
		ctx.arc(Xcenter, Ycenter, mapRadiusPx, 0, 2 * Math.PI);
		ctx.clip();
		ctx.filter = "blur(3px)";
		ctx.drawImage(graylineCacheCanvas, 0, 0);
		ctx.filter = "none";
		ctx.restore();
		drawSubsolarPoint(ctx, subsolar, centerLat, centerLon, mapRadiusPx, zoomKm);
	}

	function drawOutsideCircleMask(ctx, mapRadiusPx){
		ctx.beginPath();
		ctx.rect(0, 0, BoxSize, BoxSize);
		ctx.arc(Xcenter, Ycenter, mapRadiusPx, 0, 2 * Math.PI, true);
		ctx.fillStyle = "#000";
		ctx.fill("evenodd");
	}

	function drawProjectedLandFill(ctx, centerLat, centerLon, mapRadiusPx, zoomKm){
		var theme = getMapThemeStyle();
		var step = 2;
		if(!locatorFillCacheCanvas){
			locatorFillCacheCanvas = document.createElement('canvas');
			locatorFillCacheCanvas.width = BoxSize;
			locatorFillCacheCanvas.height = BoxSize;
		}
		var fillKey = String(centerLat) + "|" + String(centerLon) + "|" + String(mapRadiusPx) + "|" + String(zoomKm) + "|" + String(step);
		if(locatorFillCacheKey === fillKey){
			ctx.drawImage(locatorFillCacheCanvas, 0, 0);
			return;
		}

		var fillCtx = locatorFillCacheCanvas.getContext('2d');
		fillCtx.clearRect(0, 0, BoxSize, BoxSize);
		fillCtx.fillStyle = theme.landFill;

		for(var py=0; py<BoxSize; py+=step){
			for(var px=0; px<BoxSize; px+=step){
				var sample = inverseProjectAzimuthal(px + step * 0.5, py + step * 0.5, centerLat, centerLon, mapRadiusPx, zoomKm);
				if(!sample){
					continue;
				}
				if(isLandLatLon(sample.lat, sample.lon)){
					fillCtx.fillRect(px, py, step, step);
				}
			}
		}

		locatorFillCacheKey = fillKey;
		ctx.drawImage(locatorFillCacheCanvas, 0, 0);
	}

	function drawLocatorMapCanvas(centerLat, centerLon, mapRadiusPx, zoomKm){
		var theme = getMapThemeStyle();
		var c = document.getElementById('Map');
		if (!c.getContext) { return; }
		var ctx = c.getContext('2d');

		ctx.clearRect(0, 0, BoxSize, BoxSize);
		ctx.fillStyle = "#000";
		ctx.fillRect(0, 0, BoxSize, BoxSize);

		if(LocatorLandFillMode){
			ctx.beginPath();
			ctx.arc(Xcenter, Ycenter, mapRadiusPx, 0, 2 * Math.PI);
			ctx.fillStyle = theme.mapBg;
			ctx.fill();

			drawProjectedLandFill(ctx, centerLat, centerLon, mapRadiusPx, zoomKm);
			drawGrayline(ctx, centerLat, centerLon, mapRadiusPx, zoomKm);
			drawOutsideCircleMask(ctx, mapRadiusPx);

			ctx.strokeStyle = theme.ring;
			ctx.lineWidth = 1;
			for(var ring = 1; ring <= 4; ring++){
				ctx.beginPath();
				ctx.arc(Xcenter, Ycenter, mapRadiusPx * ring / 4, 0, 2 * Math.PI);
				ctx.stroke();
			}
		}else{
			ctx.save();
			ctx.beginPath();
			ctx.arc(Xcenter, Ycenter, mapRadiusPx, 0, 2 * Math.PI);
			ctx.clip();
			ctx.fillStyle = theme.mapBg;
			ctx.fillRect(0, 0, BoxSize, BoxSize);
			ctx.strokeStyle = theme.ring;
			ctx.lineWidth = 1;
			for(var ring = 1; ring <= 4; ring++){
				ctx.beginPath();
				ctx.arc(Xcenter, Ycenter, mapRadiusPx * ring / 4, 0, 2 * Math.PI);
				ctx.stroke();
			}
			drawGeoLineCollection(ctx, LAND_OUTLINES, centerLat, centerLon, mapRadiusPx, zoomKm, theme.landStroke, 1.2);
			drawGrayline(ctx, centerLat, centerLon, mapRadiusPx, zoomKm);
			ctx.restore();
		}
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
			drawGrayline(ctx, center.lat, center.lon, mapRadiusPx, zoomKm);
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
			drawGrayline(ctx, center.lat, center.lon, mapRadiusPx, zoomKm);
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
			var pulseMs = 240;
			var pulseGain = 0;
			if(AzimuthPulseStart > 0){
				var pulseAge = new Date().getTime() - AzimuthPulseStart;
				if(pulseAge < pulseMs){
					pulseGain = 1 - pulseAge / pulseMs;
				}else{
					AzimuthPulseStart = 0;
				}
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
				var pulseFont = 100 + Math.round(pulseGain * 10);
				az.font = "bold " + String(pulseFont) + "px Arial";
				az.shadowColor = "rgba(255,255,255," + String((0.12 + pulseGain * 0.28).toFixed(2)) + ")";
				az.shadowBlur = 4 + pulseGain * 16;
				az.fillText(ShowAzimuth+String.fromCharCode(176), Xcoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2), Ycoordinate(Number(Azimuth) + Number(AzShift) + 180, BoxSize*0.2));
				az.shadowBlur = 0;
				az.shadowColor = "transparent";
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
