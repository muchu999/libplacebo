/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoSettings.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "profiles/ProfileManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#include "video/VideoDatabase.h"
#include "video/ViewModeSettings.h"
#include "windowing/WinSystem.h"
#include "libplacebo/options.h"

#include "..\VideoPlayer\VideoRenderers\windows\RendererPL.h"

#include <utility>
#include <filesystem/File.h>
#include <utils/URIUtils.h>

#define SETTING_VIDEO_VIEW_MODE           "video.viewmode"
#define SETTING_VIDEO_ZOOM                "video.zoom"
#define SETTING_VIDEO_PIXEL_RATIO         "video.pixelratio"
#define SETTING_VIDEO_BRIGHTNESS          "video.brightness"
#define SETTING_VIDEO_CONTRAST            "video.contrast"
#define SETTING_VIDEO_GAMMA               "video.gamma"
#define SETTING_VIDEO_NONLIN_STRETCH      "video.nonlinearstretch"
#define SETTING_VIDEO_POSTPROCESS         "video.postprocess"
#define SETTING_VIDEO_VERTICAL_SHIFT      "video.verticalshift"
#define SETTING_VIDEO_TONEMAP_METHOD      "video.tonemapmethod"
#define SETTING_VIDEO_TONEMAP_PARAM       "video.tonemapparam"
#define SETTING_VIDEO_ORIENTATION         "video.orientation"

#define SETTING_VIDEO_VDPAU_NOISE         "vdpau.noise"
#define SETTING_VIDEO_VDPAU_SHARPNESS     "vdpau.sharpness"

#define SETTING_VIDEO_INTERLACEMETHOD     "video.interlacemethod"
#define SETTING_VIDEO_SCALINGMETHOD       "video.scalingmethod"

#define SETTING_VIDEO_STEREOSCOPICMODE    "video.stereoscopicmode"
#define SETTING_VIDEO_STEREOSCOPICINVERT  "video.stereoscopicinvert"

#define SETTING_VIDEO_LIBPLACEBO          "video.libplacebo"

#define SETTING_VIDEO_MAKE_DEFAULT        "video.save"
#define SETTING_VIDEO_CALIBRATION         "video.calibration"
#define SETTING_VIDEO_STREAM              "video.stream"

#define SETTING_LIBPLACEBO_SIGMOID_CENTER         "video.libplacebo.sigmoid.center"
#define SETTING_LIBPLACEBO_SIGMOID_SLOPE          "video.libplacebo.sigmoid.slope"
#define SETTING_LIB_PLACEBO_COLOR_ADJUSTMENT_ENABLED "video.libplacebo.color_adjustment.enabled"
#define SETTING_LIB_PLACEBO_SATURATION            "video.libplacebo.color_adjustment.saturation"
#define SETTING_LIB_PLACEBO_HUE                   "video.libplacebo.hue"
#define SETTING_LIB_PLACEBO_TEMPERATURE           "video.libplacebo.color_adjustment.temperature"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_ENABLED              "video.libplacebo.peak_detect_params.enabled"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SMOOTHING_PERIOD     "video.libplacebo.peak_detect_params.smoothing_period"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SCENE_THRESHOLD_LOW  "video.libplacebo.peak_detect_params.scene_threshold_low"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SCENE_THRESHOLD_HIGH "video.libplacebo.peak_detect_params.scene_threshold_high"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_PERCENTILE           "video.libplacebo.peak_detect_params.percentile"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_BLACK_CUTOFF         "video.libplacebo.peak_detect_params.black_cutoff"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_ALLOW_DELAYED        "video.libplacebo.peak_detect_params.allow_delayed"
#define SETTING_LIB_PLACEBO_UPSCALER                                "video.libplacebo.upscaler"
#define SETTING_LIB_PLACEBO_DOWNSCALER                              "video.libplacebo.downscaler"
#define SETTING_LIB_PLACEBO_PLANE_UPSCALER                          "video.libplacebo.plane_upscaler"
#define SETTING_LIB_PLACEBO_PLANE_DOWNSCALER                        "video.libplacebo.plane_dowsnscaler"
#define SETTING_LIB_PLACEBO_FRAME_MIXER                             "video.libplacebo.frame_mixer"
#define SETTING_LIB_PLACEBO_COLOR_MAP_GAMUT_MAPPING                           "video.libplacebo.gammut_map_funtion"
#define SETTING_LIB_PLACEBO_COLOR_MAP_TONE_MAPPING                            "video.libplacebo.tone_map_funtion"
#define SETTING_LIB_PLACEBO_COLOR_MAP_CONTRAST_RECOVERY             "video.libplacebo.color_map_contrast_recovery"
#define SETTING_LIB_PLACEBO_COLOR_MAP_CONTRAST_SMOOTHNESS           "video.libplacebo.color_map_contrast_smoothness"
#define SETTING_LIB_PLACEBO_LOAD_PRESET_DEFAULT                     "video.libplacebo.load_preset_default"
#define SETTING_LIB_PLACEBO_LOAD_PRESET_FAST                        "video.libplacebo.load_preset_fast"
#define SETTING_LIB_PLACEBO_LOAD_PRESET_HIGH_QUALITY                "video.libplacebo.load_preset_high_quality"
#define SETTING_LIB_PLACEBO_LOAD_FROM_FILE                          "video.libplacebo.load_from_file"
#define SETTING_LIB_PLACEBO_SAVE_TO_FILE                            "video.libplacebo.save_to_file"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_LOAD_PRESET_DEFAULT         "video.libplacebo.peak_detect_load_preset_default"
#define SETTING_LIB_PLACEBO_PEAK_DETECT_LOAD_PRESET_HIGH_QUALITY    "video.libplacebo.peak_detect_load_preset_high_quality"
#define SETTING_LIB_PLACEBO_DEBAND_ENABLED                          "video.libplacebo.deband_enabled"
#define SETTING_LIB_PLACEBO_DEBAND_LOAD_PRESET                      "video.libplacebo.deband_load_preset"
#define SETTING_LIB_PLACEBO_DEBAND_GRAIN                            "video.libplacebo.deband_grain"
#define SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL0                   "video.libplacebo.deband_grain_neutral0"
#define SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL1                   "video.libplacebo.deband_grain_neutral1"
#define SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL2                   "video.libplacebo.deband_grain_neutral2"
#define SETTING_LIB_PLACEBO_DEBAND_ITERATIONS                       "video.libplacebo.deband_iterations"
#define SETTING_LIB_PLACEBO_DEBAND_RADIUS                           "video.libplacebo.deband_radius"
#define SETTING_LIB_PLACEBO_DEBAND_THRESHOLD                        "video.libplacebo.deband_threshold"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LOAD_PRESET_DEFAULT           "video.libplacebo.color_map_load_preset_default"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LOAD_PRESET_HIGH_QUALITY      "video.libplacebo.color_map_load_preset_high_quality"
#define SETTING_LIB_PLACEBO_COLOR_MAP_ENABLED                       "video.libplacebo.color_map_enabled"
#define SETTING_LIB_PLACEBO_DEINTERLACE_ENABLED                     "video.libplacebo.deinterlace_enabled"
#define SETTING_LIB_PLACEBO_DEINTERLACE_ALGO                        "video.libplacebo.deinterlace_algo"
#define SETTING_LIB_PLACEBO_DEINTERLACE_SKIP_SPATIAL_CHECK          "video.libplacebo.deinterlace_spatial_check"
#define SETTING_LIB_PLACEBO_SIGMOID_ENABLED                         "video.libplacebo.sigmoid_enabled"
#define SETTING_LIB_PLACEBO_SIGMOID_LOAD_PRESET_DEFAULT             "video.libplacebo.sigmoid_load_preset_default"
#define SETTING_LIB_PLACEBO_SIGMOID_CENTER                          "video.libplacebo.sigmoid_center"
#define SETTING_LIB_PLACEBO_SIGMOID_SLOPE                           "video.libplacebo.sigmoid_slope"
#define SETTING_LIB_PLACEBO_CONE_ENABLED                            "video.libplacebo.cone_enabled"
#define SETTING_LIB_PLACEBO_CONE_CONES                              "video.libplacebo.cone_cones"
#define SETTING_LIB_PLACEBO_CONE_STRENGTH                           "video.libplacebo.cone_strength"
#define SETTING_LIB_PLACEBO_DITHER_ENABLED                          "video.libplacebo.dither_enabled"
#define SETTING_LIB_PLACEBO_DITHER_LOAD_PRESET_DEFAULT              "video.libplacebo.dither_load_preset_default"
#define SETTING_LIB_PLACEBO_DITHER_METHOD                           "video.libplacebo.dither_method"
#define SETTING_LIB_PLACEBO_DITHER_LUT_SIZE                         "video.libplacebo.dither_lut_size"
#define SETTING_LIB_PLACEBO_DITHER_TEMPORAL                         "video.libplacebo.dither_temporal"
#define SETTING_LIB_PLACEBO_DITHER_TRANSFER                         "video.libplacebo.dither_transfer"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_EXPOSURE                 "video.libplacebo.tone_constants_exposure"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_ADAPTATION          "video.libplacebo.tone_constants_knee_adaptation"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_DEFAULT             "video.libplacebo.tone_constants_knee_default"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_MAXIMUM             "video.libplacebo.tone_constants_knee_maximum"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_MINIMUM             "video.libplacebo.tone_constants_knee_minimum"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_OFFSET              "video.libplacebo.tone_constants_knee_offset"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_LINEAR_KNEE              "video.libplacebo.tone_constants_linear_knee"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_REINHARD_CONTRAST        "video.libplacebo.tone_constants_reinhard_contrast"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_SLOPE_OFFSET             "video.libplacebo.tone_constants_slope_offset"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_SLOPE_TUNING             "video.libplacebo.tone_constants_slope_tuning"
#define SETTING_LIB_PLACEBO_TONE_CONSTANTS_SPLINE_CONTRAST          "video.libplacebo.tone_constants_spline_contrast"
#define SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_COLORIMETRIC_GAMMA      "video.libplacebo.gamut_constants_colorimetric_gamma"
#define SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_PERCEPTUAL_DEADZONE     "video.libplacebo.gamut_constants_perceptual_deadzone"
#define SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_SOFTCLIP_DESAT          "video.libplacebo.gamut_constants_softclip_desat"
#define SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_SOFTCLIP_KNEE           "video.libplacebo.gamut_constants_softclip_knee"
#define SETTING_LIB_PLACEBO_COLOR_MAP_GAMUT_EXPANSION                   "video.libplacebo.gamut_constants_expansion"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_HUE          "video.libplacebo.color_map_visualize_hue"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_LUT          "video.libplacebo.color_map_visualize_lut"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_X0      "video.libplacebo.color_map_visualize_rect_x0"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_X1      "video.libplacebo.color_map_visualize_rect_x1"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_Y0      "video.libplacebo.color_map_visualize_rect_y0"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_Y1      "video.libplacebo.color_map_visualize_rect_y1"
#define SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_THETA        "video.libplacebo.color_map_visualize_theta"
#define SETTING_LIB_PLACEBO_COLOR_MAP_INVERSE_TONE_MAPPING "video.libplacebo.color_map_inverse_tone_mapping"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_I         "video.libplacebo.color_map_lut3d_size_i"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_C         "video.libplacebo.color_map_lut3d_size_c"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_H         "video.libplacebo.color_map_lut3d_size_h"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_TRICUBIC       "video.libplacebo.color_map_lut3d_tricubic"
#define SETTING_LIB_PLACEBO_COLOR_MAP_LUT_SIZE             "video.libplacebo.color_map_lut_size"
#define SETTING_LIB_PLACEBO_COLOR_MAP_SHOW_CLIPPING        "video.libplacebo.color_map_show_clipping"
#define SETTING_LIB_PLACEBO_COLOR_MAP_INTENT               "video.libplacebo.color_map_map_intent"
#define SETTING_LIB_PLACEBO_COLOR_MAP_FORCE_TONE_MAPPING_LUT "video.libplacebo.color_map_force_tone_mapping_lut"
#define SETTING_LIB_PLACEBO_ANTIRINGING_STRENGTH                "video.libplacebo.antiringing_strength"
#define SETTING_LIB_PLACEBO_CORRECT_SUBPIXEL_OFFSET             "video.libplacebo.correct_subpixel_offset"
#define SETTING_LIB_PLACEBO_DISABLE_BUILTIN_SCALERS             "video.libplacebo.disable_builtin_scalers"
#define SETTING_LIB_PLACEBO_DISABLE_DITHER_GAMMA_CORRECTION     "video.libplacebo.disable_dither_gamma_correction"
#define SETTING_LIB_PLACEBO_DISABLE_LINEAR_SCALING              "video.libplacebo.disable_linear_scaling"
#define SETTING_LIB_PLACEBO_DYNAMIC_CONSTANTS                   "video.libplacebo.dynamic_constant"
#define SETTING_LIB_PLACEBO_ERROR_DIFFUSION                     "video.libplacebo.error_diffusion"
#define SETTING_LIB_PLACEBO_FORCE_DITHER                        "video.libplacebo.force_dither"
#define SETTING_LIB_PLACEBO_FORCE_LOW_BIT_DEPTH_FBOS            "video.libplacebo.force_low_bit_depth_fbos"
#define SETTING_LIB_PLACEBO_IGNORE_ICC_PROFILES                 "video.libplacebo.ignore_icc_profiles"
#define SETTING_LIB_PLACEBO_PRESERVE_MIXING_CACHE               "video.libplacebo.preserve_mixing_cache"
#define SETTING_LIB_PLACEBO_SKIP_ANTI_ALIASING                  "video.libplacebo.skip_anti_aliasing"
#define SETTING_LIB_PLACEBO_SKIP_CACHING_SINGLE_FRAME           "video.libplacebo.skip_caching_single_frame"
#define SETTING_LIB_PLACEBO_DISPLAY_PEAK_LUMINANCE              "video.libplacebo.display_peak_luminance"
#define SETTING_LIB_PLACEBO_TARGET_COLORSPACE_HINT              "video.libplacebo.target_colorspace_hint"
#define SETTING_LIB_PLACEBO_TARGET_COLORSPACE_HINT_MODE         "video.libplacebo.target_colorspace_hint_mode"


#define CreateGroup(thegroup,thecategory) std::shared_ptr<CSettingGroup> thegroup = AddGroup(thecategory); if (thegroup == NULL) {CLog::Log(LOGERROR, "CGUIDialogLibplacebo: unable to setup settings");  return; }


CGUIDialogVideoSettings::CGUIDialogVideoSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "DialogSettings.xml")
{ 
}

CGUIDialogVideoSettings::~CGUIDialogVideoSettings() = default;


void CGUIDialogVideoSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const std::string &settingId = setting->GetId();
  CVideoSettings vs = appPlayer->GetVideoSettings();
  pl_options m_placeboOptions = vs.m_placeboOptions->getPlOptions();
  if (settingId == SETTING_VIDEO_INTERLACEMETHOD)
  {
    vs.m_InterlaceMethod = static_cast<EINTERLACEMETHOD>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_VIDEO_SCALINGMETHOD)
  {
    vs.m_ScalingMethod = static_cast<ESCALINGMETHOD>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_VIDEO_STREAM)
  {
    m_videoStream = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    // only change the video stream if a different one has been asked for
    if (appPlayer->GetVideoStream() != m_videoStream)
    {
      appPlayer->SetVideoStream(m_videoStream); // Set the video stream to the one selected
    }
  }
  else if (settingId == SETTING_VIDEO_VIEW_MODE)
  {
    int value = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();

    appPlayer->SetRenderViewMode(value, vs.m_CustomZoomAmount, vs.m_CustomPixelRatio,
                                 vs.m_CustomVerticalShift, vs.m_CustomNonLinStretch);

    m_viewModeChanged = true;
    GetSettingsManager()->SetNumber(SETTING_VIDEO_ZOOM, static_cast<double>(vs.m_CustomZoomAmount));
    GetSettingsManager()->SetNumber(SETTING_VIDEO_PIXEL_RATIO,
                                    static_cast<double>(vs.m_CustomPixelRatio));
    GetSettingsManager()->SetNumber(SETTING_VIDEO_VERTICAL_SHIFT,
                                    static_cast<double>(vs.m_CustomVerticalShift));
    GetSettingsManager()->SetBool(SETTING_VIDEO_NONLIN_STRETCH, vs.m_CustomNonLinStretch);
    m_viewModeChanged = false;
  }
  else if (settingId == SETTING_VIDEO_ZOOM ||
           settingId == SETTING_VIDEO_VERTICAL_SHIFT ||
           settingId == SETTING_VIDEO_PIXEL_RATIO ||
           settingId == SETTING_VIDEO_NONLIN_STRETCH)
  {
    if (settingId == SETTING_VIDEO_ZOOM)
      vs.m_CustomZoomAmount = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_VERTICAL_SHIFT)
      vs.m_CustomVerticalShift = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_PIXEL_RATIO)
      vs.m_CustomPixelRatio = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_NONLIN_STRETCH)
      vs.m_CustomNonLinStretch = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();

    // try changing the view mode to custom. If it already is set to custom
    // manually call the render manager
    if (GetSettingsManager()->GetInt(SETTING_VIDEO_VIEW_MODE) != ViewModeCustom)
      GetSettingsManager()->SetInt(SETTING_VIDEO_VIEW_MODE, ViewModeCustom);
    else
      appPlayer->SetRenderViewMode(vs.m_ViewMode, vs.m_CustomZoomAmount, vs.m_CustomPixelRatio,
                                   vs.m_CustomVerticalShift, vs.m_CustomNonLinStretch);
  }
  else if (settingId == SETTING_VIDEO_POSTPROCESS)
  {
    vs.m_PostProcess = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_BRIGHTNESS)
  {
    vs.m_Brightness = static_cast<float>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_adjustment.brightness = vs.m_Brightness / 50.0 - 1.0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_CONTRAST)
  {
    vs.m_Contrast = static_cast<float>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_adjustment.contrast = (pow(10.0, (vs.m_Contrast - 50.0) / 25.0) - 0.01) * 100.0 / 99.0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_GAMMA)
  {
    vs.m_Gamma = static_cast<float>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_adjustment.gamma = (pow(10.0, (vs.m_Gamma - 20.0) / 40.0) - pow(10,-0.5)) * 1.0/(1.0-pow(10,-0.5));
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_VDPAU_NOISE)
  {
    vs.m_NoiseReduction = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_VDPAU_SHARPNESS)
  {
    vs.m_Sharpness = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_TONEMAP_METHOD)
  {
    vs.m_ToneMapMethod = static_cast<ETONEMAPMETHOD>(
    std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_TONEMAP_PARAM)
  {
    vs.m_ToneMapParam = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_ORIENTATION)
  {
    vs.m_Orientation = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_VIDEO_STEREOSCOPICMODE)
  {
    vs.m_StereoMode = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_STEREOSCOPICINVERT)
  {
    vs.m_StereoInvert = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_SATURATION)
  {
    vs.m_PlaceboSaturation = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_adjustment.saturation = vs.m_PlaceboSaturation;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_HUE)
  {
    vs.m_PlaceboHue = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_adjustment.hue = fmod(vs.m_PlaceboHue, 360.0) * M_PI / 180.0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TEMPERATURE)
  {
    vs.m_PlaceboTemperature = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_adjustment.temperature= (vs.m_PlaceboTemperature-6500.0)/3500.0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_ENABLED)
  {
    vs.m_PlaceboPeakDetectEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.peak_detect_params = vs.m_PlaceboPeakDetectEnabled ? &m_placeboOptions->peak_detect_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_ADJUSTMENT_ENABLED)
  {
    vs.m_PlaceboColorAdjustmentEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.color_adjustment = vs.m_PlaceboColorAdjustmentEnabled ? &m_placeboOptions->color_adjustment : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SMOOTHING_PERIOD)
  {
    vs.m_PlaceboPeakDetectSmoothingPeriod = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->peak_detect_params.smoothing_period = vs.m_PlaceboPeakDetectSmoothingPeriod;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SCENE_THRESHOLD_LOW)
  {
    vs.m_PlaceboPeakDetectSceneThresholdLow = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->peak_detect_params.scene_threshold_low = vs.m_PlaceboPeakDetectSceneThresholdLow;
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SCENE_THRESHOLD_HIGH)
  {
    vs.m_PlaceboPeakDetectSceneThresholdHigh = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->peak_detect_params.scene_threshold_high = vs.m_PlaceboPeakDetectSceneThresholdHigh;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_PERCENTILE)
  {
    vs.m_PlaceboPeakDetectPercentile = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->peak_detect_params.percentile = vs.m_PlaceboPeakDetectPercentile;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_BLACK_CUTOFF)
  {
    vs.m_PlaceboPeakDetectBlackCutoff = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->peak_detect_params.black_cutoff = vs.m_PlaceboPeakDetectBlackCutoff;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_ALLOW_DELAYED)
  {
    vs.m_PlaceboPeakDetectAllowDelayed = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->peak_detect_params.allow_delayed = vs.m_PlaceboPeakDetectAllowDelayed;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_UPSCALER)
  {

    vs.m_PlaceboUpscaler = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->params.upscaler = vs.m_PlaceboUpscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboUpscaler];
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DOWNSCALER)
  {
    vs.m_PlaceboDownscaler = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->params.downscaler = vs.m_PlaceboDownscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboDownscaler];
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PLANE_UPSCALER)
  {
    vs.m_PlaceboPlaneUpscaler = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->params.plane_upscaler = vs.m_PlaceboPlaneUpscaler == -1 ? NULL: pl_filter_configs[vs.m_PlaceboPlaneUpscaler];
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PLANE_DOWNSCALER)
  {
    vs.m_PlaceboPlaneDownscaler = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->params.plane_downscaler = vs.m_PlaceboPlaneDownscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboPlaneDownscaler];
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_FRAME_MIXER)
  {
    vs.m_PlaceboFrameMixer = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->params.frame_mixer = vs.m_PlaceboFrameMixer == -1 ? NULL : pl_filter_configs[vs.m_PlaceboFrameMixer];
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_GAMUT_MAPPING)
  {
    vs.m_PlaceboColorMapGamutMapping = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->color_map_params.gamut_mapping = vs.m_PlaceboColorMapGamutMapping == -1 ? NULL : pl_gamut_map_functions[vs.m_PlaceboColorMapGamutMapping];
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_TONE_MAPPING)
  {
    vs.m_PlaceboColorMapToneMapping = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    m_placeboOptions->color_map_params.tone_mapping_function = vs.m_PlaceboColorMapToneMapping == -1 ? NULL : pl_tone_map_functions[vs.m_PlaceboColorMapToneMapping];
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_ENABLED)
  {
    vs.m_PlaceboDebandEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.deband_params = vs.m_PlaceboDebandEnabled ? &m_placeboOptions->deband_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_GRAIN)
  {
    vs.m_PlaceboDebandGrain = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->deband_params.grain = vs.m_PlaceboDebandGrain;
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL0)
  {
    vs.m_PlaceboDebandGrainNeutral0 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->deband_params.grain_neutral[0] = vs.m_PlaceboDebandGrainNeutral0;
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL1)
  {
    vs.m_PlaceboDebandGrainNeutral1 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->deband_params.grain_neutral[1] = vs.m_PlaceboDebandGrainNeutral1;
    appPlayer->SetVideoSettings(vs);
    }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL2)
  {
    vs.m_PlaceboDebandGrainNeutral2 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->deband_params.grain_neutral[2] = vs.m_PlaceboDebandGrainNeutral2;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_ITERATIONS)
  {
    vs.m_PlaceboDebandIterations = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->deband_params.iterations = vs.m_PlaceboDebandIterations;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_RADIUS)
  {
    vs.m_PlaceboDebandRadius = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->deband_params.radius = vs.m_PlaceboDebandRadius;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_THRESHOLD)
  {
    vs.m_PlaceboDebandThreshold = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->deband_params.threshold = vs.m_PlaceboDebandThreshold;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_ENABLED)
  {
    vs.m_PlaceboColorMapEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.color_map_params = vs.m_PlaceboColorMapEnabled ? &m_placeboOptions->color_map_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DISPLAY_PEAK_LUMINANCE)
  {
    vs.m_PlaceboDisplayPeakLuminance = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TARGET_COLORSPACE_HINT)
  {
    vs.m_PlaceboTargetColorspaceHint = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TARGET_COLORSPACE_HINT_MODE)
  {
    vs.m_PlaceboTargetColorspaceHintMode = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEINTERLACE_ENABLED)
  {
    vs.m_PlaceboDeinterlaceEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.deinterlace_params = vs.m_PlaceboDeinterlaceEnabled ? &m_placeboOptions->deinterlace_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEINTERLACE_ALGO)
  {
    vs.m_PlaceboDeinterlaceAlgo = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->deinterlace_params.algo = (enum pl_deinterlace_algorithm) vs.m_PlaceboDeinterlaceAlgo;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEINTERLACE_SKIP_SPATIAL_CHECK)
  {
    vs.m_PlaceboDeinterlaceSkipSpatialCheck = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->deinterlace_params.skip_spatial_check = vs.m_PlaceboDeinterlaceSkipSpatialCheck;
    appPlayer->SetVideoSettings(vs);
  }
    else if (settingId == SETTING_LIB_PLACEBO_SIGMOID_ENABLED)
  {
    vs.m_PlaceboSigmoidEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.sigmoid_params = vs.m_PlaceboSigmoidEnabled ? &m_placeboOptions->sigmoid_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_SIGMOID_CENTER)
  {
    vs.m_PlaceboSigmoidCenter = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->sigmoid_params.center = vs.m_PlaceboSigmoidCenter;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_SIGMOID_SLOPE)
  {
    vs.m_PlaceboSigmoidSlope = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
    m_placeboOptions->sigmoid_params.slope = vs.m_PlaceboSigmoidSlope;
  }
  else if (settingId == SETTING_LIB_PLACEBO_CONE_ENABLED)
  {
    vs.m_PlaceboConeEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.cone_params = vs.m_PlaceboConeEnabled ? &m_placeboOptions->cone_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_CONE_CONES)
  {
    vs.m_PlaceboConeCones = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->cone_params.cones = (enum pl_cone)vs.m_PlaceboConeCones; 
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_CONE_STRENGTH)
  {
    vs.m_PlaceboConeStrength = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->cone_params.strength = vs.m_PlaceboConeStrength;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DITHER_ENABLED)
  {
    vs.m_PlaceboDitherEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.dither_params = vs.m_PlaceboDitherEnabled ? &m_placeboOptions->dither_params : NULL;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DITHER_METHOD)
  {
    vs.m_PlaceboDitherMethod = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->dither_params.method = (enum pl_dither_method)vs.m_PlaceboDitherMethod; 
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DITHER_LUT_SIZE)
  {
    vs.m_PlaceboDitherLutSize = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->dither_params.lut_size = vs.m_PlaceboDitherLutSize;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DITHER_TEMPORAL)
  {
    vs.m_PlaceboDitherTemporal = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->dither_params.temporal = vs.m_PlaceboDitherTemporal;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DITHER_TRANSFER)
  {
    vs.m_PlaceboDitherTransfer = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->dither_params.transfer = (enum pl_color_transfer) vs.m_PlaceboDitherTransfer;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_EXPOSURE)
  {
    vs.m_PlaceboToneConstantExposure = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.exposure = vs.m_PlaceboToneConstantExposure;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_ADAPTATION)
  {
    vs.m_PlaceboToneConstantKneeAdaptation = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.knee_adaptation = vs.m_PlaceboToneConstantKneeAdaptation;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_DEFAULT)
  {
    vs.m_PlaceboToneConstantKneeDefault = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.knee_default = vs.m_PlaceboToneConstantKneeDefault;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_MAXIMUM)
  {
    vs.m_PlaceboToneConstantKneeMaximum = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.knee_maximum = vs.m_PlaceboToneConstantKneeMaximum;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_MINIMUM)
  {
    vs.m_PlaceboToneConstantKneeMinimum = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.knee_minimum = vs.m_PlaceboToneConstantKneeMinimum;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_OFFSET)
  {
    vs.m_PlaceboToneConstantKneeOffset = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.knee_offset = vs.m_PlaceboToneConstantKneeOffset;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_LINEAR_KNEE)
  {
    vs.m_PlaceboToneConstantLinearKnee = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.linear_knee = vs.m_PlaceboToneConstantLinearKnee;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_REINHARD_CONTRAST)
  {
    vs.m_PlaceboToneConstantReinhardContrast = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.reinhard_contrast = vs.m_PlaceboToneConstantReinhardContrast;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_SLOPE_OFFSET)
  {
    vs.m_PlaceboToneConstantSlopeOffset = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.slope_offset = vs.m_PlaceboToneConstantSlopeOffset;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_SLOPE_TUNING)
  {
    vs.m_PlaceboToneConstantSlopeTuning = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.slope_tuning = vs.m_PlaceboToneConstantSlopeTuning;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_TONE_CONSTANTS_SPLINE_CONTRAST)
  {
    vs.m_PlaceboToneConstantSplineContrast = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.tone_constants.spline_contrast = vs.m_PlaceboToneConstantSplineContrast;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_LUT)
  {
    vs.m_PlaceboColorMapVisualizeLut = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->color_map_params.visualize_lut = vs.m_PlaceboColorMapVisualizeLut;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_X0)
  {
    vs.m_PlaceboColorMapVisualizeRectX0 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.visualize_rect.x0 = vs.m_PlaceboColorMapVisualizeRectX0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_X1)
  {
    vs.m_PlaceboColorMapVisualizeRectX1 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.visualize_rect.x1 = vs.m_PlaceboColorMapVisualizeRectX1;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_Y0)
  {
    vs.m_PlaceboColorMapVisualizeRectY0 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.visualize_rect.y0 = vs.m_PlaceboColorMapVisualizeRectY0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_Y1)
  {
    vs.m_PlaceboColorMapVisualizeRectY1 = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.visualize_rect.y1 = vs.m_PlaceboColorMapVisualizeRectY1;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_HUE)
  {
    vs.m_PlaceboColorMapVisualizeHue = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.visualize_hue = fmod(vs.m_PlaceboColorMapVisualizeHue, 360.0) * M_PI / 180.0;;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_THETA)
  {
    vs.m_PlaceboColorMapVisualizeTheta = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.visualize_theta = fmod(vs.m_PlaceboColorMapVisualizeTheta,360.0)* M_PI / 180.0;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_CONTRAST_RECOVERY)
  {
    vs.m_PlaceboColorMapContrastRecovery = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.contrast_recovery = vs.m_PlaceboColorMapContrastRecovery;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_CONTRAST_SMOOTHNESS)
  {
    vs.m_PlaceboColorMapContrastSmoothness = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.contrast_smoothness = vs.m_PlaceboColorMapContrastSmoothness;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_GAMUT_EXPANSION)
  {
    vs.m_PlaceboColorMapGamutExpansion = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->color_map_params.gamut_expansion = vs.m_PlaceboColorMapGamutExpansion;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_COLORIMETRIC_GAMMA)
  {
    vs.m_PlaceboGamutConstantsColorimetricGamma = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.gamut_constants.colorimetric_gamma = vs.m_PlaceboGamutConstantsColorimetricGamma;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_PERCEPTUAL_DEADZONE)
  {
    vs.m_PlaceboGamutConstantsPerceptualDeadzone = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.gamut_constants.perceptual_deadzone = vs.m_PlaceboGamutConstantsPerceptualDeadzone;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_SOFTCLIP_DESAT)
  {
    vs.m_PlaceboGamutConstantsSoftclipDesat = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.gamut_constants.softclip_desat = vs.m_PlaceboGamutConstantsSoftclipDesat;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_SOFTCLIP_KNEE)
  {
    vs.m_PlaceboGamutConstantsSoftclipKnee = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->color_map_params.gamut_constants.softclip_knee = vs.m_PlaceboGamutConstantsSoftclipKnee;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_INVERSE_TONE_MAPPING)
  {
    vs.m_PlaceboColorMapInverseToneMapping = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->color_map_params.inverse_tone_mapping = vs.m_PlaceboColorMapInverseToneMapping;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_I)
  {
    vs.m_PlaceboColorMapLut3dSizeI = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_map_params.lut3d_size[0] = vs.m_PlaceboColorMapLut3dSizeI;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_C)
  {
    vs.m_PlaceboColorMapLut3dSizeC = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_map_params.lut3d_size[1] = vs.m_PlaceboColorMapLut3dSizeC;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_H)
  {
    vs.m_PlaceboColorMapLut3dSizeH = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_map_params.lut3d_size[2] = vs.m_PlaceboColorMapLut3dSizeH;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_TRICUBIC)
  {
    vs.m_PlaceboColorMapLut3dTricubic = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->color_map_params.lut3d_tricubic = vs.m_PlaceboColorMapLut3dTricubic;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LUT_SIZE)
  {
    vs.m_PlaceboColorMapLutSize = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_map_params.lut_size = vs.m_PlaceboColorMapLutSize;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_SHOW_CLIPPING)
  {
    vs.m_PlaceboColorMapShowClipping = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->color_map_params.show_clipping = vs.m_PlaceboColorMapShowClipping;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_INTENT)
  {
    vs.m_PlaceboColorMapIntent = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->color_map_params.intent = (pl_rendering_intent)vs.m_PlaceboColorMapIntent;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_FORCE_TONE_MAPPING_LUT)
  {
    vs.m_PlaceboColorMapForceToneMappingLut = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->color_map_params.force_tone_mapping_lut = vs.m_PlaceboColorMapForceToneMappingLut;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_ANTIRINGING_STRENGTH)
  {
    vs.m_PlaceboAntiringingStrength = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    m_placeboOptions->params.antiringing_strength = vs.m_PlaceboAntiringingStrength;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_CORRECT_SUBPIXEL_OFFSET)
  {
    vs.m_PlaceboCorrectSubpixelOffset = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.correct_subpixel_offsets = vs.m_PlaceboCorrectSubpixelOffset;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DISABLE_BUILTIN_SCALERS)
  {
    vs.m_PlaceboDisableBuiltinScalers = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.disable_builtin_scalers = vs.m_PlaceboDisableBuiltinScalers;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DISABLE_DITHER_GAMMA_CORRECTION)
  {
    vs.m_PlaceboDisableDitherGammaCorrection = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.disable_dither_gamma_correction = vs.m_PlaceboDisableDitherGammaCorrection;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DISABLE_LINEAR_SCALING)
  {
    vs.m_PlaceboDisableLinearScaling = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.disable_linear_scaling = vs.m_PlaceboDisableLinearScaling;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_DYNAMIC_CONSTANTS)
  {
    vs.m_PlaceboDynamicConstant = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.dynamic_constants = vs.m_PlaceboDynamicConstant;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_ERROR_DIFFUSION)
  {
    vs.m_PlaceboErrorDiffusion = static_cast<int>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    m_placeboOptions->params.error_diffusion = vs.m_PlaceboErrorDiffusion == -1 ? NULL : pl_error_diffusion_kernels[vs.m_PlaceboErrorDiffusion];
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_FORCE_DITHER)
  {
    vs.m_PlaceboForceDither = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.force_dither = vs.m_PlaceboForceDither;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_FORCE_LOW_BIT_DEPTH_FBOS)
  {
    vs.m_PlaceboForceLowBitDepthFbos = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.force_low_bit_depth_fbos = vs.m_PlaceboForceLowBitDepthFbos;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_IGNORE_ICC_PROFILES)
  {
    vs.m_PlaceboIgnoreIccProfiles = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.ignore_icc_profiles = vs.m_PlaceboIgnoreIccProfiles;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_PRESERVE_MIXING_CACHE)
  {
    vs.m_PlaceboPreserveMixingCache = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.preserve_mixing_cache = vs.m_PlaceboPreserveMixingCache;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_SKIP_ANTI_ALIASING)
  {
    vs.m_PlaceboSkipAntiAliasing = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.skip_anti_aliasing = vs.m_PlaceboSkipAntiAliasing;
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_LIB_PLACEBO_SKIP_CACHING_SINGLE_FRAME)
  {
    vs.m_PlaceboSkipCachingSingleFrame = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    m_placeboOptions->params.skip_caching_single_frame = vs.m_PlaceboSkipCachingSingleFrame;
  }
}

int CGUIDialogVideoSettings::getErrorDiffusionIndexFromDescription(std::string description)
{
  const struct pl_error_diffusion_kernel* f;
  for (int i = 0; i < pl_num_error_diffusion_kernels; i++)
  {
    f = pl_error_diffusion_kernels[i];
    if (f->description != nullptr)
      if (description == std::string(f->description))
        return i;
  }
  return -1;
}

int CGUIDialogVideoSettings::getColorMapIntentIndexFromDescription(std::string description)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;
  PlColorMapIntentOptionFiller(asetting, alist, value);
  if (description == "")
    return -1;

  for (int i = 0; i < alist.size(); i++)
  {
    if (alist[i].label == description)
      return i;
  }
  return -1;
}

int CGUIDialogVideoSettings::getDitherMethodIndexFromDescription(std::string description)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;
  PlDitherMethodOptionFiller(asetting, alist, value);
  if (description == "")
    return -1;

  for (int i = 0; i < alist.size(); i++)
  {
    if (alist[i].label == description)
      return i;
  }
  return -1;
}

int CGUIDialogVideoSettings::getDitherTransferIndexFromDescription(std::string description)
{
  for (int i = 0; i < PL_COLOR_TRC_COUNT; i++)
  {
    if (pl_color_transfer_name(static_cast<pl_color_transfer>(i)) == description)
      return i;
  }
  return -1;
}

int CGUIDialogVideoSettings::getConeConesIndexFromDescription(std::string description)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;
  PlConeConesOptionFiller(asetting, alist, value);
  if (description == "")
    return -1;

  for (int i = 0; i < alist.size(); i++)
  {
    if (alist[i].label == description)
      return i;
  }
  return -1;
}


int CGUIDialogVideoSettings::getFilterIndexFromDescription(std::string description)
{
  const struct pl_filter_config* f;
  for (int i = 0; i < pl_num_filter_configs; i++)
  {
    f = pl_filter_configs[i];
    if(f->description != nullptr)
      if (description == std::string(f->description))
        return i;
  }
  return -1;
}

int CGUIDialogVideoSettings::getToneMapIndexFromDescription(std::string description)
{
  const struct pl_tone_map_function* f;
  for (int i = 0; i < pl_num_tone_map_functions; i++)
  {
    f = pl_tone_map_functions[i];
    if (f->description != nullptr)
      if (description == f->description)
        return i;
  }
  return -1;
}

int CGUIDialogVideoSettings::getGamutMapIndexFromDescription(std::string description)
{
  const struct pl_gamut_map_function* f;
  for (int i = 0; i < pl_num_gamut_map_functions; i++)
  {
    f = pl_gamut_map_functions[i];
    if (f->description != nullptr)
      if (description == f->description)
        return i;
  }
  return -1;
}


int CGUIDialogVideoSettings::getDeinterlaceAlgoIndexFromDescription(std::string description)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;
  PlDeinterlaceAlgoOptionFiller(asetting, alist, value);
  if (description == "")
    return -1;

  for (int i = 0; i < alist.size(); i++)
  {
    if (alist[i].label == description)
      return i;
  }
  return -1;
}


void CGUIDialogVideoSettings::UpdateVideoSettingsFromLibPLaceboParams(CVideoSettings &vs)
{
  pl_options m_placeboOptions = vs.m_placeboOptions->getPlOptions();
  
  vs.m_Brightness = (m_placeboOptions->color_adjustment.brightness + 1.0) * 50.0;
  vs.m_Contrast = log10f(m_placeboOptions->color_adjustment.contrast * 99.0/100.0 + 0.01)*25.0+50.0;
  vs.m_Gamma = log10f(m_placeboOptions->color_adjustment.gamma *(1.0-pow(10,-0.5)) + pow(10,-0.5)) * 40.0 + 20.0;
  
  vs.m_PlaceboColorAdjustmentEnabled = m_placeboOptions->params.color_adjustment != NULL;
  vs.m_PlaceboSaturation = m_placeboOptions->color_adjustment.saturation;
  vs.m_PlaceboHue         = fmod(m_placeboOptions->color_adjustment.hue * 180.0 / M_PI, 360.0);
  vs.m_PlaceboTemperature = m_placeboOptions->color_adjustment.temperature * 3500.0 + 6500.0;

  vs.m_PlaceboPeakDetectEnabled            = m_placeboOptions->params.peak_detect_params != NULL;
  vs.m_PlaceboPeakDetectSmoothingPeriod = m_placeboOptions->peak_detect_params.smoothing_period;
  vs.m_PlaceboPeakDetectSceneThresholdLow = m_placeboOptions->peak_detect_params.scene_threshold_low;
  vs.m_PlaceboPeakDetectSceneThresholdHigh = m_placeboOptions->peak_detect_params.scene_threshold_high;
  vs.m_PlaceboPeakDetectPercentile = m_placeboOptions->peak_detect_params.percentile;
  vs.m_PlaceboPeakDetectBlackCutoff = m_placeboOptions->peak_detect_params.black_cutoff;
  vs.m_PlaceboPeakDetectAllowDelayed = m_placeboOptions->peak_detect_params.allow_delayed;

  vs.m_PlaceboUpscaler =        m_placeboOptions->params.upscaler         == NULL ? -1 : getFilterIndexFromDescription(m_placeboOptions->params.upscaler->description);
  vs.m_PlaceboDownscaler =      m_placeboOptions->params.downscaler       == NULL ? -1 : getFilterIndexFromDescription(m_placeboOptions->params.downscaler->description);
  vs.m_PlaceboPlaneUpscaler =   m_placeboOptions->params.plane_upscaler   == NULL ? -1 : getFilterIndexFromDescription(m_placeboOptions->params.plane_upscaler->description);
  vs.m_PlaceboPlaneDownscaler = m_placeboOptions->params.plane_downscaler == NULL ? -1 : getFilterIndexFromDescription(m_placeboOptions->params.plane_downscaler->description);
  vs.m_PlaceboFrameMixer =      m_placeboOptions->params.frame_mixer      == NULL ? -1 : getFilterIndexFromDescription(m_placeboOptions->params.frame_mixer->description);

  vs.m_PlaceboDebandEnabled = m_placeboOptions->params.deband_params != NULL;
  vs.m_PlaceboDebandGrain = m_placeboOptions->deband_params.grain;
  vs.m_PlaceboDebandGrainNeutral0 = m_placeboOptions->deband_params.grain_neutral[0];
  vs.m_PlaceboDebandGrainNeutral1 = m_placeboOptions->deband_params.grain_neutral[1];
  vs.m_PlaceboDebandGrainNeutral2 = m_placeboOptions->deband_params.grain_neutral[2];
  vs.m_PlaceboDebandIterations = m_placeboOptions->deband_params.iterations;
  vs.m_PlaceboDebandRadius = m_placeboOptions->deband_params.radius;
  vs.m_PlaceboDebandThreshold = m_placeboOptions->deband_params.threshold;

  vs.m_PlaceboColorMapEnabled = m_placeboOptions->params.color_map_params != NULL;
  vs.m_PlaceboColorMapGamutMapping = m_placeboOptions->color_map_params.gamut_mapping == NULL ? -1 : getGamutMapIndexFromDescription(m_placeboOptions->color_map_params.gamut_mapping->description); 
  vs.m_PlaceboColorMapToneMapping = m_placeboOptions->color_map_params.tone_mapping_function == NULL ? -1 : getGamutMapIndexFromDescription(m_placeboOptions->color_map_params.tone_mapping_function->description);
  vs.m_PlaceboColorMapContrastRecovery = m_placeboOptions->color_map_params.contrast_recovery;
  vs.m_PlaceboColorMapContrastSmoothness = m_placeboOptions->color_map_params.contrast_smoothness;
  vs.m_PlaceboColorMapGamutExpansion = m_placeboOptions->color_map_params.gamut_expansion;
  vs.m_PlaceboColorMapInverseToneMapping = m_placeboOptions->color_map_params.inverse_tone_mapping;
  vs.m_PlaceboColorMapLut3dSizeI = m_placeboOptions->color_map_params.lut3d_size[0];
  vs.m_PlaceboColorMapLut3dSizeC = m_placeboOptions->color_map_params.lut3d_size[1];
  vs.m_PlaceboColorMapLut3dSizeH = m_placeboOptions->color_map_params.lut3d_size[2];
  vs.m_PlaceboColorMapLut3dTricubic = m_placeboOptions->color_map_params.lut3d_tricubic;
  vs.m_PlaceboColorMapLutSize = m_placeboOptions->color_map_params.lut_size;
  vs.m_PlaceboColorMapShowClipping = m_placeboOptions->color_map_params.show_clipping;
  vs.m_PlaceboColorMapIntent = m_placeboOptions->color_map_params.intent;
  vs.m_PlaceboColorMapForceToneMappingLut = m_placeboOptions->color_map_params.force_tone_mapping_lut;

  vs.m_PlaceboDeinterlaceEnabled = m_placeboOptions->params.deinterlace_params != NULL;
  vs.m_PlaceboDeinterlaceAlgo = (int)m_placeboOptions->deinterlace_params.algo; 
  vs.m_PlaceboDeinterlaceSkipSpatialCheck = m_placeboOptions->deinterlace_params.skip_spatial_check;

  vs.m_PlaceboSigmoidEnabled = m_placeboOptions->params.sigmoid_params != NULL;
  vs.m_PlaceboSigmoidCenter = m_placeboOptions->sigmoid_params.center;
  vs.m_PlaceboSigmoidSlope = m_placeboOptions->sigmoid_params.slope;

  vs.m_PlaceboConeEnabled = m_placeboOptions->params.cone_params != NULL;
  vs.m_PlaceboConeCones = (int)m_placeboOptions->cone_params.cones;
  vs.m_PlaceboConeStrength = m_placeboOptions->cone_params.strength;

  vs.m_PlaceboDitherEnabled = m_placeboOptions->params.dither_params != NULL;
  vs.m_PlaceboDitherMethod = (int)m_placeboOptions->dither_params.method;
  vs.m_PlaceboDitherLutSize = m_placeboOptions->dither_params.lut_size;
  vs.m_PlaceboDitherTemporal = m_placeboOptions->dither_params.temporal;
  vs.m_PlaceboDitherTransfer = (int)m_placeboOptions->dither_params.transfer;

  vs.m_PlaceboToneConstantExposure = m_placeboOptions->color_map_params.tone_constants.exposure;
  vs.m_PlaceboToneConstantKneeAdaptation = m_placeboOptions->color_map_params.tone_constants.knee_adaptation;
  vs.m_PlaceboToneConstantKneeDefault = m_placeboOptions->color_map_params.tone_constants.knee_default;
  vs.m_PlaceboToneConstantKneeMaximum = m_placeboOptions->color_map_params.tone_constants.knee_maximum;
  vs.m_PlaceboToneConstantKneeMinimum = m_placeboOptions->color_map_params.tone_constants.knee_minimum;
  vs.m_PlaceboToneConstantKneeOffset = m_placeboOptions->color_map_params.tone_constants.knee_offset;
  vs.m_PlaceboToneConstantLinearKnee = m_placeboOptions->color_map_params.tone_constants.linear_knee;
  vs.m_PlaceboToneConstantReinhardContrast = m_placeboOptions->color_map_params.tone_constants.reinhard_contrast;
  vs.m_PlaceboToneConstantSlopeOffset = m_placeboOptions->color_map_params.tone_constants.slope_offset;
  vs.m_PlaceboToneConstantSlopeTuning = m_placeboOptions->color_map_params.tone_constants.slope_tuning;
  vs.m_PlaceboToneConstantSplineContrast = m_placeboOptions->color_map_params.tone_constants.spline_contrast;

  vs.m_PlaceboColorMapVisualizeLut    = m_placeboOptions->color_map_params.visualize_lut;
  vs.m_PlaceboColorMapVisualizeRectX0 = m_placeboOptions->color_map_params.visualize_rect.x0;
  vs.m_PlaceboColorMapVisualizeRectX1 = m_placeboOptions->color_map_params.visualize_rect.x1;
  vs.m_PlaceboColorMapVisualizeRectY0 = m_placeboOptions->color_map_params.visualize_rect.y0;
  vs.m_PlaceboColorMapVisualizeRectY1 = m_placeboOptions->color_map_params.visualize_rect.y1;
  vs.m_PlaceboColorMapVisualizeHue    = fmod(m_placeboOptions->color_map_params.visualize_hue * 180.0 / M_PI, 360.0);
  vs.m_PlaceboColorMapVisualizeTheta  = fmod(m_placeboOptions->color_map_params.visualize_theta * 180.0 / M_PI, 360.0);

  vs.m_PlaceboGamutConstantsColorimetricGamma = m_placeboOptions->color_map_params.gamut_constants.colorimetric_gamma;
  vs.m_PlaceboGamutConstantsPerceptualDeadzone = m_placeboOptions->color_map_params.gamut_constants.perceptual_deadzone;
  vs.m_PlaceboGamutConstantsSoftclipDesat = m_placeboOptions->color_map_params.gamut_constants.softclip_desat;
  vs.m_PlaceboGamutConstantsSoftclipKnee = m_placeboOptions->color_map_params.gamut_constants.softclip_knee;

  vs.m_PlaceboAntiringingStrength = m_placeboOptions->params.antiringing_strength;
  vs.m_PlaceboCorrectSubpixelOffset = m_placeboOptions->params.correct_subpixel_offsets;
  vs.m_PlaceboDisableBuiltinScalers = m_placeboOptions->params.disable_builtin_scalers;
  vs.m_PlaceboDisableDitherGammaCorrection = m_placeboOptions->params.disable_dither_gamma_correction;
  vs.m_PlaceboDisableLinearScaling = m_placeboOptions->params.disable_linear_scaling;
  vs.m_PlaceboDynamicConstant = m_placeboOptions->params.dynamic_constants;

  vs.m_PlaceboErrorDiffusion = m_placeboOptions->params.error_diffusion == NULL ? -1 : getErrorDiffusionIndexFromDescription(m_placeboOptions->params.error_diffusion->description);
  vs.m_PlaceboForceDither = m_placeboOptions->params.force_dither;
  vs.m_PlaceboForceLowBitDepthFbos = m_placeboOptions->params.force_low_bit_depth_fbos;
  vs.m_PlaceboIgnoreIccProfiles = m_placeboOptions->params.ignore_icc_profiles;
  vs.m_PlaceboPreserveMixingCache = m_placeboOptions->params.preserve_mixing_cache;
  vs.m_PlaceboSkipAntiAliasing = m_placeboOptions->params.skip_anti_aliasing;
  vs.m_PlaceboSkipCachingSingleFrame = m_placeboOptions->params.skip_caching_single_frame;
  
}

void CGUIDialogVideoSettings::UpdateLibPLaceboParamsFromVideoSettings(CVideoSettings& vs)
{
  pl_options m_placeboOptions = vs.m_placeboOptions->getPlOptions();
  
  m_placeboOptions->params.color_adjustment = vs.m_PlaceboColorAdjustmentEnabled ? &m_placeboOptions->color_adjustment : NULL;
  m_placeboOptions->color_adjustment.brightness = vs.m_Brightness / 50.0 - 1.0;
  m_placeboOptions->color_adjustment.contrast = (pow(10.0, (vs.m_Contrast - 50.0) / 25.0) - 0.01) * 100.0 / 99.0;
  m_placeboOptions->color_adjustment.gamma = (pow(10.0, (vs.m_Gamma - 20.0) / 40.0) - pow(10, -0.5)) * 1.0 / (1.0 - pow(10, -0.5));
  m_placeboOptions->color_adjustment.saturation = vs.m_PlaceboSaturation;
  m_placeboOptions->color_adjustment.hue = fmod(vs.m_PlaceboHue, 360.0) * M_PI / 180.0;
  m_placeboOptions->color_adjustment.temperature = (vs.m_PlaceboTemperature - 6500.0) / 3500.0;

  m_placeboOptions->params.peak_detect_params = vs.m_PlaceboPeakDetectEnabled ? &m_placeboOptions->peak_detect_params : NULL;
  m_placeboOptions->peak_detect_params.smoothing_period = vs.m_PlaceboPeakDetectSmoothingPeriod;
  m_placeboOptions->peak_detect_params.scene_threshold_low = vs.m_PlaceboPeakDetectSceneThresholdLow;
  m_placeboOptions->peak_detect_params.scene_threshold_high = vs.m_PlaceboPeakDetectSceneThresholdHigh;
  m_placeboOptions->peak_detect_params.percentile = vs.m_PlaceboPeakDetectPercentile;
  m_placeboOptions->peak_detect_params.black_cutoff = vs.m_PlaceboPeakDetectBlackCutoff;
  m_placeboOptions->peak_detect_params.allow_delayed = vs.m_PlaceboPeakDetectAllowDelayed;

  m_placeboOptions->params.upscaler = vs.m_PlaceboUpscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboUpscaler];
  m_placeboOptions->params.downscaler = vs.m_PlaceboDownscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboDownscaler];
  m_placeboOptions->params.plane_upscaler = vs.m_PlaceboPlaneUpscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboPlaneUpscaler];
  m_placeboOptions->params.plane_downscaler = vs.m_PlaceboPlaneDownscaler == -1 ? NULL : pl_filter_configs[vs.m_PlaceboPlaneDownscaler];
  m_placeboOptions->params.frame_mixer = vs.m_PlaceboFrameMixer == -1 ? NULL : pl_filter_configs[vs.m_PlaceboFrameMixer];
  m_placeboOptions->color_map_params.gamut_mapping = vs.m_PlaceboColorMapGamutMapping == -1 ? NULL : pl_gamut_map_functions[vs.m_PlaceboColorMapGamutMapping];
  m_placeboOptions->color_map_params.tone_mapping_function = vs.m_PlaceboColorMapToneMapping == -1 ? NULL : pl_tone_map_functions[vs.m_PlaceboColorMapToneMapping];

  m_placeboOptions->params.deband_params = vs.m_PlaceboDebandEnabled ? &m_placeboOptions->deband_params : NULL;
  m_placeboOptions->deband_params.grain = vs.m_PlaceboDebandGrain;
  m_placeboOptions->deband_params.grain_neutral[0] = vs.m_PlaceboDebandGrainNeutral0;
  m_placeboOptions->deband_params.grain_neutral[1] = vs.m_PlaceboDebandGrainNeutral1;
  m_placeboOptions->deband_params.grain_neutral[2] = vs.m_PlaceboDebandGrainNeutral2;
  m_placeboOptions->deband_params.iterations = vs.m_PlaceboDebandIterations;
  m_placeboOptions->deband_params.radius = vs.m_PlaceboDebandRadius;
  m_placeboOptions->deband_params.threshold = vs.m_PlaceboDebandThreshold;

  m_placeboOptions->params.color_map_params = vs.m_PlaceboColorMapEnabled ? &m_placeboOptions->color_map_params : NULL;
  // peak
  m_placeboOptions->color_map_params.contrast_recovery = vs.m_PlaceboColorMapContrastRecovery;
  m_placeboOptions->color_map_params.contrast_smoothness = vs.m_PlaceboColorMapContrastSmoothness;
  m_placeboOptions->color_map_params.gamut_expansion = vs.m_PlaceboColorMapGamutExpansion;

  m_placeboOptions->color_map_params.inverse_tone_mapping = vs.m_PlaceboColorMapInverseToneMapping;
  m_placeboOptions->color_map_params.lut3d_size[0] = vs.m_PlaceboColorMapLut3dSizeI;
  m_placeboOptions->color_map_params.lut3d_size[1] = vs.m_PlaceboColorMapLut3dSizeC;
  m_placeboOptions->color_map_params.lut3d_size[2] = vs.m_PlaceboColorMapLut3dSizeH;
  m_placeboOptions->color_map_params.lut3d_tricubic = vs.m_PlaceboColorMapLut3dTricubic;
  m_placeboOptions->color_map_params.lut_size = vs.m_PlaceboColorMapLutSize;
  m_placeboOptions->color_map_params.show_clipping = vs.m_PlaceboColorMapShowClipping;

  m_placeboOptions->color_map_params.intent = (pl_rendering_intent)vs.m_PlaceboColorMapIntent;
  m_placeboOptions->color_map_params.force_tone_mapping_lut = vs.m_PlaceboColorMapForceToneMappingLut;

  m_placeboOptions->params.deinterlace_params = vs.m_PlaceboDeinterlaceEnabled ? &m_placeboOptions->deinterlace_params : NULL;
  m_placeboOptions->deinterlace_params.algo = (enum pl_deinterlace_algorithm)vs.m_PlaceboDeinterlaceAlgo;
  m_placeboOptions->deinterlace_params.skip_spatial_check = vs.m_PlaceboDeinterlaceSkipSpatialCheck;
  m_placeboOptions->params.sigmoid_params = vs.m_PlaceboSigmoidEnabled ? &m_placeboOptions->sigmoid_params : NULL;
  m_placeboOptions->sigmoid_params.center = vs.m_PlaceboSigmoidCenter;
  m_placeboOptions->sigmoid_params.slope = vs.m_PlaceboSigmoidSlope;

  m_placeboOptions->params.cone_params = vs.m_PlaceboConeEnabled ? &m_placeboOptions->cone_params : NULL;
  m_placeboOptions->cone_params.cones = (enum pl_cone)vs.m_PlaceboConeCones;
  m_placeboOptions->cone_params.strength = vs.m_PlaceboConeStrength;

  m_placeboOptions->params.dither_params = vs.m_PlaceboDitherEnabled ? &m_placeboOptions->dither_params : NULL;
  m_placeboOptions->dither_params.method = (enum pl_dither_method)vs.m_PlaceboDitherMethod;
  m_placeboOptions->dither_params.lut_size = vs.m_PlaceboDitherLutSize;
  m_placeboOptions->dither_params.temporal = vs.m_PlaceboDitherTemporal;
  m_placeboOptions->dither_params.transfer = (enum pl_color_transfer)vs.m_PlaceboDitherTransfer;

  m_placeboOptions->color_map_params.tone_constants.exposure = vs.m_PlaceboToneConstantExposure;
  m_placeboOptions->color_map_params.tone_constants.knee_adaptation = vs.m_PlaceboToneConstantKneeAdaptation;
  m_placeboOptions->color_map_params.tone_constants.knee_default = vs.m_PlaceboToneConstantKneeDefault;
  m_placeboOptions->color_map_params.tone_constants.knee_maximum = vs.m_PlaceboToneConstantKneeMaximum;
  m_placeboOptions->color_map_params.tone_constants.knee_minimum = vs.m_PlaceboToneConstantKneeMinimum;
  m_placeboOptions->color_map_params.tone_constants.knee_offset = vs.m_PlaceboToneConstantKneeOffset;
  m_placeboOptions->color_map_params.tone_constants.linear_knee = vs.m_PlaceboToneConstantLinearKnee;
  m_placeboOptions->color_map_params.tone_constants.reinhard_contrast = vs.m_PlaceboToneConstantReinhardContrast;
  m_placeboOptions->color_map_params.tone_constants.slope_offset = vs.m_PlaceboToneConstantSlopeOffset;
  m_placeboOptions->color_map_params.tone_constants.slope_tuning = vs.m_PlaceboToneConstantSlopeTuning;
  m_placeboOptions->color_map_params.tone_constants.spline_contrast = vs.m_PlaceboToneConstantSplineContrast;

  m_placeboOptions->color_map_params.visualize_lut = vs.m_PlaceboColorMapVisualizeLut;
  m_placeboOptions->color_map_params.visualize_rect.x0 = vs.m_PlaceboColorMapVisualizeRectX0;
  m_placeboOptions->color_map_params.visualize_rect.x1 = vs.m_PlaceboColorMapVisualizeRectX1;
  m_placeboOptions->color_map_params.visualize_rect.y0 = vs.m_PlaceboColorMapVisualizeRectY0;
  m_placeboOptions->color_map_params.visualize_rect.y1 = vs.m_PlaceboColorMapVisualizeRectY1;
  m_placeboOptions->color_map_params.visualize_hue = fmod(vs.m_PlaceboColorMapVisualizeHue, 360.0) * M_PI / 180.0;;
  m_placeboOptions->color_map_params.visualize_theta = fmod(vs.m_PlaceboColorMapVisualizeTheta, 360.0) * M_PI / 180.0;

  m_placeboOptions->color_map_params.gamut_constants.colorimetric_gamma = vs.m_PlaceboGamutConstantsColorimetricGamma;
  m_placeboOptions->color_map_params.gamut_constants.perceptual_deadzone = vs.m_PlaceboGamutConstantsPerceptualDeadzone;
  m_placeboOptions->color_map_params.gamut_constants.softclip_desat = vs.m_PlaceboGamutConstantsSoftclipDesat;
  m_placeboOptions->color_map_params.gamut_constants.softclip_knee = vs.m_PlaceboGamutConstantsSoftclipKnee;

  m_placeboOptions->params.antiringing_strength = vs.m_PlaceboAntiringingStrength;
  m_placeboOptions->params.correct_subpixel_offsets = vs.m_PlaceboCorrectSubpixelOffset;
  m_placeboOptions->params.disable_builtin_scalers = vs.m_PlaceboDisableBuiltinScalers;
  m_placeboOptions->params.disable_dither_gamma_correction = vs.m_PlaceboDisableDitherGammaCorrection;
  m_placeboOptions->params.disable_linear_scaling = vs.m_PlaceboDisableLinearScaling;
  m_placeboOptions->params.dynamic_constants = vs.m_PlaceboDynamicConstant;

  m_placeboOptions->params.error_diffusion = vs.m_PlaceboErrorDiffusion == -1 ? NULL : pl_error_diffusion_kernels[vs.m_PlaceboErrorDiffusion];
  m_placeboOptions->params.force_dither = vs.m_PlaceboForceDither;
  m_placeboOptions->params.force_low_bit_depth_fbos = vs.m_PlaceboForceLowBitDepthFbos;
  m_placeboOptions->params.ignore_icc_profiles = vs.m_PlaceboIgnoreIccProfiles;
  m_placeboOptions->params.preserve_mixing_cache = vs.m_PlaceboPreserveMixingCache;
  m_placeboOptions->params.skip_anti_aliasing = vs.m_PlaceboSkipAntiAliasing;
  m_placeboOptions->params.skip_caching_single_frame = vs.m_PlaceboSkipCachingSingleFrame;
}

void CGUIDialogVideoSettings::SaveLibplaceboSettings(const CVideoSettings& vs, const std::string path)
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement rootElement("libPlaceboSettings");
  rootElement.SetAttribute(SETTING_XML_ROOT_VERSION, "1.0");
  TiXmlNode* lpNode = xmlDoc.InsertEndChild(rootElement);

  if (!lpNode)
  {
    CLog::LogF(LOGERROR, "Failed to create XML node for LibPLacebo settings file \"{}\"", path);
  }
  else
  {
    XMLUtils::SetFloat(lpNode, "placebodisplaypeakluminance", vs.m_PlaceboDisplayPeakLuminance);
    XMLUtils::SetInt(lpNode, "placebotargetcolorspacehint", vs.m_PlaceboTargetColorspaceHint);
    XMLUtils::SetInt(lpNode, "placebotargetcolorspacehintmode", vs.m_PlaceboTargetColorspaceHintMode);

    XMLUtils::SetBoolean(lpNode, "placebocoloradjustmentenabled", vs.m_PlaceboColorAdjustmentEnabled);
    XMLUtils::SetFloat(lpNode, "saturation", vs.m_PlaceboSaturation);
    XMLUtils::SetFloat(lpNode, "hue", vs.m_PlaceboHue);
    XMLUtils::SetFloat(lpNode, "temperature", vs.m_PlaceboTemperature);

    XMLUtils::SetBoolean(lpNode, "placebopeakdetectenabled", vs.m_PlaceboPeakDetectEnabled);
    XMLUtils::SetFloat(lpNode, "placebopeakdetectsmoothingperiod", vs.m_PlaceboPeakDetectSmoothingPeriod);
    XMLUtils::SetFloat(lpNode, "placebopeakdetectscenethresholdlow", vs.m_PlaceboPeakDetectSceneThresholdLow);
    XMLUtils::SetFloat(lpNode, "placebopeakdetectscenethresholdhigh", vs.m_PlaceboPeakDetectSceneThresholdHigh);
    XMLUtils::SetFloat(lpNode, "placebopeakdetectpercentile", vs.m_PlaceboPeakDetectPercentile);
    XMLUtils::SetFloat(lpNode, "placebopeakdetectblackcutoff", vs.m_PlaceboPeakDetectBlackCutoff);
    XMLUtils::SetBoolean(lpNode, "placebopeakdetectallowdelayed", vs.m_PlaceboPeakDetectAllowDelayed);

    XMLUtils::SetString(lpNode, "placeboupscaler", vs.m_PlaceboUpscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboUpscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboUpscaler]->description);
    XMLUtils::SetString(lpNode, "placebodownscaler", vs.m_PlaceboDownscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboDownscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboDownscaler]->description);
    XMLUtils::SetString(lpNode, "placeboplaneupscaler", vs.m_PlaceboPlaneUpscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboPlaneUpscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboPlaneUpscaler]->description);
    XMLUtils::SetString(lpNode, "placeboplanedownscaler", vs.m_PlaceboPlaneDownscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboPlaneDownscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboPlaneDownscaler]->description);
    XMLUtils::SetString(lpNode, "placeboframemixer", vs.m_PlaceboFrameMixer == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboFrameMixer]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboFrameMixer]->description);

    XMLUtils::SetBoolean(lpNode, "placebodebandenabled", vs.m_PlaceboDebandEnabled);
    XMLUtils::SetFloat(lpNode, "placebodebandgrain", vs.m_PlaceboDebandGrain);
    XMLUtils::SetFloat(lpNode, "placebodebandgrainneutral0", vs.m_PlaceboDebandGrainNeutral0);
    XMLUtils::SetFloat(lpNode, "placebodebandgrainneutral1", vs.m_PlaceboDebandGrainNeutral1);
    XMLUtils::SetFloat(lpNode, "placebodebandgrainneutral2", vs.m_PlaceboDebandGrainNeutral2);
    XMLUtils::SetInt(lpNode, "placebodebanditerations", vs.m_PlaceboDebandIterations);
    XMLUtils::SetFloat(lpNode, "placebodebandradius", vs.m_PlaceboDebandRadius);
    XMLUtils::SetFloat(lpNode, "placebodebandthreshold", vs.m_PlaceboDebandThreshold);

    XMLUtils::SetBoolean(lpNode, "placebocolormapenabled", vs.m_PlaceboColorMapEnabled);
    XMLUtils::SetFloat(lpNode, "placebocolormapcontrastrecovery", vs.m_PlaceboColorMapContrastRecovery);
    XMLUtils::SetFloat(lpNode, "placebocolormapcontrastsmoothness", vs.m_PlaceboColorMapContrastSmoothness);
    XMLUtils::SetBoolean(lpNode, "placebocolormapgamutexpansion", vs.m_PlaceboColorMapGamutExpansion);
    XMLUtils::SetString(lpNode, "placebocolormapgamutmapping", vs.m_PlaceboColorMapGamutMapping == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboColorMapGamutMapping]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboColorMapGamutMapping]->description);
    XMLUtils::SetString(lpNode, "placebocolormaptonemapping", vs.m_PlaceboColorMapToneMapping == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboColorMapToneMapping]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboColorMapToneMapping]->description);
    XMLUtils::SetBoolean(lpNode, "placebocolormapinversetonemapping", vs.m_PlaceboColorMapInverseToneMapping);
    XMLUtils::SetInt(lpNode, "placebocolormaplut3dsizei", vs.m_PlaceboColorMapLut3dSizeI);
    XMLUtils::SetInt(lpNode, "placebocolormaplut3dsizec", vs.m_PlaceboColorMapLut3dSizeC);
    XMLUtils::SetInt(lpNode, "placebocolormaplut3dsizeh", vs.m_PlaceboColorMapLut3dSizeH);
    XMLUtils::SetBoolean(lpNode, "placebocolormaplut3dtricubic", vs.m_PlaceboColorMapLut3dTricubic);
    XMLUtils::SetInt(lpNode, "placebocolormaplutsize", vs.m_PlaceboColorMapLutSize);
    XMLUtils::SetBoolean(lpNode, "placebocolormapshowclipping", vs.m_PlaceboColorMapShowClipping);
    XMLUtils::SetString(lpNode, "placebocolormapintent", getColorMapIntentDescriptionFromIndex(vs.m_PlaceboColorMapIntent));
    XMLUtils::SetBoolean(lpNode, "placebocolormapforcetonemappinglut", vs.m_PlaceboColorMapForceToneMappingLut);

    XMLUtils::SetBoolean(lpNode, "placebodeinterlaceenabled", vs.m_PlaceboDeinterlaceEnabled);
    XMLUtils::SetString(lpNode, "placebodeinterlacealgo", getDeinterlaceAlgoDescriptionFromIndex(vs.m_PlaceboDeinterlaceAlgo));
    XMLUtils::SetBoolean(lpNode, "placebodeinterlaceskipspatialcheck", vs.m_PlaceboDeinterlaceSkipSpatialCheck);

    XMLUtils::SetBoolean(lpNode, "placeboconeenabled", vs.m_PlaceboConeEnabled);
    XMLUtils::SetString(lpNode, "placeboconecones", getConeConesDescriptionFromIndex(vs.m_PlaceboConeCones));
    XMLUtils::SetFloat(lpNode, "placeboconestrength", vs.m_PlaceboConeStrength);

    XMLUtils::SetBoolean(lpNode, "placebosigmoidenabled", vs.m_PlaceboSigmoidEnabled);
    XMLUtils::SetFloat(lpNode, "placebosigmoidcenter", vs.m_PlaceboSigmoidCenter);
    XMLUtils::SetFloat(lpNode, "placebosigmoidslope", vs.m_PlaceboSigmoidSlope);

    XMLUtils::SetBoolean(lpNode, "placeboditherenabled", vs.m_PlaceboDitherEnabled);
    XMLUtils::SetString(lpNode, "placebodithermethod", getDitherMethodDescriptionFromIndex(vs.m_PlaceboDitherMethod));
    XMLUtils::SetInt(lpNode, "placeboditherlutsize", vs.m_PlaceboDitherLutSize);
    XMLUtils::SetBoolean(lpNode, "placebodithertemporal", vs.m_PlaceboDitherTemporal);
    XMLUtils::SetString(lpNode, "placebodithertransfer", getDitherTransferDescriptionFromIndex(vs.m_PlaceboDitherTransfer));

    XMLUtils::SetFloat(lpNode, "placebotoneconstantsexposure", vs.m_PlaceboToneConstantExposure);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantskneeadaptation", vs.m_PlaceboToneConstantKneeAdaptation);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantskneedefault", vs.m_PlaceboToneConstantKneeDefault);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantskneemaximum", vs.m_PlaceboToneConstantKneeMaximum);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantskneeminimum", vs.m_PlaceboToneConstantKneeMinimum);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantskneeoffset", vs.m_PlaceboToneConstantKneeOffset);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantslinearknee", vs.m_PlaceboToneConstantLinearKnee);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantsreinhardcontrast", vs.m_PlaceboToneConstantReinhardContrast);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantsslopeoffset", vs.m_PlaceboToneConstantSlopeOffset);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantsslopetuning", vs.m_PlaceboToneConstantSlopeTuning);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantssplinecontrast", vs.m_PlaceboToneConstantSplineContrast);

    XMLUtils::SetBoolean(lpNode, "placebotoneconstantscolormapvisualizelut", vs.m_PlaceboColorMapVisualizeLut);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolormapvisualizerectx0", vs.m_PlaceboColorMapVisualizeRectX0);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolormapvisualizerectx1", vs.m_PlaceboColorMapVisualizeRectX1);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolormapvisualizerecty0", vs.m_PlaceboColorMapVisualizeRectY0);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolormapvisualizerecty1", vs.m_PlaceboColorMapVisualizeRectY1);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolormapvisualizehue", vs.m_PlaceboColorMapVisualizeHue);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolormapvisualizetheta", vs.m_PlaceboColorMapVisualizeTheta);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantscolorimetricgamma", vs.m_PlaceboGamutConstantsColorimetricGamma);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantsperceptualdeadzone", vs.m_PlaceboGamutConstantsPerceptualDeadzone);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantssoftclipdesat", vs.m_PlaceboGamutConstantsSoftclipDesat);
    XMLUtils::SetFloat(lpNode, "placebotoneconstantssoftclipknee", vs.m_PlaceboGamutConstantsSoftclipKnee);

    XMLUtils::SetFloat(lpNode, "placebotoantiringingstrength", vs.m_PlaceboAntiringingStrength);
    XMLUtils::SetBoolean(lpNode, "placebotocorrectsubpixeloffset", vs.m_PlaceboCorrectSubpixelOffset);
    XMLUtils::SetBoolean(lpNode, "placebotodisablebuiltinscalers", vs.m_PlaceboDisableBuiltinScalers);
    XMLUtils::SetBoolean(lpNode, "placebotodisabledithergammacorrection", vs.m_PlaceboDisableDitherGammaCorrection);
    XMLUtils::SetBoolean(lpNode, "placebotodisablelinearscaling", vs.m_PlaceboDisableLinearScaling);
    XMLUtils::SetBoolean(lpNode, "placebotodynamicconstant", vs.m_PlaceboDynamicConstant);

    XMLUtils::SetString(lpNode, "placebotoerrordiffusion", getDiffusionKernelDescriptionFromIndex(vs.m_PlaceboErrorDiffusion));
    XMLUtils::SetBoolean(lpNode, "placebotoforcedither", vs.m_PlaceboForceDither);
    XMLUtils::SetBoolean(lpNode, "placebotoforcelowbitdepthfbos", vs.m_PlaceboForceLowBitDepthFbos);
    XMLUtils::SetBoolean(lpNode, "placebotoignoreiccprofiles", vs.m_PlaceboIgnoreIccProfiles);
    XMLUtils::SetBoolean(lpNode, "placebotopreservemixingcache", vs.m_PlaceboPreserveMixingCache);
    XMLUtils::SetBoolean(lpNode, "placebotoskipantialiasing", vs.m_PlaceboSkipAntiAliasing);
    XMLUtils::SetBoolean(lpNode, "placebotoskipcachingsingleframe", vs.m_PlaceboSkipCachingSingleFrame);

    if (!xmlDoc.SaveFile(path))
      CLog::LogF(LOGERROR, "Failed to save LibPLacebo settings to file \"{}\"", path);
  }
}

void CGUIDialogVideoSettings::SaveLibplaceboSettings(const CVideoSettings& vs, TiXmlNode* settings)
{

    XMLUtils::SetFloat(settings, "placebodisplaypeakluminance", vs.m_PlaceboDisplayPeakLuminance);
    XMLUtils::SetInt(settings, "placebotargetcolorspacehint", vs.m_PlaceboTargetColorspaceHint);
    XMLUtils::SetInt(settings, "placebotargetcolorspacehintmode", vs.m_PlaceboTargetColorspaceHintMode);

    XMLUtils::SetBoolean(settings, "placebocoloradjustmentenabled", vs.m_PlaceboColorAdjustmentEnabled);
    XMLUtils::SetFloat(settings, "saturation", vs.m_PlaceboSaturation);
    XMLUtils::SetFloat(settings, "hue", vs.m_PlaceboHue);
    XMLUtils::SetFloat(settings, "temperature", vs.m_PlaceboTemperature);

    XMLUtils::SetBoolean(settings, "placebopeakdetectenabled", vs.m_PlaceboPeakDetectEnabled);
    XMLUtils::SetFloat(settings, "placebopeakdetectsmoothingperiod", vs.m_PlaceboPeakDetectSmoothingPeriod);
    XMLUtils::SetFloat(settings, "placebopeakdetectscenethresholdlow", vs.m_PlaceboPeakDetectSceneThresholdLow);
    XMLUtils::SetFloat(settings, "placebopeakdetectscenethresholdhigh", vs.m_PlaceboPeakDetectSceneThresholdHigh);
    XMLUtils::SetFloat(settings, "placebopeakdetectpercentile", vs.m_PlaceboPeakDetectPercentile);
    XMLUtils::SetFloat(settings, "placebopeakdetectblackcutoff", vs.m_PlaceboPeakDetectBlackCutoff);
    XMLUtils::SetBoolean(settings, "placebopeakdetectallowdelayed", vs.m_PlaceboPeakDetectAllowDelayed);

    XMLUtils::SetString(settings, "placeboupscaler", vs.m_PlaceboUpscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboUpscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboUpscaler]->description);
    XMLUtils::SetString(settings, "placebodownscaler", vs.m_PlaceboDownscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboDownscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboDownscaler]->description);
    XMLUtils::SetString(settings, "placeboplaneupscaler", vs.m_PlaceboPlaneUpscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboPlaneUpscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboPlaneUpscaler]->description);
    XMLUtils::SetString(settings, "placeboplanedownscaler", vs.m_PlaceboPlaneDownscaler == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboPlaneDownscaler]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboPlaneDownscaler]->description);
    XMLUtils::SetString(settings, "placeboframemixer", vs.m_PlaceboFrameMixer == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboFrameMixer]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboFrameMixer]->description);

    XMLUtils::SetBoolean(settings, "placebodebandenabled", vs.m_PlaceboDebandEnabled);
    XMLUtils::SetFloat(settings, "placebodebandgrain", vs.m_PlaceboDebandGrain);
    XMLUtils::SetFloat(settings, "placebodebandgrainneutral0", vs.m_PlaceboDebandGrainNeutral0);
    XMLUtils::SetFloat(settings, "placebodebandgrainneutral1", vs.m_PlaceboDebandGrainNeutral1);
    XMLUtils::SetFloat(settings, "placebodebandgrainneutral2", vs.m_PlaceboDebandGrainNeutral2);
    XMLUtils::SetInt(settings, "placebodebanditerations", vs.m_PlaceboDebandIterations);
    XMLUtils::SetFloat(settings, "placebodebandradius", vs.m_PlaceboDebandRadius);
    XMLUtils::SetFloat(settings, "placebodebandthreshold", vs.m_PlaceboDebandThreshold);

    XMLUtils::SetBoolean(settings, "placebocolormapenabled", vs.m_PlaceboColorMapEnabled);
    XMLUtils::SetFloat(settings, "placebocolormapcontrastrecovery", vs.m_PlaceboColorMapContrastRecovery);
    XMLUtils::SetFloat(settings, "placebocolormapcontrastsmoothness", vs.m_PlaceboColorMapContrastSmoothness);
    XMLUtils::SetBoolean(settings, "placebocolormapgamutexpansion", vs.m_PlaceboColorMapGamutExpansion);
    XMLUtils::SetString(settings, "placebocolormapgamutmapping", vs.m_PlaceboColorMapGamutMapping == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboColorMapGamutMapping]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboColorMapGamutMapping]->description);
    XMLUtils::SetString(settings, "placebocolormaptonemapping", vs.m_PlaceboColorMapToneMapping == -1 ? "disabled" : pl_filter_configs[vs.m_PlaceboColorMapToneMapping]->description == nullptr ? "" : pl_filter_configs[vs.m_PlaceboColorMapToneMapping]->description);
    XMLUtils::SetBoolean(settings, "placebocolormapinversetonemapping", vs.m_PlaceboColorMapInverseToneMapping);
    XMLUtils::SetInt(settings, "placebocolormaplut3dsizei", vs.m_PlaceboColorMapLut3dSizeI);
    XMLUtils::SetInt(settings, "placebocolormaplut3dsizec", vs.m_PlaceboColorMapLut3dSizeC);
    XMLUtils::SetInt(settings, "placebocolormaplut3dsizeh", vs.m_PlaceboColorMapLut3dSizeH);
    XMLUtils::SetBoolean(settings, "placebocolormaplut3dtricubic", vs.m_PlaceboColorMapLut3dTricubic);
    XMLUtils::SetInt(settings, "placebocolormaplutsize", vs.m_PlaceboColorMapLutSize);
    XMLUtils::SetBoolean(settings, "placebocolormapshowclipping", vs.m_PlaceboColorMapShowClipping);
    XMLUtils::SetString(settings, "placebocolormapintent", getColorMapIntentDescriptionFromIndex(vs.m_PlaceboColorMapIntent));
    XMLUtils::SetBoolean(settings, "placebocolormapforcetonemappinglut", vs.m_PlaceboColorMapForceToneMappingLut);

    XMLUtils::SetBoolean(settings, "placebodeinterlaceenabled", vs.m_PlaceboDeinterlaceEnabled);
    XMLUtils::SetString(settings, "placebodeinterlacealgo", getDeinterlaceAlgoDescriptionFromIndex(vs.m_PlaceboDeinterlaceAlgo));
    XMLUtils::SetBoolean(settings, "placebodeinterlaceskipspatialcheck", vs.m_PlaceboDeinterlaceSkipSpatialCheck);

    XMLUtils::SetBoolean(settings, "placeboconeenabled", vs.m_PlaceboConeEnabled);
    XMLUtils::SetString(settings, "placeboconecones", getConeConesDescriptionFromIndex(vs.m_PlaceboConeCones));
    XMLUtils::SetFloat(settings, "placeboconestrength", vs.m_PlaceboConeStrength);
    XMLUtils::SetBoolean(settings, "placebosigmoidenabled", vs.m_PlaceboSigmoidEnabled);
    XMLUtils::SetFloat(settings, "placebosigmoidcenter", vs.m_PlaceboSigmoidCenter);
    XMLUtils::SetFloat(settings, "placebosigmoidslope", vs.m_PlaceboSigmoidSlope);

    XMLUtils::SetBoolean(settings, "placeboditherenabled", vs.m_PlaceboDitherEnabled);
    XMLUtils::SetString(settings, "placebodithermethod", getDitherMethodDescriptionFromIndex(vs.m_PlaceboDitherMethod));
    XMLUtils::SetInt(settings, "placeboditherlutsize", vs.m_PlaceboDitherLutSize);
    XMLUtils::SetBoolean(settings, "placebodithertemporal", vs.m_PlaceboDitherTemporal);
    XMLUtils::SetString(settings, "placebodithertransfer", getDitherTransferDescriptionFromIndex(vs.m_PlaceboDitherTransfer));

    XMLUtils::SetFloat(settings, "placebotoneconstantsexposure", vs.m_PlaceboToneConstantExposure);
    XMLUtils::SetFloat(settings, "placebotoneconstantskneeadaptation", vs.m_PlaceboToneConstantKneeAdaptation);
    XMLUtils::SetFloat(settings, "placebotoneconstantskneedefault", vs.m_PlaceboToneConstantKneeDefault);
    XMLUtils::SetFloat(settings, "placebotoneconstantskneemaximum", vs.m_PlaceboToneConstantKneeMaximum);
    XMLUtils::SetFloat(settings, "placebotoneconstantskneeminimum", vs.m_PlaceboToneConstantKneeMinimum);
    XMLUtils::SetFloat(settings, "placebotoneconstantskneeoffset", vs.m_PlaceboToneConstantKneeOffset);
    XMLUtils::SetFloat(settings, "placebotoneconstantslinearknee", vs.m_PlaceboToneConstantLinearKnee);
    XMLUtils::SetFloat(settings, "placebotoneconstantsreinhardcontrast", vs.m_PlaceboToneConstantReinhardContrast);
    XMLUtils::SetFloat(settings, "placebotoneconstantsslopeoffset", vs.m_PlaceboToneConstantSlopeOffset);
    XMLUtils::SetFloat(settings, "placebotoneconstantsslopetuning", vs.m_PlaceboToneConstantSlopeTuning);
    XMLUtils::SetFloat(settings, "placebotoneconstantssplinecontrast", vs.m_PlaceboToneConstantSplineContrast);

    XMLUtils::SetBoolean(settings, "placebotoneconstantscolormapvisualizelut", vs.m_PlaceboColorMapVisualizeLut);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolormapvisualizerectx0", vs.m_PlaceboColorMapVisualizeRectX0);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolormapvisualizerectx1", vs.m_PlaceboColorMapVisualizeRectX1);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolormapvisualizerecty0", vs.m_PlaceboColorMapVisualizeRectY0);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolormapvisualizerecty1", vs.m_PlaceboColorMapVisualizeRectY1);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolormapvisualizehue", vs.m_PlaceboColorMapVisualizeHue);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolormapvisualizetheta", vs.m_PlaceboColorMapVisualizeTheta);
    XMLUtils::SetFloat(settings, "placebotoneconstantscolorimetricgamma", vs.m_PlaceboGamutConstantsColorimetricGamma);
    XMLUtils::SetFloat(settings, "placebotoneconstantsperceptualdeadzone", vs.m_PlaceboGamutConstantsPerceptualDeadzone);
    XMLUtils::SetFloat(settings, "placebotoneconstantssoftclipdesat", vs.m_PlaceboGamutConstantsSoftclipDesat);
    XMLUtils::SetFloat(settings, "placebotoneconstantssoftclipknee", vs.m_PlaceboGamutConstantsSoftclipKnee);

    XMLUtils::SetFloat(settings, "placebotoantiringingstrength", vs.m_PlaceboAntiringingStrength);
    XMLUtils::SetBoolean(settings, "placebotocorrectsubpixeloffset", vs.m_PlaceboCorrectSubpixelOffset);
    XMLUtils::SetBoolean(settings, "placebotodisablebuiltinscalers", vs.m_PlaceboDisableBuiltinScalers);
    XMLUtils::SetBoolean(settings, "placebotodisabledithergammacorrection", vs.m_PlaceboDisableDitherGammaCorrection);
    XMLUtils::SetBoolean(settings, "placebotodisabledithergammacorrection", vs.m_PlaceboDisableDitherGammaCorrection);
    XMLUtils::SetBoolean(settings, "placebotodisabledithergammacorrection", vs.m_PlaceboDisableDitherGammaCorrection);

    XMLUtils::SetString(settings, "placebotoerrordiffusion", getDiffusionKernelDescriptionFromIndex(vs.m_PlaceboErrorDiffusion));
    XMLUtils::SetBoolean(settings, "placebotoforcedither", vs.m_PlaceboForceDither);
    XMLUtils::SetBoolean(settings, "placebotoforcelowbitdepthfbos", vs.m_PlaceboForceLowBitDepthFbos);
    XMLUtils::SetBoolean(settings, "placebotoignoreiccprofiles", vs.m_PlaceboIgnoreIccProfiles);
    XMLUtils::SetBoolean(settings, "placebotopreservemixingcache", vs.m_PlaceboPreserveMixingCache);
    XMLUtils::SetBoolean(settings, "placebotoskipantialiasing", vs.m_PlaceboSkipAntiAliasing);
    XMLUtils::SetBoolean(settings, "placebotoskipcachingsingleframe", vs.m_PlaceboSkipCachingSingleFrame);
}



void CGUIDialogVideoSettings::SaveLibplaceboSettings(const CVideoSettings &vs)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // prompt for a name
  std::string fileName;
  std::string filePath;
  if (!CGUIKeyboardFactory::ShowAndGetInput(fileName, CVariant{ CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(55323) },false) || fileName.empty())
  {
	return;
  }
  if (!URIUtils::HasExtension(fileName), ".xml")
    fileName += ".xml";
  filePath = URIUtils::AddFileToFolder("special://masterprofile/", fileName);
  //if(XFILE::CFile::Exists());

  SaveLibplaceboSettings(vs, fileName);
}


void CGUIDialogVideoSettings::LoadLibplaceboSettings(CVideoSettings& vs)
{
  std::string path;
  if (!CGUIDialogFileBrowser::ShowAndGetFile("special://masterprofile/", ".xml", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(55322), path))
  {
    return;
  }

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(path))
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: Error loading LipPlacebo settings {}, Line {}\n{}", path, xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return;
  }

  CLog::Log(LOGDEBUG, "CGUIDialogVideoSettings: loading LipPlacebo settings from {}", path);
  CGUIDialogVideoSettings::LoadLibplaceboSettings(vs, path);
}


bool CGUIDialogVideoSettings::LoadLibplaceboSettings(CVideoSettings& vs, std::string path)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(path))
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: Error loading LipPlacebo settings {}, Line {}\n{}", path, xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  CLog::Log(LOGDEBUG, "CGUIDialogVideoSettings: loading LipPlacebo settings from {}", path);
  const TiXmlElement* lpNode = xmlDoc.FirstChildElement("libPlaceboSettings");
  if (!lpNode)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: Error loading LipPlacebo settings, missing <libPlaceboSettings> element");
    return false;
  }

  XMLUtils::GetFloat(lpNode, "placebodisplaypeakluminance", vs.m_PlaceboDisplayPeakLuminance);
  XMLUtils::GetInt(lpNode, "placebotargetcolorspacehint", vs.m_PlaceboTargetColorspaceHint);
  XMLUtils::GetInt(lpNode, "placebotargetcolorspacehintmode", vs.m_PlaceboTargetColorspaceHintMode);
  std::string value;

  XMLUtils::GetBoolean(lpNode, "placebocoloradjustmentenabled", vs.m_PlaceboColorAdjustmentEnabled);
  XMLUtils::GetFloat(lpNode, "saturation", vs.m_PlaceboSaturation);
  XMLUtils::GetFloat(lpNode, "hue", vs.m_PlaceboHue);
  XMLUtils::GetFloat(lpNode, "temperature", vs.m_PlaceboTemperature);

  XMLUtils::GetBoolean(lpNode, "placebopeakdetectenabled", vs.m_PlaceboPeakDetectEnabled);
  XMLUtils::GetFloat(lpNode, "placebopeakdetectsmoothingperiod", vs.m_PlaceboPeakDetectSmoothingPeriod);
  XMLUtils::GetFloat(lpNode, "placebopeakdetectscenethresholdlow", vs.m_PlaceboPeakDetectSceneThresholdLow);
  XMLUtils::GetFloat(lpNode, "placebopeakdetectscenethresholdhigh", vs.m_PlaceboPeakDetectSceneThresholdHigh);
  XMLUtils::GetFloat(lpNode, "placebopeakdetectpercentile", vs.m_PlaceboPeakDetectPercentile);
  XMLUtils::GetFloat(lpNode, "placebopeakdetectblackcutoff", vs.m_PlaceboPeakDetectBlackCutoff);
  XMLUtils::GetBoolean(lpNode, "placebopeakdetectallowdelayed", vs.m_PlaceboPeakDetectAllowDelayed);
  XMLUtils::GetString(lpNode, "placeboupscaler", value);
  XMLUtils::GetString(lpNode, "placebodownscaler", value);
  XMLUtils::GetString(lpNode, "placeboplaneupscaler", value);
  XMLUtils::GetString(lpNode, "placeboplanedownscaler", value);
  XMLUtils::GetString(lpNode, "placeboframemixer", value);
  XMLUtils::GetBoolean(lpNode, "placebodebandenabled", vs.m_PlaceboDebandEnabled);
  XMLUtils::GetFloat(lpNode, "placebodebandgrain", vs.m_PlaceboDebandGrain);
  XMLUtils::GetFloat(lpNode, "placebodebandgrainneutral0", vs.m_PlaceboDebandGrainNeutral0);
  XMLUtils::GetFloat(lpNode, "placebodebandgrainneutral1", vs.m_PlaceboDebandGrainNeutral1);
  XMLUtils::GetFloat(lpNode, "placebodebandgrainneutral2", vs.m_PlaceboDebandGrainNeutral2);
  XMLUtils::GetInt(lpNode, "placebodebanditerations", vs.m_PlaceboDebandIterations);
  XMLUtils::GetFloat(lpNode, "placebodebandradius", vs.m_PlaceboDebandRadius);
  XMLUtils::GetFloat(lpNode, "placebodebandthreshold", vs.m_PlaceboDebandThreshold);
  XMLUtils::GetBoolean(lpNode, "placebocolormapenabled", vs.m_PlaceboColorMapEnabled);
  XMLUtils::GetFloat(lpNode, "placebocolormapcontrastrecovery", vs.m_PlaceboColorMapContrastRecovery);
  XMLUtils::GetFloat(lpNode, "placebocolormapcontrastsmoothness", vs.m_PlaceboColorMapContrastSmoothness);
  XMLUtils::GetBoolean(lpNode, "placebocolormapgamutexpansion", vs.m_PlaceboColorMapGamutExpansion);
  XMLUtils::GetString(lpNode, "placebocolormapgamutmapping", value);
  XMLUtils::GetString(lpNode, "placebocolormaptonemapping", value);
  XMLUtils::GetBoolean(lpNode, "placebocolormapinversetonemapping", vs.m_PlaceboColorMapInverseToneMapping);
  XMLUtils::GetInt(lpNode, "placebocolormaplut3dsizei", vs.m_PlaceboColorMapLut3dSizeI);
  XMLUtils::GetInt(lpNode, "placebocolormaplut3dsizec", vs.m_PlaceboColorMapLut3dSizeC);
  XMLUtils::GetInt(lpNode, "placebocolormaplut3dsizeh", vs.m_PlaceboColorMapLut3dSizeH);
  XMLUtils::GetBoolean(lpNode, "placebocolormaplut3dtricubic", vs.m_PlaceboColorMapLut3dTricubic);
  XMLUtils::GetInt(lpNode, "placebocolormaplutsize", vs.m_PlaceboColorMapLutSize);
  XMLUtils::GetBoolean(lpNode, "placebocolormapshowclipping", vs.m_PlaceboColorMapShowClipping);
  XMLUtils::GetString(lpNode, "placebocolormapintent", value);
  XMLUtils::GetBoolean(lpNode, "placebocolormapforcetonemappinglut", vs.m_PlaceboColorMapForceToneMappingLut);

  XMLUtils::GetBoolean(lpNode, "placebodeinterlaceenabled", vs.m_PlaceboDeinterlaceEnabled);
  XMLUtils::GetString(lpNode, "placebodeinterlacealgo", value);
  XMLUtils::GetBoolean(lpNode, "placebodeinterlaceskipspatialcheck", vs.m_PlaceboDeinterlaceSkipSpatialCheck);
  XMLUtils::GetBoolean(lpNode, "placebodesigmoidenabled", vs.m_PlaceboSigmoidEnabled);
  XMLUtils::GetFloat(lpNode, "placebosoigmoidcenter", vs.m_PlaceboSigmoidCenter);
  XMLUtils::GetFloat(lpNode, "placebosigmoidslope", vs.m_PlaceboSigmoidSlope);
  XMLUtils::GetBoolean(lpNode, "placeboconeenabled", vs.m_PlaceboConeEnabled);
  XMLUtils::GetString(lpNode, "placeboconecones", value);
  XMLUtils::GetFloat(lpNode, "placeboconestrength", vs.m_PlaceboConeStrength);

  XMLUtils::GetBoolean(lpNode, "placeboditherenabled", vs.m_PlaceboDitherEnabled);
  XMLUtils::GetString(lpNode, "placebodithermethod", value);
  XMLUtils::GetInt(lpNode, "placeboditherlutsize", vs.m_PlaceboDitherLutSize);
  XMLUtils::GetBoolean(lpNode, "placebodithertemporal", vs.m_PlaceboDitherTemporal);
  XMLUtils::GetString(lpNode, "placebodithertransfer", value);

  XMLUtils::GetFloat(lpNode, "placebotoneconstantsexposure", vs.m_PlaceboToneConstantExposure);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantskneeadaptation", vs.m_PlaceboToneConstantKneeAdaptation);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantskneedefault", vs.m_PlaceboToneConstantKneeDefault);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantskneemaximum", vs.m_PlaceboToneConstantKneeMaximum);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantskneeminimum", vs.m_PlaceboToneConstantKneeMinimum);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantskneeoffset", vs.m_PlaceboToneConstantKneeOffset);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantslinearknee", vs.m_PlaceboToneConstantLinearKnee);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantsreinhardcontrast", vs.m_PlaceboToneConstantReinhardContrast);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantsslopeoffset", vs.m_PlaceboToneConstantSlopeOffset);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantsslopetuning", vs.m_PlaceboToneConstantSlopeTuning);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantssplinecontrast", vs.m_PlaceboToneConstantSplineContrast);

  XMLUtils::GetBoolean(lpNode, "placebotoneconstantscolormapvisualizelut", vs.m_PlaceboColorMapVisualizeLut);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolormapvisualizerectx0", vs.m_PlaceboColorMapVisualizeRectX0);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolormapvisualizerectx1", vs.m_PlaceboColorMapVisualizeRectX1);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolormapvisualizerecty0", vs.m_PlaceboColorMapVisualizeRectY0);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolormapvisualizerecty1", vs.m_PlaceboColorMapVisualizeRectY1);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolormapvisualizehue", vs.m_PlaceboColorMapVisualizeHue);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolormapvisualizetheta", vs.m_PlaceboColorMapVisualizeTheta);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantscolorimetricgamma", vs.m_PlaceboGamutConstantsColorimetricGamma);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantsperceptualdeadzone", vs.m_PlaceboGamutConstantsPerceptualDeadzone);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantssoftclipdesat", vs.m_PlaceboGamutConstantsSoftclipDesat);
  XMLUtils::GetFloat(lpNode, "placebotoneconstantssoftclipknee", vs.m_PlaceboGamutConstantsSoftclipKnee);
  XMLUtils::GetFloat(lpNode, "placebotoantiringingstrength", vs.m_PlaceboAntiringingStrength);
  XMLUtils::GetBoolean(lpNode, "placebotocorrectsubpixeloffset", vs.m_PlaceboCorrectSubpixelOffset);
  XMLUtils::GetBoolean(lpNode, "placebotodisablebuiltinscalers", vs.m_PlaceboDisableBuiltinScalers);
  XMLUtils::GetBoolean(lpNode, "placebotodisabledithergammacorrection", vs.m_PlaceboDisableDitherGammaCorrection);
  XMLUtils::GetBoolean(lpNode, "placebotodisablelinearscaling", vs.m_PlaceboDisableLinearScaling);
  XMLUtils::GetBoolean(lpNode, "placebotodynamicconstant", vs.m_PlaceboDynamicConstant);
  XMLUtils::GetString(lpNode, "placebotoerrordiffusion", value);
  XMLUtils::GetBoolean(lpNode, "placebotoforcedither", vs.m_PlaceboForceDither);
  XMLUtils::GetBoolean(lpNode, "placebotoforcelowbitdepthfbos", vs.m_PlaceboForceLowBitDepthFbos);
  XMLUtils::GetBoolean(lpNode, "placebotoignoreiccprofiles", vs.m_PlaceboIgnoreIccProfiles);
  XMLUtils::GetBoolean(lpNode, "placebotopreservemixingcache", vs.m_PlaceboPreserveMixingCache);
  XMLUtils::GetBoolean(lpNode, "placebotoskipantialiasing", vs.m_PlaceboSkipAntiAliasing);
  XMLUtils::GetBoolean(lpNode, "placebotoskipcachingsingleframe", vs.m_PlaceboSkipCachingSingleFrame);

  UpdateLibPLaceboParamsFromVideoSettings(vs);
  return true;
}


bool CGUIDialogVideoSettings::LoadLibplaceboSettings(CVideoSettings& vs, const TiXmlNode* settings)
{
  if (!settings)
    return false;

  //std::unique_lock lock(m_critical);
  const TiXmlElement* pElement = settings->FirstChildElement("defaultvideosettings");
  if (pElement)
  {
  XMLUtils::GetFloat(pElement, "placebodisplaypeakluminance", vs.m_PlaceboDisplayPeakLuminance);
  XMLUtils::GetInt(pElement, "placebotargetcolorspacehint", vs.m_PlaceboTargetColorspaceHint);
  XMLUtils::GetInt(pElement, "placebotargetcolorspacehintmode", vs.m_PlaceboTargetColorspaceHintMode);
  std::string value;

  XMLUtils::GetBoolean(pElement, "placebocoloradjustmentenabled", vs.m_PlaceboColorAdjustmentEnabled);
  XMLUtils::GetFloat(pElement, "saturation", vs.m_PlaceboSaturation);
  XMLUtils::GetFloat(pElement, "hue", vs.m_PlaceboHue);
  XMLUtils::GetFloat(pElement, "temperature", vs.m_PlaceboTemperature);

  XMLUtils::GetBoolean(pElement, "placebopeakdetectenabled", vs.m_PlaceboPeakDetectEnabled);
  XMLUtils::GetFloat(pElement, "placebopeakdetectsmoothingperiod", vs.m_PlaceboPeakDetectSmoothingPeriod);
  XMLUtils::GetFloat(pElement, "placebopeakdetectscenethresholdlow", vs.m_PlaceboPeakDetectSceneThresholdLow);
  XMLUtils::GetFloat(pElement, "placebopeakdetectscenethresholdhigh", vs.m_PlaceboPeakDetectSceneThresholdHigh);
  XMLUtils::GetFloat(pElement, "placebopeakdetectpercentile", vs.m_PlaceboPeakDetectPercentile);
  XMLUtils::GetFloat(pElement, "placebopeakdetectblackcutoff", vs.m_PlaceboPeakDetectBlackCutoff);
  XMLUtils::GetBoolean(pElement, "placebopeakdetectallowdelayed", vs.m_PlaceboPeakDetectAllowDelayed);
  XMLUtils::GetString(pElement, "placeboupscaler", value);
  XMLUtils::GetString(pElement, "placebodownscaler", value);
  XMLUtils::GetString(pElement, "placeboplaneupscaler", value);
  XMLUtils::GetString(pElement, "placeboplanedownscaler", value);
  XMLUtils::GetString(pElement, "placeboframemixer", value);
  XMLUtils::GetBoolean(pElement, "placebodebandenabled", vs.m_PlaceboDebandEnabled);
  XMLUtils::GetFloat(pElement, "placebodebandgrain", vs.m_PlaceboDebandGrain);
  XMLUtils::GetFloat(pElement, "placebodebandgrainneutral0", vs.m_PlaceboDebandGrainNeutral0);
  XMLUtils::GetFloat(pElement, "placebodebandgrainneutral1", vs.m_PlaceboDebandGrainNeutral1);
  XMLUtils::GetFloat(pElement, "placebodebandgrainneutral2", vs.m_PlaceboDebandGrainNeutral2);
  XMLUtils::GetInt(pElement, "placebodebanditerations", vs.m_PlaceboDebandIterations);
  XMLUtils::GetFloat(pElement, "placebodebandradius", vs.m_PlaceboDebandRadius);
  XMLUtils::GetFloat(pElement, "placebodebandthreshold", vs.m_PlaceboDebandThreshold);
  XMLUtils::GetBoolean(pElement, "placebocolormapenabled", vs.m_PlaceboColorMapEnabled);
  XMLUtils::GetFloat(pElement, "placebocolormapcontrastrecovery", vs.m_PlaceboColorMapContrastRecovery);
  XMLUtils::GetFloat(pElement, "placebocolormapcontrastsmoothness", vs.m_PlaceboColorMapContrastSmoothness);
  XMLUtils::GetBoolean(pElement, "placebocolormapgamutexpansion", vs.m_PlaceboColorMapGamutExpansion);
  XMLUtils::GetString(pElement, "placebocolormapgamutmapping", value);
  XMLUtils::GetString(pElement, "placebocolormaptonemapping", value);
  XMLUtils::GetBoolean(pElement, "placebocolormapinversetonemapping", vs.m_PlaceboColorMapInverseToneMapping);
  XMLUtils::GetInt(pElement, "placebocolormaplut3dsizei", vs.m_PlaceboColorMapLut3dSizeI);
  XMLUtils::GetInt(pElement, "placebocolormaplut3dsizec", vs.m_PlaceboColorMapLut3dSizeC);
  XMLUtils::GetInt(pElement, "placebocolormaplut3dsizeh", vs.m_PlaceboColorMapLut3dSizeH);
  XMLUtils::GetBoolean(pElement, "placebocolormaplut3dtricubic", vs.m_PlaceboColorMapLut3dTricubic);
  XMLUtils::GetInt(pElement, "placebocolormaplutsize", vs.m_PlaceboColorMapLutSize);
  XMLUtils::GetBoolean(pElement, "placebocolormapshowclipping", vs.m_PlaceboColorMapShowClipping);
  XMLUtils::GetString(pElement, "placebocolormapintent", value);
  XMLUtils::GetBoolean(pElement, "placebocolormapforcetonemappinglut", vs.m_PlaceboColorMapForceToneMappingLut);

  XMLUtils::GetBoolean(pElement, "placebodeinterlaceenabled", vs.m_PlaceboDeinterlaceEnabled);
  XMLUtils::GetString(pElement, "placebodeinterlacealgo", value);
  XMLUtils::GetBoolean(pElement, "placebodeinterlaceskipspatialcheck", vs.m_PlaceboDeinterlaceSkipSpatialCheck);
  XMLUtils::GetBoolean(pElement, "placebodesigmoidenabled", vs.m_PlaceboSigmoidEnabled);
  XMLUtils::GetFloat(pElement, "placebosoigmoidcenter", vs.m_PlaceboSigmoidCenter);
  XMLUtils::GetFloat(pElement, "placebosigmoidslope", vs.m_PlaceboSigmoidSlope);
  XMLUtils::GetBoolean(pElement, "placeboconeenabled", vs.m_PlaceboConeEnabled);
  XMLUtils::GetString(pElement, "placeboconecones", value);
  XMLUtils::GetFloat(pElement, "placeboconestrength", vs.m_PlaceboConeStrength);

  XMLUtils::GetBoolean(pElement, "placeboditherenabled", vs.m_PlaceboDitherEnabled);
  XMLUtils::GetString(pElement, "placebodithermethod", value);
  XMLUtils::GetInt(pElement, "placeboditherlutsize", vs.m_PlaceboDitherLutSize);
  XMLUtils::GetBoolean(pElement, "placebodithertemporal", vs.m_PlaceboDitherTemporal);
  XMLUtils::GetString(pElement, "placebodithertransfer", value);

  XMLUtils::GetFloat(pElement, "placebotoneconstantsexposure", vs.m_PlaceboToneConstantExposure);
  XMLUtils::GetFloat(pElement, "placebotoneconstantskneeadaptation", vs.m_PlaceboToneConstantKneeAdaptation);
  XMLUtils::GetFloat(pElement, "placebotoneconstantskneedefault", vs.m_PlaceboToneConstantKneeDefault);
  XMLUtils::GetFloat(pElement, "placebotoneconstantskneemaximum", vs.m_PlaceboToneConstantKneeMaximum);
  XMLUtils::GetFloat(pElement, "placebotoneconstantskneeminimum", vs.m_PlaceboToneConstantKneeMinimum);
  XMLUtils::GetFloat(pElement, "placebotoneconstantskneeoffset", vs.m_PlaceboToneConstantKneeOffset);
  XMLUtils::GetFloat(pElement, "placebotoneconstantslinearknee", vs.m_PlaceboToneConstantLinearKnee);
  XMLUtils::GetFloat(pElement, "placebotoneconstantsreinhardcontrast", vs.m_PlaceboToneConstantReinhardContrast);
  XMLUtils::GetFloat(pElement, "placebotoneconstantsslopeoffset", vs.m_PlaceboToneConstantSlopeOffset);
  XMLUtils::GetFloat(pElement, "placebotoneconstantsslopetuning", vs.m_PlaceboToneConstantSlopeTuning);
  XMLUtils::GetFloat(pElement, "placebotoneconstantssplinecontrast", vs.m_PlaceboToneConstantSplineContrast);

  XMLUtils::GetBoolean(pElement, "placebotoneconstantscolormapvisualizelut", vs.m_PlaceboColorMapVisualizeLut);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolormapvisualizerectx0", vs.m_PlaceboColorMapVisualizeRectX0);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolormapvisualizerectx1", vs.m_PlaceboColorMapVisualizeRectX1);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolormapvisualizerecty0", vs.m_PlaceboColorMapVisualizeRectY0);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolormapvisualizerecty1", vs.m_PlaceboColorMapVisualizeRectY1);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolormapvisualizehue", vs.m_PlaceboColorMapVisualizeHue);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolormapvisualizetheta", vs.m_PlaceboColorMapVisualizeTheta);
  XMLUtils::GetFloat(pElement, "placebotoneconstantscolorimetricgamma", vs.m_PlaceboGamutConstantsColorimetricGamma);
  XMLUtils::GetFloat(pElement, "placebotoneconstantsperceptualdeadzone", vs.m_PlaceboGamutConstantsPerceptualDeadzone);
  XMLUtils::GetFloat(pElement, "placebotoneconstantssoftclipdesat", vs.m_PlaceboGamutConstantsSoftclipDesat);
  XMLUtils::GetFloat(pElement, "placebotoneconstantssoftclipknee", vs.m_PlaceboGamutConstantsSoftclipKnee);
  XMLUtils::GetFloat(pElement, "placebotoantiringingstrength", vs.m_PlaceboAntiringingStrength);
  XMLUtils::GetBoolean(pElement, "placebotocorrectsubpixeloffset", vs.m_PlaceboCorrectSubpixelOffset);
  XMLUtils::GetBoolean(pElement, "placebotodisablebuiltinscalers", vs.m_PlaceboDisableBuiltinScalers);
  XMLUtils::GetBoolean(pElement, "placebotodisabledithergammacorrection", vs.m_PlaceboDisableDitherGammaCorrection);
  XMLUtils::GetBoolean(pElement, "placebotodisablelinearscaling", vs.m_PlaceboDisableLinearScaling);
  XMLUtils::GetBoolean(pElement, "placebotodynamicconstant", vs.m_PlaceboDynamicConstant);
  XMLUtils::GetString(pElement, "placebotoerrordiffusion", value);
  XMLUtils::GetBoolean(pElement, "placebotoforcedither", vs.m_PlaceboForceDither);
  XMLUtils::GetBoolean(pElement, "placebotoforcelowbitdepthfbos", vs.m_PlaceboForceLowBitDepthFbos);
  XMLUtils::GetBoolean(pElement, "placebotoignoreiccprofiles", vs.m_PlaceboIgnoreIccProfiles);
  XMLUtils::GetBoolean(pElement, "placebotopreservemixingcache", vs.m_PlaceboPreserveMixingCache);
  XMLUtils::GetBoolean(pElement, "placebotoskipantialiasing", vs.m_PlaceboSkipAntiAliasing);
  XMLUtils::GetBoolean(pElement, "placebotoskipcachingsingleframe", vs.m_PlaceboSkipCachingSingleFrame);

  UpdateLibPLaceboParamsFromVideoSettings(vs);
  return true;
  }
}

void CGUIDialogVideoSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  CVideoSettings vs = appPlayer->GetVideoSettings();
  pl_options m_placeboOptions = vs.m_placeboOptions->getPlOptions();
  if (settingId == SETTING_VIDEO_CALIBRATION)
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    auto settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return;

    auto settings = settingsComponent->GetSettings();
    if (!settings)
      return;

    auto calibsetting = settings->GetSetting(CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
    if (!calibsetting)
    {
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
      return;
    }

    // launch calibration window
    if (profileManager->GetMasterProfile().getLockMode() != LockMode::EVERYONE &&
        g_passwordManager.CheckSettingLevelLock(calibsetting->GetLevel()))
      return;

    CServiceBroker::GetGUI()->GetWindowManager().ForceActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  //! @todo implement
  else if (settingId == SETTING_VIDEO_MAKE_DEFAULT)
  {
    Save();
  }
  else if (settingId == SETTING_LIB_PLACEBO_LOAD_PRESET_DEFAULT)
  {
    vs.m_placeboOptions->resetPlOptions(PlOptionsWrapper::DEFAULT);
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_LOAD_PRESET_FAST)
  {
    vs.m_placeboOptions->resetPlOptions(PlOptionsWrapper::FAST);
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_LOAD_PRESET_HIGH_QUALITY)
  {
    vs.m_placeboOptions->resetPlOptions(PlOptionsWrapper::HIGH_QUALITY);
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();

  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_LOAD_PRESET_DEFAULT)
  {
    m_placeboOptions->peak_detect_params = pl_peak_detect_default_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_PEAK_DETECT_LOAD_PRESET_HIGH_QUALITY)
  {
    m_placeboOptions->peak_detect_params = pl_peak_detect_high_quality_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_DEBAND_LOAD_PRESET)
  {
    m_placeboOptions->deband_params = pl_deband_default_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LOAD_PRESET_DEFAULT)
  {
    m_placeboOptions->color_map_params = pl_color_map_default_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_COLOR_MAP_LOAD_PRESET_HIGH_QUALITY)
  {
    m_placeboOptions->color_map_params = pl_color_map_high_quality_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_SIGMOID_LOAD_PRESET_DEFAULT)
  {
    m_placeboOptions->sigmoid_params = pl_sigmoid_default_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_DITHER_LOAD_PRESET_DEFAULT)
  {
    m_placeboOptions->dither_params = pl_dither_default_params;
    UpdateVideoSettingsFromLibPLaceboParams(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_LOAD_FROM_FILE)
  {
	vs.m_placeboOptions->resetPlOptions(PlOptionsWrapper::DEFAULT); //make sure all options are set, even if not present in the file
    LoadLibplaceboSettings(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
  else if (settingId == SETTING_LIB_PLACEBO_SAVE_TO_FILE)
  {
    SaveLibplaceboSettings(vs);
    appPlayer->SetVideoSettings(vs);
    SetupView();
  }
}

bool CGUIDialogVideoSettings::Save()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (profileManager->GetMasterProfile().getLockMode() != LockMode::EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevel::Expert))
    return true;

  // prompt user if they are sure
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant(12376), CVariant(12377)))
  { // reset the settings
    CVideoDatabase db;
    if (!db.Open())
      return true;
    db.EraseAllVideoSettings();
    db.Close();

    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();

    CMediaSettings::GetInstance().GetDefaultVideoSettings() = appPlayer->GetVideoSettings();
    CMediaSettings::GetInstance().GetDefaultVideoSettings().m_SubtitleStream = -1;
    CMediaSettings::GetInstance().GetDefaultVideoSettings().m_AudioStream = -1;
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }

  return true;
}

void CGUIDialogVideoSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(13395);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

//-------------------------------------------------------
// 
// 
// 
//-------------------------------------------------------
void CGUIDialogVideoSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("videosettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CreateGroup(groupVideoStream, category);
  CreateGroup(groupVideo, category);
  CreateGroup(groupStereoscopic, category);
  CreateGroup(groupSaveAsDefault, category);
  CreateGroup(groupLpFile, category);
  CreateGroup(groupLpReset, category);
  CreateGroup(groupOptions, category);
  CreateGroup(groupColorAjustment, category);
  CreateGroup(groupPeakDetect, category);
  CreateGroup(groupColorMap, category);
  CreateGroup(groupToneMappingConstants, category);
  CreateGroup(groupGamutMappingConstants, category);
  CreateGroup(groupScaler, category);
  CreateGroup(groupDeband, category);
  CreateGroup(groupSigmoid, category);
  CreateGroup(groupDither, category);
  CreateGroup(groupCone, category);
  CreateGroup(groupDeinterlace, category);
  CreateGroup(groupMisc, category);



  auto skin = CServiceBroker::GetGUI()->GetSkinInfo();
  const bool usePopup = skin && skin->HasSkinFile("DialogSlider.xml");

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const CVideoSettings videoSettings = appPlayer->GetVideoSettings();


  TranslatableIntegerSettingOptions entries;

    // cl not sure how to handle interlacing...
    if (appPlayer->Supports(RENDERFEATURE_LIBPLACEBO))
    {
    }
    else
    {
      entries.clear();
      entries.emplace_back(16039, VS_INTERLACEMETHOD_NONE);
      entries.emplace_back(16019, VS_INTERLACEMETHOD_AUTO);
      entries.emplace_back(20131, VS_INTERLACEMETHOD_RENDER_BLEND);
      entries.emplace_back(20129, VS_INTERLACEMETHOD_RENDER_WEAVE);
      entries.emplace_back(16021, VS_INTERLACEMETHOD_RENDER_BOB);
      entries.emplace_back(16020, VS_INTERLACEMETHOD_DEINTERLACE);
      entries.emplace_back(16036, VS_INTERLACEMETHOD_DEINTERLACE_HALF);
      entries.emplace_back(16311, VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL);
      entries.emplace_back(16310, VS_INTERLACEMETHOD_VDPAU_TEMPORAL);
      entries.emplace_back(16325, VS_INTERLACEMETHOD_VDPAU_BOB);
      entries.emplace_back(16318, VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF);
      entries.emplace_back(16317, VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF);
      entries.emplace_back(16327, VS_INTERLACEMETHOD_VAAPI_BOB);
      entries.emplace_back(16328, VS_INTERLACEMETHOD_VAAPI_MADI);
      entries.emplace_back(16329, VS_INTERLACEMETHOD_VAAPI_MACI);
      entries.emplace_back(16320, VS_INTERLACEMETHOD_DXVA_AUTO);

      /* remove unsupported methods */
      for (TranslatableIntegerSettingOptions::iterator it = entries.begin(); it != entries.end(); )
      {
        if (appPlayer->Supports(static_cast<EINTERLACEMETHOD>(it->value)))
          ++it;
        else
          it = entries.erase(it);
      }
      if (!entries.empty())
      {
        EINTERLACEMETHOD method = videoSettings.m_InterlaceMethod;
        if (!appPlayer->Supports(method))
        {
          method = appPlayer->GetDeinterlacingMethodDefault();
        }
        AddSpinner(groupVideo, SETTING_VIDEO_INTERLACEMETHOD, 16038, SettingLevel::Basic, static_cast<int>(method), entries);
    }

    entries.clear();
    entries.emplace_back(16301, VS_SCALINGMETHOD_NEAREST);
    entries.emplace_back(16302, VS_SCALINGMETHOD_LINEAR);
    entries.emplace_back(16303, VS_SCALINGMETHOD_CUBIC_B_SPLINE);
    entries.emplace_back(16314, VS_SCALINGMETHOD_CUBIC_MITCHELL);
    entries.emplace_back(16321, VS_SCALINGMETHOD_CUBIC_CATMULL);
    entries.emplace_back(16326, VS_SCALINGMETHOD_CUBIC_0_075);
    entries.emplace_back(16330, VS_SCALINGMETHOD_CUBIC_0_1);
    entries.emplace_back(16304, VS_SCALINGMETHOD_LANCZOS2);
    entries.emplace_back(16323, VS_SCALINGMETHOD_SPLINE36_FAST);
    entries.emplace_back(16315, VS_SCALINGMETHOD_LANCZOS3_FAST);
    entries.emplace_back(16322, VS_SCALINGMETHOD_SPLINE36);
    entries.emplace_back(16305, VS_SCALINGMETHOD_LANCZOS3);
    entries.emplace_back(16306, VS_SCALINGMETHOD_SINC8);
    entries.emplace_back(16307, VS_SCALINGMETHOD_BICUBIC_SOFTWARE);
    entries.emplace_back(16308, VS_SCALINGMETHOD_LANCZOS_SOFTWARE);
    entries.emplace_back(16309, VS_SCALINGMETHOD_SINC_SOFTWARE);
    entries.emplace_back(13120, VS_SCALINGMETHOD_VDPAU_HARDWARE);
    entries.emplace_back(16319, VS_SCALINGMETHOD_DXVA_HARDWARE);
    entries.emplace_back(16316, VS_SCALINGMETHOD_AUTO);

    /* remove unsupported methods */
    for(TranslatableIntegerSettingOptions::iterator it = entries.begin(); it != entries.end(); )
    {
      if (appPlayer->Supports(static_cast<ESCALINGMETHOD>(it->value)))
        ++it;
      else
        it = entries.erase(it);
    }
    AddSpinner(groupVideo, SETTING_VIDEO_SCALINGMETHOD, 16300, SettingLevel::Basic, static_cast<int>(videoSettings.m_ScalingMethod), entries);
  }


  AddVideoStreams(groupVideoStream, SETTING_VIDEO_STREAM);

  if (appPlayer->Supports(RENDERFEATURE_STRETCH) || appPlayer->Supports(RENDERFEATURE_PIXEL_RATIO))
  {
    AddList(groupVideo, SETTING_VIDEO_VIEW_MODE, 629, SettingLevel::Basic, videoSettings.m_ViewMode, CViewModeSettings::ViewModesFiller, 629);
  }
  if (appPlayer->Supports(RENDERFEATURE_ZOOM))
    AddSlider(groupVideo, SETTING_VIDEO_ZOOM, 216, SettingLevel::Basic,
              videoSettings.m_CustomZoomAmount, "{:2.2f}", 0.5f, 0.01f, 2.0f, 216, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_VERTICAL_SHIFT))
    AddSlider(groupVideo, SETTING_VIDEO_VERTICAL_SHIFT, 225, SettingLevel::Basic,
              videoSettings.m_CustomVerticalShift, "{:2.2f}", -2.0f, 0.01f, 2.0f, 225, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_PIXEL_RATIO))
    AddSlider(groupVideo, SETTING_VIDEO_PIXEL_RATIO, 217, SettingLevel::Basic,
              videoSettings.m_CustomPixelRatio, "{:2.2f}", 0.5f, 0.01f, 2.0f, 217, usePopup);

  AddList(groupVideo, SETTING_VIDEO_ORIENTATION, 21843, SettingLevel::Basic, videoSettings.m_Orientation, CGUIDialogVideoSettings::VideoOrientationFiller, 21843);

    if (appPlayer->Supports(RENDERFEATURE_POSTPROCESS))    AddToggle(groupVideo, SETTING_VIDEO_POSTPROCESS, 16400, SettingLevel::Basic, videoSettings.m_PostProcess);
    if (!appPlayer->Supports(RENDERFEATURE_LIBPLACEBO))
    {
      if (appPlayer->Supports(RENDERFEATURE_BRIGHTNESS))     AddPercentageSlider(groupVideo, SETTING_VIDEO_BRIGHTNESS, 464, SettingLevel::Basic, static_cast<int>(videoSettings.m_Brightness), 14047, 1, 464, usePopup);
      if (appPlayer->Supports(RENDERFEATURE_CONTRAST))       AddPercentageSlider(groupVideo, SETTING_VIDEO_CONTRAST, 465, SettingLevel::Basic, static_cast<int>(videoSettings.m_Contrast), 14047, 1, 465, usePopup);
      if (appPlayer->Supports(RENDERFEATURE_GAMMA))          AddPercentageSlider(groupVideo, SETTING_VIDEO_GAMMA, 466, SettingLevel::Basic, static_cast<int>(videoSettings.m_Gamma), 14047, 1, 466, usePopup);
    }
    if (appPlayer->Supports(RENDERFEATURE_NOISE))          AddSlider(groupVideo, SETTING_VIDEO_VDPAU_NOISE, 16312, SettingLevel::Basic,              videoSettings.m_NoiseReduction, "{:2.2f}", 0.0f, 0.01f, 1.0f, 16312, usePopup);  
    if (appPlayer->Supports(RENDERFEATURE_SHARPNESS))      AddSlider(groupVideo, SETTING_VIDEO_VDPAU_SHARPNESS, 16313, SettingLevel::Basic, videoSettings.m_Sharpness, "{:2.2f}", -1.0f, 0.02f, 1.0f, 16313, usePopup);  
    if (appPlayer->Supports(RENDERFEATURE_NONLINSTRETCH))  AddToggle(groupVideo, SETTING_VIDEO_NONLIN_STRETCH, 659, SettingLevel::Basic, videoSettings.m_CustomNonLinStretch);

  // tone mapping
  if (appPlayer->Supports(RENDERFEATURE_TONEMAP))
  {
    const bool visible = !CServiceBroker::GetWinSystem()->IsHDRDisplaySettingEnabled();
    entries.clear();
    entries.emplace_back(36554, VS_TONEMAPMETHOD_OFF);
    entries.emplace_back(36555, VS_TONEMAPMETHOD_REINHARD);
    entries.emplace_back(36557, VS_TONEMAPMETHOD_ACES);
    entries.emplace_back(36558, VS_TONEMAPMETHOD_HABLE);

    AddSpinner(groupVideo, SETTING_VIDEO_TONEMAP_METHOD, 36553, SettingLevel::Basic,
               videoSettings.m_ToneMapMethod, entries, false, visible);
    AddSlider(groupVideo, SETTING_VIDEO_TONEMAP_PARAM, 36556, SettingLevel::Basic,
              videoSettings.m_ToneMapParam, "{:2.2f}", 0.1f, 0.1f, 5.0f, 36556, usePopup, false,
              visible);
  }

  // stereoscopic settings
  entries.clear();
  entries.emplace_back(16316, static_cast<int>(RenderStereoMode::OFF));
  entries.emplace_back(36503, static_cast<int>(RenderStereoMode::SPLIT_HORIZONTAL));
  entries.emplace_back(36504, static_cast<int>(RenderStereoMode::SPLIT_VERTICAL));
  AddSpinner(groupStereoscopic, SETTING_VIDEO_STEREOSCOPICMODE, 36535, SettingLevel::Basic, videoSettings.m_StereoMode, entries);
  AddToggle(groupStereoscopic, SETTING_VIDEO_STEREOSCOPICINVERT, 36536, SettingLevel::Basic, videoSettings.m_StereoInvert);

  // general settings
  AddButton(groupSaveAsDefault, SETTING_VIDEO_MAKE_DEFAULT, 12376, SettingLevel::Basic);
  AddButton(groupSaveAsDefault, SETTING_VIDEO_CALIBRATION, 214, SettingLevel::Basic);

  // libplacebo settings
  if (appPlayer->Supports(RENDERFEATURE_LIBPLACEBO))
  {
    AddButton(groupLpFile, SETTING_LIB_PLACEBO_SAVE_TO_FILE, 55323, SettingLevel::Basic);
    AddButton(groupLpFile, SETTING_LIB_PLACEBO_LOAD_FROM_FILE,   55322, SettingLevel::Basic);

    AddButton(groupLpReset, SETTING_LIB_PLACEBO_LOAD_PRESET_DEFAULT,      55231, SettingLevel::Basic);
    AddButton(groupLpReset, SETTING_LIB_PLACEBO_LOAD_PRESET_FAST,         55232, SettingLevel::Basic);
    AddButton(groupLpReset, SETTING_LIB_PLACEBO_LOAD_PRESET_HIGH_QUALITY, 55233, SettingLevel::Basic);

    // Render Options
    AddSlider(groupOptions, SETTING_LIB_PLACEBO_DISPLAY_PEAK_LUMINANCE, 55313, SettingLevel::Basic, videoSettings.m_PlaceboDisplayPeakLuminance, "{0:5.0f}", (float)1.0, (float)1, (float)10000.0, 55313, usePopup);
    entries.clear();
    entries.emplace_back(55315, static_cast<int>(SettinglibPlaceboTargetColorspaceHint::AUTO));
    entries.emplace_back(55316, static_cast<int>(SettinglibPlaceboTargetColorspaceHint::NO));
    entries.emplace_back(55317, static_cast<int>(SettinglibPlaceboTargetColorspaceHint::YES));
    AddSpinner(groupOptions, SETTING_LIB_PLACEBO_TARGET_COLORSPACE_HINT, 55314, SettingLevel::Basic, videoSettings.m_PlaceboTargetColorspaceHint, entries);
    entries.clear();
    entries.emplace_back(55319, static_cast<int>(SettinglibPlaceboTargetColorspaceHintMode::TARGET));
    entries.emplace_back(55320, static_cast<int>(SettinglibPlaceboTargetColorspaceHintMode::SOURCE));
    entries.emplace_back(55321, static_cast<int>(SettinglibPlaceboTargetColorspaceHintMode::SOURCE_DYNAMIC));
    AddSpinner(groupOptions, SETTING_LIB_PLACEBO_TARGET_COLORSPACE_HINT_MODE, 55318, SettingLevel::Basic, videoSettings.m_PlaceboTargetColorspaceHintMode, entries);
    
    // Color_Adjustment
    AddToggle(groupColorAjustment, SETTING_LIB_PLACEBO_COLOR_ADJUSTMENT_ENABLED, 55221, SettingLevel::Basic, videoSettings.m_PlaceboColorAdjustmentEnabled);
    AddPercentageSlider(groupColorAjustment, SETTING_VIDEO_BRIGHTNESS, 464, SettingLevel::Basic, static_cast<int>(videoSettings.m_Brightness), 14047, 1, 464, usePopup);
    AddPercentageSlider(groupColorAjustment, SETTING_VIDEO_CONTRAST, 465, SettingLevel::Basic, static_cast<int>(videoSettings.m_Contrast), 14047, 1, 465, usePopup);
    AddPercentageSlider(groupColorAjustment, SETTING_VIDEO_GAMMA, 466, SettingLevel::Basic, static_cast<int>(videoSettings.m_Gamma), 14047, 1, 466, usePopup);
    AddSlider          (groupColorAjustment, SETTING_LIB_PLACEBO_HUE,                      55222, SettingLevel::Basic, videoSettings.m_PlaceboHue,        "{0:3.0f}", (float)0.0, (float)1.0, (float)360.0, 55222, usePopup);
    AddSlider          (groupColorAjustment, SETTING_LIB_PLACEBO_SATURATION,               55210, SettingLevel::Basic, videoSettings.m_PlaceboSaturation, "{0:2.2f}", (float)0.0, (float)0.01, (float)100.0, 55210, usePopup);
    AddSlider          (groupColorAjustment, SETTING_LIB_PLACEBO_TEMPERATURE,              55212, SettingLevel::Basic, videoSettings.m_PlaceboTemperature, "{0:6.0f}K", (float)1700, (float)10.0, (float)10000, 55212, usePopup);

    // Peak_Detection
    // cl parameters range conversion..., not just this group
    AddToggle(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_ENABLED, 55213, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectEnabled);
    AddButton(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_LOAD_PRESET_DEFAULT, 55234, SettingLevel::Basic);
    AddButton(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_LOAD_PRESET_HIGH_QUALITY, 55235, SettingLevel::Basic);
    AddSlider(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SMOOTHING_PERIOD,                  55214, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectSmoothingPeriod,    "{0:4.0f}", (float)0.0,  (float)1.0, (float)1000.0, 55214, usePopup);
    AddSlider(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SCENE_THRESHOLD_LOW,               55215, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectSceneThresholdLow,  "{0:5.1f}", (float)0.0,  (float)0.1, (float)100.0,  55215, usePopup);
    AddSlider(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_SCENE_THRESHOLD_HIGH,              55216, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectSceneThresholdHigh, "{0:5.1f}", (float)0.0,  (float)0.1, (float)100.0,  55216, usePopup);
    AddSlider(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_PERCENTILE,                        55218, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectPercentile,         "{0:6.3f}", (float)95.0, (float)0.001, (float)100.0,  55218, usePopup);
    AddSlider(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_BLACK_CUTOFF,                      55219, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectBlackCutoff,        "{0:5.1f}", (float)0.0,  (float)0.1, (float)100.0,  55219, usePopup);
    // deprecated AddToggle(groupPeakDetect, SETTING_LIB_PLACEBO_PEAK_DETECT_PARAMS_ALLOW_DELAYED,                     55220, SettingLevel::Basic, videoSettings.m_PlaceboPeakDetectAllowDelayed);

    // Scalers
    AddList(groupScaler, SETTING_LIB_PLACEBO_UPSCALER,         55223, SettingLevel::Basic, videoSettings.m_PlaceboUpscaler, PlUpscalerOptionFiller, 55223);
    AddList(groupScaler, SETTING_LIB_PLACEBO_DOWNSCALER,       55224, SettingLevel::Basic, videoSettings.m_PlaceboDownscaler, PlDownscalerOptionFiller, 55224);
    AddList(groupScaler, SETTING_LIB_PLACEBO_PLANE_UPSCALER,   55225, SettingLevel::Basic, videoSettings.m_PlaceboPlaneUpscaler, PlUpscalerOptionFiller, 55225);
    AddList(groupScaler, SETTING_LIB_PLACEBO_PLANE_DOWNSCALER, 55226, SettingLevel::Basic, videoSettings.m_PlaceboPlaneDownscaler, PlDownscalerOptionFiller, 55226);
    AddList(groupScaler, SETTING_LIB_PLACEBO_FRAME_MIXER,      55227, SettingLevel::Basic, videoSettings.m_PlaceboFrameMixer, PlFrameMixerOptionFiller, 55227);


    // Color map
    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_ENABLED,                  55248, SettingLevel::Basic, videoSettings.m_PlaceboColorMapEnabled);
    AddButton(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LOAD_PRESET_DEFAULT,      55246, SettingLevel::Basic);
    AddButton(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LOAD_PRESET_HIGH_QUALITY, 55247, SettingLevel::Basic);
    AddList(groupColorMap,     SETTING_LIB_PLACEBO_COLOR_MAP_GAMUT_MAPPING,            55228, SettingLevel::Basic, videoSettings.m_PlaceboColorMapGamutMapping, PlColorMapGamutMapFunctionOptionFiller, 55228);
    AddList(groupColorMap,     SETTING_LIB_PLACEBO_COLOR_MAP_TONE_MAPPING,             55236, SettingLevel::Basic, videoSettings.m_PlaceboColorMapToneMapping,  PlColorMapToneMapFunctionOptionFiller, 55236);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_CONTRAST_RECOVERY,        55284, SettingLevel::Basic, videoSettings.m_PlaceboColorMapContrastRecovery,   "{0:4.2f}", (float)0.0, (float)0.01, (float)2.0, 55284, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_CONTRAST_SMOOTHNESS,      55285, SettingLevel::Basic, videoSettings.m_PlaceboColorMapContrastSmoothness, "{0:4.1f}", (float)1.0, (float)0.1, (float)32.0, 55285, usePopup);
    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_GAMUT_EXPANSION,          55290, SettingLevel::Basic, videoSettings.m_PlaceboColorMapGamutExpansion);

    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_INVERSE_TONE_MAPPING,     55291, SettingLevel::Basic, videoSettings.m_PlaceboColorMapInverseToneMapping);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_I,             55292, SettingLevel::Basic, videoSettings.m_PlaceboColorMapLut3dSizeI, -1, 0, 1, 1024, 55292, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_C,             55293, SettingLevel::Basic, videoSettings.m_PlaceboColorMapLut3dSizeC, -1, 0, 1, 1024, 55293, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_SIZE_H,             55294, SettingLevel::Basic, videoSettings.m_PlaceboColorMapLut3dSizeH, -1, 0, 1, 1024, 55294, usePopup);
    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LUT3D_TRICUBIC,           55295, SettingLevel::Basic, videoSettings.m_PlaceboColorMapLut3dTricubic);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_LUT_SIZE,                 55296, SettingLevel::Basic, videoSettings.m_PlaceboColorMapLutSize, -1, 0, 1, 1024, 55296, usePopup);
    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_SHOW_CLIPPING,            55297, SettingLevel::Basic, videoSettings.m_PlaceboColorMapShowClipping);
    AddList(groupColorMap,     SETTING_LIB_PLACEBO_COLOR_MAP_INTENT,                   55298, SettingLevel::Basic, videoSettings.m_PlaceboColorMapIntent, PlColorMapIntentOptionFiller, 55298);
    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_FORCE_TONE_MAPPING_LUT,   55299, SettingLevel::Basic, videoSettings.m_PlaceboColorMapForceToneMappingLut);

    AddToggle(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_LUT,     55277, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeLut);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_X0, 55278, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeRectX0, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55278, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_X1, 55279, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeRectX1, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55279, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_Y0, 55280, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeRectY0, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55280, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_RECT_Y1, 55281, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeRectY1, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55281, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_HUE,     55282, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeHue,    "{0:3.0f}", (float)0.0, (float)1.0, (float)360.0, 55282, usePopup);
    AddSlider(groupColorMap,   SETTING_LIB_PLACEBO_COLOR_MAP_VISUALIZE_THETA,   55283, SettingLevel::Basic, videoSettings.m_PlaceboColorMapVisualizeTheta,  "{0:3.0f}", (float)0.0, (float)1.0, (float)360.0, 55283, usePopup);
    
    // Tone mapping constants
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_EXPOSURE, 55266, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantExposure, "{0:4.1f}", (float)0.0, (float)0.1, (float)10.0, 55266, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_ADAPTATION, 55267, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantKneeAdaptation, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55267, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_DEFAULT, 55268, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantKneeDefault, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55268, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_MAXIMUM, 55269, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantKneeMaximum, "{0:4.2f}", (float)0.5, (float)0.01, (float)1.0, 55269, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_MINIMUM, 55270, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantKneeMinimum, "{0:4.2f}", (float)0.0, (float)0.01, (float)0.5, 55270, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_KNEE_OFFSET, 55271, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantKneeOffset, "{0:4.2f}", (float)0.5, (float)0.01, (float)2.0, 55271, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_LINEAR_KNEE, 55272, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantLinearKnee, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55272, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_REINHARD_CONTRAST, 55273, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantReinhardContrast, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55273, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_SLOPE_OFFSET, 55274, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantSlopeOffset, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55274, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_SLOPE_TUNING, 55275, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantSlopeTuning, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55275, usePopup);
    AddSlider(groupToneMappingConstants, SETTING_LIB_PLACEBO_TONE_CONSTANTS_SPLINE_CONTRAST, 55276, SettingLevel::Basic, videoSettings.m_PlaceboToneConstantSplineContrast, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.5, 55276, usePopup);

    // Gamut mapping constants
    AddSlider(groupGamutMappingConstants, SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_COLORIMETRIC_GAMMA, 55286, SettingLevel::Basic, videoSettings.m_PlaceboGamutConstantsColorimetricGamma, "{0:4.1f}", (float)0.0, (float)0.1, (float)10.0, 55286, usePopup);
    AddSlider(groupGamutMappingConstants, SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_PERCEPTUAL_DEADZONE, 55287, SettingLevel::Basic, videoSettings.m_PlaceboGamutConstantsPerceptualDeadzone, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55287, usePopup);
    AddSlider(groupGamutMappingConstants, SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_SOFTCLIP_DESAT, 55288, SettingLevel::Basic, videoSettings.m_PlaceboGamutConstantsSoftclipDesat, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55288, usePopup);
    AddSlider(groupGamutMappingConstants, SETTING_LIB_PLACEBO_GAMUT_CONSTANTS_SOFTCLIP_KNEE, 55289, SettingLevel::Basic, videoSettings.m_PlaceboGamutConstantsSoftclipKnee, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55289, usePopup);

    // Deband
    AddToggle(groupDeband, SETTING_LIB_PLACEBO_DEBAND_ENABLED,        55237, SettingLevel::Basic, videoSettings.m_PlaceboDebandEnabled);
    AddButton(groupDeband, SETTING_LIB_PLACEBO_DEBAND_LOAD_PRESET,    55245, SettingLevel::Basic);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_GRAIN,          55238, SettingLevel::Basic, videoSettings.m_PlaceboDebandGrain,        "{0:4.0f}", (float)0.0, (float)1.0, (float)1000.0, 55238, usePopup);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL0, 55239, SettingLevel::Basic, videoSettings.m_PlaceboDebandGrainNeutral0, "{0:4.0f}", (float)0.0, (float)1.0, (float)1000.0, 55239, usePopup);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL1, 55240, SettingLevel::Basic, videoSettings.m_PlaceboDebandGrainNeutral1, "{0:4.0f}", (float)0.0, (float)1.0, (float)1000.0, 55240, usePopup);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_GRAIN_NEUTRAL2, 55241, SettingLevel::Basic, videoSettings.m_PlaceboDebandGrainNeutral2, "{0:4.0f}", (float)0.0, (float)1.0, (float)1000.0, 55241, usePopup);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_ITERATIONS,     55242, SettingLevel::Basic, videoSettings.m_PlaceboDebandIterations,    -1, 0, 1, 16, 55242, usePopup);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_RADIUS,         55243, SettingLevel::Basic, videoSettings.m_PlaceboDebandRadius,       "{0:4.0f}", (float)0.0, (float)1.0, (float)1000.0, 55243, usePopup);
    AddSlider(groupDeband, SETTING_LIB_PLACEBO_DEBAND_THRESHOLD,      55244, SettingLevel::Basic, videoSettings.m_PlaceboDebandThreshold,    "{0:4.0f}", (float)0.0, (float)1.0, (float)1000.0, 55244, usePopup);
    
    // Deinterlace
    AddToggle(groupDeinterlace, SETTING_LIB_PLACEBO_DEINTERLACE_ENABLED,             55249, SettingLevel::Basic, videoSettings.m_PlaceboDeinterlaceEnabled);
    AddList(groupDeinterlace,   SETTING_LIB_PLACEBO_DEINTERLACE_ALGO,                55250, SettingLevel::Basic, videoSettings.m_PlaceboDeinterlaceAlgo, PlDeinterlaceAlgoOptionFiller, 55250);
    AddToggle(groupDeinterlace, SETTING_LIB_PLACEBO_DEINTERLACE_SKIP_SPATIAL_CHECK,  55251, SettingLevel::Basic, videoSettings.m_PlaceboDeinterlaceSkipSpatialCheck);

    // Sigmoid 
    AddToggle(groupSigmoid, SETTING_LIB_PLACEBO_SIGMOID_ENABLED,             55252, SettingLevel::Basic, videoSettings.m_PlaceboSigmoidEnabled);
    AddButton(groupSigmoid, SETTING_LIB_PLACEBO_SIGMOID_LOAD_PRESET_DEFAULT, 55255, SettingLevel::Basic);
    AddSlider(groupSigmoid, SETTING_LIB_PLACEBO_SIGMOID_CENTER,              55253, SettingLevel::Basic, videoSettings.m_PlaceboSigmoidCenter, "{0:4.2f}", (float)0.0, (float)0.01, (float)1.0, 55253, usePopup);
    AddSlider(groupSigmoid, SETTING_LIB_PLACEBO_SIGMOID_SLOPE,               55254, SettingLevel::Basic, videoSettings.m_PlaceboSigmoidSlope, "{0:4.1f}", (float)1.0, (float)1.0, (float)20.0, 55254, usePopup);

    // Cones 
    AddToggle(groupCone, SETTING_LIB_PLACEBO_CONE_ENABLED,             55256, SettingLevel::Basic, videoSettings.m_PlaceboConeEnabled);
    AddList(groupCone,   SETTING_LIB_PLACEBO_CONE_CONES,               55258, SettingLevel::Basic, videoSettings.m_PlaceboConeCones, PlConeConesOptionFiller, 55258);
    AddSlider(groupCone, SETTING_LIB_PLACEBO_CONE_STRENGTH,            55259, SettingLevel::Basic, videoSettings.m_PlaceboConeStrength, "{0:5.2f}", (float)0.0, (float)0.01, (float)10.0, 55259, usePopup);

    // Dither
    AddToggle(groupDither, SETTING_LIB_PLACEBO_DITHER_ENABLED,             55260, SettingLevel::Basic, videoSettings.m_PlaceboDitherEnabled);
    AddButton(groupDither, SETTING_LIB_PLACEBO_DITHER_LOAD_PRESET_DEFAULT, 55261, SettingLevel::Basic);
    AddList(groupDither,   SETTING_LIB_PLACEBO_DITHER_METHOD,              55262, SettingLevel::Basic, videoSettings.m_PlaceboDitherMethod, PlDitherMethodOptionFiller, 55262);
    AddSlider(groupDither, SETTING_LIB_PLACEBO_DITHER_LUT_SIZE,            55263, SettingLevel::Basic, videoSettings.m_PlaceboDitherLutSize, -1, 1, 1, 8, 55263, usePopup);
    AddToggle(groupDither, SETTING_LIB_PLACEBO_DITHER_TEMPORAL,            55264, SettingLevel::Basic, videoSettings.m_PlaceboDitherTemporal);
    AddList(groupDither,   SETTING_LIB_PLACEBO_DITHER_TRANSFER,            55265, SettingLevel::Basic, videoSettings.m_PlaceboDitherTransfer, PlDitherTransferOptionFiller, 55265);
        
    AddSlider(groupMisc, SETTING_LIB_PLACEBO_ANTIRINGING_STRENGTH,            55300, SettingLevel::Basic, videoSettings.m_PlaceboAntiringingStrength, "{0:3.2f}", (float)0.0, (float)0.01, (float)1.0, 55300, usePopup);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_CORRECT_SUBPIXEL_OFFSET,         55301, SettingLevel::Basic, videoSettings.m_PlaceboCorrectSubpixelOffset);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_DISABLE_BUILTIN_SCALERS,         55302, SettingLevel::Basic, videoSettings.m_PlaceboDisableBuiltinScalers);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_DISABLE_DITHER_GAMMA_CORRECTION, 55303, SettingLevel::Basic, videoSettings.m_PlaceboDisableDitherGammaCorrection);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_DISABLE_LINEAR_SCALING,          55304, SettingLevel::Basic, videoSettings.m_PlaceboDisableLinearScaling);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_DYNAMIC_CONSTANTS,               55305, SettingLevel::Basic, videoSettings.m_PlaceboDynamicConstant);
    AddList(groupMisc,   SETTING_LIB_PLACEBO_ERROR_DIFFUSION,                 55306, SettingLevel::Basic, videoSettings.m_PlaceboErrorDiffusion, PlDiffusionKernelOptionFiller, 55306);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_FORCE_DITHER,                    55307, SettingLevel::Basic, videoSettings.m_PlaceboForceDither);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_FORCE_LOW_BIT_DEPTH_FBOS,        55308, SettingLevel::Basic, videoSettings.m_PlaceboForceLowBitDepthFbos);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_IGNORE_ICC_PROFILES,             55309, SettingLevel::Basic, videoSettings.m_PlaceboIgnoreIccProfiles);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_PRESERVE_MIXING_CACHE,           55310, SettingLevel::Basic, videoSettings.m_PlaceboPreserveMixingCache);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_SKIP_ANTI_ALIASING,              55311, SettingLevel::Basic, videoSettings.m_PlaceboSkipAntiAliasing);
    AddToggle(groupMisc, SETTING_LIB_PLACEBO_SKIP_CACHING_SINGLE_FRAME,       55312, SettingLevel::Basic, videoSettings.m_PlaceboSkipCachingSingleFrame);


 
/* Available options leftover from structure

// Need a way to input/read a big table....
// videoSettings.m_placeboOptions->params.lut;
// videoSettings.m_placeboOptions->params.lut_entries; //deprecated, set to 256
// videoSettings.m_placeboOptions->params.lut_type;

    
// Deprecated, More for kodi integration, use pl_frame.icc
videoSettings.m_placeboOptions->icc_params;
videoSettings.m_placeboOptions->icc_params.cache->params;
videoSettings.m_placeboOptions->icc_params.cache_load;
videoSettings.m_placeboOptions->icc_params.cache_priv;
videoSettings.m_placeboOptions->icc_params.cache_save;
videoSettings.m_placeboOptions->icc_params.force_bpc;
videoSettings.m_placeboOptions->icc_params.intent;
videoSettings.m_placeboOptions->icc_params.max_luma;
videoSettings.m_placeboOptions->icc_params.size_b;
videoSettings.m_placeboOptions->icc_params.size_g;
videoSettings.m_placeboOptions->icc_params.size_r;

// More for kodi integration
videoSettings.m_placeboOptions->params.background;                  // no
videoSettings.m_placeboOptions->params.background_color;            // no
videoSettings.m_placeboOptions->params.background_transparency;     // no
videoSettings.m_placeboOptions->params.blend_against_tiles;         // no
videoSettings.m_placeboOptions->params.blur_radius;                 // no
videoSettings.m_placeboOptions->params.border;                      // no
videoSettings.m_placeboOptions->params.corner_rounding;             // no
videoSettings.m_placeboOptions->params.disable_fbos;                // no
videoSettings.m_placeboOptions->params.hooks;                       // no
videoSettings.m_placeboOptions->params.info_callback;               // allows gathering stats on shader usage, cache usage, etc. not really useful for end-users but could be used for debugging or advanced users
videoSettings.m_placeboOptions->params.info_priv;                   // "
videoSettings.m_placeboOptions->params.num_hooks;                   // no
videoSettings.m_placeboOptions->params.tile_colors;                 // no
videoSettings.m_placeboOptions->params.tile_size;                   // no
videoSettings.m_placeboOptions->params.polar_cutoff;                // deprecated // hard-coded as 1e-3
videoSettings.m_placeboOptions->params.allow_delayed_peak_detect;   //deprecated
videoSettings.m_placeboOptions->params.skip_target_clearing;        //deprecated
videoSettings.m_placeboOptions->color_map_params.tone_mapping_crosstalk; // deprecated, now hard-coded as 0.04
videoSettings.m_placeboOptions->color_map_params.tone_mapping_mode;      // deprecated
videoSettings.m_placeboOptions->color_map_params.tone_mapping_param;     // deprecated
videoSettings.m_placeboOptions->color_map_params.hybrid_mix;             // deprecated

// More for kodi integration
videoSettings.m_placeboOptions->color_map_params.metadata;               // {"any",  PL_HDR_METADATA_ANY},{"none", PL_HDR_METADATA_NONE},{"hdr10",PL_HDR_METADATA_HDR10},{"hdr10plus", PL_HDR_METADATA_HDR10PLUS},{"cie_y",PL_HDR_METADATA_CIE_Y})),

// More for kodi integration
videoSettings.m_placeboOptions->blend_params;
videoSettings.m_placeboOptions->blend_params.dst_alpha;
videoSettings.m_placeboOptions->blend_params.dst_rgb;
videoSettings.m_placeboOptions->blend_params.src_alpha;
videoSettings.m_placeboOptions->blend_params.src_rgb;

// More for kodi integration
videoSettings.m_placeboOptions->distort_params;
videoSettings.m_placeboOptions->distort_params.address_mode;
videoSettings.m_placeboOptions->distort_params.alpha_mode;
videoSettings.m_placeboOptions->distort_params.bicubic;
videoSettings.m_placeboOptions->distort_params.constrain;
videoSettings.m_placeboOptions->distort_params.transform;
videoSettings.m_placeboOptions->distort_params.transform.c;
videoSettings.m_placeboOptions->distort_params.transform.mat;
videoSettings.m_placeboOptions->distort_params.unscaled;


//upscaler/downscaler/frame_mixer/plane upscaler/plane downscaler have a list of preset filters but using the "custom" filter, the values can be set individiually
videoSettings.m_placeboOptions->upscaler;
videoSettings.m_placeboOptions->upscaler.allowed;
videoSettings.m_placeboOptions->upscaler.antiring;
videoSettings.m_placeboOptions->upscaler.blur;
videoSettings.m_placeboOptions->upscaler.clamp;
videoSettings.m_placeboOptions->upscaler.description;
videoSettings.m_placeboOptions->upscaler.kernel; ///
videoSettings.m_placeboOptions->upscaler.name;
videoSettings.m_placeboOptions->upscaler.params;
videoSettings.m_placeboOptions->upscaler.polar;
videoSettings.m_placeboOptions->upscaler.radius;
videoSettings.m_placeboOptions->upscaler.recommended;
videoSettings.m_placeboOptions->upscaler.taper;
videoSettings.m_placeboOptions->upscaler.window; ///
videoSettings.m_placeboOptions->upscaler.wparams;
  */

  }
}

void CGUIDialogVideoSettings::AddVideoStreams(const std::shared_ptr<CSettingGroup>& group,
                                              const std::string& settingId)
{
  if (group == NULL || settingId.empty())
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  m_videoStream = appPlayer->GetVideoStream();
  if (m_videoStream < 0)
    m_videoStream = 0;

  AddList(group, settingId, 38031, SettingLevel::Basic, m_videoStream, VideoStreamsOptionFiller, 38031);
}

std::string CGUIDialogVideoSettings::getDiffusionKernelDescriptionFromIndex(int index)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;

  PlDiffusionKernelOptionFiller(asetting, alist, value);

  if (index == -1)
    return "disabled";
  else if (index < 0 || index >= alist.size())
    return "";
  else
    return alist[index].label;
}

std::string CGUIDialogVideoSettings::getDitherTransferDescriptionFromIndex(int index)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;

  PlDitherTransferOptionFiller(asetting, alist, value);

  if (index < 0 || index >= alist.size())
    return "";
  else
    return alist[index].label;
}

std::string CGUIDialogVideoSettings::getDitherMethodDescriptionFromIndex(int index)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;

  PlDitherMethodOptionFiller(asetting, alist, value);

  if (index < 0 || index >= alist.size())
    return "";
  else
    return alist[index].label;
}

std::string CGUIDialogVideoSettings::getDeinterlaceAlgoDescriptionFromIndex(int index)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;

  PlDeinterlaceAlgoOptionFiller(asetting, alist, value);

  if (index < 0 || index >= alist.size())
    return "";
  else
    return alist[index].label;
}


std::string CGUIDialogVideoSettings::getConeConesDescriptionFromIndex(int index)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;

  PlConeConesOptionFiller(asetting, alist, value);

  if (index < 0 || index >= alist.size())
    return "";
  else
    return alist[index].label;
}

std::string CGUIDialogVideoSettings::getColorMapIntentDescriptionFromIndex(int index)
{
  std::vector<IntegerSettingOption> alist;
  int value;
  const std::shared_ptr<const CSetting> asetting;

  PlColorMapIntentOptionFiller(asetting, alist, value);

  if (index+1 < 0 || index+1 >= alist.size())
    return "";
  else
    return alist[index+1].label;
}


void CGUIDialogVideoSettings::PlDiffusionKernelOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
  {
  const struct pl_error_diffusion_kernel* f;
  list.emplace_back("Disabled", -1);
  for (int i = 0; i < pl_num_error_diffusion_kernels; i++)
    {
      f = pl_error_diffusion_kernels[i];
      if (f->description)
        list.emplace_back(f->description, i);
    }
  }

void CGUIDialogVideoSettings::PlColorMapIntentOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  list.emplace_back("Auto", PL_INTENT_AUTO);
  list.emplace_back("Perceptual", PL_INTENT_PERCEPTUAL);
  list.emplace_back("Relative colorimetric", PL_INTENT_RELATIVE_COLORIMETRIC);
  list.emplace_back("Saturation", PL_INTENT_SATURATION);
  list.emplace_back("Absolute colorimetric", PL_INTENT_ABSOLUTE_COLORIMETRIC);
}

void CGUIDialogVideoSettings::PlDitherMethodOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  list.emplace_back("Blue noise", PL_DITHER_BLUE_NOISE);
  list.emplace_back("Ordered LUT", PL_DITHER_ORDERED_LUT);
  list.emplace_back("Ordered fixed", PL_DITHER_ORDERED_FIXED);
  list.emplace_back("White noise", PL_DITHER_WHITE_NOISE);
}

void CGUIDialogVideoSettings::PlDitherTransferOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  const struct pl_tone_map_function* f;
  for (int i = 0; i < PL_COLOR_TRC_COUNT; i++)
  {
    if (pl_color_transfer_name(static_cast<pl_color_transfer>(i)))
      list.emplace_back(pl_color_transfer_name(static_cast<pl_color_transfer>(i)), i);
  }
}

void CGUIDialogVideoSettings::PlConeConesOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  list.emplace_back("None", PL_CONE_NONE);
  list.emplace_back("L", PL_CONE_L);
  list.emplace_back("M", PL_CONE_M);
  list.emplace_back("S", PL_CONE_S);
  list.emplace_back("LM", PL_CONE_L | PL_CONE_M);
  list.emplace_back("MS", PL_CONE_M | PL_CONE_S);
  list.emplace_back("LS", PL_CONE_L | PL_CONE_S);
  list.emplace_back("LMS", PL_CONE_L | PL_CONE_M | PL_CONE_S);

}

void CGUIDialogVideoSettings::PlDeinterlaceAlgoOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  list.emplace_back("WEAVE", PL_DEINTERLACE_WEAVE);
  list.emplace_back("BOB",   PL_DEINTERLACE_BOB);
  list.emplace_back("YADIF", PL_DEINTERLACE_YADIF);
  list.emplace_back("BWDIF", PL_DEINTERLACE_BWDIF);

}

void CGUIDialogVideoSettings::PlColorMapToneMapFunctionOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  const struct pl_tone_map_function* f;

  list.emplace_back("Disabled", -1);
  for (int i = 0; i < pl_num_tone_map_functions; i++)
  {
    std::string strItem;
    f = pl_tone_map_functions[i];
    if (f->description)
      list.emplace_back(f->description, i);
  }
}

void CGUIDialogVideoSettings::PlColorMapGamutMapFunctionOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  const struct pl_gamut_map_function* f;

  list.emplace_back("Disabled", -1);
  for (int i = 0; i < pl_num_gamut_map_functions; i++)
  {
    std::string strItem;
    f = pl_gamut_map_functions[i];
    if (f->description)
      list.emplace_back(f->description, i);
  }
}

void CGUIDialogVideoSettings::PlUpscalerOptionFiller(const std::shared_ptr<const CSetting>& setting,std::vector<IntegerSettingOption>& list, int& current)
{
  const struct pl_filter_config* f;
  
  list.emplace_back("Disabled", -1);
  for (int i = 0; i < pl_num_filter_configs; i++)
  {
    std::string strItem;
    f = pl_filter_configs[i];
    if (!f->description)
      continue;
    if (!(f->allowed & PL_FILTER_UPSCALING))
      continue;
    if (!(f->recommended & PL_FILTER_UPSCALING))
      continue;
    list.emplace_back(f->description, i);
  }
}

void CGUIDialogVideoSettings::PlDownscalerOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  const struct pl_filter_config* f;

  list.emplace_back("Disabled", -1);
  for (int i = 0; i < pl_num_filter_configs; i++)
  {
    std::string strItem;
    f = pl_filter_configs[i];
    if (!f->description)
      continue;
    if (!(f->allowed & PL_FILTER_DOWNSCALING))
      continue;
    if (!(f->recommended & PL_FILTER_DOWNSCALING))
      continue;
    list.emplace_back(f->description, i);
  }
}

void CGUIDialogVideoSettings::PlFrameMixerOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current)
{
  const struct pl_filter_config* f;
  
  list.emplace_back("Disabled", -1);
  for (int i = 0; i < pl_num_filter_configs; i++)
  {
    std::string strItem;
    f = pl_filter_configs[i];
    if (!f->description)
      continue;
    if (!(f->allowed & PL_FILTER_FRAME_MIXING))
      continue;
    if (!(f->recommended & PL_FILTER_FRAME_MIXING))
      continue;
    list.emplace_back(f->description, i);
  }
}





void CGUIDialogVideoSettings::VideoStreamsOptionFiller(
    const std::shared_ptr<const CSetting>& setting,
    std::vector<IntegerSettingOption>& list,
    int& current)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  int videoStreamCount = appPlayer->GetVideoStreamCount();
  // cycle through each video stream and add it to our list control
  for (int i = 0; i < videoStreamCount; ++i)
  {
    std::string strItem;
    std::string strLanguage;

    VideoStreamInfo info;
    appPlayer->GetVideoStreamInfo(i, info);

    g_LangCodeExpander.Lookup(info.language, strLanguage);

    if (!info.name.empty())
    {
      if (!strLanguage.empty())
        strItem = StringUtils::Format("{} - {}", strLanguage, info.name);
      else
        strItem = info.name;
    }
    else if (!strLanguage.empty())
    {
        strItem = strLanguage;
    }

    if (info.codecName.empty())
      strItem += StringUtils::Format(" ({}x{}", info.width, info.height);
    else
      strItem += StringUtils::Format(" ({}, {}x{}", info.codecName, info.width, info.height);

    if (info.bitrate)
      strItem += StringUtils::Format(", {} bps)", info.bitrate);
    else
      strItem += ")";

    strItem += FormatFlags(info.flags);
    strItem += StringUtils::Format(" ({}/{})", i + 1, videoStreamCount);
    list.emplace_back(strItem, i);
  }

  if (list.empty())
  {
    list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(231), -1);
    current = -1;
  }
}

void CGUIDialogVideoSettings::VideoOrientationFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<IntegerSettingOption>& list,
    int& /*current*/)
{
  list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(687), 0);
  list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35229), 90);
  list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35230), 180);
  list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35231), 270);
}

std::string CGUIDialogVideoSettings::FormatFlags(StreamFlags flags)
{
  std::vector<std::string> localizedFlags;
  if (flags & StreamFlags::FLAG_DEFAULT)
    localizedFlags.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(39105));
  if (flags & StreamFlags::FLAG_FORCED)
    localizedFlags.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(39106));
  if (flags & StreamFlags::FLAG_HEARING_IMPAIRED)
    localizedFlags.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(39107));
  if (flags &  StreamFlags::FLAG_VISUAL_IMPAIRED)
    localizedFlags.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(39108));

  std::string formated = StringUtils::Join(localizedFlags, ", ");

  if (!formated.empty())
    formated = StringUtils::Format(" [{}]", formated);

  return formated;
}
