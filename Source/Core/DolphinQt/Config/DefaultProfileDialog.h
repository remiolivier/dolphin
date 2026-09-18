// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

class InputConfig;
class QVBoxLayout;
class QWidget;

class DefaultProfileDialog final : public QDialog
{
public:
  explicit DefaultProfileDialog(InputConfig* input_config, QWidget* parent = nullptr);

private:
  void RefreshProfiles();

  InputConfig* const m_input_config;
  QVBoxLayout* m_profiles_layout;
};
