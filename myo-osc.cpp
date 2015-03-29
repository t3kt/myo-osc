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

#include "MyoOscGenerator.h"

#include <stdexcept>
#include "optionparser.h"

enum optionIndex {
  UNKNOWN,
  ACCEL,
  GYRO,
  ORIENT,
  POSE,
  EMG,
  SYNC,
  RSSI,
  CONSOLE,
  HELP
};
enum OptionType {DISABLE, ENABLE, OTHER};
const char usageText[] =
"USAGE: myo-osc [options] <host> <port>\n"
"   Myo-OSC sends OSC output over UDP from the input of a Thalmic Myo armband.\n"
"   IP address defaults to 127.0.0.1/localhost\n"
"   Port defaults to 7777\n"
"   by Samy Kamkar -- http://samy.pl -- code@samy.pl\n"
"   modified by tekt -- https://t3kt.net\n";
const option::Descriptor usage[] = {
  {UNKNOWN, 0, "", "", option::Arg::None, usageText},
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
  {RSSI, ENABLE, "r", "rssi", option::Arg::None, "--rssi Enable RSSI (signal strength) output"},
  {RSSI, DISABLE, "R", "norssi", option::Arg::None, "--norssi Disable RSSI (signal strength) output"},
  {SYNC, ENABLE, "s", "sync", option::Arg::None, "--emg Enable sync/unsync output"},
  {SYNC, DISABLE, "S", "nosync", option::Arg::None, "--noemg Disable sync/unsync output"},
  {CONSOLE, DISABLE, "C", "noconsole", option::Arg::None, "--noconsole Disable console value display"},
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
    return false;
  }
  
  settings->port = 7777;
  settings->hostname = "127.0.0.1";
  settings->console = true;
  settings->rssi = false;
  
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
      case CONSOLE:
        settings->console = opt.type() == ENABLE;
        break;
      case RSSI:
        settings->rssi = opt.type() == ENABLE;
        break;
      case UNKNOWN:
        std::cout << "Unknown option: " << std::string(opt.name, opt.namelen) << "\n\n";
        option::printUsage(std::cout, usage);
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
    std::cout << "strange number of non-option arguments: " << parse.nonOptionsCount() << "\n\n";
    option::printUsage(std::cout, usage);
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
    if (!parseArgs(argc, argv, &settings))
      return 1;
    
//    std::cout << settings;
    
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
    MyoOscGenerator collector(settings);
    
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
