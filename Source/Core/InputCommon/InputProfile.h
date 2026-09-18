// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class InputConfig;

namespace ControllerEmu
{
class EmulatedController;
}

#include <optional>
#include <string>
#include <vector>

namespace InputProfile
{
std::vector<std::string> GetProfilesFromSetting(const std::string& setting,
                                                const std::string& root);

std::vector<std::string> GetUserProfiles(const InputConfig* input_config);
std::optional<std::string> GetProfileDevice(const std::string& profile_path);
bool IsDefaultProfile(const std::string& profile_path);
bool RemoveDefaultProfile(const std::string& profile_path);
bool SetDefaultProfile(InputConfig* input_config, const std::string& profile_path, bool enabled);
bool HasDefaultProfileForDevice(const InputConfig* input_config, const std::string& device,
                                const std::string& excluded_profile_path = {});
bool ApplyDefaultProfile(InputConfig* input_config, ControllerEmu::EmulatedController* controller,
                         const std::string& device);
void ApplyDefaultProfiles(InputConfig* input_config);
bool HasControllerMappings(const ControllerEmu::EmulatedController* controller);
bool LoadProfile(ControllerEmu::EmulatedController* controller, InputConfig* input_config,
                 const std::string& profile_path, bool replace_existing);

enum class CycleDirection : int
{
  Forward = 1,
  Backward = -1
};

class ProfileCycler
{
public:
  void NextWiimoteProfile(int controller_index);
  void PreviousWiimoteProfile(int controller_index);
  void NextWiimoteProfileForGame(int controller_index);
  void PreviousWiimoteProfileForGame(int controller_index);

private:
  void CycleProfile(CycleDirection cycle_direction, InputConfig* device_configuration,
                    int& profile_index, int controller_index);
  void CycleProfileForGame(CycleDirection cycle_direction, InputConfig* device_configuration,
                           int& profile_index, const std::string& setting, int controller_index);
  std::vector<std::string> GetProfilesForDevice(InputConfig* device_configuration);
  std::string GetProfile(CycleDirection cycle_direction, int& profile_index,
                         const std::vector<std::string>& profiles);
  std::vector<std::string> GetMatchingProfilesFromSetting(const std::string& setting,
                                                          const std::vector<std::string>& profiles,
                                                          InputConfig* device_configuration);
  void UpdateToProfile(const std::string& profile_filename,
                       ControllerEmu::EmulatedController* controller,
                       InputConfig* device_configuration);
  std::string GetWiimoteInputProfilesForGame(int controller_index);

  int m_wiimote_profile_index = 0;
};
}  // namespace InputProfile
