// Record & Playback Robotic Arm + Web UI. Serves a web page at 192.168.4.1 with sliders for each servo, Record/Play/Clear.

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

//Network
const char* ap_ssid = "Bhakti";
const char* ap_pass = "123456789"; 

WebServer server(80);

// Servo & pins 
static const int servoPin0 = 14; // Base
static const int servoPin1 = 27; // Shoulder
static const int servoPin2 = 25; // Elbow1
static const int servoPin3 = 32; // Elbow2 

Servo Servo_0;
Servo Servo_1;
Servo Servo_2;
Servo Servo_3;

// Recording storage 
int saved_data[700];
int array_index = 0;

bool recordOn = false; 

const int MIN_ANGLE = 10;
const int MAX_ANGLE = 170;
const int PLAY_DELAY_MS = 80; // delay between steps during playback

// Helper
int clampAngle(int a) {
  if (a < MIN_ANGLE) return MIN_ANGLE;
  if (a > MAX_ANGLE) return MAX_ANGLE;
  return a;
}


// HTML page 
const char MAIN_page[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>TechControl - Record & Play Robot Arm</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Times New Roman', Times, serif, Helvetica, sans-serif; text-align:left; padding:10px; background:#fff; }
    h1 { color:#117A72; text-align:center; }
    .row { display:flex; align-items:center; margin:10px 0; }
    .label { width:110px; font-weight:bold; font-size:18px; }
    input[type=range] { flex:1; -webkit-appearance:none; height:12px; background:#e5e5e5; border-radius:6px; outline:none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance:none; width:28px; height:28px; border-radius:50%; background:#ff4d4d; box-shadow:0 2px 0 rgba(0,0,0,0.25); }
    .val { width:44px; text-align:center; margin-left:8px; font-weight:bold; }
    .btn { display:inline-block; padding:12px 28px; border-radius:26px; color:#fff; text-decoration:none; margin:6px; box-shadow: 0 4px 0 rgba(0,0,0,0.2); cursor:pointer; }
    .btn-red { background:#e53935; }
    .btn-green { background:#2e7d32; }
    .small { font-size:14px; margin-left:6px; vertical-align:middle; }
    #status { margin-top:12px; font-weight:bold; }
  </style>
</head>
<body>
  <h1>TechControl</h1>
  <h3 style="text-align:center;margin-top:-6px;">Record and Play Back Robot Arm</h3>

  <div class="row"><div class="label">Gripper:</div><input id="g0" type="range" min="10" max="170" value="90" oninput="sliderChange(4,this.value)"><div class="val" id="v3"></div></div>
  <div class="row"><div class="label">Elbow1:</div><input id="s2" type="range" min="10" max="170" value="90" oninput="sliderChange(2,this.value)"><div class="val" id="v2"></div></div>
  <div class="row"><div class="label">Elbow2:</div><input id="s3" type="range" min="10" max="170" value="90" oninput="sliderChange(3,this.value)"><div class="val" id="v3"></div></div>
  <div class="row"><div class="label">Shoulder:</div><input id="s1" type="range" min="10" max="170" value="90" oninput="sliderChange(1,this.value)"><div class="val" id="v1"></div></div>
  <div class="row"><div class="label">Base:</div><input id="s0" type="range" min="10" max="170" value="90" oninput="sliderChange(0,this.value)"><div class="val" id="v0"></div></div>
 
  <div style="margin-top:10px;">
    <span class="small">Record</span>
    <button id="recBtn" class="btn btn-red" onclick="toggleRecord()">OFF</button>
    
    <span class="small">Play</span>
    <button id="playBtn" class="btn btn-green" onclick="play()">ON</button>    
  </div>


<script>
  // Send slider updates to server (will fail if no server is running)
  function sliderChange(idx, val) {
    document.getElementById('v'+idx).innerText = val;
    fetch('/set?servo=' + idx + '&pos=' + val).catch(err => console.log('err',err));
    // update saved steps count
    setTimeout(updateStatus, 50);
  }

  // Toggle recording state
  function toggleRecord() {
    let btn = document.getElementById('recBtn');
    const want = (btn.innerText === 'OFF') ? 1 : 0;
    fetch('/record?on=' + want)
      .then(()=> {
        if(want==1){ btn.innerText='ON'; btn.style.background='#b71c1c'; }
        else { btn.innerText='OFF'; btn.style.background='#e53935'; }
        updateStatus();
      }).catch(e=>console.log(e));
  }

  function play() {
    fetch('/play').then(()=> {
      // playback runs on device; optionally poll status if needed
    }).catch(e=>console.log(e));
  }

  // get saved_steps and record state
  function updateStatus() {
    fetch('/status').then(r=>r.json()).then(js=>{
      document.getElementById('status').innerText = 'Saved steps: ' + js.steps + (js.record ? '    (recording)' : '');
      const recBtn = document.getElementById('recBtn');
      if(js.record){ recBtn.innerText='ON'; recBtn.style.background='#b71c1c'; } else { recBtn.innerText='OFF'; recBtn.style.background='#e53935'; }
    }).catch(e=>console.log(e));
  }

  // initialize status every 1s
  setInterval(updateStatus, 1000);
  updateStatus();
</script>
</body>
</html>

)rawliteral";

// HTTP handlers 
void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

// set endpoint -> moves servo and possibly record
void handleSet() {
  if (!server.hasArg("servo") || !server.hasArg("pos")) {
    server.send(400, "text/plain", "Missing params");
    return;
  }
  int servo = server.arg("servo").toInt();
  int pos = server.arg("pos").toInt();
  pos = clampAngle(pos);

  switch (servo) {
    case 0: Servo_0.write(pos); break;
    case 1: Servo_1.write(pos); break;
    case 2: Servo_2.write(pos); break;
    case 3: Servo_3.write(pos); break;
    default:
      server.send(400, "text/plain", "Invalid servo");
      return;
  }

  // If recording: save action in array
  if (recordOn && array_index < (int) (sizeof(saved_data)/sizeof(saved_data[0]))) {
    int code = servo * 1000 + pos;
    saved_data[array_index++] = code;
  }

  server.send(200, "text/plain", "OK");
}

// record endpoint
void handleRecord() {
  if (!server.hasArg("on")) {
    server.send(400, "text/plain", "Missing on param");
    return;
  }
  int on = server.arg("on").toInt();
  recordOn = (on != 0);
  server.send(200, "text/plain", recordOn ? "Recording ON" : "Recording OFF");
}

// clear endpoint
void handleClear() {
  array_index = 0;
  server.send(200, "text/plain", "Cleared");
}

// play endpoint
void handlePlay() {
  // run playback 
  for (int i = 0; i < array_index; ++i) {
    int code = saved_data[i];
    int servo = code / 1000;
    int pos = code % 1000;
    pos = clampAngle(pos);

    switch (servo) {
      case 0: Servo_0.write(pos); break;
      case 1: Servo_1.write(pos); break;
      case 2: Servo_2.write(pos); break;
      case 3: Servo_3.write(pos); break;
    }
    delay(PLAY_DELAY_MS);
  }
  server.send(200, "text/plain", "Playback done");
}

void handleStatus() {
  String payload = String("{\"steps\":") + String(array_index) + String(",\"record\":") + String(recordOn ? "1" : "0") + String("}");
  server.send(200, "application/json", payload);
}

// Setup and loop 
void setup() {
  Serial.begin(115200);
  delay(200);

  // Attach servos 
  Servo_0.attach(servoPin0);
  Servo_1.attach(servoPin1);
  Servo_2.attach(servoPin2);
  Servo_3.attach(servoPin3);

  // Set initial positions
  Servo_0.write(70);
  Servo_1.write(100);
  Servo_2.write(110);
  Servo_3.write(10);

  // Start Wi-Fi Access Point
  Serial.println("Starting AP...");
  WiFi.softAP(ap_ssid, ap_pass);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); 

  // Setup server routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);     // servo control & record
  server.on("/record", handleRecord);
  server.on("/clear", handleClear);
  server.on("/play", handlePlay);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // handle client requests
  server.handleClient();
}
