#include <ArduinoBLE.h> // CMD + Click to get the library: http://librarymanager/All#Arduino_LSM9DS1

/*
  Change "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a" to match the sender.
  You can generate you own 'unique id' here: https://www.uuidgenerator.net/
*/
const char *deviceServiceUuid = "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a";
const char *deviceServiceCharacteristicUuid = "f938f115-3221-43f3-a577-673541325a69";

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
  Serial.println(gesture);
}