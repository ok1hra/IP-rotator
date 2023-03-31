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
					<span style="color: #fff;" id="AntName2"> </span> | PWR
					<span style="color: #fff; font-weight: bold;" id="ADCValue">0</span> V | raw
					<span style="font-weight: bold;" id="AZValue">0</span>&deg; |
					<a href="/set" onclick="window.open( this.href, this.href, 'width=700,height=940,left=0,top=0,menubar=no,location=no,status=no' ); return false;"); return false;\">SETUP</a>

				</span>
				<br>
				<span style="color: #666; font-size: 73%" id="mac"> </span><span id="OnlineStatus" style="color: #666; font-size: 73%"></span>
			</p>
		</div>
	</div>

	<script>
	var BoxSize = 600;
	var Target  = 0;
	var AzShift = 0;
	var AzRange = 0;
	// var Azimuth = 122;
	var AzimuthTmp = 0;
	var AntRadiationAngle = 0;
	var Status = 4;
	var StatusTmp = 0;
	var MapUrl = 0;
	var OnlineTimeStamp = 0;

	var Xcenter = BoxSize/2;
	var Ycenter = BoxSize/2;


	// setInterval(function.reload(){ map();}, 600000); //mSeconds update rate
	setInterval(function() { map();}, 600000); //mSeconds update rate
	setInterval(function() { getData();}, 500); //mSeconds update rate
	setInterval(function() { CheckOnline();}, 2000); //mSeconds update rate
	getSet();
	setTimeout(() => { map(); }, 1000);
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
	    }
	  };
	  mhttp.open("GET", "readMapUrl", true);
	  mhttp.send();

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
	}

	function getData() {
	  var xhttp = new XMLHttpRequest();
	  xhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
				if( Number(this.responseText)<11){

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

	function map(){
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
			var ShowAzimuth = Number(Azimuth) + Number(AzShift);
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
	  direction.arc(Xcenter, Ycenter, BoxSize/2*0.9, 0, 2 * Math.PI);
	  direction.lineWidth = 2;
	  direction.strokeStyle = '#c0c0c0';
	  direction.font = "20px Arial";
		direction.textAlign = 'center';
		direction.textBaseline = 'middle';
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
