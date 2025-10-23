#include <SoftwareSerial.h>
#include <Servo.h>

SoftwareSerial BT_Serial(2, 3); // RX, TX for Bluetooth communication

// Define car motor control pins
#define carIn1 8   // Car motor control pins
#define carIn2 9
#define carIn3 10
#define carIn4 11

// Define arm control pins on the second L298N
#define armIn1 4  // Connect to IN1 on 2nd L298N
#define armIn2 5  // Connect to IN2 on 2nd L298N
#define armIn3 6  // Connect to IN3 on 2nd L298N (Gripper control)
#define armIn4 7  // Connect to IN4 on 2nd L298N (Gripper control)

// Define ultrasonic sensor and servo pins
const int trigPin = A1;   // Trigger pin for ultrasonic sensor
const int echoPin = A2;   // Echo pin for ultrasonic sensor
const int servoPin = A0;  // Servo pin

// Create a Servo object
Servo myServo;

// Variables
char bt_data;  // Bluetooth command data
bool armModeActive = false;
unsigned long previousMillis = 0;
const long functionRunTime = 100;
bool functionActive = false;

bool sweepActive = false;  // Flag for controlling servo sweep

void setup() {
  Serial.begin(9600);
  BT_Serial.begin(9600);

  // Initialize servo
  myServo.attach(servoPin);
  myServo.write(0);  // Start at 0 degrees (default position)

  // Initialize ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set car motor pins as output
  pinMode(carIn1, OUTPUT);
  pinMode(carIn2, OUTPUT);
  pinMode(carIn3, OUTPUT);
  pinMode(carIn4, OUTPUT);

  // Set arm motor pins as output
  pinMode(armIn1, OUTPUT);
  pinMode(armIn2, OUTPUT);
  pinMode(armIn3, OUTPUT);
  pinMode(armIn4, OUTPUT);

  stopMotors();  // Ensure all motors are initially stopped
}

void loop() {
  // Check if Bluetooth data is available
  if (BT_Serial.available() > 0) {
    bt_data = BT_Serial.read();
    Serial.println(bt_data);
    handleCommands();
  }

  // If sweep is active, move the servo and check distance
  if (sweepActive) {
    // Sweep the servo from 0 to 180 degrees
    for (int angle = 0; angle <= 180; angle += 10) {
      if (!sweepActive) {  // Check if sweep has been stopped
        stopSweep();
        return; // Exit the loop and stop sweeping
      }

      myServo.write(angle);
      delay(500); // Wait for the servo to reach the target position

      // Get the distance from the ultrasonic sensor
      long duration = getUltrasonicDistance();
      long distance = duration * 0.0344 / 2;  // Convert duration to distance in cm

      // If distance is less than 35 cm, send the degree over Bluetooth
      if (distance < 35) {
        BT_Serial.println(angle);  // Send only the degree value
      }
    }

    // Sweep the servo back from 180 to 0 degrees
    for (int angle = 180; angle >= 0; angle -= 10) {
      if (!sweepActive) {  // Check if sweep has been stopped
        stopSweep();
        return; // Exit the loop and stop sweeping
      }

      myServo.write(angle);
      delay(500); // Wait for the servo to reach the target position

      // Get the distance from the ultrasonic sensor
      long duration = getUltrasonicDistance();
      long distance = duration * 0.0344 / 2;  // Convert duration to distance in cm

      // If distance is less than 35 cm, send the degree over Bluetooth
      if (distance < 35) {
        BT_Serial.println(angle);  // Send only the degree value
      }
    }

    delay(500); // 2 seconds delay before starting the next sweep
  }

  if (functionActive) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= functionRunTime) {
      stopArm();  // Stop arm motor after 0.1 seconds
      functionActive = false;
    }
  }
}

// Function to read the distance from the ultrasonic sensor
long getUltrasonicDistance() {
  // Trigger the ultrasonic sensor to send a pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the time it takes for the pulse to return
  long duration = pulseIn(echoPin, HIGH);
  return duration;
}

// Handle Bluetooth commands
void handleCommands() {
  // Car control commands
  if (bt_data == 'f') forward();
  else if (bt_data == 'b') backward();
  else if (bt_data == 'l') turnLeft();
  else if (bt_data == 'r') turnRight();
  else if (bt_data == 's') stopMotors();

  // Arm control commands
  else if (bt_data == 'U') moveArmUp();
  else if (bt_data == 'D') moveArmDown();
  else if (bt_data == 'O') openGripper();
  else if (bt_data == 'C') closeGripper();

  // Servo sweep control
  else if (bt_data == 'y') {
    sweepActive = true;
    Serial.println("Servo sweep started.");
  } else if (bt_data == 'x') {
    sweepActive = false;
    stopSweep();
    Serial.println("Servo sweep stopped.");
  }
}

// Function to stop the servo sweep and return to the neutral position
void stopSweep() {
  myServo.write(0);  // Set servo to 0 degrees (default position)
  Serial.println("Sweep stopped and servo returned to 0 degrees.");
}

// Car motor control functions
void forward() {
  digitalWrite(carIn1, HIGH);
  digitalWrite(carIn2, LOW);
  digitalWrite(carIn3, LOW);
  digitalWrite(carIn4, HIGH);
}

void backward() {
  digitalWrite(carIn1, LOW);
  digitalWrite(carIn2, HIGH);
  digitalWrite(carIn3, HIGH);
  digitalWrite(carIn4, LOW);
}

void turnLeft() {
  digitalWrite(carIn1, HIGH);
  digitalWrite(carIn2, LOW);
  digitalWrite(carIn3, HIGH);
  digitalWrite(carIn4, LOW);
}

void turnRight() {
  digitalWrite(carIn1, LOW);
  digitalWrite(carIn2, HIGH);
  digitalWrite(carIn3, LOW);
  digitalWrite(carIn4, HIGH);
}

void stopMotors() {
  digitalWrite(carIn1, LOW);
  digitalWrite(carIn2, LOW);
  digitalWrite(carIn3, LOW);
  digitalWrite(carIn4, LOW);
}

// Arm control functions
void moveArmUp() {
  digitalWrite(armIn1, HIGH);
  digitalWrite(armIn2, LOW);
  startTimer();
}

void moveArmDown() {
  digitalWrite(armIn1, LOW);
  digitalWrite(armIn2, HIGH);
  startTimer();
}

void openGripper() {
  digitalWrite(armIn3, LOW);
  digitalWrite(armIn4, HIGH);
  startTimer();
}

void closeGripper() {
  digitalWrite(armIn3, HIGH);
  digitalWrite(armIn4, LOW);
  startTimer();
}

void stopArm() {
  digitalWrite(armIn1, LOW);
  digitalWrite(armIn2, LOW);
  digitalWrite(armIn3, LOW);
  digitalWrite(armIn4, LOW);
}

void startTimer() {
  previousMillis = millis();
  functionActive = true;
}
