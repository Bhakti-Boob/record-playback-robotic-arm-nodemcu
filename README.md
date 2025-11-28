# Record & Playback Pick‑and‑Place Robotic Arm using NodeMCU 
Robotic arm with 4‑DOF that can record joint motions and play them back either from a web UI (sliders in a browser) or from potentiometers connected directly to the controller.

# Overview
This project implements pick‑and‑place robotic arm driven by four servos and controlled by NodeMCU. The arm supports record & playback in two variants:  
- Web UI mode: a self‑hosted access point serves an HTML page at `192.168.4.1` with sliders for each joint and buttons to Record and Play
- Potentiometer mode: four pots directly drive the joints, their movements are sampled and stored, then replayed on command via the serial monitor
Typical use cases include demonstrating basic robotics, motion programming and human‑in‑the‑loop teaching of repeated pick‑and‑place tasks.

# Features
- 4‑axis servo robotic arm (base, shoulder, elbow 1, elbow 2 and gripper) driven by `ESP32Servo`
- Record & playback of sequences using a compact integer encoding (`servo_id * 1000 + angle`) in an in‑memory array
- Standalone Wi‑Fi access point with HTTP server (ESP32 `WiFi` + `WebServer`):  
  - Live slider control of each servo  
  - Record toggle and Play controls 
  - JSON `/status` endpoint reporting step count and record state.
- Alternative potentiometer control variant that:  
  - Reads analog values from four pots, maps to servo angles (10–170°) 
  - Debounces readings by double sampling for stability
  - Stores only changed positions per joint to avoid duplicate entries

# Hardware and components used
- ESP32 / NodeMCU board with Wi‑Fi
- 4 × servo motors for base and joints (e.g. MG995 / SG90)
- Option A (Web UI):  
  - External 5 V supply for servos  
- Option B (Pots):  
  - 4 × potentiometers (one per joint) to generate position commands

Servos are powered from a separate 5 V source with shared ground to avoid overloading the ESP32’s 3.3 V regulator

# Future Improvements
- Add a dedicated gripper channel and UI control in the web interface.​
- Store multiple named motion programs in flash and select them from the UI.​​
- Add safety limits and smooth interpolation to avoid abrupt servo moves.

# How to Use This Repository
- /src/ – Arduino sketches for the robotic arm
- /circuit_diagrams/ – Schematics, wiring diagrams
- /docs/ – Photos and videos of the assembled prototype
