# [Myo-OSC]

OSC bridge for Thalmic Myo gesture control armband for Windows and Mac OS X 10.8+.

Original version by [Samy Kamkar](http://samy.pl), this fork modified by [tekt](http://t3kt.net).

Myo-OSC is a C++ application designed to take hand gestures, accelerometer, gyroscope, magnetometer, and other data from the [Thalmic Labs Myo](https://www.thalmic.com/en/myo/) armband and output it over OSC. This allows incredible control of virtually any device or application just by waving or flailing your arm or hand around like a madman.

Binaries for both Windows and OS X are included in the root directory (myo-osc.win.exe / myo-osc.osx.bin).

From Samy Kamkar:
> I've built this and integrated it to control various things in my house and car using Raspberry Pi and Arduinos, including lights (over RF), TV (over IR), receiver (over IR), iTunes, coffee machine (over BLE), music in my car (over BLE), presentations (over IR), and Ableton/music production/DJing/VJing (over MIDI and OSC).
> You can also send any OSC data over a network.
> By [@SamyKamkar](https://twitter.com/samykamkar) // <code@samy.pl> // <http://samy.pl> // Mar 3, 2014

Code available on [github](http://github.com/t3kt/myo-osc)

------

## Troubleshooting
On OS X, if you receiver an error message when running the trainer (LSOpenURLsWithRole() failed with error -10810), run this from Terminal:

chmod +x myo-trainer.app/Contents/MacOS/myo-trainer

Then rerun the trainer.

## Download
You can acquire Myo-OSC from github (redirects to latest branch): <http://samy.pl/myo-osc/>

------

## Usage
$./myo-osc [options] [IP address (default: 127.0.0.1)] [OSC port (default: 7777)]
* Options:
  * --config path/to/config.json or --config '{"accel":true}'
    * *see JSON section below*
  * --[no]accel [<path>] Enable/disable accelerometer output, using OSC <path> if specified
    * default path "/myo/accel"
  * --[no]gyro [<path>] Enable/disable gyroscope output, using OSC <path> if specified
    * default path "/myo/gyro"
  * --[no]orient [<path>] Enable/disable orientation output, using OSC <path> if specified
    * default path "/myo/orientation"
  * --[no]quat [<path>] Enable/disable orientation quaternion output, using OSC <path> if specified
    * default path "/myo/orientationquat"
  * --[no]pose [<path>] Enable/disable pose output, using OSC <path> if specified
    * default path "/myo/pose"
  * --[no]emg [<path>] Enable/disable EMG output, using OSC <path> if specified
    * default path "/myo/emg"
  * --[no]rssi [<path>] Enable/disable RSSI (signal strength) output, using OSC <path> if specified
    * default path "/myo/rssi"
  * --[no]sync [<path>] Enable/disable sync/unsync output, using OSC <path> if specified
    * default path "/myo/arm"
  * --log Enable OSC debug logging.
  * --help Print usage and exit.

## JSON Configuration
Configuration JSON provides a way to set the same options as can be set with command line flags, but with more detailed settings.
The config json should have the following structure:
```
{
   "host": "localhost",
   "port": 12345,
   "console": true|false,
   "logOsc": true|false,

   "accel": __output_type_settings__,
   "gyro": __output_type_settings__,
   "orientation": __output_type_settings__,
   "orientationQuat": __output_type_settings__,
   "pos": __output_type_settings__,
   "emg": __output_type_settings__,
   "sync": __output_type_settings__,
   "rssi": __output_type_settings__
}
```

Each output type (e.g. accel, gyro, emg) can be specified in one of two ways. To enable/disable it with its default settings, use a simple boolean true/false. To provide detailed settings, use an object with the following structure:
```
{
	"enabled": true|false,
	"path": "/foo/accel",
	"scale":  "none" | "scale" | "clamp",  // "none" means just send the raw values,
								   "scale" means scale the value from the "in" range to the "out" range,
								   "clamp" means scale, and clamp the output to the "out" range (so if it's greater than the max, it just outputs the max, etc.)
	"in": [-5.0, 5.0],   // expected range for the sensor value, either an array of 2 numbers, or an object (see "out")
	"out": { "min": -20, "max": 25}   // range to scale the sensor value to, either an array of 2 numbers or an object with "min" and "max" fields
}
```

* "scale" - one of the following strings:
  * "none" (default) - output the raw sensor values
  * "scale" - scale the values based on the "in"/"out" ranges
  * "clamp" - scale the values based on the "in"/"out" ranges, but clamp the output to the "out" range
* "in" - expected range for the sensor value, either an array of 2 numbers or an object such as {"min": -20, "max": 25}
* "out" - range to scale the sensor value to (same format as "in")

## OSC Output
```
/myo/pose s MAC s pose

/myo/accel s MAC f X_vector3 f Y_vector3 f Z_vector3

/myo/gyro s MAC f X_vector3 f Y_vector3 f Z_vector3

/myo/orientation s MAC f X_quaternion f Y_quaternion f Z_quaternion f W_quaternion f roll f pitch f yaw
```

*Note:* Thalmic removed the unique MAC address in one of the alpha/beta versions of the SDK, so now the MAC will always return 00:00:00:00:00:00. Hoping this changes in a future version, or perhaps I can access the BLE stack directly and grab the MAC as it's publicly available.

Examples:

```
/myo/pose s f2-e0-66-5d-90-8a s fist

/myo/accel s f2-e0-66-5d-90-8a f 0.95849609375 f 0.26953125 f 0.20068359375

/myo/gyro s f2-e0-66-5d-90-8a f -7.14111280441284 f -15.8081045150757 f 5.43212842941284

/myo/orientation s f2-e0-66-5d-90-8a f -0.00215625390410423 f 3.37712929902281e-43 f -0.0021759606897831 f 0 f 0.928672909736633 f -1.23411226272583 f 1.45198452472687
```


------

### Third Party Libraries

#### oscpack
myo-osc uses [oscpack](https://code.google.com/p/oscpack/), a C++ OSC (Open Sound Control) library.

#### picojson
myo-osc uses [picojson](https://github.com/kazuho/picojson), a C++ JSON parsing library.

### Hardware
[Thalmic Labs Myo](https://www.thalmic.com/en/myo/).

## Questions?
From tekt:
> If you have questions or comments, I can be reached at int3kt@gmail.com
> Check out <http://t3kt.net> for my other projects
> Also, I'm on twitter [@int3kt](https://twitter.com/int3kt)
> Also, I'm on facebook [@int3kt](https://facebook.com/int3kt)

From Samy Kamkar:
> Feel free to contact me with any questions!
> You can reach me at <code@samy.pl>.
> Follow [@SamyKamkar](https://twitter.com/samykamkar) on Twitter or check out <http://samy.pl> for my other projects.
