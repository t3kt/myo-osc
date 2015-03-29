// Myo-OSC, by Samy Kamkar
// outputs all data from the Myo armband over OSC
// usage: myo-osc osc.ip.address osc.port
//
// http://samy.pl - code@samy.pl - 03/03/2014
//
// also makes use of oscpack from http://www.rossbencina.com/code/oscpack
//
// some code originally from hello-myo:
// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.

// stop oscpack sprintf warnings
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>

// add oscpack
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#include "optionparser.h"

#define OUTPUT_BUFFER_SIZE 1024

struct Settings {
  bool accel;
  bool gyro;
  bool orientation;
  bool pose;
  bool emg;
  bool sync;
  bool console;
  
  std::string hostname;
  int port;
};

std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  return os << "Settings<\n"
  << "  accel: " << settings.accel << "\n"
  << "  gyro: " << settings.gyro << "\n"
  << "  orientation: " << settings.orientation << "\n"
  << "  pose: " << settings.pose << "\n"
  << "  emg: " << settings.emg << "\n"
  << "  sync: " << settings.sync << "\n"
  << "  console: " << settings.console << "\n"
  << ">\n";
}

// Classes that inherit from myo::DeviceListener can be used to receive events from Myo devices. DeviceListener
// provides several virtual functions for handling different kinds of events. If you do not override an event, the
// default behavior is to do nothing.
class OscGenerator : public myo::DeviceListener {
public:
  explicit OscGenerator(Settings settings)
  : settings(settings), onArm(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
  {
    transmitSocket = new UdpTransmitSocket(IpEndpointName(settings.hostname.c_str(), settings.port));
  }
  
  ~OscGenerator() override {
    if (transmitSocket != nullptr) {
      delete transmitSocket;
    }
  }
  
  
  // units of g
  void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel) override
  {
    if (!settings.accel)
      return;
    a_x = accel.x();
    a_y = accel.y();
    a_z = accel.z();
    
    send(beginMessage("/myo/accel")
         << accel.x() << accel.y() << accel.z() << osc::EndMessage);
  }
  
  // units of deg/s
  void onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro) override
  {
    if (!settings.gyro)
      return;
    g_x = gyro.x();
    g_y = gyro.y();
    g_z = gyro.z();
    
    send(beginMessage("/myo/gyro")
         << g_x << g_y << g_z << osc::EndMessage);
  }
  
  // onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
  // as a unit quaternion.
  void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat) override
  {
    if (!settings.orientation)
      return;
    using std::atan2;
    using std::asin;
    using std::sqrt;
    
    // Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
    float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
                       1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
    float pitch = asin(2.0f * (quat.w() * quat.y() - quat.z() * quat.x()));
    float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
                      1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));
    
    send(beginMessage("/myo/orientation")
         << quat.x() << quat.y() << quat.z() << quat.w()
         << roll << pitch << yaw << osc::EndMessage);
    
    // Convert the floating point angles in radians to a scale from 0 to 20.
    roll_w = static_cast<int>((roll + (float)M_PI)/(M_PI * 2.0f) * 18);
    pitch_w = static_cast<int>((pitch + (float)M_PI/2.0f)/M_PI * 18);
    yaw_w = static_cast<int>((yaw + (float)M_PI)/(M_PI * 2.0f) * 18);
  }
  
  // onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
  // making a fist, or not making a fist anymore.
  void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose) override
  {
    if (!settings.pose)
      return;
    currentPose = pose;
    
    send(beginMessage("/myo/pose")
      << currentPose.toString().c_str() << osc::EndMessage);
    
    // Vibrate the Myo whenever we've detected that the user has made a fist.
    if (pose == myo::Pose::fist) {
      myo->vibrate(myo::Myo::vibrationShort);
    }
  }
  
  void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg) override {
    if (!settings.emg)
      return;
    send(beginMessage("/myo/emg")
      << emg[0] << emg[1] << emg[2] << emg[3]
      << emg[4] << emg[5] << emg[6] << emg[7]
      << osc::EndMessage);
  }
  
  // onArmSync() is called whenever Myo has recognized a setup gesture after someone has put it on their
  // arm. This lets Myo know which arm it's on and which way it's facing.
  void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection) override
  {
    if (!settings.sync)
      return;
    onArm = true;
    whichArm = arm;
    
    send(beginMessage("/myo/onarm")
         << (whichArm == myo::armLeft ? "L" : "R") << osc::EndMessage);
  }
  
  // onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
  // it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
  // when Myo is moved around on the arm.
  void onArmUnsync(myo::Myo* myo, uint64_t timestamp) override
  {
    if (!settings.sync)
      return;
    onArm = false;
    send(beginMessage("/myo/onarmlost")
         << osc::EndMessage);
  }
  
  inline osc::OutboundPacketStream beginMessage(const char* message) {
    osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);
    p << osc::BeginMessage(message);
    return p;
  }
  
  void send(const osc::OutboundPacketStream& p) {
    transmitSocket->Send(p.Data(), p.Size());
  }
  
  // There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
  // For this example, the functions overridden above are sufficient.
  
  // We define this function to print the current values that were updated by the on...() functions above.
  void print()
  {
    if (!settings.console)
      return;
    // Clear the current line
    std::cout << '\r';
    
    // Print out the orientation. Orientation data is always available, even if no arm is currently recognized.
    std::cout << '[' << std::string(roll_w, '*') << std::string(18 - roll_w, ' ') << ']'
    << '[' << std::string(pitch_w, '*') << std::string(18 - pitch_w, ' ') << ']'
    << '[' << std::string(yaw_w, '*') << std::string(18 - yaw_w, ' ') << ']';
    
    if (onArm) {
      // Print out the currently recognized pose and which arm Myo is being worn on.
      
      // Pose::toString() provides the human-readable name of a pose. We can also output a Pose directly to an
      // output stream (e.g. std::cout << currentPose;). In this case we want to get the pose name's length so
      // that we can fill the rest of the field with spaces below, so we obtain it as a string using toString().
      std::string poseString = currentPose.toString();
      
      std::cout << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
      << '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
    } else {
      // Print out a placeholder for the arm and pose when Myo doesn't currently know which arm it's on.
      std::cout << "[?]" << '[' << std::string(14, ' ') << ']';
    }
    
    std::cout << std::flush;
  }
  
  // These values are set by onArmRecognized() and onArmLost() above.
  bool onArm;
  myo::Arm whichArm;
  
  // These values are set by onOrientationData() and onPose() above.
  int roll_w, pitch_w, yaw_w;
  float w, x, y, z, roll, pitch, yaw, a_x, a_y, a_z, g_x, g_y, g_z;
  myo::Pose currentPose;
  char buffer[OUTPUT_BUFFER_SIZE];
  
  UdpTransmitSocket* transmitSocket;
  Settings settings;
};

enum optionIndex {
  UNKNOWN,
  ACCEL,
  GYRO,
  ORIENT,
  POSE,
  EMG,
  SYNC,
  HELP
};
enum OptionType {DISABLE, ENABLE, OTHER};
const option::Descriptor usage[] = {
  {UNKNOWN, 0, "", "", option::Arg::None, "USAGE: myo-osc [options] <host> <port>\n\n"},
  {ACCEL, ENABLE, "a", "accel", option::Arg::None, "--accel Enable accelerometer output"},
  {ACCEL, DISABLE, "A", "noaccel", option::Arg::None, "--noaccel Disable accelerometer output"},
  {GYRO, ENABLE, "g", "gyro", option::Arg::None, "--gyro Enable gyroscope output"},
  {GYRO, DISABLE, "G", "nogyro", option::Arg::None, "--nogyro Disable gyroscope output"},
  {ORIENT, ENABLE, "o", "orient", option::Arg::None, "--orient Enable orientation output"},
  {ORIENT, DISABLE, "O", "noorient", option::Arg::None, "--noorient Disable orientation output"},
  {POSE, ENABLE, "p", "pose", option::Arg::None, "--pose Enable pose output"},
  {POSE, DISABLE, "P", "nopose", option::Arg::None, "--nopose Disable pose output"},
  {EMG, ENABLE, "e", "emg", option::Arg::None, "--emg Enable EMG output"},
  {EMG, DISABLE, "E", "noemg", option::Arg::None, "--noemg Disable EMG output"},
  {SYNC, ENABLE, "s", "sync", option::Arg::None, "--emg Enable sync/unsync output"},
  {SYNC, DISABLE, "S", "nosync", option::Arg::None, "--noemg Disable sync/unsync output"},
  {HELP, 0, "", "help", option::Arg::None, "--help Print usage and exit."},
  {0, 0, 0, 0, 0, 0},
};

bool parseArgs(int argc, char **argv, Settings* settings) {
  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
  option::Stats  stats(usage, argc, argv);
  std::vector<option::Option> options(stats.options_max);
  std::vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(usage, argc, argv, options.data(), buffer.data());
  
  if (parse.error())
    return false;
  
  if (options[HELP] && argc == 0) {
    option::printUsage(std::cout, usage);
    exit(0);
  }
  
  settings->port = 7777;
  settings->hostname = "127.0.0.1";
  settings->console = true;
  
  if (options[ACCEL].count() ||
      options[GYRO].count() ||
      options[ORIENT].count() ||
      options[POSE].count() ||
      options[EMG].count() ||
      options[SYNC].count()) {
    settings->accel = false;
    settings->gyro = false;
    settings->orientation = false;
    settings->pose = false;
    settings->emg = false;
    settings->sync = false;
  } else {
    settings->accel = true;
    settings->gyro = true;
    settings->orientation = true;
    settings->pose = true;
    settings->emg = true;
    settings->sync = true;
  }
  
  for (const auto& opt : options) {
    switch (opt.index()) {
      case ACCEL:
        settings->accel = opt.type() == ENABLE;
        break;
      case GYRO:
        settings->gyro = opt.type() == ENABLE;
        break;
      case ORIENT:
        settings->orientation = opt.type() == ENABLE;
        break;
      case POSE:
        settings->pose = opt.type() == ENABLE;
        break;
      case EMG:
        settings->emg = opt.type() == ENABLE;
        break;
      case SYNC:
        settings->sync = opt.type() == ENABLE;
        break;
      case UNKNOWN:
        std::cout << "Unknown option: " << std::string(opt.name, opt.namelen) << "\n";
        return false;
      default:
        break;
    }
  }
  if (parse.nonOptionsCount() == 2) {
    settings->hostname = parse.nonOption(0);
    settings->port = atoi(parse.nonOption(1));
  } else if (parse.nonOptionsCount() == 1) {
    settings->port = atoi(parse.nonOption(0));
  } else {
    std::cout << "strange number of non-option arguments: " << parse.nonOptionsCount() << "\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv)
{
  Settings settings;
  // We catch any exceptions that might occur below -- see the catch statement for more details.
  try
  {
//    if (argc != 3 && argc != 2 && argc != 1)
//    {
//      std::cout << "\nusage: " << argv[0] << " [IP address] <port>\n\n" <<
//      "Myo-OSC sends OSC output over UDP from the input of a Thalmic Myo armband.\n" <<
//      "IP address defaults to 127.0.0.1/localhost\n\n" <<
//      "by Samy Kamkar -- http://samy.pl -- code@samy.pl\n";
//      exit(0);
//    }
    if (!parseArgs(argc, argv, &settings))
      return 1;
    
    std::cout << settings;
    
//    if (argc == 1)
//    {
//    }
//    else if (argc == 2)
//    {
//      settings.port = atoi(argv[1]);
//    }
//    else if (argc == 3)
//    {
//      settings.hostname = argv[1];
//      settings.port = atoi(argv[2]);
//    }
//    else
//    {
//      std::cout << "well this awkward -- weird argc: " << argc << "\n";
//      exit(0);
//    }
    std::cout << "Sending Myo OSC to " << settings.hostname << ":" << settings.port << "\n";
    
    
    // First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
    // publishing your application. The Hub provides access to one or more Myos.
    myo::Hub hub("com.samy.myo-osc");
    
    std::cout << "Attempting to find a Myo..." << std::endl;
    
    // Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
    // immediately.
    // waitForAnyMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
    // if that fails, the function will return a null pointer.
    myo::Myo* myo = hub.waitForMyo(10000);
    
    // If waitForAnyMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
    if (!myo) {
      throw std::runtime_error("Unable to find a Myo!");
    }
    
    // We've found a Myo.
    std::cout << "Connected to a Myo armband!" << std::endl << std::endl;
    
    if (settings.emg)
      myo->setStreamEmg(myo::Myo::streamEmgEnabled);
    
    // Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
    OscGenerator collector(settings);
    
    // Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
    // Hub::run() to send events to all registered device listeners.
    hub.addListener(&collector);
    
    // Finally we enter our main loop.
    while (1) {
      // In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
      // In this case, we wish to update our display 20 times a second, so we run for 1000/20 milliseconds.
      hub.run(1000/20);
      // After processing events, we call the print() member function we defined above to print out the values we've
      // obtained from any events that have occurred.
      collector.print();
    }
    
    // If a standard exception occurred, we print out its message and exit.
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Press enter to continue.";
    std::cin.ignore();
    return 1;
  }
}
