// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <RadioLib.h>
#include <SPI.h>
#include <og3/config_interface.h>
#include <og3/lora.h>
#include <sys/types.h>

#include <memory>

namespace og3 {
namespace lora {

namespace {
unsigned varFlag(const LoRaModule::Options& opts, LoRaModule::OptionSelect osel) {
  auto check_set = [&osel](unsigned flags, LoRaModule::OptionSelect selmask, unsigned flag,
                           bool invert = false) -> unsigned {
    const bool bit_set = osel & selmask;
    if (invert ? !bit_set : bit_set) {
      flags |= flag;
    }
    return flags;
  };
  unsigned flags = check_set(0x0, opts.publish_options, VariableBase::kNoPublish, true);
  flags = check_set(flags, opts.config_options, VariableBase::kConfig);
  return check_set(flags, opts.settable_options, VariableBase::kSettable);
}

LoRaModule* s_lora_module = nullptr;
}  // namespace

const char* SpreadingFactorVariable::kNames[6] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
const char* SignalBandwidthVariable::kNames[2] = {"125k", "500k"};
const char* FrequencyVariable::kNames[3] = {"433MHz", "868MHz", "915MHz"};

const char SpreadingFactorVariable::kName[] = "spreading_factor";
const char SpreadingFactorVariable::kDesc[] = "spreading factor";

const char SignalBandwidthVariable::kName[] = "signal_bandwidth";
const char SignalBandwidthVariable::kDesc[] = "signal_bandwidth";

const char FrequencyVariable::kName[] = "frequency";

SpreadingFactorVariable::SpreadingFactorVariable(SpreadingFactor value, unsigned flags,
                                                 VariableGroup& vg)
    : EnumStrVariable<SpreadingFactor>(kName, value, kDesc, SpreadingFactor::kSF12, kNames, flags,
                                       vg) {}

SignalBandwidthVariable::SignalBandwidthVariable(SignalBandwidth value, unsigned flags,
                                                 VariableGroup& vg)
    : EnumStrVariable<SignalBandwidth>(kName, value, kDesc, SignalBandwidth::k500kHz, kNames, flags,
                                       vg) {}

FrequencyVariable::FrequencyVariable(Frequency value, unsigned flags, VariableGroup& vg)
    : EnumStrVariable<Frequency>(kName, value, kName, Frequency::k915MHz, kNames, flags, vg) {}

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

}  // namespace lora

const char LoRaModule::kName[] = "lora";
const char LoRaModule::kConfigUrl[] = "/lora/update";

LoRaModule::LoRaModule(const LoRaModule::Options& options, App* app, VariableGroup& vg,
                       const std::function<void()>& on_initialized)
    : Module(kName, &app->module_system()),
      m_app(app),
      m_vg(vg),
      m_on_initialized(on_initialized),
      m_gpio_ss(options.gpio_ss),
      m_gpio_rst(options.gpio_rst),
      m_gpio_dio0(options.gpio_dio0),
      m_gpio_dio1(options.gpio_dio1),
      m_max_setup_tries(options.max_setup_tries),
      m_sync_word("sync_word", options.sync_word, nullptr, "sync word",
                  lora::varFlag(options, kOptionSyncWord), vg),
      m_enable_crc("enable_crc", options.enable_crc, "enable crc",
                   lora::varFlag(options, kOptionEnableCrc), vg),
      m_frequency(options.frequency, lora::varFlag(options, kOptionFrequency), vg),
      m_spreading_factor(options.spreading_factor, lora::varFlag(options, kOptionSpreadingFactor),
                         vg),
      m_signal_bandwidth(options.signal_bandwidth, lora::varFlag(options, kOptionSignalBandwidth),
                         vg),
      m_max_payload("max_payload", 0, nullptr, "max payload", 0, vg) {
  if (options.on_transmit_done) {
    set_on_transmit_done(options.on_transmit_done);
  }
  require(ConfigInterface::kName, &m_config);
  add_init_fn([this]() {
    if (m_config) {
      m_config->read_config(m_vg);
    }
    m_max_payload =
        lora::usa_max_payload_bytes(m_spreading_factor.value(), m_signal_bandwidth.value());
  });
  add_start_fn([this]() { this->setup_lora(); });
}

void LoRaModule::set_on_transmit_done(const std::function<void()>& fn) {
  m_on_transmit_done = fn;
  if (!lora::s_lora_module) {
    lora::s_lora_module = this;
  }
}

void LoRaModule::config_lora() {
  if (!m_radio) {
    return;
  }
  m_radio->setFrequency(m_frequency.to_uint() / 1000000.0f);
  m_radio->setSpreadingFactor(m_spreading_factor.to_uint());
  m_radio->setBandwidth(m_signal_bandwidth.to_uint() / 1000.0f);
  m_radio->setSyncWord(static_cast<uint8_t>(m_sync_word.value()));
  if (m_enable_crc.value()) {
    m_radio->setCRC(true);
  } else {
    m_radio->setCRC(false);
  }
  m_app->log().logf("LoRa configured: %s %s bw:%s sync_word:0x%02x%s usa:pktsize:%u",
                    m_frequency.string().c_str(), m_spreading_factor.string().c_str(),
                    m_signal_bandwidth.string().c_str(), m_sync_word.value(),
                    m_enable_crc.value() ? " (CRC)" : "", usa_max_payload());

  // Start in receive mode
  m_radio->startReceive();
}

void LoRaModule::setup_lora() {
  m_init_tries += 1;
  m_app->log().debugf("Initializing RadioLib (try %u/%u).", m_init_tries, m_max_setup_tries);

  if (!m_mod) {
    m_mod = new ::Module(m_gpio_ss, m_gpio_dio0, m_gpio_rst, m_gpio_dio1);
    m_radio = new ::SX1276(m_mod);
  }

  int state = m_radio->begin(m_frequency.to_uint() / 1000000.0f);
  if (state != RADIOLIB_ERR_NONE) {
    m_app->log().logf("Failed to setup LoRa: %u/%u tries (code %d).", m_init_tries,
                      m_max_setup_tries, state);
    if (m_init_tries < m_max_setup_tries) {
      m_app->tasks().runIn(500, [this]() { setup_lora(); });
    } else {
      m_app->log().logf("Failed to setup LoRa.");
    }
    return;
  }

  m_is_ok = true;
  m_app->log().logf("Setup LoRa in %u tries.", m_init_tries);
  config_lora();

  if (m_on_initialized) {
    m_app->log().debug("Calling LoRaModule on_initialized().");
    m_on_initialized();
  }
}

void LoRaModule::transmit_done_callback() {
  m_app->tasks().runIn(1, [this]() {
    if (m_on_transmit_done) {
      m_on_transmit_done();
    }
    m_is_transmitting = false;
    // Go back to receive mode
    if (m_radio) {
      m_radio->startReceive();
    }
  });
}

void LoRaModule::send_packet(const u_int8_t* buffer, size_t num_bytes) {
  if (!m_is_ok || !m_radio) {
    return;
  }
  m_is_transmitting = true;
  // RadioLib transmit is blocking by default.
  int state = m_radio->transmit(const_cast<uint8_t*>(buffer), num_bytes);
  if (state != RADIOLIB_ERR_NONE) {
    m_app->log().logf("LoRa transmit failed (code %d).", state);
  }
  transmit_done_callback();
}

int LoRaModule::poll_packet(uint8_t* buffer, size_t max_bytes) {
  if (!m_is_ok || !m_radio || m_is_transmitting) {
    return 0;
  }

  // Check if a packet was received (non-blocking)
  // In RadioLib, we can check if the IRQ for RX done is set.
  // For SX127x, this is usually DIO0.
  // If we don't have interrupts, we can check the IRQ flags.
  if (::digitalRead(m_gpio_dio0) == HIGH) {
    int state = m_radio->readData(buffer, max_bytes);
    if (state == RADIOLIB_ERR_NONE) {
      m_last_rssi = m_radio->getRSSI();
      m_last_snr = m_radio->getSNR();
      int len = m_radio->getPacketLength();
      // Restart receive mode
      m_radio->startReceive();
      return len;
    } else {
      // Error or CRC mismatch, restart receive anyway
      m_radio->startReceive();
    }
  }
  return 0;
}

void LoRaModule::sleep() {
  if (m_radio) {
    m_radio->sleep();
  }
}

void LoRaModule::standby() {
  if (m_radio) {
    m_radio->standby();
  }
}

}  // namespace og3
