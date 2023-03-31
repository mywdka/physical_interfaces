#include <Interfaces-Sense_inferencing.h> // Change to your model library
#include <ArduinoBLE.h>                   // Bluetooth library
#include <Arduino_APDS9960.h>             // Gesture library
#include <Arduino_LSM9DS1.h>              // Gyroscope, accelerometer library

#define CONVERT_G_TO_MS2 9.80665f
#define MAX_ACCEPTED_RANGE 2.0f

static bool debug_nn = false;

/*
  Change "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a" to match the receiver.
  You can generate you own 'unique id' here: https://www.uuidgenerator.net/
*/
const char *deviceServiceUuid = "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a";
const char *deviceServiceCharacteristicUuid = "f938f115-3221-43f3-a577-673541325a69";

int gesture = -1;
int oldGestureValue = -1;

void setup()
{
  Serial.begin(9600);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  if (!IMU.begin())
  {
    Serial.println("- Failed to initialize accelerometer!");
  }
  else
  {
    Serial.println("- Accelerometer initialized!");
  }

  if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3)
  {
    Serial.println("EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (x,y,z)");
    return;
  }

  if (!BLE.begin())
  {
    Serial.println("- Starting BLE module failed!");
    while (1)
      ;
  }

  BLE.setLocalName("Nano 33 BLE Sense (Central)");
  BLE.advertise();
}

void loop()
{
  connectToPeripheral();
}

void connectToPeripheral()
{
  BLEDevice peripheral;

  Serial.println("- Discovering peripheral device...");

  do
  {
    BLE.scanForUuid(deviceServiceUuid);
    peripheral = BLE.available();
  } while (!peripheral);

  if (peripheral)
  {
    Serial.println("* Peripheral device found!");
    Serial.print("* Device MAC address: ");
    Serial.println(peripheral.address());
    Serial.print("* Device name: ");
    Serial.println(peripheral.localName());
    Serial.print("* Advertised service UUID: ");
    Serial.println(peripheral.advertisedServiceUuid());
    Serial.println(" ");
    BLE.stopScan();
    controlPeripheral(peripheral);
  }
}

void controlPeripheral(BLEDevice peripheral)
{
  Serial.println("- Connecting to peripheral device...");

  if (peripheral.connect())
  {
    Serial.println("* Connected to peripheral device!");
    Serial.println(" ");
    // Change LED to green
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
  }
  else
  {
    Serial.println("* Connection to peripheral device failed!");
    Serial.println(" ");
    return;
  }

  Serial.println("- Discovering peripheral device attributes...");
  if (peripheral.discoverAttributes())
  {
    Serial.println("* Peripheral device attributes discovered!");
    Serial.println(" ");
  }
  else
  {
    Serial.println("* Peripheral device attributes discovery failed!");
    Serial.println(" ");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic gestureCharacteristic = peripheral.characteristic(deviceServiceCharacteristicUuid);

  if (!gestureCharacteristic)
  {
    Serial.println("* Peripheral device does not have gesture_type characteristic!");
    peripheral.disconnect();
    return;
  }
  else if (!gestureCharacteristic.canWrite())
  {
    Serial.println("* Peripheral does not have a writable gesture_type characteristic!");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected())
  {
    gesture = gestureDetection();

    if (oldGestureValue != gesture)
    {
      oldGestureValue = gesture;
      Serial.print("- Writing value to gesture characteristic: ");
      Serial.println(gesture);
      gestureCharacteristic.writeValue((byte)gesture);
      Serial.println("- Writing value to gesture characteristic done!");
      Serial.println(" ");
    }
  }
  Serial.println("- Peripheral device disconnected!");
  // Change LED to red
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
  digitalWrite(LED_BUILTIN, LOW);
}

int gestureDetection()
{
  int gesture_index;
  Serial.println("- Start inferencing in 2 second...");
  delay(2000);
  Serial.println("- Getting accelorometer sample...");

  // Allocate a buffer here for the values we'll read from the IMU
  float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3)
  {
    // Determine the next tick (and then sleep later)
    uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

    IMU.readAcceleration(buffer[ix], buffer[ix + 1], buffer[ix + 2]);

    for (int i = 0; i < 3; i++)
    {
      if (fabs(buffer[ix + i]) > MAX_ACCEPTED_RANGE)
      {
        buffer[ix + i] = ei_get_sign(buffer[ix + i]) * MAX_ACCEPTED_RANGE;
      }
    }

    buffer[ix + 0] *= CONVERT_G_TO_MS2;
    buffer[ix + 1] *= CONVERT_G_TO_MS2;
    buffer[ix + 2] *= CONVERT_G_TO_MS2;

    delayMicroseconds(next_tick - micros());
  }

  // Turn the raw buffer in a signal which we can the classify
  signal_t signal;
  int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0)
  {
    Serial.println("Failed to create a signal!");
    return -1;
  }

  // Run the classifier
  ei_impulse_result_t result = {0};

  err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK)
  {
    Serial.println("Failed to run classifier!");
    return -1;
  }

  for (int ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
  {
    if (result.classification[ix].value > 0.8)
    {
      gesture_index = ix;
      Serial.println(" ");
      Serial.print("* Detected gesture: ");
      Serial.print(result.classification[ix].label);
      Serial.print(" with index number: ");
      Serial.println(ix);
      Serial.println(" ");
    }
  }
  return gesture_index;
}

float ei_get_sign(float number)
{
  return (number >= 0.0) ? 1.0 : -1.0;
}