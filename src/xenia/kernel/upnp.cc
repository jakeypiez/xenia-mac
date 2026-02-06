/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2023 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/upnp.h"
#include "util/net_utils.h"
#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"

#include <chrono>
#include <thread>

#include "third_party/miniupnp/miniupnpc/include/miniwget.h"
#include "third_party/miniupnp/miniupnpc/include/upnpcommands.h"

DEFINE_string(upnp_root, "", "UPnP Root Device", "Live");

DEFINE_bool(upnp, false, "Automatically port forward using UPnP", "Live");

DECLARE_bool(logging);

using namespace xe::threading;

// Credit:
// https://github.com/RPCS3/rpcs3/blob/05b6108c66b2cb5bdbb97ff2412d4d39aa7dc331/rpcs3/Emu/NP/upnp_handler.cpp
namespace xe {
namespace kernel {

UPnP::UPnP() {}

UPnP::~UPnP() {
  std::lock_guard lock(mutex_);

  for (const auto& [protocol, prot_bindings] : port_bindings_) {
    for (const auto& [internal_port, external_port] : prot_bindings) {
      RemovePortExternal(external_port, protocol);
    }
  }

  delete igd_data_;
  delete igd_urls_;

  active_ = false;
}

bool UPnP::GetAndParseUPnPXmlData(std::string url) {
  int xml_description_size = 0;
  int status_code = 0;

  std::memset(igd_data_, 0, sizeof(IGDdatas));
  std::memset(igd_urls_, 0, sizeof(UPNPUrls));

  const char* xml_description = static_cast<const char*>(
      miniwget(url.c_str(), &xml_description_size, 1, &status_code));

  if (!xml_description) {
    return false;
  }

  parserootdesc(xml_description, xml_description_size, igd_data_);
  GetUPNPUrls(igd_urls_, igd_data_, url.c_str(), 1);
  delete xml_description;
  return true;
}

bool UPnP::LoadSavedUPnPDevice() {
  const std::string device_url = cvars::upnp_root;

  if (device_url.empty()) {
    return false;
  }

  if (!GetAndParseUPnPXmlData(device_url)) {
    XELOGI("UPnP: Saved UPnP({}) device isn't available anymore", device_url);
    return false;
  }

  XELOGI("UPnP: Saved UPnP({}) enabled", device_url);
  RefreshPortsTimer();
  active_ = true;
  return true;
}

const UPNPDev* UPnP::GetDeviceByName(const UPNPDev* device_list,
                                     std::string device_name) {
  const UPNPDev* device = device_list;

  for (; device; device = device->pNext) {
    // No more devices, we haven't found anything.
    if (!device) {
      return device;
    }

    const std::string current_device_name = std::string(device->st);
    if (current_device_name.find(device_name) != std::string::npos) {
      break;
    }
  }
  return device;
}

std::string UPnP::DiscoverUPnPDevice() {
  XELOGI("UPnP: Starting UPnP search");

  const char* mcast_if = multicast_interface_.empty()
                             ? nullptr
                             : multicast_interface_.c_str();
  if (mcast_if) {
    XELOGI("UPnP: Using multicast interface: {}", mcast_if);
  }

  // Pass "" for minissdpdsock to skip minissdpd (not available on macOS)
  int error = 0;
  UPNPDev* device_list = upnpDiscover(5000, mcast_if, "", 0, 0, 2, &error);
  if (error) {
    XELOGE("UPnP: upnpDiscover error code: {}", error);
    return "";
  }

  if (!device_list) {
    XELOGE("UPnP: No UPnP devices responded (timeout)");
    return "";
  }

  // Log all discovered devices for debugging
  for (const UPNPDev* d = device_list; d; d = d->pNext) {
    XELOGI("UPnP: Discovered device: {} at {}", d->st, d->descURL);
  }

  const UPNPDev* device = GetDeviceByName(device_list, "InternetGatewayDevice");
  std::string descURL = device ? std::string(device->descURL) : "";
  freeUPNPDevlist(device_list);

  if (descURL.empty()) {
    XELOGE("UPnP: No InternetGatewayDevice found in device list");
  }

  return descURL;
}

void UPnP::Initialize(const std::string& multicast_if) {
  std::lock_guard lock(mutex_);

  multicast_interface_ = multicast_if;

  if (LoadSavedUPnPDevice()) {
    return;
  }

  // Retry up to 3 times — the first attempt may fail on macOS while the
  // Local Network permission dialog is showing (sendto returns EHOSTUNREACH
  // until the user clicks Allow).
  for (int attempt = 1; attempt <= 3; ++attempt) {
    if (SearchUPnP()) {
      RefreshPortsTimer();
      active_ = true;
      XELOGI("UPnP: Initialized successfully, ready for port mapping");

      // Re-add any existing port bindings (e.g. after router UPnP reset)
      if (!port_bindings_.empty()) {
        XELOGI("UPnP: Re-adding {} existing port binding(s)",
               port_bindings_.size());
        RefreshPorts(multicast_interface_);
      }
      return;
    }
    if (attempt < 3) {
      XELOGI("UPnP: Attempt {}/3 failed, retrying in 3 seconds...", attempt);
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  }
  XELOGE("UPnP: Initialization failed after 3 attempts, port mapping disabled");
};

void UPnP::Deactivate() {
  std::lock_guard lock(mutex_);
  active_ = false;
  // Clear saved device so next Initialize does a fresh discovery
  cvars::upnp_root = "";
  OVERRIDE_string(upnp_root, cvars::upnp_root);
  XELOGI("UPnP: Deactivated");
}

bool UPnP::SearchUPnP() {
  if (active_) {
    std::lock_guard lock(mutex_);
  }

  std::string descURL = DiscoverUPnPDevice();
  if (descURL.empty()) {
    XELOGE("No UPNP device was found");
    return false;
  }

  if (!GetAndParseUPnPXmlData(descURL)) {
    XELOGE("Failed to retrieve UPNP xml for {}", descURL);
    return false;
  }

  XELOGI("Found UPnP device at {}", descURL);

  cvars::upnp_root = descURL;
  OVERRIDE_string(upnp_root, cvars::upnp_root);
  return true;
};

uint32_t UPnP::AddPort(std::string_view addr, uint16_t internal_port,
                       std::string_view protocol) {
  if (!active_) {
    XELOGW("UPnP: AddPort called but UPnP is not active (no gateway found)");
    return UPNPCOMMAND_UNKNOWN_ERROR;
  }

  std::lock_guard lock(mutex_);

  internal_port = GetMappedBindPort(internal_port);

  const uint16_t external_port = internal_port;
  const std::string internal_port_str = fmt::format("{}", internal_port);
  const std::string external_port_str = fmt::format("{}", external_port);

  int result = UPNP_AddPortMapping(
      igd_urls_->controlURL, igd_data_->first.servicetype,
      external_port_str.c_str(), internal_port_str.c_str(), addr.data(),
      "Xenia", protocol.data(), nullptr, "3600");

  if (result == OnlyPermanentLeasesSupported) {
    XELOGI("Router only supports permanent lease times on port mappings.");
    result = UPNP_AddPortMapping(
        igd_urls_->controlURL, igd_data_->first.servicetype,
        external_port_str.c_str(), internal_port_str.c_str(), addr.data(),
        "Xenia", protocol.data(), nullptr, "0");
  }

  if (result != UPNPCOMMAND_SUCCESS) {
    if (result == HTTP_UNAUTHORIZED) {
      XELOGI("UPnP Unauthorized!");
    }

    XELOGI("Failed to bind port!!! {}:{}({}) to IGD:{}", addr, internal_port,
           protocol, external_port);

    XELOGI("UPnP error code {}", result);
    port_binding_results_[std::string(protocol)][external_port] = result;
    return result;
  }

  const bool is_bound =
      port_bindings_[std::string(protocol)].find(internal_port) !=
      port_bindings_[std::string(protocol)].cend();

  std::string message_type = "updated";
  if (!is_bound) {
    port_bindings_[std::string(protocol)][internal_port] = external_port;
    message_type = "bound";
  }

  XELOGI("Successfully {} {}:{}({}) to IGD:{}", message_type, addr,
         internal_port, protocol, external_port);

  port_binding_results_[std::string(protocol)][external_port] = result;

  return result;
}

void UPnP::RemovePort(uint16_t internal_port, std::string_view protocol) {
  if (!active_) {
    return;
  }

  std::lock_guard lock(mutex_);

  internal_port = GetMappedBindPort(internal_port);

  const std::string str_protocol(protocol);

  if (port_bindings_.find(str_protocol) == port_bindings_.cend()) {
    return;
  }

  if (port_bindings_.at(str_protocol).find(internal_port) ==
      port_bindings_.at(str_protocol).cend()) {
    XELOGE("Tried to unbind port mapping {} to IGD({}) but it isn't bound",
           internal_port, protocol);
    return;
  }

  const uint16_t external_port =
      port_bindings_.at(str_protocol).at(internal_port);

  RemovePortExternal(external_port, protocol);

  XELOGE("Successfully deleted port mapping {} to IGD:{}({})", internal_port,
         external_port, protocol);
}

void UPnP::RemovePortExternal(uint16_t external_port, std::string_view protocol,
                              bool verbose) {
  const std::string str_ext_port = fmt::format("{}", external_port);
  const int result = UPNP_DeletePortMapping(
      igd_urls_->controlURL, igd_data_->first.servicetype, str_ext_port.c_str(),
      protocol.data(), nullptr);

  if (result != 0 && verbose) {
    XELOGE("Failed to delete port mapping IGD:{}({}): {}", str_ext_port,
           protocol, result);
  }
}

void UPnP::RefreshPorts(std::string_view addr) {
  if (!leases_supported_) {
    return;
  }

  for (const auto& [protocol, port_bindings] : port_bindings_) {
    for (const auto& [internal_port, external_port] : port_bindings) {
      AddPort(addr, external_port, protocol);
    }
  }
}

// Update the UPnP lease time every 45 minutes
// Not using HighResolutionTimer because it's only used for small tasks.
void UPnP::RefreshPortsTimer() {
  // Only setup timer once
  if (active_) {
    return;
  }

  const auto interval = std::chrono::minutes(45);

  auto run = [&](void*) { RefreshPorts(GetLocalIP()); };

  wait_item_ = QueueTimerRecurring(run, nullptr,
                                   TimerQueueWaitItem::clock::now(), interval);
}

uint16_t UPnP::GetMappedConnectPort(uint16_t external_port) {
  if (mapped_connect_ports_.find(external_port) !=
      mapped_connect_ports_.end()) {
    return mapped_connect_ports_[external_port];
  }

  if (mapped_connect_ports_.find(0) != mapped_connect_ports_.end()) {
    if (cvars::logging) {
      XELOGI("Using wildcard connect port for guest port {}!", external_port);
    }

    return mapped_connect_ports_[0];
  }

  if (cvars::logging) {
    XELOGI("Using connect port {}", external_port);
  }

  return external_port;
}

uint16_t UPnP::GetMappedBindPort(uint16_t external_port) {
  if (mapped_bind_ports_.find(external_port) != mapped_bind_ports_.end()) {
    return mapped_bind_ports_[external_port];
  }

  if (mapped_bind_ports_.find(0) != mapped_bind_ports_.end()) {
    if (cvars::logging) {
      XELOGI("Using wildcard bind port for guest port {}!", external_port);
    }

    return mapped_bind_ports_[0];
  }

  return external_port;
}

const bool UPnP::GetRefreshedUnauthorized() const {
  return refreshed_unauthorized_;
}

void UPnP::SetRefreshedUnauthorized(const bool refreshed) {
  refreshed_unauthorized_ = refreshed;
}

const std::string UPnP::GetLocalIP() {
  char lanaddr[64] = "";
  int size = 0;
  miniwget_getaddr(cvars::upnp_root.c_str(), &size, lanaddr, sizeof(lanaddr), 0,
                   0);

  return lanaddr;
}

}  // namespace kernel
}  // namespace xe