#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#define AIN1 26
#define AIN2 25
#define PWMA 33 
#define BIN1 14
#define BIN2 27
#define PWMB 32
#define STBY 12
#define KICKER_PIN 13 
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
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="theme-color" content="#1a1a1a">
  <title>CrevoBot Controller</title>
  <style>
    body {
      background-color: #1a1a1a; color: white;
      font-family: 'Courier New', Courier, monospace;
      margin: 0; padding: 0;
      display: flex; height: 100vh; width: 100vw;
      overflow: hidden;
      user-select: none; -webkit-user-select: none;
      touch-action: none;
    }
    #fs-btn {
      position: absolute; top: 15px; left: 15px; z-index: 9999;
      background: rgba(255, 0, 0, 0.7); 
      border: 2px solid white; border-radius: 8px;
      color: white; padding: 8px 12px;
      font-weight: bold; font-size: 14px;
      cursor: pointer;
    }
    .left-panel {
      width: 40%; height: 100%;
      display: flex; flex-direction: column;
      justify-content: center; align-items: center;
      border-right: 2px solid #444;
      background: #222;
    }
    .right-panel {
      width: 60%; height: 100%;
      display: flex; justify-content: center; align-items: center;
      background: #1a1a1a;
    }
    .slider-box { width: 90%; text-align: center; margin-bottom: 20px; }
    input[type=range] { width: 100%; height: 20px; accent-color: #00cec9; cursor: pointer; }
    label { font-size: 16px; color: #00cec9; font-weight: bold; display: block; margin-bottom: 5px; }
    .kick-btn {
      width: 90%; height: 80px;
      background: #c0392b; border: 2px solid white; border-radius: 12px;
      color: white; font-size: 24px; font-weight: bold;
      box-shadow: 0 6px #7f261d; cursor: pointer;
      margin-top: 10px;
    }
    .kick-btn:active, .kick-btn.active { 
      transform: translateY(4px); box-shadow: 0 2px #7f261d; background: #e74c3c; 
    }
    .d-pad {
      display: grid;
      grid-template-columns: 90px 90px 90px;
      grid-template-rows: 90px 90px 90px;
      gap: 15px;
    }
    .btn {
      width: 100%; height: 100%;
      background: #27ae60; border: none; border-radius: 50%;
      color: white; font-size: 35px;
      box-shadow: 0 6px #1e8449; cursor: pointer;
      display: flex; justify-content: center; align-items: center;
    }
    .btn:active, .btn.active {
      transform: scale(0.95); box-shadow: 0 2px #1e8449; background: #2ecc71;
    }
    .btn-stop { 
      background: #f39c12; border-radius: 12px;
      font-size: 20px; font-weight: bold; box-shadow: 0 6px #d68910; 
    }
    .btn-stop:active { background: #f1c40f; box-shadow: 0 2px #d68910; }
    #debug-box {
      position: absolute; bottom: 5px; left: 5px; z-index: 100;
      font-size: 12px; color: yellow; pointer-events: none;
      background: rgba(0,0,0,0.6); padding: 4px 8px; border-radius: 4px;
    }
    #rotate-msg { display: none; }
    @media screen and (orientation: portrait) {
      #rotate-msg {
        display: flex; position: fixed; top:0; left:0; width:100%; height:100%;
        background: black; z-index:10000; 
        justify-content:center; align-items:center; flex-direction: column;
        color: white; text-align: center;
      }
      .left-panel, .right-panel, #fs-btn { display: none; }
    }
  </style>
</head>
<body>
  <div id="rotate-msg"><h1>⚠️</h1><h2>PYE TO BOSSS</h2><p>Landscape Penak ki lhoo</p></div>
  <button id="fs-btn" onclick="toggleFullScreen()">&#x26F6; FULLSCREEN</button>
  <div id="debug-box">Status: READY</div>
  <div class="left-panel">
    <div class="slider-box">
      <label>PWM MOTOR: <span id="speedVal">200</span></label>
      <input type="range" min="0" max="255" value="200" oninput="setVal('Speed', this.value)">
    </div>
    <div class="slider-box">
      <label>PWM KICKER: <span id="kickVal">200</span></label>
      <input type="range" min="0" max="255" value="200" oninput="setVal('KickPower', this.value)">
    </div>
    <button class="kick-btn" 
            onpointerdown="sendKick('1', this)" 
            onpointerup="sendKick('0', this)" 
            onpointerleave="sendKick('0', this)">TENDANGG NDASEE</button>
  </div>
  <div class="right-panel">
    <div class="d-pad">
      <div></div>
      <button class="btn" onpointerdown="move('F', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9650;</button>
      <div></div>
      <button class="btn" onpointerdown="move('L', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9664;</button>
      <button class="btn btn-stop" onpointerdown="stop(this)">STOP</button>
      <button class="btn" onpointerdown="move('R', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9654;</button>
      <div></div>
      <button class="btn" onpointerdown="move('B', this)" onpointerup="stop(this)" onpointerleave="stop(this)">&#9660;</button>
      <div></div>
    </div>
  </div>
<script>
  document.addEventListener('contextmenu', event => event.preventDefault());
  function log(msg) {
    document.getElementById('debug-box').innerText = "Last Action: " + msg;
  }
  function toggleFullScreen() {
    var doc = window.document;
    var docEl = doc.documentElement;
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
    log("MOVE " + dir);
    sendReq("/move?dir=" + dir, true);
  }
  function stop(btn) {
    if(btn) btn.classList.remove('active');
    log("STOP");
    sendReq("/move?dir=S", true);
  }
  function sendKick(state, btn) {
    if(state == '1') {
      btn.setPointerCapture(event.pointerId);
      btn.classList.add('active');
      log("KICK ON");
    } else {
      btn.classList.remove('active');
      log("KICK OFF");
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

  if (dir == "F") { a1=HIGH; a2=LOW; b1=HIGH; b2=LOW; }
  else if (dir == "B") { a1=LOW; a2=HIGH; b1=LOW; b2=HIGH; }
  else if (dir == "L") { a1=LOW; a2=HIGH; b1=HIGH; b2=LOW; }
  else if (dir == "R") { a1=HIGH; a2=LOW; b1=LOW; b2=HIGH; }

  digitalWrite(AIN1, a1); digitalWrite(AIN2, a2);
  digitalWrite(BIN1, b1); digitalWrite(BIN2, b2);
  ledcWrite(pwmChannelA, motorSpeed); 
  ledcWrite(pwmChannelB, motorSpeed);
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
