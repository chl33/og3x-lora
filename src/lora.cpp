// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/config_interface.h>
#include <og3/lora.h>

namespace og3 {
namespace lora {

namespace {
unsigned varFlag(const LoRaModule::Options& opts, LoRaModule::OptionSelect osel) {
  auto check_set = [&osel](unsigned flags, LoRaModule::OptionSelect selmask, unsigned flag,
                           bool invert = false) -> unsigned {
    bool check = static_cast<unsigned>(osel) & static_cast<unsigned>(selmask);
    if (invert) {
      check = !check;
    }
    if (check) {
      flags |= flag;
    }
    return flags;
  };
  unsigned flags = check_set(0x0, opts.publish_options, VariableBase::kNoPublish, true);
  flags = check_set(flags, opts.config_options, VariableBase::kConfig);
  return check_set(flags, opts.settable_options, VariableBase::kSettable);
}
}  // namespace

const char* SpreadingFactorVariable::kNames[6] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
const char* SignalBandwidthVariable::kNames[2] = {"125k", "500k"};
const char* FrequencyVariable::kNames[3] = {"433MHz", "k868MHz", "k915MHz"};

const char SpreadingFactorVariable::kName[] = "spreading_factor";
const char SpreadingFactorVariable::kDesc[] = "spreading factor";

const char SignalBandwidthVariable::kName[] = "signal_bandwidth";
const char SignalBandwidthVariable::kDesc[] = "signal_bandwidth";

const char FrequencyVariable::kName[] = "frequency";

SpreadingFactorVariable::SpreadingFactorVariable(const char* name, SpreadingFactor value,
                                                 const char* desc, unsigned flags,
                                                 VariableGroup& vg)
    : EnumStrVariable<SpreadingFactor>(name, value, desc, SpreadingFactor::kSF10, kNames, flags,
                                       vg) {}

SignalBandwidthVariable::SignalBandwidthVariable(const char* name, SignalBandwidth value,
                                                 const char* desc, unsigned flags,
                                                 VariableGroup& vg)
    : EnumStrVariable<SignalBandwidth>(name, value, desc, SignalBandwidth::k500kHz, kNames, flags,
                                       vg) {}

FrequencyVariable::FrequencyVariable(const char* name, Frequency value, const char* desc,
                                     unsigned flags, VariableGroup& vg)
    : EnumStrVariable<Frequency>(name, value, desc, Frequency::k915MHz, kNames, flags, vg) {}

unsigned SpreadingFactorVariable::to_uint() const {
  switch (value()) {
    case SpreadingFactor::kSF7:
      return 7;
    case SpreadingFactor::kSF8:
      return 8;
    case SpreadingFactor::kSF9:
      return 9;
    case SpreadingFactor::kSF10:
      return 10;
    case SpreadingFactor::kSF11:
      return 11;
    case SpreadingFactor::kSF12:
      return 12;
  }
  return 8;
}

unsigned SignalBandwidthVariable::to_uint() const {
  switch (value()) {
    case SignalBandwidth::k125kHz:
      return 125 * kThousand;
    case SignalBandwidth::k500kHz:
      return 500 * kThousand;
  }
  return 125 * kThousand;
}

unsigned FrequencyVariable::to_uint() const {
  switch (value()) {
    case Frequency::k433MHz:
      return kMillion * 433;
    case Frequency::k868MHz:
      return kMillion * 868;
    case Frequency::k915MHz:
      return kMillion * 915;
  }
  return kMillion * 915;
}

// Values to keep transmission time under 400msec.
// See https://www.thethingsnetwork.org/airtime-calculator/
unsigned usa_max_payload_bytes(SpreadingFactor spreading_factor, SignalBandwidth signal_bandwidth) {
  switch (signal_bandwidth) {
    case SignalBandwidth::k125kHz:
      switch (spreading_factor) {
        case SpreadingFactor::kSF7:
          return 242;
        case SpreadingFactor::kSF8:
          return 125;
        case SpreadingFactor::kSF9:
          return 53;
        case SpreadingFactor::kSF10:
          return 11;
        case SpreadingFactor::kSF11:
        case SpreadingFactor::kSF12:
          break;  // too slow
      }
      break;
    case SignalBandwidth::k500kHz:
      switch (spreading_factor) {
        case SpreadingFactor::kSF7:
          return 222;
        case SpreadingFactor::kSF8:
          return 222;
        case SpreadingFactor::kSF9:
          return 222;
        case SpreadingFactor::kSF10:
          return 222;
        case SpreadingFactor::kSF11:
          return 109;
        case SpreadingFactor::kSF12:
          return 33;
      }
      break;
  }
  return 0;
}

#if 0
Variable<unsigned> s_lora_sync_word("sync_word", 0xF3, nullptr, "sync word", kLoraVarFlags,
                                    s_lora_vg);
#endif

}  // namespace lora

const char LoRaModule::kConfigUrl[] = "/lora/update";

LoRaModule::LoRaModule(const char* name, const LoRaModule::Options& options, App* app,
                       VariableGroup& vg, const std::function<void()>& on_initialized)
    : Module(name, &app->module_system()),
      m_app(app),
      m_dependencies(ConfigInterface::kName),
      m_vg(vg),
      m_on_initialized(on_initialized),
      m_gpio_ss(options.gpio_ss),
      m_gpio_rst(options.gpio_rst),
      m_gpio_dio0(options.gpio_dio0),
      m_max_setup_tries(options.max_setup_tries),
      m_str_sync_word(std::string(name) + "_sync_word"),
      m_str_enable_crc(std::string(name) + "_enable_crc"),
      m_str_frequency(std::string(name) + "_" + lora::FrequencyVariable::kName),
      m_str_spreading_factor(std::string(name) + "_" + lora::SpreadingFactorVariable::kName),
      m_str_signal_bandwidth(std::string(name) + "_" + lora::SignalBandwidthVariable::kName),
      m_sync_word(m_str_sync_word.c_str(), options.sync_word, nullptr, nullptr,
                  lora::varFlag(options, OptionSelect::kSyncWord), vg),
      m_enable_crc(m_str_enable_crc.c_str(), options.enable_crc, nullptr,
                   lora::varFlag(options, OptionSelect::kEnableCrc), vg),
      m_frequency(m_str_frequency.c_str(), options.frequency, nullptr,
                  lora::varFlag(options, OptionSelect::kFrequency), vg),
      m_spreading_factor(m_str_spreading_factor.c_str(), options.spreading_factor, nullptr,
                         lora::varFlag(options, OptionSelect::kSpreadingFactor), vg),
      m_signal_bandwidth(m_str_signal_bandwidth.c_str(), options.signal_bandwidth, nullptr,
                         lora::varFlag(options, OptionSelect::kSignalBandwidth), vg) {
  setDependencies(&m_dependencies);
  add_link_fn([this](og3::NameToModule& name_to_module) -> bool {
    m_config = ConfigInterface::get(name_to_module);
    return m_config != nullptr;
  });
  add_init_fn([this]() {
    if (m_config) {
      m_config->read_config(m_vg);
    }
    // setup LoRa transceiver module
    m_app->log().debug("Calling LoRa.setPins().");
    LoRa.setPins(m_gpio_ss, m_gpio_rst, m_gpio_dio0);
    if (m_on_initialized) {
      LoRa.setSpreadingFactor(m_spreading_factor.to_uint());
      LoRa.setSignalBandwidth(m_signal_bandwidth.to_uint());
      LoRa.setSyncWord(m_sync_word.value());
      if (m_enable_crc.value()) {
        LoRa.enableCrc();
      }
      m_app->log().debug("Calling LoRaModule on_initialized().");
      m_on_initialized();
      m_app->log().debug("LoRaModule on_initialized() finished.");
    }
  });
  add_start_fn([this]() { this->setup_lora(); });
}

void LoRaModule::load_config() {
  if (!m_config || !m_on_initialized) {
    return;
  }
  LoRa.setFrequency(m_frequency.to_uint());
  LoRa.setSpreadingFactor(m_spreading_factor.to_uint());
  LoRa.setSignalBandwidth(m_signal_bandwidth.to_uint());
  LoRa.setSyncWord(m_sync_word.value());
  if (m_enable_crc.value()) {
    LoRa.enableCrc();
  } else {
    LoRa.disableCrc();
  }
}

void LoRaModule::setup_lora() {
  m_init_tries += 1;
  m_app->log().debugf("Calling LoRa.begin() (try %u/%u).", m_init_tries, m_max_setup_tries);
  if (!LoRa.begin(m_frequency.to_uint())) {
    m_app->log().logf("Failed to setup LoRa: %u/%u tries.", m_init_tries, m_max_setup_tries);
    if (m_init_tries < m_max_setup_tries) {
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
