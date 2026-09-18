// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/DefaultProfileDialog.h"

#include <map>
#include <optional>
#include <vector>

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include "Common/StringUtil.h"

#include "DolphinQt/QtUtils/ClearLayoutRecursively.h"

#include "InputCommon/ControllerInterface/CoreDevice.h"
#include "InputCommon/InputConfig.h"
#include "InputCommon/InputProfile.h"

namespace
{
struct ProfileEntry
{
  std::string path;
  QString name;
};

struct DeviceGroup
{
  QString display_name;
  std::vector<ProfileEntry> profiles;
};
}  // namespace

DefaultProfileDialog::DefaultProfileDialog(InputConfig* input_config, QWidget* parent)
    : QDialog(parent), m_input_config(input_config)
{
  setWindowTitle(tr("Default Profiles"));
  setMinimumWidth(420);

  auto* main_layout = new QVBoxLayout(this);

  auto* description = new QLabel(
      tr("Choose the default profile Dolphin should load automatically for each controller."));
  description->setWordWrap(true);
  main_layout->addWidget(description);

  m_profiles_layout = new QVBoxLayout;
  main_layout->addLayout(m_profiles_layout);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  main_layout->addWidget(buttons);

  RefreshProfiles();
}

void DefaultProfileDialog::RefreshProfiles()
{
  ClearLayoutRecursively(m_profiles_layout);

  const auto profiles = InputProfile::GetUserProfiles(m_input_config);
  if (profiles.empty())
  {
    auto* empty_label = new QLabel(
        tr("No controller profiles found. Configure a controller and save a profile first. "
           "It will then appear here."));
    empty_label->setWordWrap(true);
    m_profiles_layout->addWidget(empty_label);
    return;
  }

  std::map<std::string, DeviceGroup> groups;
  std::vector<ProfileEntry> unassigned;

  for (const std::string& profile : profiles)
  {
    std::string profile_name;
    SplitPath(profile, nullptr, &profile_name, nullptr);
    ProfileEntry entry{profile, QString::fromStdString(profile_name)};

    const auto device = InputProfile::GetProfileDevice(profile);
    if (!device)
    {
      unassigned.emplace_back(std::move(entry));
      continue;
    }

    ciface::Core::DeviceQualifier qualifier;
    qualifier.FromString(*device);
    qualifier.cid = -1;
    const std::string identity = qualifier.ToString();

    auto& group = groups[identity];
    group.display_name = QString::fromStdString(qualifier.name);
    group.profiles.emplace_back(std::move(entry));
  }

  const auto add_device_group = [this](const QString& title,
                                       const std::vector<ProfileEntry>& entries) {
    auto* group_box = new QGroupBox(title);
    auto* group_layout = new QVBoxLayout(group_box);
    auto* button_group = new QButtonGroup(group_box);

    auto* no_default = new QRadioButton(tr("No default"));
    button_group->addButton(no_default);
    group_layout->addWidget(no_default);

    QRadioButton* selected_button = nullptr;
    for (const auto& entry : entries)
    {
      auto* profile_button = new QRadioButton(entry.name);
      button_group->addButton(profile_button);
      group_layout->addWidget(profile_button);

      if (InputProfile::IsDefaultProfile(entry.path))
      {
        profile_button->setChecked(true);
        selected_button = profile_button;
      }

      connect(profile_button, &QRadioButton::clicked, this, [this, path = entry.path] {
        InputProfile::SetDefaultProfile(m_input_config, path, true);
        InputProfile::ApplyDefaultProfiles(m_input_config);
        RefreshProfiles();
      });
    }

    if (!selected_button)
      no_default->setChecked(true);

    connect(no_default, &QRadioButton::clicked, this, [this, entries] {
      for (const auto& entry : entries)
        InputProfile::RemoveDefaultProfile(entry.path);
      RefreshProfiles();
    });

    m_profiles_layout->addWidget(group_box);
  };

  for (const auto& entry : groups)
  {
    const auto& group = entry.second;
    add_device_group(group.display_name, group.profiles);
  }

  if (!unassigned.empty())
  {
    auto* group_box = new QGroupBox(tr("Unassigned"));
    auto* group_layout = new QVBoxLayout(group_box);
    for (const auto& entry : unassigned)
    {
      auto* profile_label = new QLabel(entry.name);
      profile_label->setEnabled(false);
      profile_label->setToolTip(tr("This profile does not have a saved controller device."));
      group_layout->addWidget(profile_label);
    }
    m_profiles_layout->addWidget(group_box);
  }

  m_profiles_layout->addStretch(1);
}
