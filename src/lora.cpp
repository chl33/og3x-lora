// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/lora.h>

namespace og3 {

LoRaModule::LoRaModule(const char* name, const LoRaModule::Options& options, App* app,
                       VariableGroup& vg, const std::function<void()>& on_initialized)
    : Module(name, &app->module_system()),
      m_options(options),
      m_on_initialized(on_initialized),
      m_app(app) {
  add_init_fn([this]() {
    // setup LoRa transceiver module
    m_app->log().debug("Calling LoRa.setPins().");
    LoRa.setPins(m_options.gpio_ss, m_options.gpio_rst, m_options.gpio_dio0);
    if (m_on_initialized) {
      m_app->log().debug("Calling LoRaModule on_initialized().");
      m_on_initialized();
      m_app->log().debug("LoRaModule on_initialized() finished.");
    }
  });
  add_start_fn([this]() { this->setup_lora(); });
}

void LoRaModule::setup_lora() {
  m_init_tries += 1;
  m_app->log().debugf("Calling LoRa.begin() (try %u/%u).", m_init_tries, m_options.max_setup_tries);
  if (!LoRa.begin(m_options.frequency)) {
    m_app->log().logf("Failed to setup LoRa: %u/%u tries.", m_init_tries,
                      m_options.max_setup_tries);
    if (m_init_tries < m_options.max_setup_tries) {
      m_app->tasks().runIn(500, [this]() { setup_lora(); });
    } else {
      m_app->log().logf("Failed to setup LoRa.");
    }
    return;
  }
  m_is_ok = true;
  m_app->log().logf("Setup LoRa in %u tries.", m_init_tries);
}

}  // namespace og3
