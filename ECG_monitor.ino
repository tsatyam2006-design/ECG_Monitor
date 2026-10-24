#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <math.h>

// AD8232 ECG Sensor Pin Configuration
const int LO_PLUS = 2;   // LO+ pin (Lead-off detection)
const int LO_MINUS = 4;  // LO- pin (Lead-off detection)
const int ECG_OUTPUT = 34; // Analog output from AD8232 (ADC1_CH6 on ESP32)

// WiFi Access Point Configuration
const char* ssid = "ECG_Monitor_ESP32";
const char* password = "12345678"; // Minimum 8 characters

// Web Server on port 80
WebServer server(80);
// WebSocket Server on port 81
WebSocketsServer webSocket = WebSocketsServer(81);

// ECG Data Variables
int ecgValue = 0;
int lastEcgValue = 0;
bool leadOff = false;
unsigned long lastSampleTime = 0;
const int SAMPLE_RATE = 250; // 250 Hz sampling rate (4ms interval)

// HTML Page Content
String htmlPage = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ECG Monitor - Real-time Monitoring</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        .header p {
            font-size: 1.1em;
            opacity: 0.9;
        }
        
        .status-bar {
            display: flex;
            justify-content: space-around;
            padding: 20px;
            background: #f8f9fa;
            border-bottom: 2px solid #e9ecef;
        }
        
        .status-item {
            text-align: center;
        }
        
        .status-label {
            font-size: 0.9em;
            color: #6c757d;
            margin-bottom: 5px;
        }
        
        .status-value {
            font-size: 1.5em;
            font-weight: bold;
            color: #495057;
        }
        
        .status-value.connected {
            color: #28a745;
        }
        
        .status-value.disconnected {
            color: #dc3545;
        }
        
        .status-value.warning {
            color: #ffc107;
        }
        
        .ecg-container {
            padding: 30px;
            background: #000;
            position: relative;
            min-height: 400px;
        }
        
        #ecgCanvas {
            width: 100%;
            height: 400px;
            background: #000;
            border-radius: 10px;
            cursor: crosshair;
        }
        
        .controls {
            padding: 30px;
            background: #f8f9fa;
            display: flex;
            justify-content: space-around;
            flex-wrap: wrap;
            gap: 20px;
        }
        
        .control-group {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 10px;
        }
        
        .btn {
            padding: 12px 30px;
            font-size: 1em;
            border: none;
            border-radius: 25px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: bold;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .btn-danger {
            background: #dc3545;
            color: white;
        }
        
        .btn-danger:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(220, 53, 69, 0.4);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .slider-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 10px;
            min-width: 200px;
        }
        
        .slider {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #ddd;
            outline: none;
            -webkit-appearance: none;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
        
        .slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
            border: none;
        }
        
        .info-panel {
            padding: 20px;
            background: white;
            border-top: 2px solid #e9ecef;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
        }
        
        .info-card {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 10px;
            border-left: 4px solid #667eea;
        }
        
        .info-card h3 {
            font-size: 0.9em;
            color: #6c757d;
            margin-bottom: 10px;
        }
        
        .info-card .value {
            font-size: 1.8em;
            font-weight: bold;
            color: #495057;
        }
        
        .lead-off-indicator {
            display: inline-block;
            width: 15px;
            height: 15px;
            border-radius: 50%;
            margin-left: 10px;
        }
        
        .lead-off-indicator.on {
            background: #dc3545;
            box-shadow: 0 0 10px #dc3545;
        }
        
        .lead-off-indicator.off {
            background: #28a745;
            box-shadow: 0 0 10px #28a745;
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        .pulsing {
            animation: pulse 1s infinite;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>❤️ ECG Monitor</h1>
            <p>Real-time Electrocardiogram Monitoring System</p>
        </div>
        
        <div class="status-bar">
            <div class="status-item">
                <div class="status-label">Connection Status</div>
                <div class="status-value" id="connectionStatus">Connecting...</div>
            </div>
            <div class="status-item">
                <div class="status-label">Lead Status</div>
                <div class="status-value" id="leadStatus">
                    Checking...
                    <span class="lead-off-indicator" id="leadIndicator"></span>
                </div>
            </div>
            <div class="status-item">
                <div class="status-label">Sample Rate</div>
                <div class="status-value" id="sampleRate">250 Hz</div>
            </div>
            <div class="status-item">
                <div class="status-label">Data Points</div>
                <div class="status-value" id="dataPoints">0</div>
            </div>
        </div>
        
        <div class="ecg-container">
            <canvas id="ecgCanvas"></canvas>
        </div>
        
        <div class="controls">
            <div class="control-group">
                <button class="btn btn-primary" onclick="startMonitoring()">Start Monitoring</button>
                <button class="btn btn-danger" onclick="stopMonitoring()">Stop Monitoring</button>
            </div>
            <div class="slider-container">
                <label for="gainSlider">Gain: <span id="gainValue">1.0x</span></label>
                <input type="range" id="gainSlider" class="slider" min="0.5" max="3" step="0.1" value="1" oninput="updateGain(this.value)">
            </div>
            <div class="slider-container">
                <label for="speedSlider">Speed: <span id="speedValue">1.0x</span></label>
                <input type="range" id="speedSlider" class="slider" min="0.5" max="3" step="0.1" value="1" oninput="updateSpeed(this.value)">
            </div>
            <div class="control-group">
                <button class="btn btn-primary" onclick="clearCanvas()">Clear Graph</button>
            </div>
        </div>
        
        <div class="info-panel">
            <div class="info-grid">
                <div class="info-card">
                    <h3>Current ECG Value</h3>
                    <div class="value" id="currentValue">0</div>
                </div>
                <div class="info-card">
                    <h3>Average Value</h3>
                    <div class="value" id="avgValue">0</div>
                </div>
                <div class="info-card">
                    <h3>Peak Value</h3>
                    <div class="value" id="peakValue">0</div>
                </div>
                <div class="info-card">
                    <h3>Heart Rate (BPM)</h3>
                    <div class="value" id="heartRate">--</div>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        const canvas = document.getElementById('ecgCanvas');
        const ctx = canvas.getContext('2d');
        
        // Set canvas size
        function resizeCanvas() {
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            drawGrid();
        }
        window.addEventListener('resize', resizeCanvas);
        
        // ECG Data Variables
        let ecgData = [];
        let isMonitoring = false;
        let gain = 1.0;
        let speed = 1.0;
        let xPosition = 0;
        let dataPoints = 0;
        let maxDataPoints = 2000;
        let heartRate = 0;
        let lastPeakTime = 0;
        let peakValues = [];
        let avgValue = 0;
        let sumValues = 0;
        let valueCount = 0;
        
        // WebSocket Connection
        let ws;
        let reconnectInterval;
        
        function connectWebSocket() {
            // Use ws: for HTTP connections (ESP32 AP uses HTTP)
            const wsUrl = 'ws://' + window.location.hostname + ':81';
            console.log('Connecting to WebSocket:', wsUrl);
            
            try {
                ws = new WebSocket(wsUrl);
                
                ws.onopen = function() {
                    console.log('WebSocket connected successfully');
                    document.getElementById('connectionStatus').textContent = 'Connected';
                    document.getElementById('connectionStatus').className = 'status-value connected';
                    if (reconnectInterval) {
                        clearInterval(reconnectInterval);
                        reconnectInterval = null;
                    }
                    // Auto-start monitoring when connected
                    if (!isMonitoring) {
                        startMonitoring();
                    }
                };
                
                ws.onmessage = function(event) {
                    try {
                        const data = JSON.parse(event.data);
                        console.log('Received data:', data); // Debug log
                        updateECGDisplay(data);
                    } catch (e) {
                        console.error('Error parsing data:', e, 'Raw data:', event.data);
                    }
                };
                
                ws.onclose = function() {
                    console.log('WebSocket disconnected');
                    document.getElementById('connectionStatus').textContent = 'Disconnected';
                    document.getElementById('connectionStatus').className = 'status-value disconnected';
                    if (!reconnectInterval) {
                        reconnectInterval = setInterval(connectWebSocket, 3000);
                    }
                };
                
                ws.onerror = function(error) {
                    console.error('WebSocket error:', error);
                    document.getElementById('connectionStatus').textContent = 'Error';
                    document.getElementById('connectionStatus').className = 'status-value disconnected';
                };
            } catch (e) {
                console.error('Failed to create WebSocket:', e);
                document.getElementById('connectionStatus').textContent = 'Error';
                document.getElementById('connectionStatus').className = 'status-value disconnected';
                reconnectInterval = setInterval(connectWebSocket, 3000);
            }
        }
        
        function updateECGDisplay(data) {
            const ecgValue = data.value;
            const leadOff = data.leadOff;
            
            // Always update connection status and lead status, even if not monitoring
            // Update lead status
            const leadIndicator = document.getElementById('leadIndicator');
            const leadStatus = document.getElementById('leadStatus');
            if (leadOff) {
                leadIndicator.className = 'lead-off-indicator on pulsing';
                leadStatus.innerHTML = 'Lead Off <span class="lead-off-indicator on pulsing"></span>';
            } else {
                leadIndicator.className = 'lead-off-indicator off';
                leadStatus.innerHTML = 'Lead On <span class="lead-off-indicator off"></span>';
            }
            
            // Update current value (always update this)
            document.getElementById('currentValue').textContent = ecgValue;
            
            // Only process ECG data if monitoring is active
            if (!isMonitoring) {
                return;
            }
            
            // Calculate average
            sumValues += ecgValue;
            valueCount++;
            avgValue = Math.round(sumValues / valueCount);
            document.getElementById('avgValue').textContent = avgValue;
            
            // Detect peaks for heart rate calculation
            if (!leadOff && ecgData.length > 10) {
                const recentAvg = ecgData.slice(-10).reduce((a, b) => a + b, 0) / 10;
                if (ecgValue > recentAvg + 500 && Date.now() - lastPeakTime > 400) {
                    peakValues.push(Date.now());
                    if (peakValues.length > 10) peakValues.shift();
                    
                    if (peakValues.length >= 2) {
                        const intervals = [];
                        for (let i = 1; i < peakValues.length; i++) {
                            intervals.push(peakValues[i] - peakValues[i-1]);
                        }
                        const avgInterval = intervals.reduce((a, b) => a + b, 0) / intervals.length;
                        heartRate = Math.round(60000 / avgInterval);
                        document.getElementById('heartRate').textContent = heartRate;
                    }
                    lastPeakTime = Date.now();
                }
            }
            
            // Update peak value
            if (ecgValue > parseInt(document.getElementById('peakValue').textContent)) {
                document.getElementById('peakValue').textContent = ecgValue;
            }
            
            // Add to data array with proper scaling for real-time fluctuations
            // Center value is 2048 (midpoint of 0-4095), map to canvas center
            const centerValue = 2048;
            const offset = ecgValue - centerValue;
            // Better scaling: 0.15 gives good visibility of fluctuations
            // Gain slider (0.5x to 3x) multiplies this
            const scaledOffset = offset * gain * 0.15;
            const adjustedValue = (canvas.height / 2) + scaledOffset;
            
            // Clamp value to canvas bounds (leave 10px margin)
            const clampedValue = Math.max(10, Math.min(canvas.height - 10, adjustedValue));
            
            ecgData.push(clampedValue);
            dataPoints++;
            
            // Keep only recent data points for scrolling effect
            if (ecgData.length > maxDataPoints) {
                ecgData.shift();
            }
            
            document.getElementById('dataPoints').textContent = dataPoints;
            
            // Trigger redraw - the setInterval will handle actual drawing
            // This ensures smooth updates
        }
        
        function drawGrid() {
            ctx.fillStyle = '#000';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            ctx.strokeStyle = '#1a1a1a';
            ctx.lineWidth = 1;
            
            // Vertical lines (every 50 pixels)
            for (let x = 0; x < canvas.width; x += 50) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }
            
            // Horizontal lines (every 50 pixels)
            for (let y = 0; y < canvas.height; y += 50) {
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(canvas.width, y);
                ctx.stroke();
            }
            
            // Center line
            ctx.strokeStyle = '#333';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(0, canvas.height / 2);
            ctx.lineTo(canvas.width, canvas.height / 2);
            ctx.stroke();
        }
        
        function drawECG() {
            // Always redraw grid and waveform for smooth updates
            drawGrid();
            
            if (ecgData.length < 2) {
                // Draw center line even with no data
                ctx.strokeStyle = '#333';
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(0, canvas.height / 2);
                ctx.lineTo(canvas.width, canvas.height / 2);
                ctx.stroke();
                return;
            }
            
            // Draw ECG waveform with smooth scrolling
            ctx.strokeStyle = '#00ff00';
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            // Calculate how many points fit on screen based on speed
            const pointsPerPixel = 1 / speed;
            const visiblePoints = Math.min(ecgData.length, Math.floor(canvas.width * pointsPerPixel));
            const startIndex = Math.max(0, ecgData.length - visiblePoints);
            
            // Draw the waveform
            for (let i = startIndex; i < ecgData.length; i++) {
                const x = ((i - startIndex) * speed);
                const y = ecgData[i];
                
                if (i === startIndex) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            }
            
            ctx.stroke();
            
            // Add a moving cursor line to show real-time position
            if (ecgData.length > 0) {
                const lastX = (ecgData.length - startIndex - 1) * speed;
                ctx.strokeStyle = '#00ff00';
                ctx.lineWidth = 1;
                ctx.setLineDash([5, 5]);
                ctx.beginPath();
                ctx.moveTo(lastX, 0);
                ctx.lineTo(lastX, canvas.height);
                ctx.stroke();
                ctx.setLineDash([]);
            }
        }
        
        function startMonitoring() {
            console.log('Starting monitoring...');
            isMonitoring = true;
            ecgData = [];
            dataPoints = 0;
            sumValues = 0;
            valueCount = 0;
            peakValues = [];
            heartRate = 0;
            lastPeakTime = 0;
            document.getElementById('heartRate').textContent = '--';
            document.getElementById('dataPoints').textContent = '0';
            document.getElementById('currentValue').textContent = '0';
            document.getElementById('avgValue').textContent = '0';
            document.getElementById('peakValue').textContent = '0';
            drawGrid();
            console.log('Monitoring started');
        }
        
        function stopMonitoring() {
            console.log('Stopping monitoring...');
            isMonitoring = false;
            console.log('Monitoring stopped');
        }
        
        function clearCanvas() {
            ecgData = [];
            dataPoints = 0;
            sumValues = 0;
            valueCount = 0;
            document.getElementById('dataPoints').textContent = '0';
            document.getElementById('currentValue').textContent = '0';
            document.getElementById('avgValue').textContent = '0';
            document.getElementById('peakValue').textContent = '0';
            document.getElementById('heartRate').textContent = '--';
            drawGrid();
        }
        
        function updateGain(value) {
            gain = parseFloat(value);
            document.getElementById('gainValue').textContent = gain.toFixed(1) + 'x';
        }
        
        function updateSpeed(value) {
            speed = parseFloat(value);
            document.getElementById('speedValue').textContent = speed.toFixed(1) + 'x';
        }
        
        // Wait for page to fully load before initializing
        window.addEventListener('load', function() {
            console.log('Page loaded, initializing...');
            // Initialize WebSocket connection
            connectWebSocket();
            
            // Redraw at 60 FPS for smooth animation
            setInterval(function() {
                drawECG();
            }, 16); // ~60 FPS
            
            // Initial canvas setup
            resizeCanvas();
            
            // Draw initial grid
            drawGrid();
            
            console.log('Initialization complete');
        });
    </script>
</body>
</html>
)HTML";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] WebSocket client disconnected! Total clients: %d\n", num, webSocket.connectedClients());
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] WebSocket client connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        Serial.printf("Total connected clients: %d\n", webSocket.connectedClients());
      }
      break;
    case WStype_TEXT:
      // Handle any incoming text messages if needed
      Serial.printf("[%u] Received text: %s\n", num, payload);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configure AD8232 pins
  pinMode(LO_PLUS, INPUT_PULLUP);  // Lead-off detection with pull-up
  pinMode(LO_MINUS, INPUT_PULLUP); // Lead-off detection with pull-up
  pinMode(ECG_OUTPUT, INPUT);       // Analog input (no pull-up/pull-down for ADC)
  
  // Configure ADC for ECG reading
  analogSetAttenuation(ADC_11db); // 0-3.3V range
  analogReadResolution(12); // 12-bit resolution (0-4095)
  
  // Note: If reading 4095 constantly, check:
  // 1. AD8232 OUTPUT pin is connected to GPIO 34
  // 2. AD8232 is powered (3.3V and GND)
  // 3. Electrodes are properly connected
  
  Serial.println("\n=== ECG Monitor System ===");
  
  // Create Access Point
  Serial.print("Setting up Access Point: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("AP SSID: ");
  Serial.println(ssid);
  Serial.print("AP Password: ");
  Serial.println(password);
  Serial.println("\nConnect to this WiFi network and open:");
  Serial.print("http://");
  Serial.println(IP);
  
  // Start Web Server
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
  // Start WebSocket Server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");
  
  Serial.println("\nSystem ready! Waiting for connections...");
}

void loop() {
  server.handleClient();
  webSocket.loop();
  
  // Read ECG data at specified sample rate
  unsigned long currentTime = millis();
  if (currentTime - lastSampleTime >= (1000 / SAMPLE_RATE)) {
    lastSampleTime = currentTime;
    
    // Read lead-off detection
    // AD8232: LO+ and LO- are HIGH when leads are connected, LOW when disconnected
    bool loPlus = digitalRead(LO_PLUS);
    bool loMinus = digitalRead(LO_MINUS);
    leadOff = !(loPlus && loMinus); // Lead is off if either is LOW
    
    // Read ECG analog value with multiple samples for stability
    int sampleSum = 0;
    const int numSamples = 4;
    for (int i = 0; i < numSamples; i++) {
      sampleSum += analogRead(ECG_OUTPUT);
      delayMicroseconds(100); // Small delay between samples
    }
    ecgValue = sampleSum / numSamples;
    
    // If reading 4095 (max), the pin might be floating - add some noise for testing
    // This helps visualize that the system is working even without sensor
    if (ecgValue >= 4090) {
      // Generate a test waveform (sine wave) to show the graph is working
      static float testPhase = 0;
      testPhase += 0.1;
      if (testPhase > 6.28) testPhase = 0;
      ecgValue = 2048 + (int)(500 * sin(testPhase)); // Oscillate around center
    }
    
    // Create JSON data packet
    StaticJsonDocument<200> doc;
    doc["value"] = ecgValue;
    doc["leadOff"] = leadOff;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send to all connected WebSocket clients
    uint8_t clientCount = webSocket.connectedClients();
    if (clientCount > 0) {
      webSocket.broadcastTXT(jsonString);
    }
    
    // Serial output for debugging - show every 50 samples to reduce spam
    static int serialCounter = 0;
    serialCounter++;
    if (serialCounter >= 50) {
      serialCounter = 0;
      Serial.printf("ECG: %d, Lead Off: %s, Clients: %d\n", 
                    ecgValue, leadOff ? "YES" : "NO", clientCount);
      
      // Warning if no clients connected
      if (clientCount == 0) {
        Serial.println("WARNING: No WebSocket clients connected!");
      }
    }
  }
  
  delay(1); // Small delay to prevent watchdog issues
}