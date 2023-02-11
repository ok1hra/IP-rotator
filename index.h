const char MAIN_page[] PROGMEM = R"=====(
	<!DOCTYPE html>
	<html>

	<head>
	<title>IP rotator</title>
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
			z-index: 1;
		}
		.form1{
			margin: auto;
			position: absolute;
			left: 0;
			top: 600px;
			z-index: 1;
		}
		.form2{
			margin: auto;
			position: absolute;
			left: 540px;
			top: 600px;
			z-index: 1;
		}
		.top{
			margin: auto;
			border: 0px solid #222222;
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
		.second{
			border: 0px solid #222222;
			position: absolute;
			left: 0;
			top: 600px;
			width: 600px;
			z-index: 0;
		}
	</style>
	<link href='http://fonts.googleapis.com/css?family=Roboto+Condensed:300italic,400italic,700italic,400,700,300&subset=latin-ext' rel='stylesheet' type='text/css'>
	</head>
	<body>

	<div style="position: relative;">
		<canvas class="mouse" id="Mouse" height="600" width="600"></canvas>
		<canvas class="top" id="Azimuth" width="600" height="600">Your browser does not support the HTML5 canvas tag.</canvas>
		<canvas class="bot" id="Map" width="600" height="600"></canvas>
		<form class="form1" name="frm0" method="post">
			<input type="text" name="ROT" size="3" value="303">
			<input type="submit" value="ROTATE" style="background: #080;">
		</form>
		<form class="form2" name="frm1" method="post">
			<input type="submit" value="STOP" style="background: ORANGE;">
		</form>
		<div class="second">
			<p style="color: #ccc; margin: 0 0 0 0; text-align: center;">
				<span style="color: #000; background: #666; padding: 4px 6px 4px 6px; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;">
				<span style="font-weight: bold;" id="AZValue">0</span>&deg; |
				<span id="AntName"> </span> |
					<span style="color: #fff; font-weight: bold;" id="ADCValue">0</span> V
				</span>
				<!--<form action="/get">
					Target <input type="text" name="input1">
					<input type="submit" value="Submit">
				</form><br>-->
			</p>
		</div>
	</div>

	<script>
	// <span style="color: #999; font-size: 800%;">318&deg;</span><br>
	// <h1 style="color: white;">adc:<span id="ADCValue">0</span></h1><br>
	// <h1 style="color: white;">az:<span id="AZValue">0</span></h1><br>
	// ToDo
	// if range < 360, then show start and stop azimuth
	// timeout - if server do not answer, then hide azimuth
	// red undervoltage
	// https://stackoverflow.com/questions/3420975/html5-canvas-zooming
	// https://stackoverflow.com/questions/29028125/reloading-an-image-with-delay-using-javascript

	var BoxSize = 600;
	var Target  = 0;
	var AzShift = 0;
	var AzRange = 450;
	// var Azimuth = 122;
	var AzimuthTmp = 0;
	var AntRadiationAngle = 0;
	var Status = 4;
	var MapUrl = 0;

	var Xcenter = BoxSize/2;
	var Ycenter = BoxSize/2;

	setInterval(function() { getData();}, 500); //mSeconds update rate
	getSet();
	setTimeout(() => { map(); }, 1000);
	Static();

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
	    }
	  };
	  mhttp.open("GET", "readMapUrl", true);
	  mhttp.send();
	}

	function getData() {
	  var xhttp = new XMLHttpRequest();
	  xhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      document.getElementById("ADCValue").innerHTML = this.responseText;
	    }
	  };
	  xhttp.open("GET", "readADC", true);
	  xhttp.send();

	  var yhttp = new XMLHttpRequest();
	  yhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      document.getElementById("AZValue").innerHTML = this.responseText;
				Azimuth = this.responseText;
				if( Math.abs(Number(AzimuthTmp)-Number(Azimuth))>1 ){	// || Status != 4
					AZ(Azimuth);
					Static();
					AzimuthTmp=Azimuth;
				}
				// console.log ('getData.Azimuth ' + Azimuth);
	    }
	  };
	  yhttp.open("GET", "readAZ", true);
	  yhttp.send();

	  var zhttp = new XMLHttpRequest();
	  zhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StatValue").innerHTML = this.responseText;
				Status = this.responseText;
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
			var AZtarget = Math.atan2(BoxSize/2 - Number(mousePos.y), Number(mousePos.x) - BoxSize/2) * 180 / Math.PI;
			AZtarget = AZtarget - 90;
			if(AZtarget<0){
				AZtarget = Math.abs(AZtarget);
			}else{
				AZtarget = 360 - AZtarget;
			}
			AZtarget = Math.round(AZtarget);
			alert( AZtarget + '°');
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

	// setInterval( location.reload(){ map();}, 5000); //mSeconds update rate
	function map(){
		var ctx = document.getElementById('Map');
		if (ctx.getContext) {
				ctx = ctx.getContext('2d');
				var img1 = new Image();
				img1.onload = function () {
						ctx.drawImage(img1, 0, 0, BoxSize, BoxSize);
				};
				// img1.src = 'https://hra.remoteqth.com/map.png';
				img1.src = String(MapUrl);

				// img1.src = "https://" + String(MapUrl);
				// console.log ('img1 ' + img1);
		    // img1.setAttribute("src", MapUrl);
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
		  az.lineWidth = 5;
			if (Status != 4) {
				az.strokeStyle = "red";
			}else if (Azimuth > 359) {
				az.strokeStyle = "orange";
			}else{
				az.strokeStyle = '#00aa00';
			}
		  az.moveTo(Xcoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.9), Ycoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.9));
		  az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.7), Ycoordinate(Number(Azimuth) + Number(AzShift), BoxSize/2*0.7));
		az.stroke();
	  az.font = "bold 100px Arial";
			az.textAlign = 'center';
			az.textBaseline = 'middle';
			var ShowAzimuth = Number(Azimuth) + Number(AzShift);
			if(ShowAzimuth > 359){
				ShowAzimuth = Number(ShowAzimuth) - 360;
			}
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
		az.beginPath();
			az.moveTo(BoxSize/2, BoxSize/2);
			az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)+Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9), Ycoordinate(Number(Azimuth) + Number(AzShift)+Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9));
			az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift), Number(BoxSize)/2*0.9), Ycoordinate(Number(Azimuth) + Number(AzShift), Number(BoxSize)/2*0.9));
			az.lineTo(Xcoordinate(Number(Azimuth) + Number(AzShift)-Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9), Ycoordinate(Number(Azimuth) + Number(AzShift)-Number(AntRadiationAngle)/2, Number(BoxSize)/2*0.9));
			// az.arc(Xcenter, Ycenter, BoxSize/2*0.9, 0, 1.8 * Math.PI);
			if (Status != 4) {
				az.fillStyle = "rgba(255, 0, 0, 0.25)";
			}else if (Azimuth > 359) {
			 az.fillStyle = "rgba(255, 165, 0, 0.25)";
			}else{
				az.fillStyle = "rgba(255, 255, 255, 0.25)";
			}
		az.fill();
	}

	function Static(){
	  var con = document.getElementById("Azimuth");
	  var angle = con.getContext("2d");

	  // overlap
	  var overlap = con.getContext("2d");
	  overlap.beginPath();
	  // overlap.rotate(90 * Math.PI / 180);
	  overlap.lineWidth = 8;
	  overlap.strokeStyle = 'orange';
		var START = 270+Number(AzShift);
		var STOP  = 270+Number(AzShift)+Number(AzRange)-360;
	  overlap.arc(Xcenter, Ycenter, BoxSize/2*0.9-8, Number(START) * Math.PI/180, Number(STOP) * Math.PI/180 );
		// console.log ('start ' + START);
		// console.log ('stop ' + STOP);
	  overlap.stroke();

	  // direction
	  var direction = con.getContext("2d");
	  direction.beginPath();
	  direction.arc(Xcenter, Ycenter, BoxSize/2*0.9, 0, 2 * Math.PI);
	  direction.lineWidth = 2;
	  direction.strokeStyle = '#c0c0c0';
	  direction.font = "20px Arial";
	  direction.fillStyle = "#808080";
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
	  	direction.fillStyle = "white";
	  direction.fillText("N", BoxSize/2, BoxSize*0.025);
	  direction.fillText("E", BoxSize*0.975, BoxSize/2);
	  direction.fillText("S", BoxSize/2, BoxSize* 0.975);
	  direction.fillText("W", BoxSize*0.025, BoxSize/2);
	  direction.stroke();
	}

	</script>
	</body>
	</html>
)=====";
