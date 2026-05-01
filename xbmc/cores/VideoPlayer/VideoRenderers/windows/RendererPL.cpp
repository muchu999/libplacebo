/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererPL.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "VideoRenderers/BaseRenderer.h"
#include "VideoRenderers/HwDecRender/DXVAEnumeratorHD.h"
#include "WIN32Util.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "utils/memcpy_sse2.h"
#include "windowing/GraphicContext.h"

#include <ppl.h>



using namespace Microsoft::WRL;

CRendererPL::~CRendererPL()
{
  pl_swapchain m_plSwapchain;

  m_plSwapchain = PL::PLInstance::Get()->GetSwapchain();
  pl_swapchain_destroy(&m_plSwapchain);

  pl_queue_destroy(&queue);
  
  //cl Force restore default color space on exit, non-hdr content messes up hdr color space, should save and restore instead
  if (DX::Windowing()->IsHDROutput())
    DX::Windowing()->SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
  else
    DX::Windowing()->SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);

}


bool CRendererPL::NeedBuffer(int idx)
{
  //cl
  /*if (m_renderBuffers[idx]->IsLoaded() && m_renderBuffers[idx]->pictureFlags & DVP_FLAG_INTERLACED)
  {
    //uint8_t PastRefs() const { return std::min(m_procCaps.m_rateCaps.PastFrames, 4u); }
    if (m_renderBuffers[idx]->frameIdx + (4 * 2u) >=
      m_renderBuffers[m_iBufferIndex]->frameIdx)
      return true;
  }*/

  return false;
}

void CRendererPL::AddVideoPicture(const VideoPicture& picture, int index)
{
  if (m_renderBuffers[index])
  {
    m_renderBuffers[index]->AppendPicture(picture);
    m_renderBuffers[index]->frameIdx = m_frameIdx;
    m_frameIdx += 2;
  }

  //CRenderBuffer* rb = m_renderBuffers[index];
  //struct pl_source_frame sframe{};
  //sframe.pts = rb->pts/1000000.0;
  //sframe.duration = rb->duration/1000000.0;
  //sframe.map = map_frame;
  //sframe.unmap = NULL;
  //sframe.frame_data = rb;
  //sframe.discard = NULL;
  //sframe.first_field = PL_FIELD_NONE;
  //if(!queue)
  //  queue = pl_queue_create(PL::PLInstance::Get()->GetGpu());
  //
  //pl_queue_push(queue, &sframe);
}


CRendererBase* CRendererPL::Create(CVideoSettings& videoSettings)
{
  return new CRendererPL(videoSettings);
}

void CRendererPL::GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture)
{
  unsigned weight = 0;
  const AVPixelFormat av_pixel_format = picture.videoBuffer->GetFormat();

  // Support common YUV formats for libplacebo
  if (av_pixel_format == AV_PIX_FMT_YUV420P ||
    av_pixel_format == AV_PIX_FMT_YUV420P10 ||
    av_pixel_format == AV_PIX_FMT_YUV420P16 ||
    av_pixel_format == AV_PIX_FMT_NV12 ||
    av_pixel_format == AV_PIX_FMT_P010 ||
    av_pixel_format == AV_PIX_FMT_P016)
  {
    weight += 800; // High priority for libplacebo
  }
  else if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    weight += 700; // Still support hardware decoded content
  }

  if (weight > 0)
    weights[RENDER_LIBPLACEBO] = weight;
}

CRendererPL::CRendererPL(CVideoSettings& videoSettings) : CRendererHQ(videoSettings)
{
  m_renderMethodName = "LibPlacebo";
  m_colorSpace = {};
  m_chromaLocation = PL_CHROMA_UNKNOWN;

}

CRenderInfo CRendererPL::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  info.m_deintMethods.push_back(VS_INTERLACEMETHOD_AUTO);

  return  info;
}

bool CRendererPL::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (!__super::Configure(picture, fps, orientation))
    return false;

  //Log initiation


  PL::PLInstance::Get()->Init();
  // Set up color space based on picture metadata
  m_colorSpace = pl_color_space{
    .primaries = pl_primaries_from_av(picture.color_primaries),
    .transfer = pl_transfer_from_av(picture.color_transfer),
  };
  m_chromaLocation = pl_chroma_from_av(picture.chroma_position);
  m_format = picture.videoBuffer->GetFormat();
  return true;
}

DEBUG_INFO_VIDEO CRendererPL::GetDebugInfo(int idx)
{
  
  CRenderBuffer* rb = m_renderBuffers[idx];
  CRenderBufferImpl* plbuffer = static_cast<CRenderBufferImpl*>(rb);
  
  DEBUG_INFO_VIDEO info;
  pl_hdr_metadata hdr = plbuffer->hdrColorSpace.hdr;
  
  info.videoSource = StringUtils::Format("Display: Format: {} Levels: full, ColorMatrix:rgb", DX::DXGIFormatToShortString(m_IntermediateTarget.GetFormat()));

  info.metaPrim = StringUtils::Format("Transfer: {} Primaries: {}", pl_color_transfer_name(m_displayTransfer), pl_color_primaries_name(m_displayPrimaries));


  info.metaLight = StringUtils::Format("Video: Matrix:{} Primaries:{} Transfer:{}", pl_color_primaries_name(m_colorSpace.primaries)
                                                                                  , pl_color_transfer_name(m_colorSpace.transfer)
                                                                                  , pl_color_system_name(m_videoMatrix));
  if (plbuffer->hasHDR10PlusMetadata)
  {
    info.shader = "Primaries (meta): ";
    info.shader += StringUtils::Format(
      "R({:.3f} {:.3f}), G({:.3f} {:.3f}), B({:.3f} {:.3f}), WP({:.3f} {:.3f})", hdr.prim.red.x, hdr.prim.red.y,
      hdr.prim.green.x, hdr.prim.green.y, hdr.prim.blue.x, hdr.prim.blue.y, hdr.prim.white.x, hdr.prim.white.y);

    info.render = StringUtils::Format("HDR light (meta): max ML: {:.0f}, min ML: {:.4f}", hdr.max_luma, hdr.min_luma);
    info.render += StringUtils::Format(", max CLL: {}, max FALL: {}", hdr.max_cll, hdr.max_fall);
  }
  //line 1 std::string videoSource;
  //2 std::string metaPrim;
  //3 std::string metaLight;
  //4 std::string shader;
  //5 std::string render;
  return info;
}

void CRendererPL::CheckVideoParameters()
{
  __super::CheckVideoParameters();

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (buf)
  {
    // Check if color space parameters have changed
    if (buf->color_space != m_lastColorSpace ||
      buf->color_transfer != m_lastColorTransfer ||
      buf->primaries != m_lastPrimaries)
    {
      m_lastColorSpace = buf->color_space;
      m_lastColorTransfer = buf->color_transfer;
      m_lastPrimaries = buf->primaries;
      m_colorSpace = pl_color_space{
        .primaries = pl_primaries_from_av(buf->primaries),
        .transfer = pl_transfer_from_av(buf->color_transfer),
      };
    }
  }
  CreateIntermediateTarget(m_viewWidth, m_viewHeight, false, DXGI_FORMAT_R10G10B10A2_UNORM);
}

CRect CRendererPL::ApplyTransforms(const CRect& destRect) const
{
  CRect result;
  CPoint rotated[4];
  ReorderDrawPoints(destRect, rotated);

  switch (m_renderOrientation)
  {
  case 90:
    result = { rotated[3], rotated[1] };
    break;
  case 180:
    result = destRect;
    break;
  case 270:
    result = { rotated[1], rotated[3] };
    break;
  default:
    result = CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(destRect);
    break;
  }

  return result;
}

enum pl_color_primaries MpGetBestPrimContainer(const struct pl_raw_primaries* gamut)
{
  enum pl_color_primaries container = PL_COLOR_PRIM_UNKNOWN;

  if (!pl_primaries_valid(gamut))
    return container;

  const struct pl_raw_primaries* best = NULL;
  for (int i = 1; i < PL_COLOR_PRIM_COUNT; i++)
  {
    pl_color_primaries prim = static_cast<pl_color_primaries>(i);
    const struct pl_raw_primaries* raw = pl_raw_primaries_get(prim);
    if (pl_raw_primaries_similar(raw, gamut)) {
      container = prim;
      best = raw;
      break;
    }

    if (pl_primaries_superset(raw, gamut) &&
      (!best || pl_primaries_superset(best, raw)))
    {
      container = prim;
      best = raw;
    }
  }

  if (!best)
    container = PL_COLOR_PRIM_BT_2020;

  return container;
}

static void ApplyTargetContrast(struct pl_color_space* color, float min_luma, float target_contrast)
{
  // Auto mode, use target value if available
  if (target_contrast) {
    color->hdr.min_luma = min_luma;
    return;
  }

  // Infinite contrast
  if (target_contrast == -1) {
    color->hdr.min_luma = 1e-7;
    //clmp_assert(color->hdr.min_luma > 0);
    return;
  }

  // Infer max_luma for current pl_color_space
  pl_nominal_luma_params pl{
        .color = color,
        // with HDR10 meta to respect value if already set
        .metadata = PL_HDR_METADATA_HDR10,
        .scaling = PL_HDR_NITS,
        .out_max = &color->hdr.max_luma
  };
  pl_color_space_nominal_luma_ex(&pl);

  color->hdr.min_luma = color->hdr.max_luma / target_contrast;
}


//------------------------------------------
//
//------------------------------------------
struct pl_color_space MpDxgiDescToColorSpace(const DXGI_OUTPUT_DESC1* desc)
{
  struct pl_color_space ret = { };
  if (!desc)
    return ret;

  ret.hdr.max_luma = desc->MaxLuminance;
  ret.hdr.min_luma = desc->MinLuminance;
  ret.hdr.max_fall = desc->MaxFullFrameLuminance;
  ret.hdr.prim.blue.x = desc->BluePrimary[0];
  ret.hdr.prim.blue.y = desc->BluePrimary[1];
  ret.hdr.prim.green.x = desc->GreenPrimary[0];
  ret.hdr.prim.green.y = desc->GreenPrimary[1];
  ret.hdr.prim.red.x = desc->RedPrimary[0];
  ret.hdr.prim.red.y = desc->RedPrimary[1];
  ret.hdr.prim.white.x = desc->WhitePoint[0];
  ret.hdr.prim.white.y = desc->WhitePoint[1];

  switch (desc->ColorSpace) {
  case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
    ret.primaries = PL_COLOR_PRIM_BT_709;
    ret.transfer = PL_COLOR_TRC_SRGB;
    break;
  case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
    ret.primaries = PL_COLOR_PRIM_BT_709;
    ret.transfer = PL_COLOR_TRC_SCRGB;
    break;
  case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
    ret.primaries = PL_COLOR_PRIM_BT_2020;
    ret.transfer = PL_COLOR_TRC_PQ;
    break;
  case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
    ret.primaries = PL_COLOR_PRIM_BT_2020;
    ret.transfer = PL_COLOR_TRC_SRGB;
    break;
  default:
    ret.primaries = PL_COLOR_PRIM_UNKNOWN;
    ret.transfer = PL_COLOR_TRC_UNKNOWN;
    break;
  }

  if (!pl_color_transfer_is_hdr(ret.transfer)) {
    // Don't use reported display peak in SDR mode, setting target peak in
    // SDR mode is very specific usecase, needs proper calibration, users
    // can set it manually.
    ret.hdr.max_luma = 0;
    ret.hdr.max_cll = 0;
    ret.hdr.max_fall = 0;
  }

  return ret;
}

void CRendererPL::ApplyTargetOptions(pl_color_space* target_csp, struct pl_frame* target, float min_luma, bool hint)
{
  
  //update_lut(p, &p->next_opts->target_lut);
  //target->lut = p->next_opts->target_lut.lut;
  //target->lut_type = p->next_opts->target_lut.type;

  //// Colorspace overrides
  //const struct gl_video_opts* opts = p->opts_cache->opts;
  //// If swapchain returned a value use this, override is used in hint
  //if (p->output_levels)
  //  target->repr.levels = p->output_levels;
  if (target_csp->primaries && (!target->color.primaries || !hint))
    target->color.primaries = target_csp->primaries;
  if (target_csp->transfer && (!target->color.transfer || !hint))
    target->color.transfer = target_csp->transfer;

  if (m_videoSettings.m_PlaceboDisplayPeakLuminance && (!target->color.hdr.max_luma || !hint))
    target->color.hdr.max_luma = m_videoSettings.m_PlaceboDisplayPeakLuminance;

  //if (opts->hdr_reference_white && (!target->color.hdr.max_luma || !hint) &&
  //  !pl_color_transfer_is_hdr(target->color.transfer)) {
  //  target->color.hdr.max_luma = opts->hdr_reference_white;
  //}
  //if ((!target->color.hdr.min_luma || !hint))
  //  ApplyTargetContrast(p, &target->color, min_luma);
  //if (opts->target_gamut)
  //  mp_parse_raw_primaries(mp_null_log, opts->target_gamut, &target->color.hdr.prim);
  //int dither_depth = opts->dither_depth;
  //if (dither_depth == 0) {
  //  struct ra_swapchain* sw = p->ra_ctx->swapchain;
  //  dither_depth = sw->fns->color_depth ? sw->fns->color_depth(sw) : 0;
  //}
  //if (dither_depth > 0) {
  //  struct pl_bit_encoding* tbits = &target->repr.bits;
  //  tbits->color_depth += dither_depth - tbits->sample_depth;
  //  tbits->sample_depth = dither_depth;
  //}

  //if (opts->icc_opts->icc_use_luma) {
  //  p->icc_params.max_luma = 0.0f;
  //}
  //else {
  //  pl_color_space_nominal_luma_ex(pl_nominal_luma_params(
  //    .color = &target->color,
  //    .metadata = PL_HDR_METADATA_HDR10, // use only static HDR nits
  //    .scaling = PL_HDR_NITS,
  //    .out_max = &p->icc_params.max_luma,
  //    ));
  //}

  //pl_icc_update(p->pllog, &p->icc_profile, NULL, &p->icc_params);
  //target->icc = p->icc_profile;
  //
}


//---------------------------------------------------
//
//
//---------------------------------------------------
void CRendererPL::RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags)
{
  CLog::Log(LOGDEBUG, "RenderImpl: Enter");

  pl_frame frameOut{};
  pl_frame frameIn{};
  pl_render_params params = m_videoSettings.m_placeboOptions->getPlOptions()->params;

  CRect src = sourceRect;
  CRect dst = HasHQScaler() ? sourceRect : ApplyTransforms(CRect(destPoints[0], destPoints[2]));
  const CRect trg(0.0f, 0.0f, static_cast<float>(target.GetWidth()), static_cast<float>(target.GetHeight()));
  CWIN32Util::CropSource(src, dst, trg, m_renderOrientation);

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (!buf || !buf->IsLoaded())
    return;
  CRenderBufferImpl* buffer = static_cast<CRenderBufferImpl*>(buf);
  buffer->m_signature = m_signatureCounter++;


  frameIn.repr.levels = buffer->full_range ? PL_COLOR_LEVELS_FULL : PL_COLOR_LEVELS_LIMITED;
  frameIn.repr.sys = pl_system_from_av(buffer->color_space);
  frameIn.repr.bits = buffer->plFormat.bits;
  frameIn.repr.alpha = PL_ALPHA_UNKNOWN;
  if (buffer->hasDoviMetadata)
  {
    pl_color_repr crpr{};
    frameIn.color = m_colorSpace;
    frameIn.repr.dovi = &buffer->doviPlMetadata;
    frameIn.repr.sys = PL_COLOR_SYSTEM_DOLBYVISION;
    frameIn.color.primaries = PL_COLOR_PRIM_BT_2020;
    frameIn.color.transfer = PL_COLOR_TRC_PQ;
    frameIn.color.hdr.min_luma = pl_hdr_rescale(PL_HDR_PQ, PL_HDR_NITS, buffer->doviColor.source_min_pq / 4095.0f);
    frameIn.color.hdr.max_luma = pl_hdr_rescale(PL_HDR_PQ, PL_HDR_NITS, buffer->doviColor.source_max_pq / 4095.0f);
    if (buffer->hasDoviExt) {
      frameIn.color.hdr.max_pq_y = buffer->doviExt.l1.max_pq / 4095.0f;
      frameIn.color.hdr.avg_pq_y = buffer->doviExt.l1.avg_pq / 4095.0f;
    }
  }
  else if (buffer->hasHDR10PlusMetadata)
  {
    pl_av_hdr_metadata metadata = {};
    metadata.clm = &buffer->lightMetadata;
    metadata.mdm = &buffer->displayMetadata;
    metadata.dhp = &buffer->hdrMetadata;
    buffer->hdrColorSpace = m_colorSpace;
    pl_map_hdr_metadata(&buffer->hdrColorSpace.hdr, &metadata);

    frameIn.color = buffer->hdrColorSpace;
  }
  else
  {
    frameIn.color = m_colorSpace;
    if(frameIn.repr.sys == PL_COLOR_SYSTEM_UNKNOWN)
      frameIn.repr.sys = PL_COLOR_SYSTEM_BT_709;
  }

  //set sample dep and others

  frameIn.num_planes = buffer->plFormat.num_planes;
  frameIn.planes[0] = buffer->plplanes[0];
  frameIn.planes[1] = buffer->plplanes[1];
  frameIn.planes[2] = buffer->plplanes[2];
  
 

  // Interlacing
  if (buffer->isInterlaced)
  {
    if ((flags & RENDER_FLAG_FIELD0) && (flags & RENDER_FLAG_TOP))
    {
      frameIn.field = PL_FIELD_TOP;
      frameIn.first_field = PL_FIELD_TOP;
    }
    else if ((flags & RENDER_FLAG_FIELD1) && (flags & RENDER_FLAG_BOT))
    {
      frameIn.field = PL_FIELD_BOTTOM;
      frameIn.first_field = PL_FIELD_TOP;
    }
    else if ((flags & RENDER_FLAG_FIELD0) && (flags & RENDER_FLAG_BOT))
    {
      frameIn.field = PL_FIELD_BOTTOM;
      frameIn.first_field = PL_FIELD_BOTTOM;
    }
    else if ((flags & RENDER_FLAG_FIELD1) && (flags & RENDER_FLAG_TOP))
    {
      frameIn.field = PL_FIELD_TOP;
      frameIn.first_field = PL_FIELD_BOTTOM;
    }
  }
  else
  {
    frameIn.field = PL_FIELD_NONE;
    frameIn.first_field = PL_FIELD_NONE;
  }

  pl_color_space target_csp{ };
  DXGI_OUTPUT_DESC1 desc;
  static DX::DeviceResources::mp_dxgi_factory_ctx ctx = {0}; //cl
  if (DX::DeviceResources::Get()->get_output_desc_from_ctx(&ctx, &desc))
    target_csp = MpDxgiDescToColorSpace(&desc);

  if (target_csp.primaries == PL_COLOR_PRIM_UNKNOWN)
    target_csp.primaries = MpGetBestPrimContainer(&target_csp.hdr.prim);
  if (!pl_color_transfer_is_hdr(target_csp.transfer)) {
    // limit min_luma to 1000:1 contrast ratio in SDR mode
    if (target_csp.hdr.min_luma > PL_COLOR_SDR_WHITE / PL_COLOR_SDR_CONTRAST)
      target_csp.hdr.min_luma = 0;
  }

  // maxFALL in display metadata is in fact MaxFullFrameLuminance. Wayland
  // reports it as maxFALL directly, but this doesn't mean the same thing.
  target_csp.hdr.max_fall = 0;

  SettinglibPlaceboTargetColorspaceHintMode hintMode = (SettinglibPlaceboTargetColorspaceHintMode)m_videoSettings.m_PlaceboTargetColorspaceHintMode;
  SettinglibPlaceboTargetColorspaceHint hintFunc = (SettinglibPlaceboTargetColorspaceHint)m_videoSettings.m_PlaceboTargetColorspaceHint;
  hint = { };
  
  bool target_hint = hintFunc == SettinglibPlaceboTargetColorspaceHint::YES || (hintFunc == SettinglibPlaceboTargetColorspaceHint::AUTO && target_csp.transfer != PL_COLOR_TRC_UNKNOWN);

  bool target_unknown = target_csp.transfer == PL_COLOR_TRC_UNKNOWN;
  if(target_csp.transfer == PL_COLOR_TRC_UNKNOWN)
  {
    if (pl_color_transfer_is_hdr(frameIn.color.transfer))
      target_csp.transfer = frameIn.color.transfer;
    else
      target_csp.transfer = pl_color_space_hdr10.transfer;
  }

  struct pl_color_space* sourceCS = &frameIn.color;
  struct pl_color_space* targetCS = &target_csp;
  hint = *sourceCS;
  // Apply target contrast to the hint, this is important for SDR, because
  // libplacebo defaults to 1000:1 contrast ratio otherwise.
  if (!hint.hdr.min_luma)
    hint.hdr.min_luma = targetCS->hdr.min_luma;
  if (hintMode == SettinglibPlaceboTargetColorspaceHintMode::TARGET) {
    hint = *targetCS;
    if (pl_color_transfer_is_hdr(hint.transfer) && !pl_primaries_valid(&hint.hdr.prim))
      pl_color_space_merge(&hint, sourceCS);
    if (targetCS->hdr.max_luma) {
      hint.hdr.max_luma = targetCS->hdr.max_luma;
      hint.hdr.min_luma = targetCS->hdr.min_luma;
      hint.hdr.max_cll = targetCS->hdr.max_cll;
      hint.hdr.max_fall = targetCS->hdr.max_fall;
    }
  }
  if (hintMode == SettinglibPlaceboTargetColorspaceHintMode::SOURCE_DYNAMIC) {
    pl_nominal_luma_params p = {
      .color = &hint,
      .metadata = PL_HDR_METADATA_ANY,
      .scaling = PL_HDR_NITS,
      .out_min = !hint.hdr.min_luma ? &hint.hdr.min_luma : NULL,
      .out_max = &hint.hdr.max_luma,
    };
    pl_color_space_nominal_luma_ex(&p);

    // Set maxCLL to dynamic max luminance. Note that libplacebo uses
    // max luminace as maxCLL in practice.
    hint.hdr.max_cll = hint.hdr.max_luma;
    // Keep maxFALL from static metadata, unless its value is too high.
    // Could be set to 0, but let's keep it for now.
    if (hint.hdr.max_fall > hint.hdr.max_cll)
      hint.hdr.max_fall = 0;
  }
  // Infer missing bits now. This is important so that we don't lose
  // information after user option overrides. For example, if the user
  // sets target_trc to PQ, but the hint(source) is SDR, we want to fill
  // in SDR luminance values instead of the default PQ range.
  struct pl_color_space source_csp = *sourceCS;
  pl_color_space_infer_map(&source_csp, &hint);

  // Always prefer target luminance and transfer for inverse tone mapping
  bool inverseToneMapping = m_videoSettings.m_placeboOptions->getPlOptions()->params.color_map_params == NULL ? false : m_videoSettings.m_placeboOptions->getPlOptions()->params.color_map_params->inverse_tone_mapping;
  if (pl_color_transfer_is_hdr(targetCS->transfer) && inverseToneMapping)
  {
    hint.transfer = targetCS->transfer;
    hint.hdr.max_luma = targetCS->hdr.max_luma;
    hint.hdr.min_luma = targetCS->hdr.min_luma;
    hint.hdr.max_cll = targetCS->hdr.max_cll;
    hint.hdr.max_fall = targetCS->hdr.max_fall;
  }

  //cl if (opts->target_prim)
  //  hint.primaries = opts->target_prim;
  //if (opts->target_gamut)
  //  mp_parse_raw_primaries(mp_null_log, opts->target_gamut, &hint.hdr.prim);
  //if (opts->target_trc)
  //  hint.transfer = opts->target_trc;

  if (m_videoSettings.m_PlaceboDisplayPeakLuminance) 
    if(hint.hdr.max_luma > m_videoSettings.m_PlaceboDisplayPeakLuminance) //cl problems with sdr when peak setting set to e.g 400 for hdr but peak calculated at e.g. 203 for sdr, image is way overblown and constant untill reaching 203 (something kinks in at 203...
      hint.hdr.max_luma = m_videoSettings.m_PlaceboDisplayPeakLuminance;


  //if (opts->hdr_reference_white && !pl_color_transfer_is_hdr(hint.transfer))
  //  hint.hdr.max_luma = opts->hdr_reference_white;

  // Always set maxCLL, display uses this metadata and we shouldn't let it
  // fallback to default value.
  if (!hint.hdr.max_cll)
    hint.hdr.max_cll = hint.hdr.max_luma;

  // If tone mapping is required, adjust maxCLL and maxFALL
  if (sourceCS->hdr.max_luma > hint.hdr.max_luma || inverseToneMapping) {
    // Set maxCLL to the target luminance if it's not already lower
    if (!hint.hdr.max_cll || hint.hdr.max_luma < hint.hdr.max_cll || inverseToneMapping)
      hint.hdr.max_cll = hint.hdr.max_luma;
    // There's no reliable way to estimate maxFALL here
    hint.hdr.max_fall = 0;
  }

  if (hint.hdr.max_cll && hint.hdr.max_fall > hint.hdr.max_cll)
    hint.hdr.max_fall = 0;

  ApplyTargetContrast(&hint, hint.hdr.min_luma, m_videoSettings.m_Contrast);

  //if (p->icc_profile)
  //  hint = p->icc_profile->csp;
  //clif (opts->icc_opts->icc_use_luma) {
  //  p->icc_params.max_luma = 0.0f;
  //}
  //else {
  //  pl_nominal_luma_params p2 {
  //    .color = &hint,
  //    .metadata = PL_HDR_METADATA_HDR10, // use only static HDR nits
  //    .scaling = PL_HDR_NITS,
  //    .out_max = &p->icc_params.max_luma,
  //  };
  //  pl_color_space_nominal_luma_ex(p);
  //}
  //pl_icc_update(p->pllog, &p->icc_profile, NULL, &p->icc_params);
  // 
  //// Update again after possible max_luma change
  //if (p->icc_profile)
  //  hint = p->icc_profile->csp;
  //external_params = set_colorspace_hint(p, &hint);

  //hint.primaries = PL_COLOR_PRIM_BT_709;
  //hint.transfer = PL_COLOR_TRC_SRGB;
  //hint.hdr.max_luma = 100.0f;

  pl_swapchain_colorspace_hint(PL::PLInstance::Get()->GetSwapchain(), &hint);

  // Need to start a frame to get the actual swapchain framebuffer, which may have a different format than the hint, 
  // also fills in the frameOut.repr with the actual swapchain framebuffer format, which is needed for the conversion
  struct pl_swapchain_frame swframe;
  if (!pl_swapchain_start_frame(PL::PLInstance::Get()->GetSwapchain(), &swframe)) 
  {
    return;
  }
  pl_frame_from_swapchain(&frameOut, &swframe);
  //frameOut.color.primaries = PL_COLOR_PRIM_BT_709;
  //frameOut.color.transfer = PL_COLOR_TRC_SRGB;
  //frameOut.color.hdr.max_luma = 100.0f;

  bool valid = false;

  // Calculate target

  //if (external_params) //cl only true if sw supports set_color(), not in windows
  //  frameOut.color = hint;

  bool strict_sw_params = target_hint && true; //clp->next_opts->target_hint_strict;
  ApplyTargetOptions(&target_csp, &frameOut, hint.hdr.min_luma, strict_sw_params);
  
  
  bool clip_gamut = pl_primaries_valid(&frameOut.color.hdr.prim);
  clip_gamut = clip_gamut && frameOut.color.transfer != PL_COLOR_TRC_SCRGB;
  if (clip_gamut) {
    // Ensure resulting gamut still fits inside container
    frameOut.color.hdr.prim = pl_primaries_clip(&frameOut.color.hdr.prim,
      pl_raw_primaries_get(frameOut.color.primaries));
  }

  //if (frameOut.color.transfer == PL_COLOR_TRC_SRGB && frame->current &&
  //  ((opts->sdr_adjust_gamma == 0 && opts->target_trc == PL_COLOR_TRC_UNKNOWN) ||
  //    opts->sdr_adjust_gamma == -1))
  //{
  //  switch (frame->current->params.color.transfer) {
  //  case PL_COLOR_TRC_BT_1886:
  //  case PL_COLOR_TRC_GAMMA22:
  //  case PL_COLOR_TRC_SRGB:
  //    frameOut.color.transfer = frame->current->params.color.transfer;
  //  }
  //}
  //if (frameOut.color.transfer == PL_COLOR_TRC_SRGB) {
  //  // sRGB reference display is pure 2.2 power function, see IEC 61966-2-1-1999.
  //  if (opts->treat_srgb_as_power22 & 2)
  //    frameOut.color.transfer = PL_COLOR_TRC_GAMMA22;

  bool target_pq = !target_unknown && target_csp.transfer == PL_COLOR_TRC_PQ;
  //if (opts->treat_srgb_as_power22 & 4 && target_pq)
  if (false && target_pq)
    frameOut.color.transfer = PL_COLOR_TRC_SRGB;

  //////////////////////////

  //TODO
  //Add icc profile
  //add cache for saving time during the compiling of glsl shaders

  //wrap the intermediate texture onthe output frame
  pl_d3d11_wrap_params d3dparams =
  {
  .tex = target.Get(),
  .array_slice = 1,
  .fmt = target.GetFormat(),
  .w = (int)target.GetWidth(),
  .h = (int)target.GetHeight()
  };
  frameOut.num_planes = 1;
  frameOut.planes[0].texture = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &d3dparams);
  frameOut.planes[0].components = 4;
  frameOut.planes[0].component_mapping[0] = PL_CHANNEL_R;
  frameOut.planes[0].component_mapping[1] = PL_CHANNEL_G;
  frameOut.planes[0].component_mapping[2] = PL_CHANNEL_B;
  frameOut.planes[0].component_mapping[3] = PL_CHANNEL_A;

  frameOut.crop.x0 = dst.x1;
  frameOut.crop.x1 = dst.x2;
  frameOut.crop.y0 = dst.y1;
  frameOut.crop.y1 = dst.y2;

  frameOut.rotation = m_renderOrientation == 90 ? PL_ROTATION_90 : m_renderOrientation == 180 ? PL_ROTATION_180 : m_renderOrientation == 270 ? PL_ROTATION_270 : PL_ROTATION_0;
 

  //Without this recent version of libplacebo would spam the debug log like crazy
  //And its also set on an info level
  params.skip_target_clearing = true;
  //this data is used for the video debug renderer
  m_displayTransfer = frameOut.color.transfer;
  m_displayPrimaries = frameOut.color.primaries;
  m_videoMatrix = frameIn.repr.sys;
  pl_frame_set_chroma_location(&frameIn, m_chromaLocation);

  CLog::Log(LOGDEBUG, "RenderImpl: pl_render_image start");
  bool res = pl_render_image(PL::PLInstance::Get()->GetRenderer(), &frameIn, &frameOut, &params);
  pl_swapchain_submit_frame(PL::PLInstance::Get()->GetSwapchain());
  CLog::Log(LOGDEBUG, "RenderImpl: pl_swapchain_submit_frame exit");
  pl_tex_destroy(PL::PLInstance::Get()->GetGpu(), &frameOut.planes[0].texture);

  //pl_swapchain_swap_buffers(PL::PLInstance::Get()->GetSwapchain()); // cl don't, vsync controled from kodi...

  // cl unclear, libplacebo disabled peak detection for dolby vision in renderer.c
  //if (vo->params) {
  //  // Augment metadata with peak detection max_pq_y / avg_pq_y
  //  vo->has_peak_detect_values = pl_renderer_get_hdr_metadata(p->rr, &vo->params->color.hdr);
  //}

  sourceRect = dst; //cl?
  CLog::Log(LOGDEBUG, "RenderImpl: Exit");
}



//---------------------------------------------------
//
//
//---------------------------------------------------
void CRendererPL::ProcessHDR(CRenderBuffer* rb)
{
  //todo fix this one sometimes it crash because we dont release texture correctly during the swap
  if (m_AutoSwitchHDR && rb->primaries == AVCOL_PRI_BT2020 &&
    (rb->color_transfer == AVCOL_TRC_SMPTE2084 || rb->color_transfer == AVCOL_TRC_ARIB_STD_B67) &&
    !DX::Windowing()->IsHDROutput())
  {
    DX::Windowing()->ToggleHDR(); // Toggle display HDR ON
  }
  m_HdrType = HDR_TYPE::HDR_HDR10; //cl


  if (!DX::Windowing()->IsHDROutput())
  {
    if (m_HdrType != HDR_TYPE::HDR_NONE_SDR)
    {
      m_HdrType = HDR_TYPE::HDR_NONE_SDR;
      m_lastHdr10 = {};
    }
    return;
  }




  CRenderBufferImpl* rbpl = static_cast<CRenderBufferImpl*>(rb);
  if(0)
  {
      if (rbpl->hasDoviMetadata)
      {
        //cl not valid if residual==false
        if (rbpl->HasHdrData())
          pl_swapchain_colorspace_hint(PL::PLInstance::Get()->GetSwapchain(), &m_colorSpace);
      }

      if (rbpl->hasHDR10PlusMetadata)
      {
        pl_av_hdr_metadata metadata = {};
        pl_hdr_metadata out = {};
        metadata.clm = &rbpl->lightMetadata;
        metadata.mdm = &rbpl->displayMetadata;
        metadata.dhp = &rbpl->hdrMetadata;
        rbpl->hdrColorSpace = m_colorSpace;

        pl_map_hdr_metadata(&rbpl->hdrColorSpace.hdr, &metadata);
        pl_swapchain_colorspace_hint(PL::PLInstance::Get()->GetSwapchain(), &rbpl->hdrColorSpace);
      }
  }
      
}

bool CRendererPL::Supports(ERENDERFEATURE feature) const
{
  //TODO
  if ( (feature == RENDERFEATURE_LIBPLACEBO) ||
       (feature == RENDERFEATURE_GAMMA) ||
       (feature == RENDERFEATURE_BRIGHTNESS) ||
       (feature == RENDERFEATURE_CONTRAST) ||
       (feature == RENDERFEATURE_ROTATION) ||
       (feature == RENDERFEATURE_STRETCH) ||
       (feature == RENDERFEATURE_ZOOM) ||
       (feature == RENDERFEATURE_VERTICAL_SHIFT) ||
       (feature == RENDERFEATURE_PIXEL_RATIO))
  {
    return true;
  }
  else 
    return false;

}

bool CRendererPL::Supports(ESCALINGMETHOD method) const
{
  // cl varies depending on upscaler/downscaler
  return false;
  // return __super::Supports(method);
}

CRenderBuffer* CRendererPL::CreateBuffer()
{
  //can we get the dxva format at this moment?
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

CRendererPL::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{
  //Is this needed??
  m_widthTex = FFALIGN(width, 32);
  m_heightTex = FFALIGN(height, 32);
  //will be set on first upload
  plFormat.num_planes = -1;
  
}

CRendererPL::CRenderBufferImpl::~CRenderBufferImpl()
{
  //cl something to release on player close???
}

void CRendererPL::CRenderBufferImpl::ReleasePicture()
{
  for (int i = 0; i < plFormat.num_planes; i++)
  {
    pl_tex_destroy(PL::PLInstance::Get()->GetGpu(), &pltex[i]);
  }

  CRenderBuffer::ReleasePicture();
}

void CRendererPL::CRenderBufferImpl::AppendPicture(const VideoPicture& picture)
{
  __super::AppendPicture(picture);
  hdrDoviRpu = picture.hdrDoviRpu;
  hdrMetadata = picture.hdrMetadata;
  doviMetadata = picture.doviMetadata;
  hdrColorSpace = picture.doviColorSpace;
  doviColorRepr = picture.doviColorRepr;
  doviPlMetadata = picture.doviPlMetadata;
  disable_residual_flag = picture.disable_residual_flag;
  hasDoviMetadata = picture.hasDoviMetadata;
  hasDoviRpuMetadata = picture.hasDoviRpuMetadata;
  hasHDR10PlusMetadata = picture.hasHDR10PlusMetadata;
  doviColor = picture.doviColor;
  doviExt = picture.doviExt;
  hasDoviExt = picture.hasDoviExt;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
	const auto hw = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer); //cl hw = height width from the video buffer
    m_widthTex = hw->width;
    m_heightTex = hw->height;
   
  }
  pts = picture.pts;
  duration = picture.iDuration;
  isInterlaced = picture.iFlags & DVP_FLAG_INTERLACED;
}

bool CRendererPL::CRenderBufferImpl::GetLibplaceboFrame(pl_frame& frame)
{
  //clif (!m_bLoaded)
  //cl  return false;
  pl_color_repr crpr{};

  if (hasDoviMetadata)
  {
    crpr = doviColorRepr;
    frame.color = hdrColorSpace;
  }

  if (hasHDR10PlusMetadata)
  {
    pl_av_hdr_metadata metadata = {};
    pl_hdr_metadata out = {};
    metadata.clm = &lightMetadata;
    metadata.mdm = &displayMetadata;
    metadata.dhp = &hdrMetadata;
    
    pl_map_hdr_metadata(&hdrColorSpace.hdr, &metadata);
    frame.color.hdr = hdrColorSpace.hdr;

    
  }
  //set sample dep and others
  crpr.bits = plFormat.bits;
  frame.repr = crpr;
  
  frame.num_planes = plFormat.num_planes;
  frame.planes[0] = plplanes[0];
  frame.planes[1] = plplanes[1];
  frame.planes[2] = plplanes[2];
  
  return true;
}

bool CRendererPL::CRenderBufferImpl::UploadBuffer()
{
  if (!videoBuffer)
    return false;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
     return UploadWrapPlanes();
  }
  else
  {
    return UploadPlanes();
  }
}

bool CRendererPL::CRenderBufferImpl::UploadPlanes()
{

  // source
  uint8_t* src[3];
  int srcStrides[3];

  const AVPixelFormat buffer_format = videoBuffer->GetFormat();

  //AV_PIX_FMT_YUV420P10LE

  int out_map[4];

  const AVPixFmtDescriptor* fmtdesc = av_pix_fmt_desc_get(buffer_format);
  videoBuffer->GetPlanes(src);
  videoBuffer->GetStrides(srcStrides);
  pl_plane_data pdata[4] = { };
  
  for (int n = 0; n < pl_plane_data_from_pixfmt(pdata, &plFormat.bits, buffer_format); n++)
  {
    pdata[n].pixels = src[n];
    pdata[n].row_stride = srcStrides[n];
    pdata[n].width = n > 0 ? m_width >> 1: m_width;
    pdata[n].height = n > 0 ? m_height >> 1 : m_height;

    if (!pl_upload_plane(PL::PLInstance::Get()->GetGpu(), &plplanes[n], &pltex[n], &pdata[n]))
    {
      CLog::Log(LOGERROR, "pl_upload_plane failed");

    }
  }
  plFormat.num_planes = 3;
  m_bLoaded = true;
  return m_bLoaded;
}

bool CRendererPL::CRenderBufferImpl::UploadWrapPlanes()
{
  ComPtr<ID3D11Resource> pResource;
  ComPtr<ID3D11Texture2D> pTexture;
  D3D11_TEXTURE2D_DESC desc;
  HRESULT hr;
  unsigned arrayIdx;

  if (FAILED(GetResource(&pResource, &arrayIdx)))
  {
    CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
    return false;
  }
  
  if (plFormat.num_planes==-1)
  {
    //fill the plane data information needed for the conversion only once
    const auto dxva_buf = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer);
    DXGI_FORMAT fmt = dxva_buf->format;
    PL::PLInstance::Get()->fill_d3d_format(&plFormat,fmt);
  }

  
  hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
  pTexture->GetDesc(&desc);

  
  
  // Wrap the plane of the D3D11 texture
  for (int i = 0; i < plFormat.num_planes; i++)
  {
    pl_d3d11_wrap_params params = {};
    params.tex = pTexture.Get();
    params.w = desc.Width / plFormat.width_div[i];
    params.h = desc.Height / plFormat.height_div[i];
    params.fmt = plFormat.planes[i];
    params.array_slice = arrayIdx;
    pltex[i] = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &params);

    if (!pltex[i])
      return false;
    plplanes[i].texture = pltex[i];
    //number of components per plane example uv is 2 in d3d the alpha is always a component but not with libplacebo
    plplanes[i].components = plFormat.components[i];
    //mapping yuv planes to rgba channels
    for (int j = 0; j < 4; j++)
      plplanes[i].component_mapping[j] = plFormat.component_mapping[i][j];

  }
  m_bLoaded = true;
  return m_bLoaded;
}

bool CRendererPL::CRenderBufferImpl::HasHdrData()
{
  return (hasHDR10PlusMetadata || hasDoviMetadata || hasDoviRpuMetadata);
}


/*
* Garbage
*   if(1)
  {
    if (buffer->isInterlaced)
    {
      if(lastBufferIndex==-1)
      {
        lastBufferIndex = m_iBufferIndex;
        return;
      }

      if ((flags & RENDER_FLAG_FIELD0) && (flags & RENDER_FLAG_TOP))
      {
        frameIn.field = PL_FIELD_TOP;
        frameIn.first_field = PL_FIELD_TOP;
      }
      else if ((flags & RENDER_FLAG_FIELD1) && (flags & RENDER_FLAG_BOT))
      {
        frameIn.field = PL_FIELD_BOTTOM;
        frameIn.first_field = PL_FIELD_TOP;
      }
      else if ((flags & RENDER_FLAG_FIELD0) && (flags & RENDER_FLAG_BOT))
      {
        frameIn.field = PL_FIELD_BOTTOM;
        frameIn.first_field = PL_FIELD_BOTTOM;
      }
      else if ((flags & RENDER_FLAG_FIELD1) && (flags & RENDER_FLAG_TOP))
      {
        frameIn.field = PL_FIELD_TOP;
        frameIn.first_field = PL_FIELD_BOTTOM;
      }
      else
      {
        frameIn.field = PL_FIELD_NONE;
		frameIn.first_field = PL_FIELD_NONE;
	  }
      frameIn.prev = &prevFrame;
      prevFrame = frameIn;
    }
    else
    {
      frameIn.field = PL_FIELD_NONE;
      frameIn.first_field = PL_FIELD_NONE;
      frameIn.prev = NULL;
    }

  else
  {
  struct pl_frame_mix mix { 0 };

  pl_frame* tmp_frame[2];
  uint64_t(tmp_sig)[2];
  float tmp_ts[2];


    pl_frame frameIn2{};
    CRenderBuffer* buf2 = m_renderBuffers[ (m_iBufferIndex - 1) % m_iNumBuffers]; //cltest
    if (!buf2)
      return;
    CRenderBufferImpl* buffer2 = static_cast<CRenderBufferImpl*>(buf2);
    if (!buffer2->GetLibplaceboFrame(frameIn2))
      return;
    if(buffer2->m_signature == 0)
      return;
    frameIn.prev = &frameIn2;
    frameIn.next = NULL;

    mix.num_frames = 2;

    tmp_frame[0] = &frameIn;
    tmp_frame[1] = &frameIn2;
    tmp_ts[0] = buf->pts/1000000.0;
    tmp_ts[1] = (buf->pts+buf->duration/2.0) / 1000000.0;
    tmp_sig[0] = buffer->m_signature;
    tmp_sig[1] = buffer2->m_signature;

  }
  else
  {
    mix.num_frames = 1;
    tmp_frame[0] = &frameIn;
    tmp_sig[0] = buffer->m_signature;
    tmp_ts[0] = buf->pts / 1000000.0;
  }

  mix.frames = const_cast<const pl_frame**>(tmp_frame);
  mix.signatures = tmp_sig;
  mix.timestamps = tmp_ts;
  mix.vsync_duration = 1.0; //buf->duration/1000000.0;

  bool res = pl_render_image_mix(PL::PLInstance::Get()->GetRenderer(), &mix, &frameOut, &params);
  }
*/