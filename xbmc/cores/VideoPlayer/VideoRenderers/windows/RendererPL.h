/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "RendererHQ.h"
#include "VideoRenderers/HwDecRender/DXVAHD.h"
#include "VideoRenderers/RenderFlags.h"

#include <map>

#include <d3d11_4.h>

//#pragma comment(lib, "libplacebo.a")
#pragma comment(lib, "libplacebo.dll.a")

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include "libplacebo/log.h"
#include "libplacebo/renderer.h"
#include "libplacebo/d3d11.h"
#include <libplacebo/options.h>
#include "libplacebo/utils/frame_queue.h"
#include "libplacebo/utils/upload.h"
#include "libplacebo/colorspace.h"
#define PL_LIBAV_IMPLEMENTATION 0
#include <libplacebo/utils/libav.h>
#include "VideoRenderers/Libplacebo/PLHelper.h"

#define MAX_FRAME_PASSES 256
#define MAX_BLEND_PASSES 8
#define MAX_BLEND_FRAMES 8

enum class SettinglibPlaceboTargetColorspaceHint
{
  AUTO = -1,
  NO = 0,
  YES = 1
};

enum class SettinglibPlaceboTargetColorspaceHintMode
{
  TARGET = 0,
  SOURCE = 1,
  SOURCE_DYNAMIC = 2
};

enum RenderMethod;

class CRendererPL : public CRendererHQ
{
    CRect ApplyTransforms(const CRect& destRect) const;
	class CRenderBufferImpl;
public:
  //~CRendererPL() = default;
  ~CRendererPL();

  pl_queue queue=NULL;

  static pl_icc_object ReadIcc(const std::string& fileName);
  static pl_custom_lut* ReadLut(const std::string& filename);
  void UpdateVideoFilters() override;
  bool NeedBuffer(int idx) override;
  CRenderInfo GetRenderInfo() override;
  bool Supports(ESCALINGMETHOD method) const override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool WantsDoublePass() override { return true; }
  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;

  void AddVideoPicture(const VideoPicture& picture, int index) override; 


  DEBUG_INFO_VIDEO GetDebugInfo(int idx) override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);
  static DXGI_FORMAT GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt);
protected:
  explicit CRendererPL(CVideoSettings& videoSettings);

  void CheckVideoParameters() override;
  void RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags) override;
  void ApplyTargetOptions(pl_color_space* target_csp, struct pl_frame* target, float min_luma, bool hint);
  CRenderBuffer* CreateBuffer() override;

  //important to override we need to let libplacebo set the swapchain csp or we would get issues with HDR
  void ProcessHDR(CRenderBuffer* rb) override;

private:
  // Color space info
  pl_color_space m_colorSpace;
  pl_color_space hint = {};
  uint64_t m_signatureCounter=1;
  int lastBufferIndex=-1;
  pl_frame prevFrame {0};

  pl_chroma_location m_chromaLocation;

  //For debug info
  pl_color_system m_videoMatrix;
  pl_color_transfer m_displayTransfer;
  pl_color_primaries m_displayPrimaries;

  // Tracking for parameter changes
  AVColorSpace m_lastColorSpace = AVCOL_SPC_UNSPECIFIED;

  //maybe remove those 2
  AVColorTransferCharacteristic m_lastColorTransfer = AVCOL_TRC_UNSPECIFIED;
  AVColorPrimaries m_lastPrimaries = AVCOL_PRI_UNSPECIFIED;

  AVPixelFormat m_format;
};

class CRendererPL::CRenderBufferImpl : public CRenderBuffer
{
public:
  void ReleasePicture() override;

  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  void AppendPicture(const VideoPicture& picture) override;
  bool UploadBuffer() override;
  bool GetLibplaceboFrame(pl_frame& frame);
  bool map_frame(pl_gpu gpu, pl_tex* tex, struct pl_source_frame* src, struct pl_frame* out_frame);
  double getPts() { return pts; }
  bool HasHdrData();
  pl_color_space hdrColorSpace; //< pl_color_space
  pl_color_repr doviColorRepr;
  pl_dovi_metadata doviPlMetadata;
  pl_hdr_metadata hdrDoviRpu; //< pl_hdr_metadata
  bool hasHDR10PlusMetadata = false;
  bool hasDoviMetadata = false;
  bool hasDoviRpuMetadata = false;
  AVDOVIColorMetadata doviColor { 0 };
  AVDOVIDmData doviExt{ 0 };
  bool hasDoviExt = false;


  bool isInterlaced = false;
  uint64_t m_signature = 0;
  AVDynamicHDRPlus hdrMetadata;
  bool disable_residual_flag;
  AVDOVIMetadata doviMetadata;
  PL::pl_d3d_format plFormat = {};
  //planes are used to create the frame
  pl_plane plplanes[3] = {};

private:
  //sw upload
  bool UploadPlanes();
  //When decoded with d3d11va
  bool UploadWrapPlanes();
  //move those to the video codec if linux start to use libplacebo


  // they are only kept for plane reference
  // we could put them to null according to libplacebo doc but it crash right away
  pl_tex pltex[3] = {};
  /* data info for dxva planes formating and bit encoding fill with plhelper
  *  this include the bit format for the color conversion 
  * */
};