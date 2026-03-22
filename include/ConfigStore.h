#pragma once

#include <Preferences.h>

#include "Types.h"

class ConfigStore {
 public:
  bool load(DeviceConfig& config);
  bool save(const DeviceConfig& config);
  void clear();

 private:
  static constexpr const char* kNamespace = "bose-remote";
};

