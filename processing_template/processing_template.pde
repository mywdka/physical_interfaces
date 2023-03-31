import processing.serial.*; 
 
Serial myPort;    // The serial port
PFont myFont;     // The display font
String inString;  // Input string from serial port
int lf = 10;      // ASCII linefeed 
 
void setup() { 
  fullScreen();
  // List all the available serial ports: 
  printArray(Serial.list()); 
  // Open whatever port is the one you're using. 
  myPort = new Serial(this, Serial.list()[1], 9600); 
  myPort.bufferUntil(lf); 
  // Set text
  textSize(300);
  //textAlign(CENTER, CENTER);
} 
 
void draw() { 
  background(0);
  if (inString != null && !inString.isEmpty()) {
    text(inString, width / 2, height / 2);
  }
} 
 
void serialEvent(Serial p) { 
  inString = p.readString(); 
} 
