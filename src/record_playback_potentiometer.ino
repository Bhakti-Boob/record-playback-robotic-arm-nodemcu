/*
  Record & Playback Robotic Arm
  - Controls 4 servos and reads 4 potentiometers to record motions, then plays them back. 
  - Serial commands:
      'R' -> start recording (turn pots to record steps)
      'P' -> play recorded sequence
*/

#include <ESP32Servo.h>   

static const int servoPin0         = 14; // Elbow 2 
static const int potentiometerPin0 = 12; // pot for Elbow 2  
static const int servoPin1         = 26; // Elbow 1
static const int potentiometerPin1 = 27; // pot for Elbow 1 
static const int servoPin2         = 33; // Shoulder
static const int potentiometerPin2 = 25; // pot for Shoulder
static const int servoPin3         = 35; // Base
static const int potentiometerPin3 = 32; // pot for Base

// Declare Servo objects 
Servo Servo_0;
Servo Servo_1;
Servo Servo_2;
Servo Servo_3;

// Global Variables
int S0_pos, S1_pos, S2_pos, S3_pos, G_pos;            // current mapped positions (angles)
int P_S0_pos, P_S1_pos, P_S2_pos, P_S3_pos, P_G_pos;  // previous-read positions for comparison
int C_S0_pos, C_S1_pos, C_S2_pos, C_S3_pos, C_G_pos;  // last saved positions to avoid duplicates
int POT_0, POT_1, POT_2, POT_3, POT_4;                // raw ADC readings for pots

int saved_data[700]; // storage for encoded recorded actions 

int array_index = 0; // next free index into saved_data
char incoming = 0;   // last serial command received ('R' or 'P')

int action_pos;      // temporary decoded angle when playing
int action_servo;    // temporary decoded servo id when playing

void setup() {
  Serial.begin(9600); 

  // Attach servo objects to their pins
  Servo_0.attach(servoPin0);
  Servo_1.attach(servoPin1);
  Servo_2.attach(servoPin2);
  Servo_3.attach(servoPin3);

  // Initialize servos to home angles.
  Servo_0.write(70);
  Servo_1.write(100);
  Servo_2.write(110);
  Servo_3.write(10);

  Serial.println("Press 'R' to Record and 'P' to play"); // user instruction
}

// Read raw analog values from all potentiometers and map them to servo angles
void Read_POT() {
   POT_0 = analogRead(potentiometerPin0);
   POT_1 = analogRead(potentiometerPin1);
   POT_2 = analogRead(potentiometerPin2);
   POT_3 = analogRead(potentiometerPin3);

   // Map raw ADC range to servo-friendly angle range (10..170)
   S0_pos = map(POT_0, 0, 1024, 10, 170);
   S1_pos = map(POT_1, 0, 1024, 10, 170);
   S2_pos = map(POT_2, 0, 1024, 10, 170);
   S3_pos = map(POT_3, 0, 1024, 10, 170);
}

// Read the pots twice and if a pot reading is stable the corresponding servo is commanded to that angle and any change is recorded
// To differentiate which servo produced the recorded entry, a base offset is added
void Record() {
  // First read
  Read_POT();
  // Save the first-read values for comparison
  P_S0_pos = S0_pos;
  P_S1_pos = S1_pos;
  P_S2_pos = S2_pos;
  P_S3_pos = S3_pos;
  // Second read to ensure the pot is stable (debounce)
  Read_POT();
  // If the reading is stable (no change between first and second read),
  // apply to servo and save the change if it differs from last saved value.
  if (P_S0_pos == S0_pos) {
    Servo_0.write(S0_pos); // move servo
    if (C_S0_pos != S0_pos) {
      // encode servo id + position and save
      saved_data[array_index] = S0_pos + 0; // servo 0 offset = 0
      array_index++; // increment storage index
    }
    C_S0_pos = S0_pos; // remember last saved position
  }

  if (P_S1_pos == S1_pos) {
    Servo_1.write(S1_pos);
    if (C_S1_pos != S1_pos) {
      saved_data[array_index] = S1_pos + 1000; // servo 1 offset = 1000
      array_index++;
    }
    C_S1_pos = S1_pos;
  }

  if (P_S2_pos == S2_pos) {
    Servo_2.write(S2_pos);
    if (C_S2_pos != S2_pos) {
      saved_data[array_index] = S2_pos + 2000; // servo 2 offset = 2000
      array_index++;
    }
    C_S2_pos = S2_pos;
  }

  if (P_S3_pos == S3_pos) {
    Servo_3.write(S3_pos);
    if (C_S3_pos != S3_pos) {
      saved_data[array_index] = S3_pos + 3000; // servo 3 offset = 3000
      array_index++;
    }
    C_S3_pos = S3_pos;
  }

  // Debug print: current mapped servo positions and saved array index
  Serial.print(S0_pos);  Serial.print("  ");
  Serial.print(S1_pos);  Serial.print("  ");
  Serial.print(S2_pos);  Serial.print("  ");
  Serial.print(S3_pos);  Serial.print("  ");
  Serial.print("Index = "); Serial.println(array_index);
  delay(100); // small delay to avoid flooding when recording
}

// Go through saved_data array and decode each entry, then command the corresponding servo to action_pos
void Play() {
  for (int Play_action = 0; Play_action < array_index; Play_action++) {
    action_servo = saved_data[Play_action] / 1000; // extract servo id (0..3)
    action_pos = saved_data[Play_action] % 1000;   // extract angle (0..999)

    switch (action_servo) {
      case 0:
        Servo_0.write(action_pos);
        break;
      case 1:
        Servo_1.write(action_pos);
        break;
      case 2:
        Servo_2.write(action_pos);
        break;
      case 3:
        Servo_3.write(action_pos);
        break;
    }
    delay(50); // short delay between actions
  }
}

void loop() {
  // Check for incoming serial data 
  if (Serial.available() > 1) {
    incoming = Serial.read();
    if (incoming == 'R') Serial.println("Robotic Arm Recording Started......");
    if (incoming == 'P') Serial.println("Playing Recorded sequence");
  }

  // If 'R' was received, call Record repeatedly 
  if (incoming == 'R') Record();

  // If 'P' was received, play back the recorded motion 
  if (incoming == 'P') Play();
}
