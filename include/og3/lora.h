// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <LoRa.h>
#include <og3/app.h>
#include <og3/constants.h>
#include <og3/variable.h>

#include <functional>

namespace og3 {

class LoRaModule : public Module {
 public:
  static constexpr unsigned kMaxInitTries = 10;

  struct Options {
    int gpio_ss = 5;    // ESP32 GPIO for SS
    int gpio_rst = 14;  // ESP32 GPIO for RST
    int gpio_dio0 = 2;  // ESP32 GPIO for DIO0
    long frequency = 915 * kMillion;
    int max_setup_tries = 5;
  };

  // on_initialized is called when LoRa.begin() has succeeded, for setting-up the module.
  LoRaModule(const char* name, const Options& options, App* app, VariableGroup& vg,
	     const std::function<void()>& on_initialized);

  bool is_ok() const { return m_is_ok; }

 private:
  void setup_lora();

  const Options m_options;
  const std::function<void()>& m_on_initialized;
  App* m_app;
  unsigned m_init_tries = 0;
  bool m_is_ok = false;
};


}  // namespace og3
