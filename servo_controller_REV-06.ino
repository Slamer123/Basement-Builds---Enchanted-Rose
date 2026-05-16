#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// WiFi credentials
const char* ssid = "Beauty-and-the-Beast";
const char* password = "12345678";  // Change this to your desired password (min 8 chars)

// Create PCA9685 object
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// DNS server for captive portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Servo configuration
#define SERVOMIN  102  // Minimum pulse length count (out of 4096)
#define SERVOMAX  512  // Maximum pulse length count (out of 4096)
#define NUM_SERVOS 6
#define MIN_SERVO_ANGLE 5    // Minimum servo angle
#define MAX_SERVO_ANGLE 175  // Maximum servo angle

// Servo states
int servoPositions[NUM_SERVOS] = {MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE};  // Current positions
bool servoStates[NUM_SERVOS] = {false, false, false, false, false, false};  // false=MIN, true=MAX
bool servoReversed[NUM_SERVOS] = {false, false, false, false, false, false};  // Reverse flag

WebServer server(80);

// Convert angle to pulse width
int angleToPulse(int angle) {
  return map(angle, MIN_SERVO_ANGLE, MAX_SERVO_ANGLE, SERVOMIN, SERVOMAX);
}

// Move servo to angle
void moveServo(int servoNum, int angle) {
  if (servoNum < 0 || servoNum >= NUM_SERVOS) return;
  
  int actualAngle = angle;
  if (servoReversed[servoNum]) {
    // Reverse the angle within the defined range
    actualAngle = MAX_SERVO_ANGLE - (angle - MIN_SERVO_ANGLE);
  }
  
  servoPositions[servoNum] = angle;
  pwm.setPWM(servoNum, 0, angleToPulse(actualAngle));
}

// Slowly move servo to target angle (for smooth motion)
void moveServoSlow(int servoNum, int targetAngle, int totalTimeMs = 1000) {
  if (servoNum < 0 || servoNum >= NUM_SERVOS) return;
  
  int currentPos = servoPositions[servoNum];
  int distance = abs(targetAngle - currentPos);
  
  if (distance == 0) return;  // Already at target
  
  int delayPerDegree = totalTimeMs / distance;  // Time per degree in ms
  
  if (currentPos < targetAngle) {
    // Move up
    for (int pos = currentPos; pos <= targetAngle; pos++) {
      moveServo(servoNum, pos);
      delay(delayPerDegree);
    }
  } else if (currentPos > targetAngle) {
    // Move down
    for (int pos = currentPos; pos >= targetAngle; pos--) {
      moveServo(servoNum, pos);
      delay(delayPerDegree);
    }
  }
}

// HTML page with embedded CSS and JavaScript
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>Servo Controller</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      min-height: 100vh;
      padding: 20px;
      color: #e0e0e0;
    }
    
    .container {
      max-width: 800px;
      margin: 0 auto;
    }
    
    h1 {
      text-align: center;
      color: white;
      margin-bottom: 30px;
      font-size: 28px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }
    
    .servo-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 15px;
      margin-bottom: 15px;
    }
    
    .servo-card {
      background: #2d2d44;
      border-radius: 15px;
      padding: 15px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    
    .servo-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
    }
    
    .servo-title {
      font-size: 20px;
      font-weight: 600;
      color: #7c8cf7;
    }
    
    .position-indicator {
      font-size: 16px;
      color: #b0b0b0;
      font-weight: 500;
    }
    
    .controls {
      display: flex;
      flex-direction: column;
      gap: 10px;
    }
    
    /* Toggle Switch */
    .toggle-container {
      display: flex;
      align-items: center;
      gap: 10px;
    }
    
    .toggle-label {
      font-size: 14px;
      color: #b0b0b0;
      font-weight: 500;
      white-space: nowrap;
    }
    
    .toggle-switch {
      position: relative;
      display: inline-block;
      width: 90px;
      height: 44px;
      background-color: #1a1a2e;
      border-radius: 22px;
      cursor: pointer;
      transition: background-color 0.3s;
      flex-shrink: 0;
    }
    
    .toggle-switch.active {
      background-color: #7c8cf7;
    }
    
    .toggle-slider {
      position: absolute;
      top: 4px;
      left: 4px;
      width: 36px;
      height: 36px;
      background-color: white;
      border-radius: 50%;
      transition: transform 0.3s;
      box-shadow: 0 2px 4px rgba(0,0,0,0.2);
    }
    
    .toggle-switch.active .toggle-slider {
      transform: translateX(46px);
    }
    
    /* Reset All Button */
    .reset-all-btn {
      width: 100%;
      padding: 18px;
      background-color: #ff6b6b;
      color: white;
      border: none;
      border-radius: 15px;
      font-size: 18px;
      font-weight: 600;
      cursor: pointer;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
      transition: all 0.3s;
      margin-top: 10px;
    }
    
    .reset-all-btn:active {
      transform: scale(0.98);
      box-shadow: 0 2px 4px rgba(0,0,0,0.2);
    }
    
    @media (max-width: 480px) {
      .servo-grid {
        grid-template-columns: 1fr;
      }
      
      h1 {
        font-size: 24px;
      }
      
      .servo-title {
        font-size: 18px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎭 Beauty and the Beast</h1>
    
    <div class="servo-grid" id="servos"></div>
    
    <button class="reset-all-btn" onclick="resetAll()">Reset All Servos</button>
  </div>

  <script>
    const NUM_SERVOS = 6;
    const MIN_SERVO_ANGLE = 5;
    const MAX_SERVO_ANGLE = 175;
    
    let servoStates = [false, false, false, false, false, false];
    let servoReversed = [false, false, false, false, false, false];
    let servoPositions = [MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE, MIN_SERVO_ANGLE];
    
    // Generate servo cards
    function generateServoCards() {
      const container = document.getElementById('servos');
      for (let i = 0; i < NUM_SERVOS; i++) {
        const card = document.createElement('div');
        card.className = 'servo-card';
        card.innerHTML = `
          <div class="servo-header">
            <span class="servo-title">Servo ${i + 1}</span>
            <span class="position-indicator" id="pos-${i}">${MIN_SERVO_ANGLE}°</span>
          </div>
          <div class="controls">
            <div class="toggle-container">
              <label class="toggle-label">Position</label>
              <div class="toggle-switch" id="toggle-${i}" onclick="toggleServo(${i})">
                <div class="toggle-slider"></div>
              </div>
            </div>
          </div>
        `;
        container.appendChild(card);
      }
    }
    
    function toggleServo(servoNum) {
      servoStates[servoNum] = !servoStates[servoNum];
      const newPos = servoStates[servoNum] ? MAX_SERVO_ANGLE : MIN_SERVO_ANGLE;
      
      // Update UI immediately for instant feedback
      updateUI(servoNum, newPos, servoReversed[servoNum]);
      
      // Send command to servo (happens in background)
      fetch(`/servo?num=${servoNum}&pos=${newPos}`)
        .then(response => response.json())
        .then(data => {
          // Confirm with actual position from server
          updateUI(servoNum, data.position, data.reversed);
        })
        .catch(error => {
          console.error('Error:', error);
        });
    }
    
    function resetAll() {
      // Update all UI immediately
      for (let i = 0; i < NUM_SERVOS; i++) {
        servoStates[i] = false;
        servoReversed[i] = false;
        updateUI(i, MIN_SERVO_ANGLE, false);
      }
      
      // Send reset command
      fetch('/reset')
        .then(response => response.json())
        .then(data => {
          console.log('Reset complete');
        })
        .catch(error => {
          console.error('Error:', error);
        });
    }
    
    function updateUI(servoNum, position, reversed) {
      servoPositions[servoNum] = position;
      servoReversed[servoNum] = reversed;
      
      // Update position indicator
      document.getElementById(`pos-${servoNum}`).textContent = position + '°';
      
      // Update toggle switch (check if closer to MAX than MIN)
      const toggle = document.getElementById(`toggle-${servoNum}`);
      const midpoint = (MIN_SERVO_ANGLE + MAX_SERVO_ANGLE) / 2;
      if (position > midpoint) {
        toggle.classList.add('active');
        servoStates[servoNum] = true;
      } else {
        toggle.classList.remove('active');
        servoStates[servoNum] = false;
      }
    }
    
    // Initialize on load
    window.onload = function() {
      generateServoCards();
      
      // Get initial states from server
      fetch('/status')
        .then(response => {
          if (!response.ok) {
            throw new Error('Status request failed');
          }
          return response.json();
        })
        .then(data => {
          console.log('Initial status received:', data);
          for (let i = 0; i < NUM_SERVOS; i++) {
            updateUI(i, data.positions[i], data.reversed[i]);
          }
        })
        .catch(error => {
          console.error('Error loading initial status:', error);
          // Fallback: initialize all servos to MIN position
          for (let i = 0; i < NUM_SERVOS; i++) {
            updateUI(i, MIN_SERVO_ANGLE, false);
          }
        });
    };
  </script>
</body>
</html>
)rawliteral";

// Handle root request
void handleRoot() {
  server.send(200, "text/html", index_html);
}

// Handle servo control
void handleServo() {
  if (server.hasArg("num") && server.hasArg("pos")) {
    int servoNum = server.arg("num").toInt();
    int position = server.arg("pos").toInt();
    
    if (servoNum >= 0 && servoNum < NUM_SERVOS && position >= MIN_SERVO_ANGLE && position <= MAX_SERVO_ANGLE) {
      moveServoSlow(servoNum, position, 1000);  // Move smoothly over 1 second
      
      String json = "{\"success\":true,\"servo\":" + String(servoNum) + 
                    ",\"position\":" + String(servoPositions[servoNum]) + 
                    ",\"reversed\":" + String(servoReversed[servoNum] ? "true" : "false") + "}";
      server.send(200, "application/json", json);
      return;
    }
  }
  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid parameters\"}");
}

// Handle reverse toggle
void handleReverse() {
  if (server.hasArg("num")) {
    int servoNum = server.arg("num").toInt();
    
    if (servoNum >= 0 && servoNum < NUM_SERVOS) {
      servoReversed[servoNum] = !servoReversed[servoNum];
      // Re-apply current position with new reverse state
      moveServo(servoNum, servoPositions[servoNum]);
      
      String json = "{\"success\":true,\"servo\":" + String(servoNum) + 
                    ",\"position\":" + String(servoPositions[servoNum]) + 
                    ",\"reversed\":" + String(servoReversed[servoNum] ? "true" : "false") + "}";
      server.send(200, "application/json", json);
      return;
    }
  }
  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid parameters\"}");
}

// Handle reset all
void handleReset() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    servoStates[i] = false;
    servoReversed[i] = false;
    moveServoSlow(i, MIN_SERVO_ANGLE, 1000);  // Move smoothly over 1 second
  }
  server.send(200, "application/json", "{\"success\":true}");
}

// Handle status request
void handleStatus() {
  Serial.println("Status request received");
  
  String json = "{\"positions\":[";
  for (int i = 0; i < NUM_SERVOS; i++) {
    json += String(servoPositions[i]);
    if (i < NUM_SERVOS - 1) json += ",";
  }
  json += "],\"reversed\":[";
  for (int i = 0; i < NUM_SERVOS; i++) {
    json += servoReversed[i] ? "true" : "false";
    if (i < NUM_SERVOS - 1) json += ",";
  }
  json += "]}";
  
  Serial.print("Sending status: ");
  Serial.println(json);
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nStarting Beauty and the Beast Servo Controller...");
  
  // Initialize I2C with XIAO ESP32-C3 pins
  Wire.begin(6, 7);  // SDA=GPIO6, SCL=GPIO7 for XIAO ESP32-C3
  
  // Initialize PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz
  delay(10);
  
  // Initialize all servos to MIN angle
  // Send command directly - if already at MIN, they won't move
  Serial.println("Initializing servos to minimum angle...");
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    moveServo(i, MIN_SERVO_ANGLE);  // Move directly to MIN position
    delay(100);
  }
  
  Serial.println("Servo initialization complete!");
  
  // Create WiFi Access Point
  Serial.println("Creating WiFi Access Point...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.println("Connect to WiFi: Beauty-and-the-Beast");
  Serial.println("Captive portal will open automatically!");
  
  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", IP);
  Serial.println("DNS server started for captive portal");
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.on("/reverse", handleReverse);
  server.on("/reset", handleReset);
  server.on("/status", handleStatus);
  server.onNotFound(handleRoot);  // Redirect all unknown requests to root
  
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  dnsServer.processNextRequest();  // Handle DNS requests for captive portal
  server.handleClient();
}
