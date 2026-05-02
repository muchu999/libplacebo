/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "cores/VideoSettings.h"

#include <string>
#include <utility>
#include <vector>


struct IntegerSettingOption;

class CGUIDialogVideoSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogVideoSettings();
  ~CGUIDialogVideoSettings() override;

  static std::string getColorMapIntentDescriptionFromIndex(int index);
  static std::string getConeConesDescriptionFromIndex(int index);
  static std::string getDitherMethodDescriptionFromIndex(int index);
  static std::string getDitherTransferDescriptionFromIndex(int index);
  static std::string getDiffusionKernelDescriptionFromIndex(int index);
  static std::string getLutTypeDescriptionFromIndex(int index);
  static std::string getDeinterlaceAlgoDescriptionFromIndex(int index);

  static int getErrorDiffusionIndexFromDescription(std::string description);
  static int getColorMapIntentIndexFromDescription(std::string description);
  static int getDitherMethodIndexFromDescription(std::string description);
  static int getDitherTransferIndexFromDescription(std::string description);
  static int getConeConesIndexFromDescription(std::string description);
  static int getFilterIndexFromDescription(std::string description);
  static int getToneMapIndexFromDescription(std::string description);
  static int getGamutMapIndexFromDescription(std::string description);
  static int getLutTypeIndexFromDescription(std::string description);
  static int getDeinterlaceAlgoIndexFromDescription(std::string description);
  static void UpdateVideoSettingsFromLibPLaceboParams(CVideoSettings &vs);
  static void UpdateLibPLaceboParamsFromVideoSettings(CVideoSettings &vs);
  static void LoadLibplaceboSettings(CVideoSettings& vs);
  static bool LoadLibplaceboSettings(CVideoSettings& vs, std::string path);
  static bool LoadLibplaceboSettings(CVideoSettings& vs, const TiXmlNode* settings);
  static void SaveLibplaceboSettings(const CVideoSettings& vs, std::string path);
  static void SaveLibplaceboSettings(const CVideoSettings& vs, TiXmlNode* settings);
  static void LoadLutFile(CVideoSettings& vs, const std::string& path);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  void AddVideoStreams(const std::shared_ptr<CSettingGroup>& group, const std::string& settingId);
  static void PlUpscalerOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlDownscalerOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlFrameMixerOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlColorMapGamutMapFunctionOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlColorMapToneMapFunctionOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlDeinterlaceAlgoOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlConeConesOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlDitherTransferOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlDitherMethodOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlColorMapIntentOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current); 
  static void PlLutTypeOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlDiffusionKernelOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);

  
  void SaveLibplaceboSettings(const CVideoSettings &vs);



  static void VideoStreamsOptionFiller(const std::shared_ptr<const CSetting>& setting,
                                       std::vector<IntegerSettingOption>& list,
                                       int& current);

  static void VideoOrientationFiller(const std::shared_ptr<const CSetting>& setting,
                                     std::vector<IntegerSettingOption>& list,
                                     int& current);

  static std::string FormatFlags(StreamFlags flags);

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override;
  void SetupView() override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;

  int previousSkinZoom = 0;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  int m_videoStream;
  bool m_viewModeChanged = false;
};
