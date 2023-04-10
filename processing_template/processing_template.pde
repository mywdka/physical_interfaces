/******************************************
**           Don't remove this           **
******************************************/
import processing.serial.*; 
Serial port;
int value;
/*****************************************
** End of part you don't need to remove **
*****************************************/


void setup() { 
  size(800, 800);
  
  
  
  
  
  
  
  /******************************************
  **     Don't remove this, only edit!     **
  ******************************************/
  printArray(Serial.list()); // Find the index of the Arduino 
  port = new Serial(this, Serial.list()[3], 9600); // Use the index here (between the '[ ]')
  port.readStringUntil(10);
  /*****************************************
  ** End of part you don't need to remove **
  *****************************************/
}
 
void draw() {
  background(0);

} 

 
 
 
 
 
 
 
 
 
 
  
/******************************************
**     Don't remove this, only edit!     **
******************************************/
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
/*****************************************
** End of part you don't need to remove **
*****************************************/
