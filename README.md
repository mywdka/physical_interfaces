# **Physical Interfaces**

This repository has all the code you need to get started with creating a physical interface using an [Edge Impulse](https://edgeimpulse.com/) machine learning model on a [Arduino micro-controller (Nano 33 BLE Sense)](https://docs.arduino.cc/hardware/nano-33-ble-sense) and [Processing](https://processing.org/).

To learn how to train your own machine learning model using Edge Impulse, visit [our wiki](https://interactionstation.wdka.hro.nl/wiki/Motion_recognition_for_the_Arduino)

## **Example model**

In the [`model`](https://github.com/mywdka/physical_interfaces/tree/main/model) folder you can find an example model which you can use with the templates. You can import this library using the [Arduino IDE](https://docs.arduino.cc/software/ide-v2) by navigating to `Sketch` → `Include Library` → `Add .ZIP Library...`

## **Arduino Template**

If you are using your own Edge Impulse model, you need to change some code in the [`arduino_template`](https://github.com/mywdka/physical_interfaces/tree/main/arduino_template) folder to make the examples work.

### **Sender**

This is the code for the sender Arduino. It means that the Arduino that is running this code will be the one you use for gesture recognition and generally is attached to a battery so that it can be used freely. When it detects a gesture it will send it over Bluetooth to the `Receiver` arduino, which is attached to your computer.

You don't have to mess around too much with the code in the [`sender.ino`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/sender/sender.ino) file in the [`arduino_template/sender`](https://github.com/mywdka/physical_interfaces/tree/main/arduino_template/sender) folder. The only parts you need to change when using your own model are [`line 1`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/sender/sender.ino#L1) and [`line 15 & 16`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/sender/sender.ino#L15&L16).

**Line 1**

```cpp
#include <Interfaces-Sense_inferencing.h> // Change to your model library
```

Change `Interfaces-Sense_inferencing.h` to the name of your imported model. You can find your model name by navigating to `Sketch` → `Include Library` within the Arduino IDE program and scrolling all the way to the bottom. Make sure you add `< >` around your model name and add the `.h` as shown in the example above.

**Line 15 & 16**

```cpp
const char* deviceServiceUuid = "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a";
const char* deviceServiceCharacteristicUuid = "f938f115-3221-43f3-a577-673541325a69";
```

In order to connect to the receiver using Bluetooth, you need to create you own unique identifier (or UUID). This is a long string of random characters. Its the easiest to generate two UUIDs using [this website](https://www.uuidgenerator.net/). Make sure to swap `"75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a"` and `"f938f115-3221-43f3-a577-673541325a69"` with your own, newly generated UUIDs. Don't forget to add double quotes `" "` around the UUID.

### **Receiver**

The [`receiver.ino`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/receiver/receiver.ino) in the [`arduino_template/receiver`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/receiver) folder is the template file that will receive the detected gestures from the `Sender` Arduino. The Arduino that will be running this code is generally attached to your computer as it sends over the detected gestured through the Serial port to Processing.

There are some lines you need to change in order to make it work with your own model. The rest is up to you! The lines you **need** to change are [`line 7 & 8`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/receiver/receiver.ino#L7&L8), [`lines 14-20`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/receiver/receiver.ino#L14-L20) and the `switch` statement at [`lines 77-115`](https://github.com/mywdka/physical_interfaces/blob/main/arduino_template/receiver/receiver.ino#L77-L114).

**Line 7 & 8**

```cpp
const char* deviceServiceUuid = "75f8c43b-fd35-4cd0-9790-3d38a2bd2a8a";
const char* deviceServiceCharacteristicUuid = "f938f115-3221-43f3-a577-673541325a69";
```

These lines need to be the same as in your `sender.ino` file. Remember the UUIDs you've generated before? Paste them here in order to make a connection between the `Sender` and `Receiver` Arduino's.

**Lines 14-20**

```cpp
enum
{
  GESTURE_NONE = -1,
  GESTURE_SLEEP = 0,
  GESTURE_UPDOWN = 1,
  GESTURE_WAVE = 2,
};
```

These are the labels you've picked while collecting data in Edge Impulse. You will need to sort your labels on alphabetical order and but them in this `enum`. This will map the detected labels to a number, so that you can use these numbers in Processing to for example show a video when we receive the number 1.

Let say I've trained a model with the labels: `running`, `walking`, `dancing` and `sleeping`. My `enum` will look like this:

```cpp
enum
{
  GESTURE_NONE = -1,
  GESTURE_RUNNING = 0,
  GESTURE_WALKING = 1,
  GESTURE_DANCING = 2,
  GESTURE_SLEEPING = 3,
};
```

Make sure you keep `GESTURE_NONE` as we use this to not send anything over to Processing (because we didn't detect anything).

**Line 77-115**

The `switch` statement is a way to run different code based on the gesture the model detected. You need to change the `cases` based on the `enum` you changed above. Taking the example I've used earlier, my switch statement will look like this:

```cpp
  case GESTURE_RUNNING:
    Serial.println(GESTURE_RUNNING); // Send number 0
    break;
  case GESTURE_WALKING:
    Serial.println(GESTURE_WALKING); // Send number 1
    break;
  case GESTURE_DANCING:
    Serial.println(GESTURE_DANCING); // Send number 2
    break;
  case GESTURE_SLEEPING:
    Serial.println(GESTURE_SLEEPING); // Send number 3
    break;
  default:
    Serial.println(GESTURE_NONE); // Send number -1
    break;
```

This will send the detected numbers over the Serial port to Processing. In the original template code there is extra code in the `cases` to turn on LEDs based on the detected gesture, but this is not mandatory.

## **Processing Template**

Documentation coming soon ...
