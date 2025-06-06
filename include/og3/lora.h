// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <LoRa.h>
#include <og3/app.h>
#include <og3/constants.h>
#include <og3/variable.h>

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
  SpreadingFactorVariable(const char* name, SpreadingFactor value, const char* desc, unsigned flags,
                          VariableGroup& vg);
  unsigned to_uint() const;

  static constexpr SpreadingFactor kDefault = SpreadingFactor::kSF7;
  static const char* kNames[6];
};

class SignalBandwidthVariable : public EnumStrVariable<SignalBandwidth> {
 public:
  static const char kName[];
  static const char kDesc[];
  SignalBandwidthVariable(const char* name, SignalBandwidth value, const char* desc, unsigned flags,
                          VariableGroup& vg);
  unsigned to_uint() const;
  static constexpr SignalBandwidth kDefault = SignalBandwidth::k125kHz;
  static const char* kNames[2];
};

class FrequencyVariable : public EnumStrVariable<Frequency> {
 public:
  static const char kName[];
  FrequencyVariable(const char* name, Frequency value, const char* desc, unsigned flags,
                    VariableGroup& vg);
  unsigned to_uint() const;

  static constexpr Frequency kDefault = Frequency::k915MHz;
  static const char* kNames[3];
};

// Values to keep transmission time under 400msec.
unsigned usa_max_payload_bytes(SpreadingFactor spreading_factor, SignalBandwidth signal_bandwidth);

}  // namespace lora

// LoRaModule helps automatically setup a LoRa radio module.
class LoRaModule : public Module {
 public:
  static const char kConfigUrl[];

  enum class OptionSelect {
    kNone = 0x0,
    kAll = 0x1F,
    kSyncWord = 0x1,
    kEnableCrc = 0x2,
    kFrequency = 0x4,
    kSpreadingFactor = 0x8,
    kSignalBandwidth = 0x10,
  };

  struct Options {
    int gpio_ss = 5;    // ESP32 GPIO for SS
    int gpio_rst = 14;  // ESP32 GPIO for RST
    int gpio_dio0 = 2;  // ESP32 GPIO for DIO0
    int max_setup_tries = 5;

    unsigned sync_word = 0x01;  // Select something between 0-0xFF
    bool enable_crc = false;

    lora::Frequency frequency = lora::FrequencyVariable::kDefault;
    lora::SpreadingFactor spreading_factor = lora::SpreadingFactorVariable::kDefault;
    lora::SignalBandwidth signal_bandwidth = lora::SignalBandwidthVariable::kDefault;

    // Which options are published via MQTT
    OptionSelect publish_options = OptionSelect::kNone;
    // Which options are saved/loaded from Flash
    OptionSelect config_options = OptionSelect::kAll;
    // Which options settable via web form.
    OptionSelect settable_options = OptionSelect::kAll;
  };

  // on_initialized is called when LoRa.begin() has succeeded, for setting-up the module.
  LoRaModule(const char* name, const Options& options, App* app, VariableGroup& vg,
             const std::function<void()>& on_initialized);

  bool is_ok() const { return m_is_ok; }

 private:
  void load_config();
  void setup_lora();

  App* m_app;
  SingleDependency m_dependencies;
  VariableGroup& m_vg;
  const std::function<void()> m_on_initialized;
  const int m_gpio_ss;
  const int m_gpio_rst;
  const int m_gpio_dio0;
  const int m_max_setup_tries;

  std::string m_str_sync_word;
  std::string m_str_enable_crc;
  std::string m_str_frequency;
  std::string m_str_spreading_factor;
  std::string m_str_signal_bandwidth;

  Variable<unsigned> m_sync_word;
  BoolVariable m_enable_crc;
  lora::FrequencyVariable m_frequency;
  lora::SpreadingFactorVariable m_spreading_factor;
  lora::SignalBandwidthVariable m_signal_bandwidth;

  ConfigInterface* m_config = nullptr;

  unsigned m_init_tries = 0;
  bool m_is_ok = false;
};

}  // namespace og3
