// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <LoRa.h>
#include <og3/app.h>
#include <og3/constants.h>
#include <og3/variable.h>
#include <sys/types.h>

#include <functional>
#include <string>

namespace og3 {
class ConfigInterface;

namespace lora {

enum class SpreadingFactor { kSF7, kSF8, kSF9, kSF10, kSF11, kSF12 };
enum class SignalBandwidth { k125kHz, k500kHz };
enum class Frequency { k433MHz, k868MHz, k915MHz };

class SpreadingFactorVariable : public EnumStrVariable<SpreadingFactor> {
 public:
  static const char kName[];
  static const char kDesc[];
  SpreadingFactorVariable(SpreadingFactor value, unsigned flags, VariableGroup& vg);
  unsigned to_uint() const;

  static constexpr SpreadingFactor kDefault = SpreadingFactor::kSF7;
  static const char* kNames[6];
};

class SignalBandwidthVariable : public EnumStrVariable<SignalBandwidth> {
 public:
  static const char kName[];
  static const char kDesc[];
  SignalBandwidthVariable(SignalBandwidth value, unsigned flags, VariableGroup& vg);
  unsigned to_uint() const;
  static constexpr SignalBandwidth kDefault = SignalBandwidth::k125kHz;
  static const char* kNames[2];
};

class FrequencyVariable : public EnumStrVariable<Frequency> {
 public:
  static const char kName[];
  FrequencyVariable(Frequency value, unsigned flags, VariableGroup& vg);
  unsigned to_uint() const;

  static constexpr Frequency kDefault = Frequency::k915MHz;
  static const char* kNames[3];
};

// Values to keep transmission time under 400msec.
unsigned usa_max_payload_bytes(SpreadingFactor spreading_factor, SignalBandwidth signal_bandwidth);

}  // namespace lora

// LoRaModule helps automatically setup a LoRa radio module.
// NOTE: The transmitt-done callback is not working for me right now.
class LoRaModule : public Module {
 public:
  static const char kName[];
  static const char kConfigUrl[];

  enum OptionSelect {
    kOptionNone = 0x0,
    kOptionAll = 0x1F,
    kOptionSyncWord = 0x1,
    kOptionEnableCrc = 0x2,
    kOptionFrequency = 0x4,
    kOptionSpreadingFactor = 0x8,
    kOptionSignalBandwidth = 0x10,
  };

  struct Options {
    int gpio_ss = 5;    // ESP32 GPIO for SS
    int gpio_rst = 14;  // ESP32 GPIO for RST
    int gpio_dio0 = 2;  // ESP32 GPIO for DIO0
    int max_setup_tries = 5;

    unsigned sync_word = 0x01;  // Select something between 0-0xFF
    bool enable_crc = false;
    std::function<void()> on_transmit_done = nullptr;

    lora::Frequency frequency = lora::FrequencyVariable::kDefault;
    lora::SpreadingFactor spreading_factor = lora::SpreadingFactorVariable::kDefault;
    lora::SignalBandwidth signal_bandwidth = lora::SignalBandwidthVariable::kDefault;

    // Which options are published via MQTT
    OptionSelect publish_options = OptionSelect::kOptionNone;
    // Which options are saved/loaded from Flash
    OptionSelect config_options = OptionSelect::kOptionAll;
    // Which options settable via web form.
    OptionSelect settable_options = OptionSelect::kOptionAll;
  };

  // on_initialized is called when LoRa.begin() has succeeded, for setting-up the module.
  LoRaModule(const Options& options, App* app, VariableGroup& vg,
             const std::function<void()>& on_initialized = nullptr);

  bool is_ok() const { return m_is_ok; }
  bool is_transmitting() const { return m_is_transmitting; }
  unsigned usa_max_payload() const {
    return lora::usa_max_payload_bytes(m_spreading_factor.value(), m_signal_bandwidth.value());
  }
  void set_on_transmit_done(const std::function<void()>& fn);

  void send_packet(const u_int8_t* buffer, size_t num_bytes);

  // This is called by an interrupt.
  void transmit_done_callback();

 private:
  void setup_lora();
  void load_config();
  void config_lora();

  App* m_app;
  SingleDependency m_dependencies;
  VariableGroup& m_vg;
  const std::function<void()> m_on_initialized;
  const int m_gpio_ss;
  const int m_gpio_rst;
  const int m_gpio_dio0;
  const int m_max_setup_tries;
  std::function<void()> m_on_transmit_done;

  Variable<unsigned> m_sync_word;
  BoolVariable m_enable_crc;
  lora::FrequencyVariable m_frequency;
  lora::SpreadingFactorVariable m_spreading_factor;
  lora::SignalBandwidthVariable m_signal_bandwidth;
  Variable<unsigned> m_max_payload;

  ConfigInterface* m_config = nullptr;

  unsigned m_init_tries = 0;
  bool m_is_ok = false;
  bool m_is_transmitting = false;
};

}  // namespace og3
