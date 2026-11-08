#pragma once

#include <JuceHeader.h>

#ifndef HELMBOY_ENABLE_STARTUP_TRACE
  #define HELMBOY_ENABLE_STARTUP_TRACE 0
#endif

#if HELMBOY_ENABLE_STARTUP_TRACE
namespace helmboy {
inline void appendStartupTrace(const char* filename, const juce::String& message) {
  auto logs_dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("helmBoy").getChildFile("logs");
  (void)logs_dir.createDirectory();
  auto timestamp = juce::Time::getCurrentTime().toString(true, true, true, true);
#if JUCE_WINDOWS
  timestamp += " | pid=" + juce::String(static_cast<int>(::GetCurrentProcessId()));
#endif
  (void)logs_dir.getChildFile(filename).appendText(timestamp + " | " + message + "\n",
                                                false, false, "\n");
}
}
  #define HELMBOY_STARTUP_TRACE(filename, message) ::helmboy::appendStartupTrace(filename, message)
#else
  #define HELMBOY_STARTUP_TRACE(filename, message) ((void)0)
#endif