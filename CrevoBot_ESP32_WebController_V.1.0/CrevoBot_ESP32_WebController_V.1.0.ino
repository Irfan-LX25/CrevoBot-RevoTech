#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#define AIN1 26
#define AIN2 25
#define PWMA 33 
#define BIN1 27
#define BIN2 14
#define PWMB 13
#define STBY 12
#define KICKER_PIN 04 
const char* ssid = "CrevoBot";
const char* password = "12345678";
AsyncWebServer server(80);
int motorSpeed = 200; 
int kickerPower = 200;
const int freq = 30000;
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmChannelKick = 2;
const int resolution = 8;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="theme-color" content="#1a1a1a">
  <title>CrevoBot V4</title>
  <style>
    body {
      background-color: #1a1a1a; color: white;
      font-family: sans-serif;
      margin: 0; padding: 0;
      width: 100vw; height: 100vh;
      overflow: hidden;
      user-select: none; -webkit-user-select: none;
      touch-action: none;
      position: relative;
    }
    .btn-round {
      width: 80px; height: 80px;
      background: rgba(46, 204, 113, 0.8);
      border: 3px solid #27ae60; border-radius: 50%;
      color: white; font-size: 24px;
      display: flex; justify-content: center; align-items: center;
      box-shadow: 0 4px #1e8449; cursor: pointer;
      margin: 5px 0;
    }
    .btn-round:active, .btn-round.active {
      transform: scale(0.95); background: #2ecc71; box-shadow: 0 2px #1e8449;
    }
    .btn-rect {
      width: 90px; height: 60px; /* Ukuran sedikit disesuaikan */
      border-radius: 12px; font-weight: bold; font-size: 18px;
      color: white; border: 2px solid white;
      box-shadow: 0 4px rgba(0,0,0,0.5);
      display: flex; justify-content: center; align-items: center;
      z-index: 10;
    }
    .btn-stop { background: #f39c12; }
    .btn-stop:active { background: #f1c40f; transform: translateY(2px); }
    
    .btn-kick { background: #c0392b; }
    .btn-kick:active, .btn-kick.active { background: #e74c3c; transform: translateY(2px); }
    .group-left {
      position: absolute; bottom: 30px; left: 30px;
      display: flex; flex-direction: column; align-items: center;
    }
    .group-right {
      position: absolute; bottom: 30px; right: 30px;
      display: flex; flex-direction: column; align-items: center;
    }
    .pos-kick {
      position: absolute;
      bottom: 80px; /* Naik dari 50 ke 80 */
      left: 140px;  /* Geser ke samping mendekati tombol arah kiri */
    }
    .pos-stop {
      position: absolute;
      bottom: 80px; /* Naik dari 50 ke 80 */
      right: 140px; /* Geser ke samping mendekati tombol arah kanan */
    }
    .slider-container {
      position: absolute;
      top: 50px;
      left: 50%;
      transform: translateX(-50%);
      width: 60%;
      background: rgba(0,0,0,0.6);
      padding: 10px; border-radius: 15px;
      display: flex; justify-content: space-around;
      align-items: center;
    }
    .slider-item { text-align: center; width: 45%; }
    input[type=range] { width: 100%; accent-color: #00cec9; }
    label { font-size: 12px; color: #ddd; font-weight: bold; }
    #fs-btn {
      position: absolute; top: 10px; left: 50%; transform: translateX(-50%);
      z-index: 999; background: rgba(255,255,255,0.2);
      border: 1px solid white; color: white; padding: 5px 10px; border-radius: 5px;
    }

    #rotate-msg { display: none; }
    @media screen and (orientation: portrait) {
      #rotate-msg {
        display: flex; position: fixed; top:0; left:0; width:100%; height:100%;
        background: black; z-index:10000; justify-content:center; align-items:center; color: white;
      }
      .group-left, .group-right, .pos-stop, .pos-kick, .slider-container { display: none; }
    }
  </style>
</head>
<body>

  <div id="rotate-msg"><h1>PUTAR HP 90&deg;</h1></div>
  <button id="fs-btn" onclick="toggleFullScreen()">&#x26F6; FULLSCREEN</button>

  <div class="slider-container">
    <div class="slider-item">
      <label>SPEED: <span id="speedVal">200</span></label>
      <input type="range" min="0" max="255" value="200" oninput="setVal('Speed', this.value)">
    </div>
    <div class="slider-item">
      <label>KICK: <span id="kickVal">200</span></label>
      <input type="range" min="0" max="255" value="200" oninput="setVal('KickPower', this.value)">
    </div>
  </div>

  <div class="group-left">
    <div class="btn-round" onpointerdown="move('L', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9664; L</div>
    <div class="btn-round" onpointerdown="move('B', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9660; B</div>
  </div>

  <div class="group-right">
    <div class="btn-round" onpointerdown="move('F', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9650; F</div>
    <div class="btn-round" onpointerdown="move('R', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9654; R</div>
  </div>

  <div class="pos-kick">
    <div class="btn-rect btn-kick" 
         onpointerdown="sendKick('1', this)" 
         onpointerup="sendKick('0', this)" 
         onpointerleave="sendKick('0', this)">SHOOT</div>
  </div>

  <div class="pos-stop">
    <div class="btn-rect btn-stop" onpointerdown="stop(this)">STOP</div>
  </div>

<script>
  document.addEventListener('contextmenu', event => event.preventDefault());
  
  function toggleFullScreen() {
    var doc = window.document; var docEl = doc.documentElement;
    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen;
    if(!doc.fullscreenElement && requestFullScreen) {
      requestFullScreen.call(docEl);
      document.getElementById('fs-btn').style.display = 'none';
    }
  }

  var lastRequest = 0;
  function sendReq(url, instant) {
    var now = Date.now();
    if (!instant && (now - lastRequest < 30)) return;
    lastRequest = now;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.send();
  }

  function move(dir, btn) {
    btn.setPointerCapture(event.pointerId);
    btn.classList.add('active');
    sendReq("/move?dir=" + dir, true);
  }

  function stop(btn) {
    if(btn) btn.classList.remove('active');
    sendReq("/move?dir=S", true);
  }

  function sendKick(state, btn) {
    if(state == '1') {
      btn.setPointerCapture(event.pointerId);
      btn.classList.add('active');
    } else {
      btn.classList.remove('active');
    }
    sendReq("/kick?state=" + state, true);
  }

  function setVal(type, val) {
    if(type == 'Speed') document.getElementById('speedVal').innerText = val;
    if(type == 'KickPower') document.getElementById('kickVal').innerText = val;
    sendReq("/set" + type + "?val=" + val, false); 
  }
</script>
</body>
</html>
)rawliteral";
void stopRobot() {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
  ledcWrite(pwmChannelA, 0); ledcWrite(pwmChannelB, 0);
}
void moveRobot(String dir) {
  int a1 = LOW, a2 = LOW, b1 = LOW, b2 = LOW;
  int currentPwm = motorSpeed; 

  if (dir == "F") { 
    a1=HIGH; a2=LOW; b1=HIGH; b2=LOW; 
    currentPwm = motorSpeed; 
  }
  else if (dir == "B") { 
    a1=LOW; a2=HIGH; b1=LOW; b2=HIGH; 
    currentPwm = motorSpeed; 
  }
  else if (dir == "L") { 
    a1=LOW; a2=HIGH; b1=HIGH; b2=LOW; 
    currentPwm = 60; // FIX 60
  }
  else if (dir == "R") { 
    a1=HIGH; a2=LOW; b1=LOW; b2=HIGH; 
    currentPwm = 60; // FIX 60
  }

  digitalWrite(AIN1, a1); digitalWrite(AIN2, a2);
  digitalWrite(BIN1, b1); digitalWrite(BIN2, b2);
  ledcWrite(pwmChannelA, currentPwm); 
  ledcWrite(pwmChannelB, currentPwm);
}
void setup() {
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT); pinMode(KICKER_PIN, OUTPUT);
  digitalWrite(STBY, HIGH);

  ledcSetup(pwmChannelA, freq, resolution);
  ledcSetup(pwmChannelB, freq, resolution);
  ledcSetup(pwmChannelKick, freq, resolution);
  ledcAttachPin(PWMA, pwmChannelA);
  ledcAttachPin(PWMB, pwmChannelB);
  ledcAttachPin(KICKER_PIN, pwmChannelKick);

  WiFi.softAP(ssid, password);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.on("/move", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("dir")) {
      String dir = request->getParam("dir")->value();
      if (dir == "S") stopRobot();
      else moveRobot(dir);
    }
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/kick", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      if(state == "1") {
        ledcWrite(pwmChannelKick, kickerPower);
      } else {
        ledcWrite(pwmChannelKick, 0);
      }
    }
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/setSpeed", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("val")) {
      motorSpeed = request->getParam("val")->value().toInt();
    }
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/setKickPower", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("val")) {
      kickerPower = request->getParam("val")->value().toInt();
    }
    request->send(200, "text/plain", "OK");
  });
  
  server.begin();
}

void loop() {}
