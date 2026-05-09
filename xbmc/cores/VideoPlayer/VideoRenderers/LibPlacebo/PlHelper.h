/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoSettings.h"
#include "libplacebo/colorspace.h"
#include "libplacebo/d3d11.h"
#include "libplacebo/log.h"
#include "libplacebo/renderer.h"





extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/dovi_meta.h>
}

#include <string>
#include <vector>
#include <strmif.h>
#include <d3d9types.h>
#include <dxva2api.h>
#include <memory>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <settings/lib/SettingDefinitions.h>
#include <settings/SettingControl.h>
#include <tinyxml.h>
#include <libplacebo/shaders/lut.h>
#include <dxgiformat.h>
#include <libplacebo/gpu.h>
#include <libplacebo/swapchain.h>
#include <libplacebo/cache.h>

#define MAX_FRAME_PASSES 256
#define MAX_BLEND_PASSES 8
#define MAX_BLEND_FRAMES 8
namespace PL
{
  typedef struct pl_d3d_format {
	pl_bit_encoding bits;     // per picture
	DXGI_FORMAT planes[4];      // DXGI format per plane
	int components[4];          // number of components per plane
	pl_channel component_mapping[4][4];
	int width_div[4];       // divide full width by this for each plane
	int height_div[4];      // divide full height by this for each plane
	char description[16];       // short description
	int num_planes;         // actual number of planes used
  } pl_d3d_format;

  class PLInstance
  {
  public:
	static std::shared_ptr<PLInstance> Get();
	PLInstance();

	virtual ~PLInstance();
	bool Init();
	void Reset();

	pl_d3d11 GetD3d11() { return m_plD3d11; }
	pl_swapchain GetSwapchain() { return m_plSwapchain; }
	pl_renderer GetRenderer() { return m_plRenderer; }
	pl_gpu GetGpu() { return m_plD3d11->gpu; }
	pl_cache* GetCache() { return &m_plCache; }

	pl_cache m_plCache;
	pl_log m_plLog;
	pl_d3d11 m_plD3d11;
	pl_swapchain m_plSwapchain;
	pl_renderer m_plRenderer;
	int CurrentPrim;
	int Currenttransfer;
	int CurrentMatrix;
	void LogCurrent();
	void fill_d3d_format(pl_d3d_format* info, DXGI_FORMAT format);
  };
}

class CPLHelper
{
public:
  static void SkinZoomUpdate(void);
  static void InitializeShaders(pl_gpu gpu);
  static void ResetShaders(CVideoSettings& vs);
  static void InitializeShaders(pl_gpu gpu, CVideoSettings& vs);
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
  static std::string getColorMapIntentDescriptionFromIndex(int index);
  static std::string getConeConesDescriptionFromIndex(int index);
  static std::string getDitherMethodDescriptionFromIndex(int index);
  static std::string getDitherTransferDescriptionFromIndex(int index);
  static std::string getDiffusionKernelDescriptionFromIndex(int index);
  static std::string getLutTypeDescriptionFromIndex(int index);
  static std::string getDeinterlaceAlgoDescriptionFromIndex(int index);
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
  static void PlShaderOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<StringSettingOption>& list, std::string& current);
  static void PlLutOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<StringSettingOption>& list, std::string& current);
  static void PlLutTypeOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void PlDiffusionKernelOptionFiller(const std::shared_ptr<const CSetting>& setting, std::vector<IntegerSettingOption>& list, int& current);
  static void SetVideoSettings(CVideoSettings& vs);
  static void UpdateVideoSettingsFromLibPLaceboParams(CVideoSettings& vs);
  static void UpdateLibPLaceboParamsFromVideoSettings(CVideoSettings& vs);
  static void LoadLibplaceboSettings(CVideoSettings& vs);
  static bool LoadLibplaceboSettings(CVideoSettings& vs, std::string path);
  static bool LoadLibplaceboSettings(CVideoSettings& vs, const TiXmlElement* pElement);
  static void SaveLibplaceboSettings(const CVideoSettings& vs, std::string path);
  static void SaveLibplaceboSettings(const CVideoSettings& vs, TiXmlNode* settings);
  static void LoadLutFile(CVideoSettings& vs, const std::string& path);
  static void AddShaderFile(pl_gpu gpu, CVideoSettings& vs, const std::string& fileName);
  static void SerializeShaders(const CVideoSettings& vs, std::string& serializedData);
  static void SerializeShaders(const CVideoSettings& vs, TiXmlNode* pNode);
  static void SaveShadersSettings(const CVideoSettings& vs, TiXmlNode* lpnode);
  static void LoadShaderSettings(CVideoSettings& vs, const std::string& data);
  static void LoadShaderSettings(CVideoSettings& vs, const TiXmlElement* pElement);
  static std::shared_ptr<const pl_custom_lut> ReadLut(const std::string& fileName);

};
