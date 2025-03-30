#line 1 "/media/nfs/home/mjd/Projects/IP-rotator/index-cal.h"
const char CAL_page[] PROGMEM = R"=====(

	<script>

	var TimeOutSec = 1200;
	var BoxSize = 600;
	var AzimuthADC = 0;
	var Endstop = 0;
	var Status = 4;
	var StatusTmp = 0;
	var CcwRaw = 0;
	var CwRaw = 0;
	var AzShift = 0;
	var AzRange = 0;
	var Uptime = 0;
	var UptimeTmp = 0;

	setInterval(function() { getData();}, 200); //mSeconds update rate
	getSet();
	setTimeout(() => { Static(); }, 1000);

	function getSet() {

		var ghttp = new XMLHttpRequest();
	  ghttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				AzShift = this.responseText;
				// console.log ('AzShift ' + AzShift);
	    }
	  };
	  ghttp.open("GET", "readStart", true);
	  ghttp.send();

	  var hhttp = new XMLHttpRequest();
	  hhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				AzRange = this.responseText;
				// console.log ('AzRange ' + AzRange);
	    }
	  };
	  hhttp.open("GET", "readMax", true);
	  hhttp.send();

	  var ihttp = new XMLHttpRequest();
	  ihttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				Endstop = this.responseText;
				// console.log ('Endstop ' + Endstop);
				// Static();
	    }
	  };
	  ihttp.open("GET", "readEndstop", true);
	  ihttp.send();

	  var lhttp = new XMLHttpRequest();
	  lhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				NoEndstopLowZone = this.responseText;
				// console.log ('Endstop ' + Endstop);
				// Static();
	    }
	  };
	  lhttp.open("GET", "readEndstopLowZone", true);
	  lhttp.send();

	  var mhttp = new XMLHttpRequest();
	  mhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				NoEndstopHighZone = this.responseText;
				// console.log ('Endstop ' + Endstop);
				// Static();
	    }
	  };
	  mhttp.open("GET", "readEndstopHighZone", true);
	  mhttp.send();

	  var jhttp = new XMLHttpRequest();
	  jhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				CcwRaw = this.responseText;
				// console.log ('Endstop ' + Endstop);
				// Static();
	    }
	  };
	  jhttp.open("GET", "readCcwraw", true);
	  jhttp.send();

	  var khttp = new XMLHttpRequest();
	  khttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StartValue").innerHTML = this.responseText;
				CwRaw = this.responseText;
				// console.log ('Endstop ' + Endstop);
				// Static();
	    }
	  };
	  khttp.open("GET", "readCwraw", true);
	  khttp.send();
	}

	function getData() {
		var fhttp = new XMLHttpRequest();
		fhttp.onreadystatechange = function() {
			if (this.readyState == 4 && this.status == 200) {
				document.getElementById("frontAZValue").innerHTML = this.responseText;
				// console.log ('OnlineTimer ' + OnlineTimer);
			}
		};
		fhttp.open("GET", "readFrontAZ", true);
		fhttp.send();

	  var whttp = new XMLHttpRequest();
	  whttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("AZadcValue").innerHTML = this.responseText;
				AzimuthADC = this.responseText;
				POINTER(AzimuthADC);
				// Static();
				// StaticBot();
				// console.log ('getData.Azimuth ' + Azimuth);
	    }
	  };
	  whttp.open("GET", "readAZadc", true);
	  whttp.send();

	  var xhttp = new XMLHttpRequest();
	  xhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("AZadcValue").innerHTML = this.responseText;
				Uptime = this.responseText;
	    }
	  };
	  xhttp.open("GET", "readUptime", true);
	  xhttp.send();

	  var zhttp = new XMLHttpRequest();
	  zhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	      // document.getElementById("StatValue").innerHTML = this.responseText;
				Status = this.responseText;
				if( Number(StatusTmp)!=Number(Status) ){
					POINTER(AzimuthADC);
				// 	AZ(Azimuth);
				// 	Static();
				// 	StaticBot();
					StatusTmp=Status;
				}
	    }
	  };
	  zhttp.open("GET", "readStat", true);
	  zhttp.send();
	}

	//---------------------------------------------------------------

	function POINTER(PointerValue){
	  var c = document.getElementById("Azimuth");
	  var pointer = c.getContext("2d");
		pointer.clearRect(0, 52, BoxSize, 98);
		pointer.lineWidth = 5;
		pointer.fillStyle = "#c00000";
		pointer.font = "bold 20px Arial";
		pointer.textAlign = 'center';
		pointer.textBaseline = 'middle';

		pointer.beginPath();
			pointer.fillStyle = "#303030";
			var CWpos = (Number(BoxSize)-60)/33* Number(CwRaw)/100+30;
			var CCWpos = (Number(BoxSize)-60)/33* Number(CcwRaw)/100+30;
			// console.log ('CcwRaw ' + CcwRaw);
			// console.log ('CwRaw ' + CwRaw);
			// console.log ('Ccw ' + CCWpos );
			// console.log ('Cw ' + CWpos);
				pointer.moveTo( CCWpos, 55);
				pointer.lineTo( CCWpos+7, 65);
				pointer.lineTo( CCWpos, 75);

				pointer.moveTo( CWpos, 55);
				pointer.lineTo( CWpos-7, 65);
				pointer.lineTo( CWpos, 75);
				pointer.fillText( Number(AzShift) + "°", CCWpos, 90+20);
				pointer.fillText( Number(AzShift) + Number(AzRange) + "°", CWpos, 90+20);
		pointer.fill();

		var Position = (Number(BoxSize)-60)/33* Number(PointerValue)/100+30;
		// console.log ('Position ' + Position);
	  pointer.beginPath();
			pointer.moveTo( Number(Position), 55);
		  pointer.lineTo( Number(Position)+5, 75);
		  pointer.lineTo( Number(Position)-5, 75);
			// console.log ('PointerValue ' + PointerValue);
			// console.log ('NoEndstopLowZone ' + NoEndstopLowZone);
			if (Status != 4) {
				pointer.fillStyle = "#c00000";
			}else{
				if(Number(PointerValue)/1000 > Number(NoEndstopLowZone) && Number(PointerValue)/1000 < Number(NoEndstopHighZone)){
					pointer.fillStyle = "#00c000";
				}else{
					pointer.fillStyle = "orange";
				}
			}
			pointer.fillText( Number(PointerValue)/1000 + "V", Number(Position), 90);
		pointer.fill();

		if(Uptime<TimeOutSec){
			UptimeTmp=TimeOutSec-Uptime;
			pointer.font = "12px Arial";
			pointer.fillStyle = "#303030";
			pointer.fillText( "Please calibrate after the parameter has stabilized ("+convertStoMs(UptimeTmp)+" recommended coutdown)", Number(BoxSize/2), 130);
		}
	}

	function convertStoMs(seconds) {
         let minutes = Math.floor(seconds / 60);
         let extraSeconds = seconds % 60;
         minutes = minutes < 10 ? "0" + minutes : minutes;
         extraSeconds = extraSeconds< 10 ? "0" + extraSeconds : extraSeconds;
         return minutes + ":" + extraSeconds ;
      }

	function Static(){
	  var con = document.getElementById("Azimuth");
	  var angle = con.getContext("2d");

	  // endstop
		if(Endstop==0){
			var endstop = con.getContext("2d");
			endstop.beginPath();
			// endstop.rotate(90 * Math.PI / 180);
			endstop.lineWidth = 8;
			endstop.strokeStyle = 'orange';
			// endstop.moveTo(30, 45);
			endstop.moveTo((Number(BoxSize)-60)/3.3*0.142+30, 45);
			endstop.lineTo((Number(BoxSize)-60)/3.3*Number(NoEndstopLowZone)+30, 45);
			// endstop.moveTo(BoxSize-30, 45);
			endstop.moveTo((Number(BoxSize)-60)/3.3*3.155+30, 45);
			endstop.lineTo((Number(BoxSize)-60)/3.3*Number(NoEndstopHighZone)+30, 45);
			// endstop.lineTo( (Number(BoxSize)-30)-(Number(BoxSize)-60)/3.3*Number(NoEndstopHighZone), 45);
			endstop.fillStyle = "orange";
			endstop.font = "bold 10px Arial";
			endstop.textAlign = 'center';
			endstop.textBaseline = 'middle';
			endstop.fillText( "SW endstop", 70, 20);
			endstop.fillText( "endstop", 550, 20);
			endstop.stroke();
		}

	  // scale
	  var scale = con.getContext("2d");
	  scale.beginPath();
	  scale.lineWidth = 2;
	  scale.strokeStyle = "#000000";
		scale.moveTo(30, 50);
		scale.lineTo(Number(BoxSize)-30, 50);
	  scale.font = "20px Arial";
		scale.textAlign = 'center';
	  scale.fillStyle = "#303030";
	    for(var i=0;i<34;i++){
				scale.moveTo( (Number(BoxSize)-60)/33*i+30, 50);
	  		if(i %10 === 0){
					scale.lineTo((Number(BoxSize)-60)/33*i+30, 30);
					scale.fillText(Number(i)/10, (Number(BoxSize)-60)/33*i+30, 20);
				}else	if(i %5 === 0){
					scale.lineTo((Number(BoxSize)-60)/33*i+30, 35);
	  		}else{
	  			scale.lineTo((Number(BoxSize)-60)/33*i+30, 40);
	  		}
	    }
	  scale.stroke();
	}


	</script>
	</body>
	</html>
)=====";
