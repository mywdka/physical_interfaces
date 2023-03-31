#include <rfri-motion_inferencing.h>
#include <ArduinoBLE.h>       // Bluetooth library
#include <Arduino_LSM9DS1.h>  //Click here to get the library: http://librarymanager/All#Arduino_LSM9DS1
#include <Arduino_LPS22HB.h>  //Click here to get the library: http://librarymanager/All#Arduino_LPS22HB
#include <Arduino_HTS221.h>   //Click here to get the library: http://librarymanager/All#Arduino_HTS221
#include <Arduino_APDS9960.h> //Click here to get the library: http://librarymanager/All#Arduino_APDS9960

/*
  Change "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a" to match the receiver.
  You can generate you own 'unique id' here: https://www.uuidgenerator.net/
*/
const char *deviceServiceUuid = "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a";
const char *deviceServiceCharacteristicUuid = "f938f115-3221-43f3-a577-673541325a69";

float confidence_threshold = 0.8;

int gesture = -1;
int oldGestureValue = -1;

enum sensor_status
{
  NOT_USED = -1,
  NOT_INIT,
  INIT,
  SAMPLED
};

typedef struct
{
  const char *name;
  float *value;
  uint8_t (*poll_sensor)(void);
  bool (*init_sensor)(void);
  sensor_status status;
} eiSensors;

#define CONVERT_G_TO_MS2 9.80665f
#define MAX_ACCEPTED_RANGE 2.0f
#define N_SENSORS 18

float ei_get_sign(float number);

bool init_IMU(void);
bool init_HTS(void);
bool init_BARO(void);
bool init_APDS(void);

uint8_t poll_acc(void);
uint8_t poll_gyr(void);
uint8_t poll_mag(void);
uint8_t poll_HTS(void);
uint8_t poll_BARO(void);
uint8_t poll_APDS_color(void);
uint8_t poll_APDS_proximity(void);
uint8_t poll_APDS_gesture(void);

static const bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

static float data[N_SENSORS];
static bool ei_connect_fusion_list(const char *input_list);

static int8_t fusion_sensors[N_SENSORS];
static int fusion_ix = 0;

/** Used sensors value function connected to label name */
eiSensors sensors[] =
    {
        "accX",
        &data[0],
        &poll_acc,
        &init_IMU,
        NOT_USED,
        "accY",
        &data[1],
        &poll_acc,
        &init_IMU,
        NOT_USED,
        "accZ",
        &data[2],
        &poll_acc,
        &init_IMU,
        NOT_USED,
        "gyrX",
        &data[3],
        &poll_gyr,
        &init_IMU,
        NOT_USED,
        "gyrY",
        &data[4],
        &poll_gyr,
        &init_IMU,
        NOT_USED,
        "gyrZ",
        &data[5],
        &poll_gyr,
        &init_IMU,
        NOT_USED,
        "magX",
        &data[6],
        &poll_mag,
        &init_IMU,
        NOT_USED,
        "magY",
        &data[7],
        &poll_mag,
        &init_IMU,
        NOT_USED,
        "magZ",
        &data[8],
        &poll_mag,
        &init_IMU,
        NOT_USED,

        "temperature",
        &data[9],
        &poll_HTS,
        &init_HTS,
        NOT_USED,
        "humidity",
        &data[10],
        &poll_HTS,
        &init_HTS,
        NOT_USED,

        "pressure",
        &data[11],
        &poll_BARO,
        &init_BARO,
        NOT_USED,

        "red",
        &data[12],
        &poll_APDS_color,
        &init_APDS,
        NOT_USED,
        "green",
        &data[13],
        &poll_APDS_color,
        &init_APDS,
        NOT_USED,
        "blue",
        &data[14],
        &poll_APDS_color,
        &init_APDS,
        NOT_USED,
        "brightness",
        &data[15],
        &poll_APDS_color,
        &init_APDS,
        NOT_USED,
        "proximity",
        &data[16],
        &poll_APDS_proximity,
        &init_APDS,
        NOT_USED,
        "gesture",
        &data[17],
        &poll_APDS_gesture,
        &init_APDS,
        NOT_USED,
};

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

  /* Connect used sensors */
  if (ei_connect_fusion_list(EI_CLASSIFIER_FUSION_AXES_STRING) == false)
  {
    ei_printf("ERR: Errors in sensor list detected\r\n");
    return;
  }

  /* Init & start sensors */

  for (int i = 0; i < fusion_ix; i++)
  {
    if (sensors[fusion_sensors[i]].status == NOT_INIT)
    {
      sensors[fusion_sensors[i]].status = (sensor_status)sensors[fusion_sensors[i]].init_sensor();
      if (!sensors[fusion_sensors[i]].status)
      {
        ei_printf("%s axis sensor initialization failed.\r\n", sensors[fusion_sensors[i]].name);
      }
      else
      {
        ei_printf("%s axis sensor initialization successful.\r\n", sensors[fusion_sensors[i]].name);
      }
    }
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
  delay(2000);

  if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != fusion_ix)
  {
    return -1;
  }

  // Allocate a buffer here for the values we'll read from the sensor
  float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME)
  {
    // Determine the next tick (and then sleep later)
    int64_t next_tick = (int64_t)micros() + ((int64_t)EI_CLASSIFIER_INTERVAL_MS * 1000);

    for (int i = 0; i < fusion_ix; i++)
    {
      if (sensors[fusion_sensors[i]].status == INIT)
      {
        sensors[fusion_sensors[i]].poll_sensor();
        sensors[fusion_sensors[i]].status = SAMPLED;
      }
      if (sensors[fusion_sensors[i]].status == SAMPLED)
      {
        buffer[ix + i] = *sensors[fusion_sensors[i]].value;
        sensors[fusion_sensors[i]].status = INIT;
      }
    }

    int64_t wait_time = next_tick - (int64_t)micros();

    if (wait_time > 0)
    {
      delayMicroseconds(wait_time);
    }
  }

  // Turn the raw buffer in a signal which we can the classify
  signal_t signal;
  int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0)
  {
    return -1;
  }

  // Run the classifier
  ei_impulse_result_t result = {0};

  err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK)
  {
    return -1;
  }

  for (int ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
  {
    if (result.classification[ix].value > confidence_threshold)
    {
#if EI_CLASSIFIER_HAS_ANOMALY == 1
      // ei_printf("    anomaly score: %.3f\r\n", result.anomaly);
#endif
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

#if !defined(EI_CLASSIFIER_SENSOR) || (EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_FUSION && EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER)
#error "Invalid model for current sensor"
#endif

/**
 * @brief Go through sensor list to find matching axis name
 *
 * @param axis_name
 * @return int8_t index in sensor list, -1 if axis name is not found
 */
static int8_t ei_find_axis(char *axis_name)
{
  int ix;
  for (ix = 0; ix < N_SENSORS; ix++)
  {
    if (strstr(axis_name, sensors[ix].name))
    {
      return ix;
    }
  }
  return -1;
}

/**
 * @brief Check if requested input list is valid sensor fusion, create sensor buffer
 *
 * @param[in]  input_list      Axes list to sample (ie. "accX + gyrY + magZ")
 * @retval  false if invalid sensor_list
 */
static bool ei_connect_fusion_list(const char *input_list)
{
  char *buff;
  bool is_fusion = false;

  /* Copy const string in heap mem */
  char *input_string = (char *)ei_malloc(strlen(input_list) + 1);
  if (input_string == NULL)
  {
    return false;
  }
  memset(input_string, 0, strlen(input_list) + 1);
  strncpy(input_string, input_list, strlen(input_list));

  /* Clear fusion sensor list */
  memset(fusion_sensors, 0, N_SENSORS);
  fusion_ix = 0;

  buff = strtok(input_string, "+");

  while (buff != NULL)
  { /* Run through buffer */
    int8_t found_axis = 0;

    is_fusion = false;
    found_axis = ei_find_axis(buff);

    if (found_axis >= 0)
    {
      if (fusion_ix < N_SENSORS)
      {
        fusion_sensors[fusion_ix++] = found_axis;
        sensors[found_axis].status = NOT_INIT;
      }
      is_fusion = true;
    }

    buff = strtok(NULL, "+ ");
  }

  ei_free(input_string);

  return is_fusion;
}

/**
 * @brief Return the sign of the number
 *
 * @param number
 * @return int 1 if positive (or 0) -1 if negative
 */
float ei_get_sign(float number)
{
  return (number >= 0.0) ? 1.0 : -1.0;
}

bool init_IMU(void)
{
  static bool init_status = false;
  if (!init_status)
  {
    init_status = IMU.begin();
  }
  return init_status;
}

bool init_HTS(void)
{
  static bool init_status = false;
  if (!init_status)
  {
    init_status = HTS.begin();
  }
  return init_status;
}

bool init_BARO(void)
{
  static bool init_status = false;
  if (!init_status)
  {
    init_status = BARO.begin();
  }
  return init_status;
}

bool init_APDS(void)
{
  static bool init_status = false;
  if (!init_status)
  {
    init_status = APDS.begin();
  }
  return init_status;
}

uint8_t poll_acc(void)
{

  if (IMU.accelerationAvailable())
  {

    IMU.readAcceleration(data[0], data[1], data[2]);

    for (int i = 0; i < 3; i++)
    {
      if (fabs(data[i]) > MAX_ACCEPTED_RANGE)
      {
        data[i] = ei_get_sign(data[i]) * MAX_ACCEPTED_RANGE;
      }
    }

    data[0] *= CONVERT_G_TO_MS2;
    data[1] *= CONVERT_G_TO_MS2;
    data[2] *= CONVERT_G_TO_MS2;
  }

  return 0;
}

uint8_t poll_gyr(void)
{

  if (IMU.gyroscopeAvailable())
  {
    IMU.readGyroscope(data[3], data[4], data[5]);
  }
  return 0;
}

uint8_t poll_mag(void)
{

  if (IMU.magneticFieldAvailable())
  {
    IMU.readMagneticField(data[6], data[7], data[8]);
  }
  return 0;
}

uint8_t poll_HTS(void)
{

  data[9] = HTS.readTemperature();
  data[10] = HTS.readHumidity();
  return 0;
}

uint8_t poll_BARO(void)
{

  data[11] = BARO.readPressure(); // (PSI/MILLIBAR/KILOPASCAL) default kPa
  return 0;
}

uint8_t poll_APDS_color(void)
{

  int temp_data[4];
  if (APDS.colorAvailable())
  {
    APDS.readColor(temp_data[0], temp_data[1], temp_data[2], temp_data[3]);

    data[12] = temp_data[0];
    data[13] = temp_data[1];
    data[14] = temp_data[2];
    data[15] = temp_data[3];
  }
}

uint8_t poll_APDS_proximity(void)
{

  if (APDS.proximityAvailable())
  {
    data[16] = (float)APDS.readProximity();
  }
  return 0;
}

uint8_t poll_APDS_gesture(void)
{
  if (APDS.gestureAvailable())
  {
    data[17] = (float)APDS.readGesture();
  }
  return 0;
}