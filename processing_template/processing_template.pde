/**
 * Bounce. 
 * 
 * When the shape hits the edge of the window, it reverses its direction. 
 * Modified from: https://processing.org/examples/bounce.html
 */
import processing.serial.*; 
 
Serial port;         // The serial port
int value;           // Input string from serial port
int lf = 10;         // ASCII linefeed 

int rad = 60;        // Width of the shape
float xpos, ypos;    // Starting position of shape    

float xspeed = 2.8;  // Speed of the shape
float yspeed = 2.2;  // Speed of the shape

int xdirection = 1;  // Left or Right
int ydirection = 1;  // Top to Bottom

void setup() { 
  size(800, 800);
  noStroke();
  frameRate(60);
  ellipseMode(RADIUS);
  
  // List all the available serial ports: 
  printArray(Serial.list()); 
  // Open whatever port is the one you're using. 
  port = new Serial(this, Serial.list()[3], 9600); 
  port.readStringUntil(lf); 
  
  // Set the starting position of the shape
  xpos = width/2;
  ypos = height/2;
} 
 
void draw() { 
  background(0);

  // Update the position of the shape
  xpos = xpos + ( xspeed * xdirection );
  ypos = ypos + ( yspeed * ydirection );
  
  // Test to see if the shape exceeds the boundaries of the screen
  // If it does, reverse its direction by multiplying by -1
  if (xpos > width-rad || xpos < rad) {
    xdirection *= -1;
  }
  if (ypos > height-rad || ypos < rad) {
    ydirection *= -1;
  }
  
  if (value == 0) {
    // Detected: SLEEP
    fill(255, 0, 0);
  } else if (value == 1) {
    // Detected: UPDOWN
    fill(0, 255, 0);
  } else if (value == 2) {
    // Detected: WAVE
    fill(0, 0, 255);
  }
  
  ellipse(xpos, ypos, rad, rad);
} 
 
void serialEvent(Serial p) {
   if (p != null) {
     try {
       String s = p.readString().trim();
       value = Integer.parseInt(s);
     } catch (NumberFormatException npe) {
       return;
     }
   }
}
