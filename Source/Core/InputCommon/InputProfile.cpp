// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "InputCommon/InputProfile.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string_view>

#include <fmt/format.h>

#include "Common/FileSearch.h"
#include "Common/FileUtil.h"
#include "Common/StringUtil.h"

#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/HW/Wiimote.h"

#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/InputConfig.h"

namespace InputProfile
{
namespace
{
constexpr int display_message_ms = 3000;
constexpr std::string_view default_profile_extension = ".auto";

std::string GetDeviceIdentity(const std::string& device)
{
  ciface::Core::DeviceQualifier qualifier;
  qualifier.FromString(device);
  qualifier.cid = -1;
  return qualifier.ToString();
}

std::string GetDefaultProfileMarkerPath(const std::string& profile_path)
{
  return profile_path + std::string(default_profile_extension);
}

std::optional<std::string> ReadDefaultProfileIdentity(const std::string& profile_path)
{
  if (!File::Exists(GetDefaultProfileMarkerPath(profile_path)))
    return std::nullopt;

  const auto device = GetProfileDevice(profile_path);
  if (!device)
    return std::nullopt;
  return GetDeviceIdentity(*device);
}

bool LoadProfileForDevice(const std::string& profile_path,
                          ControllerEmu::EmulatedController* controller, InputConfig* input_config,
                          const std::string& device)
{
  if (!LoadProfile(controller, input_config, profile_path, true))
    return false;

  controller->SetDefaultDevice(device);
  controller->UpdateReferences(g_controller_interface);
  return true;
}
}  // namespace

std::vector<std::string> GetUserProfiles(const InputConfig* input_config)
{
  return Common::DoFileSearch(input_config->GetUserProfileDirectoryPath(), ".ini", false);
}

std::optional<std::string> GetProfileDevice(const std::string& profile_path)
{
  Common::IniFile ini_file;
  if (!ini_file.Load(profile_path))
    return std::nullopt;

  std::string device;
  if (!ini_file.GetOrCreateSection("Profile")->Get("Device", &device) || device.empty())
    return std::nullopt;

  return device;
}

bool IsDefaultProfile(const std::string& profile_path)
{
  return File::Exists(GetDefaultProfileMarkerPath(profile_path));
}

bool RemoveDefaultProfile(const std::string& profile_path)
{
  const std::string marker_path = GetDefaultProfileMarkerPath(profile_path);
  return !File::Exists(marker_path) ||
         File::Delete(marker_path, File::IfAbsentBehavior::NoConsoleWarning);
}

bool SetDefaultProfile(InputConfig* input_config, const std::string& profile_path, bool enabled)
{
  if (!enabled)
    return RemoveDefaultProfile(profile_path);

  const auto device = GetProfileDevice(profile_path);
  if (!device)
    return false;

  const std::string identity = GetDeviceIdentity(*device);
  if (identity.empty())
    return false;

  for (const std::string& other_profile : GetUserProfiles(input_config))
  {
    if (other_profile == profile_path || !IsDefaultProfile(other_profile))
      continue;

    const auto other_device = GetProfileDevice(other_profile);
    if (other_device && GetDeviceIdentity(*other_device) == identity)
      File::Delete(GetDefaultProfileMarkerPath(other_profile),
                   File::IfAbsentBehavior::NoConsoleWarning);
  }

  return File::WriteStringToFile(GetDefaultProfileMarkerPath(profile_path), identity);
}

bool HasDefaultProfileForDevice(const InputConfig* input_config, const std::string& device,
                                const std::string& excluded_profile_path)
{
  const std::string identity = GetDeviceIdentity(device);
  return std::ranges::any_of(GetUserProfiles(input_config), [&](const std::string& profile) {
    if (profile == excluded_profile_path || !IsDefaultProfile(profile))
      return false;

    const auto profile_device = GetProfileDevice(profile);
    return profile_device && GetDeviceIdentity(*profile_device) == identity;
  });
}

bool ApplyDefaultProfile(InputConfig* input_config, ControllerEmu::EmulatedController* controller,
                         const std::string& device)
{
  const std::string identity = GetDeviceIdentity(device);
  const auto profiles = GetUserProfiles(input_config);
  const auto profile = std::ranges::find_if(profiles, [&](const std::string& candidate) {
    const auto default_identity = ReadDefaultProfileIdentity(candidate);
    return default_identity && *default_identity == identity;
  });

  return profile != profiles.end() &&
         LoadProfileForDevice(*profile, controller, input_config, device);
}

void ApplyDefaultProfiles(InputConfig* input_config)
{
  const auto devices = g_controller_interface.GetAllDevices();
  std::vector<bool> claimed_devices(devices.size());
  std::vector<bool> matched_controllers(input_config->GetControllerCount());

  const auto apply_device = [&](int controller_index, size_t device_index) {
    auto* controller = input_config->GetController(controller_index);
    ciface::Core::DeviceQualifier connected;
    connected.FromDevice(devices[device_index].get());
    ApplyDefaultProfile(input_config, controller, connected.ToString());
    claimed_devices[device_index] = true;
    matched_controllers[controller_index] = true;
  };

  // Preserve exact device assignments first so identical controllers stay in their current slots.
  for (int controller_index = 0; controller_index < input_config->GetControllerCount();
       ++controller_index)
  {
    if (!input_config->IsControllerControlledByGamepadDevice(controller_index) ||
        HasControllerMappings(input_config->GetController(controller_index)))
      continue;

    const auto& configured = input_config->GetController(controller_index)->GetDefaultDevice();
    for (size_t device_index = 0; device_index < devices.size(); ++device_index)
    {
      if (!claimed_devices[device_index] && configured == devices[device_index].get())
      {
        apply_device(controller_index, device_index);
        break;
      }
    }
  }

  // If a backend changed a device ID after reconnecting, match the same source and device name.
  for (int controller_index = 0; controller_index < input_config->GetControllerCount();
       ++controller_index)
  {
    if (matched_controllers[controller_index] ||
        !input_config->IsControllerControlledByGamepadDevice(controller_index) ||
        HasControllerMappings(input_config->GetController(controller_index)))
    {
      continue;
    }

    const auto& configured = input_config->GetController(controller_index)->GetDefaultDevice();
    for (size_t device_index = 0; device_index < devices.size(); ++device_index)
    {
      if (!claimed_devices[device_index] && devices[device_index]->GetSource() == configured.source &&
          devices[device_index]->GetName() == configured.name)
      {
        apply_device(controller_index, device_index);
        break;
      }
    }
  }
}

bool HasControllerMappings(const ControllerEmu::EmulatedController* controller)
{
  Common::IniFile::Section section;
  const_cast<ControllerEmu::EmulatedController*>(controller)->SaveConfig(&section);
  return std::ranges::any_of(section.GetValues(), [](const auto& entry) {
    return entry.first != "Device" && !entry.second.empty();
  });
}

bool LoadProfile(ControllerEmu::EmulatedController* controller, InputConfig* input_config,
                 const std::string& profile_path, bool replace_existing)
{
  Common::IniFile profile_ini;
  if (!profile_ini.Load(profile_path))
    return false;

  auto* profile = profile_ini.GetOrCreateSection("Profile");
  if (replace_existing)
  {
    controller->LoadConfig(profile);
  }
  else
  {
    Common::IniFile::Section merged;
    controller->SaveConfig(&merged);
    for (const auto& [key, value] : profile->GetValues())
      merged.Set(key, value);
    controller->LoadConfig(&merged);
  }

  controller->UpdateReferences(g_controller_interface);
  input_config->GenerateControllerTextures(profile_ini);
  return true;
}

std::vector<std::string> GetProfilesFromSetting(const std::string& setting, const std::string& root)
{
  const auto& setting_choices = SplitString(setting, ',');

  std::vector<std::string> result;
  for (const std::string& setting_choice : setting_choices)
  {
    const std::string path = root + std::string(StripWhitespace(setting_choice));
    if (File::IsDirectory(path))
    {
      const auto files_under_directory = Common::DoFileSearch(path, ".ini", true);
      result.insert(result.end(), files_under_directory.begin(), files_under_directory.end());
    }
    else
    {
      const std::string file_path = path + ".ini";
      if (File::Exists(file_path))
      {
        result.push_back(file_path);
      }
    }
  }

  return result;
}

std::vector<std::string> ProfileCycler::GetProfilesForDevice(InputConfig* device_configuration)
{
  const std::string device_profile_root_location(
      device_configuration->GetUserProfileDirectoryPath());
  return Common::DoFileSearch(device_profile_root_location, ".ini", true);
}

std::string ProfileCycler::GetProfile(CycleDirection cycle_direction, int& profile_index,
                                      const std::vector<std::string>& profiles)
{
  // update the index and bind it to the number of available strings
  auto positive_modulo = [](int& i, int n) { i = (i % n + n) % n; };
  profile_index += static_cast<int>(cycle_direction);
  positive_modulo(profile_index, static_cast<int>(profiles.size()));

  return profiles[profile_index];
}

void ProfileCycler::UpdateToProfile(const std::string& profile_filename,
                                    ControllerEmu::EmulatedController* controller,
                                    InputConfig* device_configuration)
{
  std::string base;
  SplitPath(profile_filename, nullptr, &base, nullptr);

  Common::IniFile ini_file;
  if (ini_file.Load(profile_filename))
  {
    Core::DisplayMessage("Loading input profile '" + base + "' for device '" +
                             controller->GetName() + "'",
                         display_message_ms);
    controller->LoadConfig(ini_file.GetOrCreateSection("Profile"));
    controller->UpdateReferences(g_controller_interface);
    device_configuration->GenerateControllerTextures(ini_file);
  }
  else
  {
    Core::DisplayMessage("Unable to load input profile '" + base + "' for device '" +
                             controller->GetName() + "'",
                         display_message_ms);
  }
}

std::vector<std::string>
ProfileCycler::GetMatchingProfilesFromSetting(const std::string& setting,
                                              const std::vector<std::string>& profiles,
                                              InputConfig* device_configuration)
{
  const std::string device_profile_root_location(
      device_configuration->GetUserProfileDirectoryPath());

  const auto& profiles_from_setting = GetProfilesFromSetting(setting, device_profile_root_location);
  if (profiles_from_setting.empty())
  {
    return {};
  }

  std::vector<std::string> result;
  std::ranges::set_intersection(profiles, profiles_from_setting, std::back_inserter(result));
  return result;
}

void ProfileCycler::CycleProfile(CycleDirection cycle_direction, InputConfig* device_configuration,
                                 int& profile_index, int controller_index)
{
  const auto& profiles = GetProfilesForDevice(device_configuration);
  if (profiles.empty())
  {
    Core::DisplayMessage("No input profiles found", display_message_ms);
    return;
  }
  const std::string profile = GetProfile(cycle_direction, profile_index, profiles);

  auto* controller = device_configuration->GetController(controller_index);
  if (controller)
  {
    UpdateToProfile(profile, controller, device_configuration);
  }
  else
  {
    Core::DisplayMessage(fmt::format("No controller found for index: {}", controller_index),
                         display_message_ms);
  }
}

void ProfileCycler::CycleProfileForGame(CycleDirection cycle_direction,
                                        InputConfig* device_configuration, int& profile_index,
                                        const std::string& setting, int controller_index)
{
  const auto& profiles = GetProfilesForDevice(device_configuration);
  if (profiles.empty())
  {
    Core::DisplayMessage("No input profiles found", display_message_ms);
    return;
  }

  if (setting.empty())
  {
    Core::DisplayMessage("No setting found for game", display_message_ms);
    return;
  }

  const auto& profiles_for_game =
      GetMatchingProfilesFromSetting(setting, profiles, device_configuration);
  if (profiles_for_game.empty())
  {
    Core::DisplayMessage("No input profiles found for game", display_message_ms);
    return;
  }

  const std::string profile = GetProfile(cycle_direction, profile_index, profiles_for_game);

  auto* controller = device_configuration->GetController(controller_index);
  if (controller)
  {
    UpdateToProfile(profile, controller, device_configuration);
  }
  else
  {
    Core::DisplayMessage(fmt::format("No controller found for index: {}", controller_index),
                         display_message_ms);
  }
}

std::string ProfileCycler::GetWiimoteInputProfilesForGame(int controller_index)
{
  Common::IniFile game_ini = SConfig::GetInstance().LoadGameIni();
  const auto* const control_section = game_ini.GetOrCreateSection("Controls");

  std::string result;
  control_section->Get(fmt::format("WiimoteProfile{}", controller_index + 1), &result);
  return result;
}

void ProfileCycler::NextWiimoteProfile(int controller_index)
{
  CycleProfile(CycleDirection::Forward, Wiimote::GetConfig(), m_wiimote_profile_index,
               controller_index);
}

void ProfileCycler::PreviousWiimoteProfile(int controller_index)
{
  CycleProfile(CycleDirection::Backward, Wiimote::GetConfig(), m_wiimote_profile_index,
               controller_index);
}

void ProfileCycler::NextWiimoteProfileForGame(int controller_index)
{
  CycleProfileForGame(CycleDirection::Forward, Wiimote::GetConfig(), m_wiimote_profile_index,
                      GetWiimoteInputProfilesForGame(controller_index), controller_index);
}

void ProfileCycler::PreviousWiimoteProfileForGame(int controller_index)
{
  CycleProfileForGame(CycleDirection::Backward, Wiimote::GetConfig(), m_wiimote_profile_index,
                      GetWiimoteInputProfilesForGame(controller_index), controller_index);
}
}  // namespace InputProfile
