#include <ArduinoBLE.h> // Bluetooth library

/*
  Change "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a" to match the sender.
  You can generate you own 'unique id' here: https://www.uuidgenerator.net/
*/
const char *deviceServiceUuid = "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a";
const char *deviceServiceCharacteristicUuid = "f938f115-3221-43f3-a577-673541325a69";

/*
  Change this to your model its labels in alpabetical order
  'GESTURE_NONE' needs to stay.
*/
enum
{
  GESTURE_NONE = -1,
  GESTURE_SLEEP = 0,
  GESTURE_UPDOWN = 1,
  GESTURE_WAVE = 2,
};

int gesture = -1;

BLEService gestureService(deviceServiceUuid);
BLEByteCharacteristic gestureCharacteristic(deviceServiceCharacteristicUuid, BLERead | BLEWrite);

void setup()
{
  Serial.begin(9600);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  if (!BLE.begin())
  {
    while (1)
      ;
  }

  BLE.setLocalName("Nano 33 BLE Sense (Peripheral)");
  BLE.setAdvertisedService(gestureService);
  gestureService.addCharacteristic(gestureCharacteristic);
  BLE.addService(gestureService);
  gestureCharacteristic.writeValue(-1);
  BLE.advertise();
}

void loop()
{
  BLEDevice central = BLE.central();
  delay(500);

  if (central)
  {
    while (central.connected())
    {
      if (gestureCharacteristic.written())
      {
        gesture = gestureCharacteristic.value();
        writeGesture(gesture);
      }
    }
  }
}

/*
  Add or remove 'cases' based on your defined 'enum' (labels)
  you've added at the top of this file (line 7)
*/
void writeGesture(int gesture)
{
  switch (gesture)
  {
  case GESTURE_UPDOWN:
    /*
      Detected 'GESTURE_UPDOWN'
      1: Turn on GREEN LED
      2: Send GESTURE_UPDOWN (1) to Processing
    */
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, HIGH);

    Serial.println(GESTURE_UPDOWN);
    break;
  case GESTURE_WAVE:
    /*
      Detected 'GESTURE_WAVE'
      1: Turn on BLUE LED
      2: Send GESTURE_WAVE (2) to Processing
    */
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, LOW);

    Serial.println(GESTURE_WAVE);
    break;
  default:
    /*
      Detected 'GESTURE_NONE' or 'GESTURE_SLEEP'
      1: Turn on RED LED
      2: Send GESTURE_NONE (-1) to Processing
    */
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);

    Serial.println(GESTURE_NONE);
    break;
  }
}