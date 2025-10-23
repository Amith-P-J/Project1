#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <math.h> // for sin(), cos()

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial BT_Serial(2, 3); // RX, TX for Bluetooth module
const int MPU = 0x68;           // I2C address of the MPU6050 accelerometer
const int buttonPin = 4;        // Push button connected to D4

int16_t AcX, AcY, AcZ;
int flag = 0;
int mode = 0; // 0 = AWD Control, 1 = Arm Mode, 2 = Ultrasonic Mode
unsigned long buttonPressTime = 0;
bool buttonPressed = false;

// Global variable for angle
int angle = 0;  // Keep track of the angle of the radar sweep
int detectedAngle = -1;  // Default to no object detected (-1 means no detected angle)
bool sweepingRightToLeft = true; // True means sweeping from right to left, False means left to right
bool radarModeActive = false;  // Flag to track radar mode

Servo myServo;  // Declare servo object
const int servoPin = 9; // Pin for servo control

void setup() {
  Serial.begin(9600);
  BT_Serial.begin(9600);

  pinMode(buttonPin, INPUT); // Push button for mode switching

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Show A.R.E.I.S. for 7 seconds
  display.setCursor(0, 0);
  display.print(F("A.R.E.I.S."));

  display.setCursor(0, 10);
  display.print(F("Autonomous"));

  display.setCursor(0, 20);
  display.print(F("Reconnaissance"));

  display.setCursor(0, 30);
  display.print(F("& Exploration"));

  display.setCursor(0, 40);
  display.print(F("Integrated"));

  display.setCursor(0, 50);
  display.print(F("System"));

  display.display();
  delay(7000);  // Wait for 7 seconds

  // Clear the display
  display.clearDisplay();
  display.display();

  // Show "A.R.E.I.S. Starting..." for 3 seconds
  display.setCursor(0, 0);
  display.print(F("A.R.E.I.S."));
  display.setCursor(0, 10);
  display.print(F("Starting..."));
  display.display();
  
  delay(3000);  // Wait for 3 seconds

  // Clear display before proceeding to mode
  display.clearDisplay();
  display.display();

  // Initialize servo
  myServo.attach(servoPin);
  myServo.write(90);  // Start at neutral position

  // Start in a clean state
  display.clearDisplay();
  display.display();

  Wire.begin();
  // Initialize MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop() {
  checkButton();  // Check if mode change button is pressed

  display.clearDisplay();
  display.setCursor(0, 0);

  // Display current mode at the top
  if (mode == 0) {  // AWD Control
    display.print("Mode: AWD Control");
    handleCarControl();
  } else if (mode == 1) {  // Arm Mode
    display.print("Mode: Arm Control");
    handleArmControl();
  } else if (mode == 2) {  // Ultrasonic Mode
    display.print("Mode: Ultrasonic");
    startRadarMode();  // Radar mode logic from Code 2
  }

  display.display();  // Update OLED with new data
}
void checkButton() {
  if (digitalRead(buttonPin) == HIGH && !buttonPressed) {
    buttonPressTime = millis();
    buttonPressed = true;
  } else if (digitalRead(buttonPin) == LOW && buttonPressed) {
    unsigned long buttonDuration = millis() - buttonPressTime;  // Duration of button press

    if (buttonDuration >= 5000) {  // 5-second press
      mode = 2;  // Switch to Ultrasonic Mode
      // Enter radar mode, send 'y' only once if not already in radar mode
      if (!radarModeActive) {
        BT_Serial.write('y');
        radarModeActive = true;
        Serial.println("Radar Mode (y command sent)");
      }
    } else if (buttonDuration >= 2000) {  // 2-second press
      if (mode == 0 || mode == 1) {
        // If in AWD or ARM mode, toggle between these modes
        mode = (mode == 0) ? 1 : 0;  // Switch between AWD (0) and ARM (1)
      } else if (mode == 2) {
        // If in Ultrasonic mode, switch to AWD mode
        mode = 0;  // Switch to AWD mode
        BT_Serial.write('x');  // Exit radar mode, send 'x' to stop it
        radarModeActive = false;
        myServo.write(90);  // Stop the servo at neutral position
        Serial.println("Switching to AWD mode from Ultrasonic");
      }
    }

    buttonPressed = false;
  }
}
void handleCarControl() {
  Read_accelerometer();
  display.setCursor(0, 10);  // Display accelerometer values below the mode
  display.print("AcX: "); display.print(AcX);
  display.setCursor(0, 20);
  display.print("AcY: "); display.print(AcY);
  display.setCursor(0, 30);
  display.print("AcZ: "); display.print(AcZ);

  if (AcX < 60 && flag == 0) { 
    flag = 1; 
    BT_Serial.write('f'); // Forward
  }
  else if (AcX > 130 && flag == 0) { 
    flag = 1; 
    BT_Serial.write('b'); // Backward
  }
  else if (AcY < 60 && flag == 0) { 
    flag = 1; 
    BT_Serial.write('l'); // Left
  }
  else if (AcY > 130 && flag == 0) { 
    flag = 1; 
    BT_Serial.write('r'); // Right
  }
  else if ((AcX > 70) && (AcX < 120) && (AcY > 70) && (AcY < 120) && (flag == 1)) { 
    flag = 0; 
    BT_Serial.write('s'); // Stop
  }
  delay(500);
}

void handleArmControl() {
  Read_accelerometer();
  display.setCursor(0, 10);  // Display accelerometer values below the mode
  display.print("AcX: "); display.print(AcX);
  display.setCursor(0, 20);
  display.print("AcY: "); display.print(AcY);
  display.setCursor(0, 30);
  display.print("AcZ: "); display.print(AcZ);

  if (AcX < 60) { 
    BT_Serial.write('U'); 
    delay(100); // Move arm up
  }  
  else if (AcX > 130) { 
    BT_Serial.write('D'); 
    delay(100); // Move arm down
  }  
  else if (AcY < 60) { 
    BT_Serial.write('O'); 
    delay(100); // Open gripper
  }  
  else if (AcY > 130) { 
    BT_Serial.write('C'); 
    delay(100); // Close gripper
  }
}

void startRadarMode() {
  // Request angle data from Uno when ultrasonic mode is activated
  // No need to send 'y' repeatedly here, we only need it once when entering radar mode.

  // Listen for angle data from Uno
  if (BT_Serial.available()) {
    detectedAngle = BT_Serial.parseInt(); // Read the detected angle from Uno

    if (detectedAngle >= 0 && detectedAngle <= 180) {
      // Draw lines for surrounding angles (±2 degrees) around detected angle
      for (int sweepAngle = detectedAngle - 10; sweepAngle <= detectedAngle + 10; sweepAngle++) {
        if (sweepAngle >= 0 && sweepAngle <= 180) {
          drawRadarLine(sweepAngle);
        }
      }
    }
  }

  // Draw the radar sweep
  drawRadarSweep();

  delay(100);  // Wait for a short period before updating
}

void drawRadarLine(int angle) {
  int radarCenterX = SCREEN_WIDTH / 2;
  int radarCenterY = SCREEN_HEIGHT;
  int radarRadius = 64;

  // Calculate the position based on the angle and distance (here we assume the distance is 35 cm)
  int distance = 35; // Fixed distance for this example

  // Map the distance to the radar radius
  int distanceLength = map(distance, 0, 40, 0, radarRadius);

  // Calculate the x and y positions for the radar line
  int objectX = radarCenterX + cos(radians(angle)) * distanceLength;
  int objectY = radarCenterY - sin(radians(angle)) * distanceLength;

  // Draw the line
  display.drawLine(radarCenterX, radarCenterY, objectX, objectY, SSD1306_WHITE);
}

void drawRadarSweep() {
  int radarCenterX = SCREEN_WIDTH / 2;
  int radarCenterY = SCREEN_HEIGHT;
  int radarRadius = 64;  // Main radar radius
  int smallRadius = 20;  // Smallest radius (aesthetic)

  // Draw the outer semi-circle (main radar field of view)
  for (int i = 0; i <= 180; i++) {  // 180° sweep
    int x = radarCenterX + cos(radians(i)) * radarRadius;
    int y = radarCenterY - sin(radians(i)) * radarRadius;
    display.drawPixel(x, y, SSD1306_WHITE);
  }

  // Draw the smallest semi-circle (just inside the main radar semi-circle)
  for (int i = 0; i <= 180; i++) {
    int x = radarCenterX + cos(radians(i)) * smallRadius;
    int y = radarCenterY - sin(radians(i)) * smallRadius;
    display.drawPixel(x, y, SSD1306_WHITE);
  }

  // Clear the previous angle value to avoid overlap
  display.fillRect(SCREEN_WIDTH - 30, 0, 30, 8, SSD1306_BLACK);  // Clear previous angle

  // Display the current radar angle at the top-right corner
  display.setCursor(SCREEN_WIDTH - 30, 0);  // Position the angle at the top-right
  display.print(angle);  // Display the current angle

  // Draw the radar sweep line
  int sweepX = radarCenterX + cos(radians(angle)) * radarRadius;
  int sweepY = radarCenterY - sin(radians(angle)) * radarRadius;
  display.drawLine(radarCenterX, radarCenterY, sweepX, sweepY, SSD1306_WHITE);

  // Update angle for the next sweep based on direction (using smaller increment)
  if (sweepingRightToLeft) {
    angle += 10;  // Slow down the sweep by reducing the increment 
    if (angle >= 180) {
      sweepingRightToLeft = false;
    }
  } else {
    angle -= 10;  // Slow down the sweep by reducing the increment 
    if (angle <= 0) {
      sweepingRightToLeft = true;
    }
  }

  delay(300);  // Increase this value to slow down the overall sweep speed
}

void Read_accelerometer() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  AcX = map(AcX, -17000, 17000, 0, 180);
  AcY = map(AcY, -17000, 17000, 0, 180);
  AcZ = map(AcZ, -17000, 17000, 0, 180);

  Serial.print(AcX); Serial.print("\t");
  Serial.print(AcY); Serial.print("\t");
  Serial.println(AcZ);
}
