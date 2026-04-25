/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/SettingControl.h"
#include "settings/SettingCreator.h"
#include "settings/SettingsBase.h"

#include <string>

class CSettingList;
class TiXmlElement;
class TiXmlNode;

/*!
 \brief Wrapper around CSettingsManager responsible for properly setting up
 the settings manager and registering all the callbacks, handlers and custom
 setting types.
 \sa CSettingsManager
 */
class CSettings : public CSettingsBase, public CSettingCreator, public CSettingControlCreator
                , private ISubSettings
{
public:

  // values for SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS
  static const int VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_MOVIES = 0;
  static const int VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_TVSHOWEPISODES = 1;
  static const int VIDEOLIBRARY_THUMB_SHOW_UNWATCHED_EPISODE = 2;
  // values for SETTING_VIDEOLIBRARY_ARTWORK_LEVEL
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_ALL = 0;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_BASIC = 1;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_CUSTOM = 2;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_NONE = 3;

  // values for SETTING_MUSICLIBRARY_ARTWORKLEVEL
  static const int MUSICLIBRARY_ARTWORK_LEVEL_ALL = 0;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_BASIC = 1;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_CUSTOM = 2;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_NONE = 3;

  // values for SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM
  static constexpr int SETTING_AUTOPLAYNEXT_MUSICVIDEOS = 0;
  static constexpr int SETTING_AUTOPLAYNEXT_TVSHOWS = 1;
  static constexpr int SETTING_AUTOPLAYNEXT_EPISODES = 2;
  static constexpr int SETTING_AUTOPLAYNEXT_MOVIES = 3;
  static constexpr int SETTING_AUTOPLAYNEXT_UNCATEGORIZED = 4;

  // values for SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS
  static const int VIDEOPLAYER_ALLOWED_HDR_TYPE_DOLBY_VISION = 0;
  static const int VIDEOPLAYER_ALLOWED_HDR_TYPE_HDR10PLUS = 1;


  CSettings() = default;
  ~CSettings() override = default;


  bool Initialize() override;
  void RegisterSubSettings(ISubSettings* subSettings);
  void UnregisterSubSettings(ISubSettings* subSettings);
  bool Load() override;
  bool Save() override;
  bool Load(const std::string &file);
  bool Load(const TiXmlElement* root);
  bool LoadHidden(const TiXmlElement *root) { return CSettingsBase::LoadHiddenValuesFromXml(root); }
  bool Save(const std::string& file) const;
  bool Save(TiXmlNode* root) const override;
  bool LoadSetting(const TiXmlNode* node, const std::string& settingId) const;
  bool GetBool(const std::string& id) const;
  void Clear() override;

protected:
  // specializations of CSettingsBase
  void InitializeSettingTypes() override;
  void InitializeControls() override;
  void InitializeOptionFillers() override;
  void UninitializeOptionFillers() override;
  void InitializeConditions() override;
  void UninitializeConditions() override;
  void InitializeDefaults() override;
  void InitializeISettingsHandlers() override;
  void UninitializeISettingsHandlers() override;
  void InitializeISubSettings() override;
  void UninitializeISubSettings() override;
  void InitializeISettingCallbacks() override;
  void UninitializeISettingCallbacks() override;

  // implementation of CSettingsBase
  bool InitializeDefinitions() override;

private:
  CSettings(const CSettings&) = delete;
  CSettings const& operator=(CSettings const&) = delete;

  bool Load(const TiXmlElement* root, bool& updated);

  // implementation of ISubSettings
  bool Load(const TiXmlNode* settings) override;

  bool Initialize(const std::string &file);
  bool Reset();

  std::set<ISubSettings*> m_subSettings;
};
