/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerVideo.h"

#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <iomanip>
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <sstream>

using namespace std::chrono_literals;

class CDVDMsgVideoCodecChange : public CDVDMsg
{
public:
  CDVDMsgVideoCodecChange(const CDVDStreamInfo& hints, std::unique_ptr<CDVDVideoCodec> codec)
    : CDVDMsg(GENERAL_STREAMCHANGE), m_codec(std::move(codec)), m_hints(hints)
  {}
  ~CDVDMsgVideoCodecChange() override = default;

  std::unique_ptr<CDVDVideoCodec> m_codec;
  CDVDStreamInfo  m_hints;
};

CVideoPlayerVideo::CVideoPlayerVideo(CDVDClock* pClock,
                                     CDVDOverlayContainer* pOverlayContainer,
                                     CDVDMessageQueue& parent,
                                     CRenderManager& renderManager,
                                     CProcessInfo& processInfo,
                                     double messageQueueTimeSize)
  : CThread("VideoPlayerVideo"),
    IDVDStreamPlayerVideo(processInfo),
    m_messageQueue("video"),
    m_messageParent(parent),
    m_renderManager(renderManager)
{
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  m_speed = DVD_PLAYSPEED_NORMAL;

  m_bRenderSubs = false;
  m_paused = false;
  m_syncState = IDVDStreamPlayer::SYNC_STARTING;
  m_iSubtitleDelay = 0;
  m_iLateFrames = 0;
  m_iDroppedRequest = 0;
  m_fForcedAspectRatio = 0;

  const int sizeMB = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_VIDEOPLAYER_QUEUEDATASIZE);

  m_messageQueue.SetMaxDataSize(sizeMB * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(messageQueueTimeSize);

  m_iDroppedFrames = 0;
  m_fFrameRate = 25;
  m_fStableFrameRate = 0.0;
  m_iFrameRateCount = 0;
  m_bAllowDrop = false;
  m_iFrameRateErr = 0;
  m_iFrameRateLength = 0;
  m_bFpsInvalid = false;
}

CVideoPlayerVideo::~CVideoPlayerVideo()
{
  m_bAbortOutput = true;
  StopThread();
}

double CVideoPlayerVideo::GetOutputDelay()
{
  double time = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET);
  if( m_fFrameRate )
    time = (time * DVD_TIME_BASE) / m_fFrameRate;
  else
    time = 0.0;

  if( m_speed != 0 )
    time = time * DVD_PLAYSPEED_NORMAL / abs(m_speed);

  return time;
}

bool CVideoPlayerVideo::OpenStream(CDVDStreamInfo hint)
{
  if (hint.flags & AV_DISPOSITION_ATTACHED_PIC)
    return false;
  if (!hint.extradata)
  {
    // codecs which require extradata
    // clang-format off
    if (hint.codec == AV_CODEC_ID_NONE ||
        hint.codec == AV_CODEC_ID_MPEG1VIDEO ||
        hint.codec == AV_CODEC_ID_MPEG2VIDEO ||
        (hint.codec == AV_CODEC_ID_H264 && (hint.codec_tag == 0 || hint.codec_tag == MKTAG('a','v','c','1') || hint.codec_tag == MKTAG('a','v','c','2'))) ||
        hint.codec == AV_CODEC_ID_HEVC ||
        hint.codec == AV_CODEC_ID_MPEG4 ||
        hint.codec == AV_CODEC_ID_WMV3 ||
        hint.codec == AV_CODEC_ID_VC1)
    {
      CLog::LogF(LOGERROR, "Codec id {} require extradata.", hint.codec);
      return false;
    }
    // clang-format on
  }

  CLog::Log(LOGINFO, "Creating video codec: {}", avcodec_get_name(hint.codec));

  if (m_messageQueue.IsInited())
  {
    if (m_pVideoCodec && !m_processInfo.IsVideoHwDecoder())
    {
      hint.codecOptions |= CODEC_ALLOW_FALLBACK;
    }

    std::unique_ptr<CDVDVideoCodec> codec = CDVDFactoryCodec::CreateVideoCodec(hint, m_processInfo);
    if (!codec)
    {
      CLog::Log(LOGINFO, "CVideoPlayerVideo::OpenStream - could not open video codec");
    }

    SendMessage(std::make_shared<CDVDMsgVideoCodecChange>(hint, std::move(codec)), 0);
  }
  else
  {
    m_processInfo.ResetVideoCodecInfo();
    hint.codecOptions |= CODEC_ALLOW_FALLBACK;

    std::unique_ptr<CDVDVideoCodec> codec = CDVDFactoryCodec::CreateVideoCodec(hint, m_processInfo);
    if (!codec)
    {
      CLog::Log(LOGERROR, "CVideoPlayerVideo::OpenStream - could not open video codec");
      return false;
    }

    OpenStream(hint, std::move(codec));
    CLog::Log(LOGINFO, "Creating video thread");
    m_messageQueue.Init();
    m_processInfo.SetLevelVQ(0);
    Create();
  }
  return true;
}

void CVideoPlayerVideo::OpenStream(CDVDStreamInfo& hint, std::unique_ptr<CDVDVideoCodec> codec)
{
  CLog::Log(LOGDEBUG, "CVideoPlayerVideo::OpenStream - open stream with codec: {}",
            avcodec_get_name(hint.codec));

  m_processInfo.GetVideoBufferManager().ReleasePools();

  //reported fps is usually not completely correct
  if (hint.fpsrate && hint.fpsscale)
  {
    m_fFrameRate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration(
                                       (double)DVD_TIME_BASE * hint.fpsscale / hint.fpsrate);

    m_bFpsInvalid = false;
    m_processInfo.SetVideoFps(static_cast<float>(m_fFrameRate));
  }
  else
  {
    m_fFrameRate = 25;
    m_bFpsInvalid = true;
    m_processInfo.SetVideoFps(0);
  }

  m_ptsTracker.ResetVFRDetection();
  ResetFrameRateCalc();

  m_iDroppedRequest = 0;
  m_iLateFrames = 0;

  if( m_fFrameRate > 120 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGERROR,
              "CVideoPlayerVideo::OpenStream - Invalid framerate {}, using forced 25fps and just "
              "trust timestamps",
              (int)m_fFrameRate);
    m_fFrameRate = 25;
  }

  // use aspect in stream if available
  if (hint.forced_aspect)
    m_fForcedAspectRatio = static_cast<float>(hint.aspect);
  else
    m_fForcedAspectRatio = 0.0f;

  if (m_pVideoCodec && m_pVideoCodec->Reconfigure(hint))
  {
    // reuse old decoder
    codec = std::move(m_pVideoCodec);
  }

  m_pVideoCodec.reset();

  if (!codec)
  {
    CLog::Log(LOGINFO, "Creating video codec: {}", avcodec_get_name(hint.codec));
    hint.codecOptions |= CODEC_ALLOW_FALLBACK;
    codec = CDVDFactoryCodec::CreateVideoCodec(hint, m_processInfo);
    if (!codec)
    {
      CLog::Log(LOGERROR, "CVideoPlayerVideo::OpenStream - could not open video codec");
      m_messageParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
      StopThread();
    }
  }

  m_pVideoCodec = std::move(codec);
  m_hints = hint;
  m_stalled = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_rewindStalled = false;
  m_packets.clear();
  m_syncState = IDVDStreamPlayer::SYNC_STARTING;
  m_renderManager.ShowVideo(false);
}

void CVideoPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0)
  {
    SendMessage(std::make_shared<CDVDMsg>(CDVDMsg::VIDEO_DRAIN), 0);
    m_messageQueue.WaitUntilEmpty();
  }

  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGINFO, "waiting for video thread to exit");

  m_bAbortOutput = true;
  StopThread();

  m_messageQueue.End();

  CLog::Log(LOGINFO, "deleting video codec");
  m_pVideoCodec.reset();

  if (m_picture.videoBuffer)
  {
    m_picture.videoBuffer->Release();
    m_picture.videoBuffer = nullptr;
  }
}

bool CVideoPlayerVideo::AcceptsData() const
{
  bool full = m_messageQueue.IsFull();
  return !full;
}

bool CVideoPlayerVideo::HasData() const
{
  return m_messageQueue.GetDataSize() > 0;
}

bool CVideoPlayerVideo::IsInited() const
{
  return m_messageQueue.IsInited();
}

inline void CVideoPlayerVideo::SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority)
{
  m_messageQueue.Put(pMsg, priority);
  m_processInfo.SetLevelVQ(m_messageQueue.GetLevel());
}

inline void CVideoPlayerVideo::SendMessageBack(const std::shared_ptr<CDVDMsg>& pMsg, int priority)
{
  m_messageQueue.PutBack(pMsg, priority);
  m_processInfo.SetLevelVQ(m_messageQueue.GetLevel());
}

inline void CVideoPlayerVideo::FlushMessages()
{
  m_messageQueue.Flush();
  m_processInfo.SetLevelVQ(m_messageQueue.GetLevel());
}

inline MsgQueueReturnCode CVideoPlayerVideo::GetMessage(std::shared_ptr<CDVDMsg>& pMsg,
                                                        std::chrono::milliseconds timeout,
                                                        int& priority)
{
  MsgQueueReturnCode ret = m_messageQueue.Get(pMsg, timeout, priority);
  m_processInfo.SetLevelVQ(m_messageQueue.GetLevel());
  return ret;
}

void CVideoPlayerVideo::Process()
{
  CLog::Log(LOGINFO, "running thread: video_thread");

  double pts = 0;
  double frametime = (double)DVD_TIME_BASE / m_fFrameRate;

  bool bRequestDrop = false;
  int iDropDirective;
  bool onlyPrioMsgs = false;

  m_videoStats.Start();
  m_droppingStats.Reset();
  m_iDroppedFrames = 0;
  m_rewindStalled = false;
  m_outputSate = OUTPUT_NORMAL;

  while (!m_bStop)
  {
    auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<double, std::micro>(m_stalled ? frametime : frametime * 10));
    int iPriority = 0;

    if (m_syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
      iPriority = 1;

    if (m_paused)
      iPriority = 1;

    if (onlyPrioMsgs)
    {
      iPriority = 1;
      timeout = 1ms;
    }

    std::shared_ptr<CDVDMsg> pMsg;
    MsgQueueReturnCode ret = GetMessage(pMsg, timeout, iPriority);

    onlyPrioMsgs = false;

    if (MSGQ_IS_ERROR(ret))
    {
      if (!m_messageQueue.ReceivedAbortRequest())
        CLog::Log(LOGERROR, "MSGQ_IS_ERROR returned true ({})", ret);

      break;
    }
    else if (ret == MSGQ_TIMEOUT)
    {
      if (m_outputSate == OUTPUT_AGAIN &&
          m_picture.videoBuffer)
      {
        m_outputSate = OutputPicture(&m_picture);
        if (m_outputSate == OUTPUT_AGAIN)
        {
          onlyPrioMsgs = true;
          continue;
        }
      }
      // don't ask for a new frame if we can't deliver it to renderer
      else if ((m_speed != DVD_PLAYSPEED_PAUSE ||
                m_processInfo.IsFrameAdvance() ||
                m_syncState != IDVDStreamPlayer::SYNC_INSYNC) && !m_paused)
      {
        if (ProcessDecoderOutput(frametime, pts))
        {
          onlyPrioMsgs = true;
          continue;
        }
      }

      // if we only wanted priority messages, this isn't a stall
      if (iPriority)
        continue;

      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if (!m_stalled)
      {
        // squeeze pictures out
        while (!m_bStop && m_pVideoCodec)
        {
          m_pVideoCodec->SetCodecControl(DVD_CODEC_CTRL_DRAIN);
          if (!ProcessDecoderOutput(frametime, pts))
            break;
        }

        CLog::Log(LOGDEBUG, "CVideoPlayerVideo - Stillframe detected, switching to forced {:f} fps",
                  m_fFrameRate);
        m_stalled = true;
        pts += frametime * 4;
      }

      // Waiting timed out, output last picture
      if (m_picture.videoBuffer)
      {
        m_picture.pts = pts;
        m_outputSate = OutputPicture(&m_picture);
        pts += frametime;
      }

      continue;
    }

    if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if (std::static_pointer_cast<CDVDMsgGeneralSynchronize>(pMsg)->Wait(100ms, SYNCSOURCE_VIDEO))
      {
        CLog::Log(LOGDEBUG, "CVideoPlayerVideo - CDVDMsg::GENERAL_SYNCHRONIZE");
      }
      else
        SendMessage(pMsg, 1); /* push back as prio message, to process other prio messages */
      m_droppingStats.Reset();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    {
      pts = std::static_pointer_cast<CDVDMsgDouble>(pMsg)->m_value;

      m_syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_droppingStats.Reset();
      m_rewindStalled = false;
      m_renderManager.ShowVideo(true);

      CLog::Log(LOGDEBUG, "CVideoPlayerVideo - CDVDMsg::GENERAL_RESYNC({:f})", pts);
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_SET_ASPECT))
    {
      CLog::Log(LOGDEBUG, "CVideoPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT");
      m_fForcedAspectRatio = static_cast<float>(*std::static_pointer_cast<CDVDMsgDouble>(pMsg));
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      if(m_pVideoCodec)
        m_pVideoCodec->Reset();

      if (m_picture.videoBuffer)
      {
        m_picture.videoBuffer->Release();
        m_picture.videoBuffer = nullptr;
      }
      m_packets.clear();
      m_droppingStats.Reset();
      m_syncState = IDVDStreamPlayer::SYNC_STARTING;
      m_renderManager.ShowVideo(false);
      m_rewindStalled = false;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private message sent by (CVideoPlayerVideo::Flush())
    {
      bool sync = std::static_pointer_cast<CDVDMsgBool>(pMsg)->m_value;
      if(m_pVideoCodec)
        m_pVideoCodec->Reset();

      if (m_picture.videoBuffer)
      {
        m_picture.videoBuffer->Release();
        m_picture.videoBuffer = nullptr;
      }
      m_packets.clear();
      pts = 0;
      m_rewindStalled = false;

      m_ptsTracker.Flush();
      //we need to recalculate the framerate
      //! @todo this needs to be set on a streamchange instead
      ResetFrameRateCalc();
      m_droppingStats.Reset();

      m_stalled = true;
      if (sync)
      {
        m_syncState = IDVDStreamPlayer::SYNC_STARTING;
        m_renderManager.ShowVideo(false);
      }

      m_renderManager.DiscardBuffer();
      FlushMessages();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = std::static_pointer_cast<CDVDMsgInt>(pMsg)->m_value;
      if (m_pVideoCodec)
        m_pVideoCodec->SetSpeed(m_speed);

      m_droppingStats.Reset();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      auto msg = std::static_pointer_cast<CDVDMsgVideoCodecChange>(pMsg);

      while (!m_bStop && m_pVideoCodec)
      {
        m_pVideoCodec->SetCodecControl(DVD_CODEC_CTRL_DRAIN);
        bool cont = ProcessDecoderOutput(frametime, pts);

        if (!cont)
          break;
      }

      OpenStream(msg->m_hints, std::move(msg->m_codec));
      msg->m_codec = NULL;
      if (m_picture.videoBuffer)
      {
        m_picture.videoBuffer->Release();
        m_picture.videoBuffer = nullptr;
      }
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_DRAIN))
    {
      while (!m_bStop && m_pVideoCodec)
      {
        m_pVideoCodec->SetCodecControl(DVD_CODEC_CTRL_DRAIN);
        if (!ProcessDecoderOutput(frametime, pts))
          break;
      }
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_PAUSE))
    {
      m_paused = std::static_pointer_cast<CDVDMsgBool>(pMsg)->m_value;
      CLog::Log(LOGDEBUG, "CVideoPlayerVideo - CDVDMsg::GENERAL_PAUSE: {}", m_paused);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_REQUEST_STATE))
    {
      SStateMsg msg;
      msg.player = VideoPlayer_VIDEO;
      msg.syncState = m_syncState;
      m_messageParent.Put(
          std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, msg));
    }
    else if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      DemuxPacket* pPacket = std::static_pointer_cast<CDVDMsgDemuxerPacket>(pMsg)->GetPacket();
      bool bPacketDrop = std::static_pointer_cast<CDVDMsgDemuxerPacket>(pMsg)->GetPacketDrop();

      if (m_stalled)
      {
        CLog::Log(LOGDEBUG, "CVideoPlayerVideo - Stillframe left, switching to normal playback");
        m_stalled = false;
      }

      bRequestDrop = false;
      iDropDirective = CalcDropRequirement(pts);
      if ((iDropDirective & DROP_VERYLATE) &&
           m_bAllowDrop &&
          !bPacketDrop)
      {
        bRequestDrop = true;
      }
      if (iDropDirective & DROP_DROPPED)
      {
        m_iDroppedFrames++;
        m_ptsTracker.Flush();
      }
      if (m_messageQueue.GetDataSize() == 0 ||  m_speed < 0)
      {
        bRequestDrop = false;
        m_iDroppedRequest = 0;
        m_iLateFrames = 0;
      }

      int codecControl = 0;
      if (iDropDirective & DROP_BUFFER_LEVEL)
        codecControl |= DVD_CODEC_CTRL_HURRY;
      if (m_speed > DVD_PLAYSPEED_NORMAL)
        codecControl |= DVD_CODEC_CTRL_NO_POSTPROC;
      if (bPacketDrop)
        codecControl |= DVD_CODEC_CTRL_DROP;
      if (bRequestDrop)
        codecControl |= DVD_CODEC_CTRL_DROP_ANY;
      if (!m_renderManager.Supports(RENDERFEATURE_ROTATION))
        codecControl |= DVD_CODEC_CTRL_ROTATE;
      m_pVideoCodec->SetCodecControl(codecControl);

      if (m_pVideoCodec->AddData(*pPacket))
      {
        // buffer packets so we can recover should decoder flush for some reason
        if (m_pVideoCodec->GetConvergeCount() > 0)
        {
          m_packets.emplace_back(pMsg, 0);
          if (m_packets.size() > m_pVideoCodec->GetConvergeCount() ||
              m_packets.size() * frametime > DVD_SEC_TO_TIME(10))
            m_packets.pop_front();
        }

        m_videoStats.AddSampleBytes(pPacket->iSize);

        if (ProcessDecoderOutput(frametime, pts))
        {
          onlyPrioMsgs = true;
        }
      }
      else
      {
        SendMessageBack(pMsg);
        onlyPrioMsgs = true;
      }
    }
  }
}

class PllTimestampEstimator {
private:
  double est_pts = -1.0;       // Our current smooth timeline position
  double est_duration = 0.0;   // Our current estimated frame duration (velocity)
  double last_raw_pts = -1.0;

  // Tuning parameters
  const double alpha = 0.1;    // Controls how fast we adjust to frame duration changes
  const double beta = 0.1;     // Controls how aggressively we correct position errors
public:
  double EstimateNextTimestamp(double raw_mkv_pts_seconds) {
	// 1. Auto Seek & Startup Detection
	if(est_pts < 0.0 || std::abs(raw_mkv_pts_seconds - last_raw_pts) > 0.5) {
	  est_pts = raw_mkv_pts_seconds;
	  last_raw_pts = raw_mkv_pts_seconds;
	  // Seed an initial guess for frame duration (e.g., ~33ms for 30fps)
	  est_duration = 0.033;
	  return raw_mkv_pts_seconds;
	}
	last_raw_pts = raw_mkv_pts_seconds;

	// 2. Predict step: where do we *think* the timeline should be?
	double predicted_pts = est_pts + est_duration;

	// 3. Error measurement: how far off is the 1ms-rounded container?
	double error = raw_mkv_pts_seconds - predicted_pts;

	// 4. Update step: Subtly nudge both our timeline position and our velocity
	est_pts = predicted_pts + (beta * error);
	est_duration = est_duration + (alpha * error);

	return est_pts;
  }

  void HardReset() {
	est_pts = -1.0;
	est_duration = 0.0;
	last_raw_pts = -1.0;
  }
};
class UniversalPTSUpsampler {
public:
  UniversalPTSUpsampler() {
	reset();
  }

  void reset() {
	is_initialized_ = false;
	frame_count_ = 0;
	last_raw_pts_ = 0;
	est_pts_ = 0.0;
	est_duration_us_ = 0.0;
	P_pts_ = 1000.0;
	P_dur_ = 1000.0;
	startup_durations_.clear();
	last_returned_pts_ = 0;
  }

  // Input: Raw PTS in microseconds (truncated to 1ms boundaries)
  // Output: Smooth, upsampled double precision PTS in microseconds
  double update(int64_t raw_pts_us) {
	frame_count_++;

	// --- ADAPTIVE SEEK DETECTION ---
	if(is_initialized_) {
	  // Use the learned frame duration to set a strict seek boundary.
	  // If the incoming frame deviates by more than 2.5 frame periods, it's a seek.
	  // (At 60fps, this triggers if the gap deviates by > ~41ms).
	  double deviation = std::abs(static_cast<double>(raw_pts_us) - (est_pts_ + est_duration_us_));

	  if(deviation > (est_duration_us_ * 2.5)) {
		reset();
		frame_count_ = 1;
	  }
	}

	// --- PHASE 1: RAPID BOOTSTRAPPING (Works for any frame rate) ---
	if(!is_initialized_) {
	  if(frame_count_ == 1) {
		est_pts_ = static_cast<double>(raw_pts_us);
		last_raw_pts_ = raw_pts_us;
		return est_pts_;
	  }

	  double instant_dur = static_cast<double>(raw_pts_us - last_raw_pts_);
	  startup_durations_.push_back(instant_dur);
	  last_raw_pts_ = raw_pts_us;

	  if(frame_count_ == 6) {
		std::sort(startup_durations_.begin(), startup_durations_.end());

		// Extract median step size (Bypasses the 1ms truncation edge cases)
		est_duration_us_ = startup_durations_ [startup_durations_.size() / 2];

		// Refine median toward the mathematical fractional average
		double sum = 0;
		for(double d : startup_durations_) sum += d;
		double avg = sum / startup_durations_.size();
		if(std::abs(est_duration_us_ - avg) < 1000.0) {
		  est_duration_us_ = avg; // Smoothly locks onto ~16666.67us or ~41666.67us
		}

		est_pts_ = static_cast<double>(raw_pts_us);
		is_initialized_ = true;
	  }

	  return static_cast<double>(raw_pts_us);
	}

	// --- PHASE 2: HIGH-INERTIA KALMAN TRACKING ---
	const double Q_pts = 5.0;
	const double Q_dur = 0.05;
	const double R_pts = 500000.0; // Heavily penalizes the 1ms (1000us) quantization error

	// 1. Predict
	double pred_pts = est_pts_ + est_duration_us_;
	double pred_duration = est_duration_us_;

	P_pts_ += P_dur_ + Q_pts;
	P_dur_ += Q_dur;

	// 2. Kalman Gain
	double K = P_pts_ / (P_pts_ + R_pts);

	// 3. Update
	double measurement = static_cast<double>(raw_pts_us);
	est_pts_ = pred_pts + K * (measurement - pred_pts);
	est_duration_us_ = pred_duration + (K * 0.05) * (measurement - pred_pts);

	P_pts_ = (1.0 - K) * P_pts_;

	// --- PHASE 3: ZERO-OVERRUN & MONOTONICITY GUARD ---
	// 1. Never let the smoothed value drift ahead of the raw measurement 
	//    by more than the maximum possible 1ms truncation window (500us radius).
	double max_allowed_pts = static_cast<double>(raw_pts_us) + 500.0;
	if(est_pts_ > max_allowed_pts) {
	  est_pts_ = max_allowed_pts;
	}

	// 2. Strict Monotonicity: Never let the timeline go backward or freeze.
	//    Ensure it always moves forward by at least a tiny fraction (e.g., 1 microsecond).
	if(est_pts_ <= last_returned_pts_) {
	  est_pts_ = last_returned_pts_ + 1.0;
	}

	last_returned_pts_ = est_pts_;
	return est_pts_;
  }

private:
  bool is_initialized_;
  int64_t frame_count_;
  int64_t last_raw_pts_;
  std::vector<double> startup_durations_;

  double est_pts_;
  double est_duration_us_;
  double P_pts_;
  double P_dur_;
  double last_returned_pts_;
};

class RuntimeFlagPTSUpsampler {
public:
  RuntimeFlagPTSUpsampler() {
	// Initialize to progressive defaults
	reset(false);
  }

  void reset(bool is_interlaced) {
	is_initialized_ = false;
	frame_count_ = 0;
	est_pts_ = 0.0;
	last_returned_pts_ = -1.0;

	// Kalman States
	est_duration_us_ = 0.0;
	P_pts_ = 1000.0;
	P_dur_ = 1000.0;

	is_interlaced_cadence_ = is_interlaced;
	update_cadence_bounds();
  }

  // Input: Raw microsecond PTS, and the current structural state flag
  // Output: Smooth upsampled double precision microsecond PTS
  double update(int64_t raw_pts_us, bool is_interlaced) {
	frame_count_++;

	// --- MID-STREAM MODE SWITCH DETECTION ---
	// If the pipeline changes the flag on the fly, adaptively shift tracking modes
	if(is_initialized_ && (is_interlaced != is_interlaced_cadence_)) {
	  is_interlaced_cadence_ = is_interlaced;

	  if(is_interlaced_cadence_) {
		// Progressive -> Interlaced: Seed fields using the last known stable velocity
		update_cadence_bounds();
	  }
	  else {
		// Interlaced -> Progressive: Re-estimate baseline velocity from the last step
		est_duration_us_ = (cadence_short_us_ + cadence_long_us_) / 2.0;
	  }
	}

	// --- ADAPTIVE SEEK DETECTION ---
	if(is_initialized_) {
	  double max_expected = is_interlaced_cadence_ ? cadence_long_us_ : est_duration_us_;
	  double deviation = std::abs(static_cast<double>(raw_pts_us) - (est_pts_ + max_expected));

	  if(deviation > (max_expected * 2.5)) {
		reset(is_interlaced); // Flush timeline, maintain current flag mode
		frame_count_ = 1;
	  }
	}

	// --- PHASE 1: INITIALIZATION ---
	if(!is_initialized_) {
	  est_pts_ = static_cast<double>(raw_pts_us);
	  is_interlaced_cadence_ = is_interlaced;
	  update_cadence_bounds();
	  is_initialized_ = true;
	  return est_pts_;
	}

	// --- PHASE 2: CADENCE-AWARE PREDICTION ---
	double current_step_prediction = est_duration_us_;

	if(is_interlaced_cadence_) {
	  // Predict both field step branches and select the mathematical closest node
	  double pred_short = est_pts_ + cadence_short_us_;
	  double pred_long = est_pts_ + cadence_long_us_;

	  if(std::abs(static_cast<double>(raw_pts_us) - pred_short) <
		std::abs(static_cast<double>(raw_pts_us) - pred_long)) {
		current_step_prediction = cadence_short_us_;
	  }
	  else {
		current_step_prediction = cadence_long_us_;
	  }
	}
	else if(frame_count_ == 2) {
	  // Progressive instant baseline lock between frame 1 and frame 2
	  est_duration_us_ = static_cast<double>(raw_pts_us) - est_pts_;
	  current_step_prediction = est_duration_us_;
	}

	// High-inertia Kalman filtering execution
	const double Q_pts = 5.0;
	const double Q_dur = 0.05;
	const double R_pts = 500000.0;

	double pred_pts = est_pts_ + current_step_prediction;
	P_pts_ += P_dur_ + Q_pts;
	P_dur_ += Q_dur;

	double K = P_pts_ / (P_pts_ + R_pts);
	double measurement = static_cast<double>(raw_pts_us);

	est_pts_ = pred_pts + K * (measurement - pred_pts);

	// Dynamically adjust internal clock references to follow target drift
	if(!is_interlaced_cadence_) {
	  est_duration_us_ += (K * 0.05) * (measurement - pred_pts);
	}
	else {
	  double error = (measurement - pred_pts) * K * 0.02;
	  cadence_short_us_ += error;
	  cadence_long_us_ += error;
	}

	P_pts_ = (1.0 - K) * P_pts_;

	// --- PHASE 3: BOUNDARY & MONOTONICITY GUARD ---
	double max_allowed_pts = static_cast<double>(raw_pts_us) + 500.0;
	if(est_pts_ > max_allowed_pts) {
	  est_pts_ = max_allowed_pts;
	}

	if(last_returned_pts_ >= 0.0 && est_pts_ <= last_returned_pts_) {
	  est_pts_ = last_returned_pts_ + 1.0;
	}

	last_returned_pts_ = est_pts_;
	return est_pts_;
  }

private:
  void update_cadence_bounds() {
	if(is_interlaced_cadence_) {
	  // Default 3:2 pulldown boundaries if initialized fresh
	  cadence_short_us_ = 33333.33;
	  cadence_long_us_ = 50000.00;
	}
	else {
	  cadence_short_us_ = 0.0;
	  cadence_long_us_ = 0.0;
	}
  }

  bool is_initialized_;
  int64_t frame_count_;
  double est_pts_;
  double est_duration_us_;
  double P_pts_;
  double P_dur_;
  double last_returned_pts_;

  bool is_interlaced_cadence_;
  double cadence_short_us_;
  double cadence_long_us_;
}; 

class UniversalFlagPTSUpsampler {
public:
  UniversalFlagPTSUpsampler() {
	reset(false);
  }

  void reset(bool is_interlaced) {
	is_initialized_ = false;
	frame_count_ = 0;
	est_pts_ = 0.0;
	last_returned_pts_ = -1.0;
	last_raw_pts_ = 0;

	// Kalman States
	est_duration_us_ = 33333.33; // Sensible 30fps baseline default
	P_pts_ = 1000.0;
	P_dur_ = 1000.0;

	is_interlaced_cadence_ = is_interlaced;
	cadence_short_us_ = 0.0;
	cadence_long_us_ = 0.0;
  }

  double update(int64_t raw_pts_us, bool is_interlaced) {
	frame_count_++;

	// --- MID-STREAM MODE SWITCH DETECTION ---
	if(is_initialized_ && (is_interlaced != is_interlaced_cadence_)) {
	  is_interlaced_cadence_ = is_interlaced;
	  if(!is_interlaced_cadence_) {
		// Interlaced -> Progressive: Collapse back to a single average velocity
		est_duration_us_ = (cadence_short_us_ + cadence_long_us_) / 2.0;
	  }
	  else {
		// Progressive -> Interlaced: Initialize cadence limits around current baseline
		cadence_short_us_ = est_duration_us_;
		cadence_long_us_ = est_duration_us_;
	  }
	}

	// --- ADAPTIVE SEEK DETECTION ---
	if(is_initialized_) {
	  double max_expected = is_interlaced_cadence_ ? cadence_long_us_ : est_duration_us_;
	  double deviation = std::abs(static_cast<double>(raw_pts_us) - (est_pts_ + max_expected));

	  if(deviation > (max_expected * 2.5)) {
		reset(is_interlaced);
		frame_count_ = 1;
	  }
	}

	// --- PHASE 1: INITIALIZATION ---
	if(!is_initialized_) {
	  est_pts_ = static_cast<double>(raw_pts_us);
	  last_raw_pts_ = raw_pts_us;
	  is_interlaced_cadence_ = is_interlaced;

	  // Seed base guesses
	  cadence_short_us_ = 33333.33;
	  cadence_long_us_ = 33333.33;

	  is_initialized_ = true;
	  return est_pts_;
	}

	// On the second frame, catch the very first delta to instantly set up our tracking velocities
	if(frame_count_ == 2) {
	  double initial_delta = static_cast<double>(raw_pts_us - last_raw_pts_);
	  if(initial_delta > 0.0) {
		est_duration_us_ = initial_delta;
		cadence_short_us_ = initial_delta;
		cadence_long_us_ = initial_delta;
	  }
	}
	last_raw_pts_ = raw_pts_us;

	// --- PHASE 2: CADENCE-AWARE PREDICTION ---
	double current_step_prediction = est_duration_us_;

	if(is_interlaced_cadence_) {
	  // Check if the current frame aligns better with the short-step or long-step timeline
	  double pred_short = est_pts_ + cadence_short_us_;
	  double pred_long = est_pts_ + cadence_long_us_;

	  if(std::abs(static_cast<double>(raw_pts_us) - pred_short) <
		std::abs(static_cast<double>(raw_pts_us) - pred_long)) {
		current_step_prediction = cadence_short_us_;
	  }
	  else {
		current_step_prediction = cadence_long_us_;
	  }
	}

	// Precision Auto-Detection: check if data is 1ms truncated or high precision
	bool is_truncated = (raw_pts_us % 1000 == 0);
	const double R_pts = is_truncated ? 500000.0 : 10.0;
	const double Q_pts = 5.0;
	const double Q_dur = 0.05;

	double pred_pts = est_pts_ + current_step_prediction;
	P_pts_ += P_dur_ + Q_pts;
	P_dur_ += Q_dur;

	double K = P_pts_ / (P_pts_ + R_pts);
	double measurement = static_cast<double>(raw_pts_us);

	est_pts_ = pred_pts + K * (measurement - pred_pts);

	// --- PHASE 3: DYNAMIC VELOCITY FEEDBACK LEARNING ---
	double error = (measurement - pred_pts) * K;
	if(!is_interlaced_cadence_) {
	  est_duration_us_ += error * 0.05;
	}
	else {
	  // Adaptive Cadence Multi-Threading:
	  // Only update the specific cadence lane that matched this frame's reality.
	  // This prevents a uniform stream from shifting, while allowing 3:2 pulldown
	  // to expand into separate 33ms and 50ms branches smoothly.
	  if(current_step_prediction == cadence_short_us_) {
		cadence_short_us_ += error * 0.04;
	  }
	  else {
		cadence_long_us_ += error * 0.04;
	  }

	  // If the values ever cross over structurally, sort them to maintain order
	  if(cadence_short_us_ > cadence_long_us_) {
		std::swap(cadence_short_us_, cadence_long_us_);
	  }
	}

	P_pts_ = (1.0 - K) * P_pts_;

	// --- PHASE 4: BOUNDARY & MONOTONICITY GUARD ---
	double max_allowed_pts = static_cast<double>(raw_pts_us) + 500.0;
	if(est_pts_ > max_allowed_pts) {
	  est_pts_ = max_allowed_pts;
	}

	if(last_returned_pts_ >= 0.0 && est_pts_ <= last_returned_pts_) {
	  est_pts_ = last_returned_pts_ + 1.0;
	}

	last_returned_pts_ = est_pts_;
	return est_pts_;
  }

private:
  bool is_initialized_;
  int64_t frame_count_;
  int64_t last_raw_pts_;
  double est_pts_;
  double est_duration_us_;
  double P_pts_;
  double P_dur_;
  double last_returned_pts_;

  bool is_interlaced_cadence_;
  double cadence_short_us_;
  double cadence_long_us_;
};

UniversalFlagPTSUpsampler upSampler;


bool CVideoPlayerVideo::ProcessDecoderOutput(double &frametime, double &pts)
{
  CDVDVideoCodec::VCReturn decoderState = m_pVideoCodec->GetPicture(&m_picture);

  if (decoderState == CDVDVideoCodec::VC_BUFFER)
  {
    return false;
  }

  // if decoder was flushed, we need to seek back again to resume rendering
  if (decoderState == CDVDVideoCodec::VC_FLUSHED)
  {
    CLog::Log(LOGDEBUG, "CVideoPlayerVideo - video decoder was flushed");
    while (!m_packets.empty())
    {
      auto msg = std::static_pointer_cast<CDVDMsgDemuxerPacket>(m_packets.front().message);
      m_packets.pop_front();

      SendMessage(msg, 10);
    }

    m_pVideoCodec->Reset();
    m_packets.clear();
    //picture.iFlags &= ~DVP_FLAG_ALLOCATED;
    m_renderManager.DiscardBuffer();
    return false;
  }

  if (decoderState == CDVDVideoCodec::VC_REOPEN)
  {
    while (!m_packets.empty())
    {
      auto msg = std::static_pointer_cast<CDVDMsgDemuxerPacket>(m_packets.front().message);
      m_packets.pop_front();
      SendMessage(msg, 10);
    }

    m_pVideoCodec->Reopen();
    m_packets.clear();
    m_renderManager.DiscardBuffer();
    return false;
  }

  // if decoder had an error, tell it to reset to avoid more problems
  if (decoderState == CDVDVideoCodec::VC_ERROR)
  {
    CLog::Log(LOGDEBUG, "CVideoPlayerVideo - video decoder returned error");
    return false;
  }

  if (decoderState == CDVDVideoCodec::VC_EOF)
  {
    if (m_syncState == IDVDStreamPlayer::SYNC_STARTING)
    {
      SStartMsg msg;
      msg.player = VideoPlayer_VIDEO;
      msg.cachetime = DVD_MSEC_TO_TIME(50);
      msg.cachetotal = DVD_MSEC_TO_TIME(100);
      msg.timestamp = DVD_NOPTS_VALUE;
      m_messageParent.Put(std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, msg));
    }
    return false;
  }

  // check for a new picture
  if (decoderState == CDVDVideoCodec::VC_PICTURE)
  {
    bool hasTimestamp = true;

    m_picture.iDuration = frametime;

    // validate picture timing,
    // if both dts/pts invalid, use pts calculated from picture.iDuration
    // if pts invalid use dts, else use picture.pts as passed
    if (m_picture.dts == DVD_NOPTS_VALUE && m_picture.pts == DVD_NOPTS_VALUE)
    {
      m_picture.pts = pts;
      hasTimestamp = false;
    }
    else if (m_picture.pts == DVD_NOPTS_VALUE)
      m_picture.pts = m_picture.dts;

	//cl Upsample pts, many containers have not very good pts precision, e.g 1ms. We keep the raw one in a new variable in case some function is not happy with the new value...
	double pts1 = m_picture.pts;
	m_picture.rawPts = m_picture.pts;  
	m_picture.pts = upSampler.update(m_picture.pts, m_picture.iFlags & DVP_FLAG_INTERLACED);  //cl don't use frametime here, it is sometime hardcoded to 1/(25fps)
	static double oldPts = 0;
	CLog::Log(LOGDEBUG, "pts1: {:f}ms, pts: {:f}ms, diff: {:f}ms, upsampler diff:{:f}ms", pts1/1000.0, m_picture.pts / 1000.0, (m_picture.pts - pts1) / 1000.0, (m_picture.pts - oldPts) / 1000.0);
	oldPts = m_picture.pts; 
		
    // use forced aspect if any
    if (m_fForcedAspectRatio != 0.0f)
    {
      m_picture.iDisplayWidth = (int) (m_picture.iDisplayHeight * m_fForcedAspectRatio);
      if (m_picture.iDisplayWidth > m_picture.iWidth)
      {
        m_picture.iDisplayWidth =  m_picture.iWidth;
        m_picture.iDisplayHeight = (int) (m_picture.iDisplayWidth / m_fForcedAspectRatio);
      }
    }

    // set stereo mode if not set by decoder
    if (m_picture.stereoMode.empty())
    {
      std::string stereoMode;
      switch (static_cast<RenderStereoMode>(m_processInfo.GetVideoSettings().m_StereoMode))
      {
        case RenderStereoMode::SPLIT_VERTICAL:
          stereoMode = "left_right";
          if (m_processInfo.GetVideoSettings().m_StereoInvert)
            stereoMode = "right_left";
          break;
        case RenderStereoMode::SPLIT_HORIZONTAL:
          stereoMode = "top_bottom";
          if (m_processInfo.GetVideoSettings().m_StereoInvert)
            stereoMode = "bottom_top";
          break;
        default:
          stereoMode = m_hints.stereo_mode;
          break;
      }
      if (!stereoMode.empty() && stereoMode != "mono")
      {
        m_picture.stereoMode = stereoMode;
      }
    }

    // if frame has a pts (usually originating from demux packet), use that
    if (m_picture.pts != DVD_NOPTS_VALUE)
    {
      pts = m_picture.pts;
    }

    double extraDelay = 0.0;
    if (m_picture.iRepeatPicture)
    {
      extraDelay = m_picture.iRepeatPicture * m_picture.iDuration;
      m_picture.iDuration += extraDelay;
    }

    m_picture.pts = pts + extraDelay;
    // guess next frame pts. iDuration is always valid
    if (m_speed != 0)
      pts += m_picture.iDuration * m_speed / abs(m_speed);

    m_outputSate = OutputPicture(&m_picture);

    if (m_outputSate == OUTPUT_AGAIN)
    {
      return true;
    }
    else if (m_outputSate == OUTPUT_ABORT)
    {
      return false;
    }
    else if ((m_outputSate == OUTPUT_DROPPED) && !(m_picture.iFlags & DVP_FLAG_DROPPED))
    {
      m_iDroppedFrames++;
      m_ptsTracker.Flush();
    }

    if (m_syncState == IDVDStreamPlayer::SYNC_STARTING &&
        m_outputSate != OUTPUT_DROPPED &&
        !(m_picture.iFlags & DVP_FLAG_DROPPED))
    {
      m_syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
      SStartMsg msg;
      msg.player = VideoPlayer_VIDEO;
      msg.cachetime = DVD_MSEC_TO_TIME(50); //! @todo implement
      msg.cachetotal = DVD_MSEC_TO_TIME(100); //! @todo implement
      msg.timestamp = hasTimestamp ? (pts + m_renderManager.GetDelay() * 1000) : DVD_NOPTS_VALUE;
      m_messageParent.Put(std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, msg));
    }

    frametime = (double)DVD_TIME_BASE / m_fFrameRate;
  }

  return true;
}

void CVideoPlayerVideo::OnExit()
{
  CLog::Log(LOGINFO, "thread end: video_thread");
}

void CVideoPlayerVideo::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    SendMessage(std::make_shared<CDVDMsgInt>(CDVDMsg::PLAYER_SETSPEED, speed), 1);
  else
    m_speed = speed;
}

void CVideoPlayerVideo::Flush(bool sync)
{
  /* flush using message as this gets called from VideoPlayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_FLUSH, sync), 1);
  m_bAbortOutput = true;
}

void CVideoPlayerVideo::ProcessOverlays(const VideoPicture* pSource, double pts)
{
  // remove any overlays that are out of time
  if (m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
    m_pOverlayContainer->CleanUp(pts - m_iSubtitleDelay);

  VecOverlays overlays;

  {
    std::unique_lock lock(*m_pOverlayContainer);

    VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
    auto it = pVecOverlays->begin();

    //Check all overlays and render those that should be rendered, based on time and forced
    //Both forced and subs should check timing
    while (it != pVecOverlays->end())
    {
      std::shared_ptr<CDVDOverlay>& pOverlay = *it++;
      if(!pOverlay->bForced && !m_bRenderSubs)
        continue;

      double pts2 = pOverlay->bForced ? pts : pts - m_iSubtitleDelay;

      if((pOverlay->iPTSStartTime <= pts2 && (pOverlay->iPTSStopTime > pts2 || pOverlay->iPTSStopTime == 0LL)))
      {
        if(pOverlay->IsOverlayType(DVDOVERLAY_TYPE_GROUP))
          overlays.insert(overlays.end(),
                          static_cast<CDVDOverlayGroup&>(*pOverlay).m_overlays.begin(),
                          static_cast<CDVDOverlayGroup&>(*pOverlay).m_overlays.end());
        else
          overlays.push_back(pOverlay);
      }
    }

    for(it = overlays.begin(); it != overlays.end(); ++it)
    {
      double pts2 = (*it)->bForced ? pts : pts - m_iSubtitleDelay;
      m_renderManager.AddOverlay(*it, pts2);
    }
  }
}

CVideoPlayerVideo::EOutputState CVideoPlayerVideo::OutputPicture(const VideoPicture* pPicture)
{
  CLog::Log(LOGDEBUG, "Start");
  m_bAbortOutput = false;

  if (m_processInfo.GetVideoStereoMode() != pPicture->stereoMode)
  {
    m_processInfo.SetVideoStereoMode(pPicture->stereoMode);
    // signal about changes in video parameters
    m_messageParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_AVCHANGE));
  }

  double config_framerate = m_bFpsInvalid ? 0.0 : m_fFrameRate;
  if (m_processInfo.GetVideoInterlaced())
  {
    if (MathUtils::FloatEquals(config_framerate, 25.0, 0.02))
      config_framerate = 50.0;
    else if (MathUtils::FloatEquals(config_framerate, 29.97, 0.02))
      config_framerate = 59.94;
  }

  int sorient = m_processInfo.GetVideoSettings().m_Orientation;
  int orientation = sorient != 0 ? (sorient + m_hints.orientation) % 360
                                 : m_hints.orientation;

  if (!m_renderManager.Configure(*pPicture,
                                static_cast<float>(config_framerate),
                                orientation,
                                m_pVideoCodec->GetAllowedReferences()))
  {
    CLog::Log(LOGERROR, "{} - failed to configure renderer", __FUNCTION__);
    return OUTPUT_ABORT;
  }

  //try to calculate the framerate
  m_ptsTracker.Add(pPicture->rawPts);  // Function not happy with upsampled PTS if it takes too much time to converge, filter changed and seems fixed but not sure enough...
  if (!m_stalled)
    CalcFrameRate();

  // signal to clock what our framerate is, it may want to adjust it's
  // speed to better match with our video renderer's output speed
  m_pClock->UpdateFramerate(m_fFrameRate);

  // calculate the time we need to delay this picture before displaying
  double iPlayingClock, iCurrentClock;

  iPlayingClock = m_pClock->GetClock(iCurrentClock, false); // snapshot current clock

  CLog::Log(LOGDEBUG, "config_framerate: {}, m_fStableFrameRate: {}, m_fFrameRate: {}, iPlayingClock: {} ", config_framerate, m_fStableFrameRate, m_fFrameRate, iPlayingClock);
  if (m_speed < 0)
  {
	double renderPts;
    int queued, discard;
    int lateframes;
    double inputPts = m_droppingStats.m_lastPts;
    m_renderManager.GetStats(lateframes, renderPts, queued, discard);
    if (pPicture->pts > renderPts || queued > 0)
    {
      if (inputPts >= renderPts)
      {
        m_rewindStalled = true;
        CThread::Sleep(50ms);
		CLog::Log(LOGDEBUG, "sleep 50ms");
      }
      return OUTPUT_DROPPED;
    }
    else if (pPicture->pts < iPlayingClock)
    {
      return OUTPUT_DROPPED;
    }
  }

  if ((pPicture->iFlags & DVP_FLAG_DROPPED))
  {
    m_droppingStats.AddOutputDropGain(pPicture->pts, 1);
    CLog::Log(LOGDEBUG, "{} - dropped in output", __FUNCTION__);
    return OUTPUT_DROPPED;
  }

  auto timeToDisplay = std::chrono::milliseconds(DVD_TIME_TO_MSEC(pPicture->pts - iPlayingClock));

  // make sure waiting time is not negative
  std::chrono::milliseconds maxWaitTime = std::min(std::max(timeToDisplay + 500ms, 50ms), 500ms);
  // don't wait when going ff
  if (m_speed > DVD_PLAYSPEED_NORMAL)
    maxWaitTime = std::max(timeToDisplay, 0ms);
  CLog::Log(LOGDEBUG, "maxWaitTime: {}", maxWaitTime);
  int buffer = m_renderManager.WaitForBuffer(m_bAbortOutput, maxWaitTime);
  if (buffer < 0)
  {
    if (m_speed != DVD_PLAYSPEED_PAUSE)
      CLog::Log(LOGWARNING, "{} - timeout waiting for buffer", __FUNCTION__);
    return OUTPUT_AGAIN;
  }

  ProcessOverlays(pPicture, pPicture->pts);

  EINTERLACEMETHOD deintMethod = EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE;
  deintMethod = m_processInfo.GetVideoSettings().m_InterlaceMethod;
  if (!m_processInfo.Supports(deintMethod))
    deintMethod = m_processInfo.GetDeinterlacingMethodDefault();

  if (!m_renderManager.AddVideoPicture(*pPicture, m_bAbortOutput, deintMethod, (m_syncState == ESyncState::SYNC_STARTING)))
  {
	m_droppingStats.AddOutputDropGain(pPicture->pts, 1);
    return OUTPUT_DROPPED;
  }

  CLog::Log(LOGDEBUG, "Done");
  return OUTPUT_NORMAL;
}

std::string CVideoPlayerVideo::GetPlayerInfo()
{
  std::ostringstream s;
  s << "vq:" << std::setw(2) << std::min(99, m_processInfo.GetLevelVQ());
  s << "% " << std::fixed << std::setprecision(3) << m_messageQueue.GetTimeSize();
  s << "s, Mb/s:" << std::fixed << std::setprecision(2)
    << static_cast<double>(GetVideoBitrate()) / (1024.0 * 1024.0);
  s << ", fr:" << std::fixed << std::setprecision(3) << m_fFrameRate;
  s << ", drop:" << m_iDroppedFrames;
  s << ", skip:" << m_renderManager.GetSkippedFrames();

  int pc = m_ptsTracker.GetPatternLength();
  if (pc > 0)
    s << ", pc:" << pc;
  else
    s << ", pc:none";

  return s.str();
}

int CVideoPlayerVideo::GetVideoBitrate()
{
  return (int)m_videoStats.GetBitrate();
}

void CVideoPlayerVideo::ResetFrameRateCalc()
{
  m_fStableFrameRate = 0.0;
  m_iFrameRateCount = 0;
  m_iFrameRateLength = 1;
  m_iFrameRateErr = 0;
  m_bAllowDrop = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoFpsDetect == 0;
}

double CVideoPlayerVideo::GetCurrentPts()
{
  double renderPts;
  int sleepTime;
  int queued, discard;

  // get render stats
  m_renderManager.GetStats(sleepTime, renderPts, queued, discard);

  if (renderPts == DVD_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;
  else if (m_stalled)
    return DVD_NOPTS_VALUE;
  else if (m_speed == DVD_PLAYSPEED_NORMAL)
  {
    if (renderPts < 0)
      renderPts = 0;
  }
  return renderPts;
}

#define MAXFRAMERATEDIFF   0.01
#define MAXFRAMESERR    1000

void CVideoPlayerVideo::CalcFrameRate()
{
  if (m_iFrameRateLength >= 128 || CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoFpsDetect == 0)
    return; //don't calculate the fps

  if (!m_ptsTracker.HasFullBuffer())
    return; //we can only calculate the frameduration if m_pullupCorrection has a full buffer

  //see if m_pullupCorrection was able to detect a pattern in the timestamps
  //and is able to calculate the correct frame duration from it
  double frameduration = m_ptsTracker.GetFrameDuration();
  if (m_ptsTracker.VFRDetection())
    frameduration = m_ptsTracker.GetMinFrameDuration();

  if ((frameduration==DVD_NOPTS_VALUE) ||
      ((CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoFpsDetect == 1) && ((m_ptsTracker.GetPatternLength() > 1) && !m_ptsTracker.VFRDetection())))
  {
    //reset the stored framerates if no good framerate was detected
    m_fStableFrameRate = 0.0;
    m_iFrameRateCount = 0;
    m_iFrameRateErr++;

    if (m_iFrameRateErr == MAXFRAMESERR && m_iFrameRateLength == 1)
    {
      CLog::Log(LOGDEBUG,
                "{} counted {} frames without being able to calculate the framerate, giving up",
                __FUNCTION__, m_iFrameRateErr);
      m_bAllowDrop = true;
      m_iFrameRateLength = 128;
    }
    return;
  }

  double framerate = DVD_TIME_BASE / frameduration;

  //store the current calculated framerate if we don't have any yet
  if (m_iFrameRateCount == 0)
  {
    m_fStableFrameRate = framerate;
    m_iFrameRateCount++;
  }
  //check if the current detected framerate matches with the stored ones
  else if (fabs(m_fStableFrameRate / m_iFrameRateCount - framerate) <= MAXFRAMERATEDIFF)
  {
    m_fStableFrameRate += framerate; //store the calculated framerate
    m_iFrameRateCount++;

    //if we've measured m_iFrameRateLength seconds of framerates,
    if (m_iFrameRateCount >= MathUtils::round_int(framerate) * m_iFrameRateLength)
    {
      //store the calculated framerate if it differs too much from m_fFrameRate
      if (fabs(m_fFrameRate - (m_fStableFrameRate / m_iFrameRateCount)) > MAXFRAMERATEDIFF || m_bFpsInvalid)
      {
        CLog::Log(LOGDEBUG, "{} framerate was:{:f} calculated:{:f}", __FUNCTION__, m_fFrameRate,
                  m_fStableFrameRate / m_iFrameRateCount);
        m_fFrameRate = m_fStableFrameRate / m_iFrameRateCount;
        m_bFpsInvalid = false;
        m_processInfo.SetVideoFps(static_cast<float>(m_fFrameRate));
      }

      //reset the stored framerates
      m_fStableFrameRate = 0.0;
      m_iFrameRateCount = 0;
      m_iFrameRateLength *= 2; //double the length we should measure framerates

      //we're allowed to drop frames because we calculated a good framerate
      m_bAllowDrop = true;
    }
  }
  else //the calculated framerate didn't match, reset the stored ones
  {
    m_fStableFrameRate = 0.0;
    m_iFrameRateCount = 0;
  }
}

int CVideoPlayerVideo::CalcDropRequirement(double pts)
{
  int result = 0;
  int lateframes;
  double iDecoderPts, iRenderPts;
  int iSkippedPicture = -1;
  int iDroppedFrames = -1;
  int iBufferLevel;
  int queued, discard;

  m_droppingStats.m_lastPts = pts;

  // get decoder stats
  if (!m_pVideoCodec->GetCodecStats(iDecoderPts, iDroppedFrames, iSkippedPicture))
    iDecoderPts = pts;
  if (iDecoderPts == DVD_NOPTS_VALUE)
    iDecoderPts = pts;

  // get render stats
  m_renderManager.GetStats(lateframes, iRenderPts, queued, discard);
  iBufferLevel = queued + discard;

  if (iBufferLevel < 0)
    result |= DROP_BUFFER_LEVEL;
  else if (iBufferLevel < 2)
  {
    result |= DROP_BUFFER_LEVEL;
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVideoPlayerVideo::CalcDropRequirement - hurry: {}",
              iBufferLevel);
  }

  if (m_bAllowDrop)
  {
    if (iSkippedPicture > 0)
    {
      CDroppingStats::CGain gain;
      gain.frames = iSkippedPicture;
      gain.pts = iDecoderPts;
      m_droppingStats.m_gain.push_back(gain);
      m_droppingStats.m_totalGain += gain.frames;
      result |= DROP_DROPPED;
      CLog::Log(LOGDEBUG, LOGVIDEO,
                "CVideoPlayerVideo::CalcDropRequirement - dropped pictures, lateframes: {}, "
                "Bufferlevel: {}, dropped: {}",
                lateframes, iBufferLevel, iSkippedPicture);
    }
    if (iDroppedFrames > 0)
    {
      CDroppingStats::CGain gain;
      gain.frames = iDroppedFrames;
      gain.pts = iDecoderPts;
      m_droppingStats.m_gain.push_back(gain);
      m_droppingStats.m_totalGain += iDroppedFrames;
      result |= DROP_DROPPED;
      CLog::Log(LOGDEBUG, LOGVIDEO,
                "CVideoPlayerVideo::CalcDropRequirement - dropped in decoder, lateframes: {}, "
                "Bufferlevel: {}, dropped: {}",
                lateframes, iBufferLevel, iDroppedFrames);
    }
  }

  // subtract gains
  while (!m_droppingStats.m_gain.empty() &&
         iRenderPts >= m_droppingStats.m_gain.front().pts)
  {
    m_droppingStats.m_totalGain -= m_droppingStats.m_gain.front().frames;
    m_droppingStats.m_gain.pop_front();
  }

  // calculate lateness
  int lateness = lateframes - m_droppingStats.m_totalGain;

  if (lateness > 0 && m_speed)
  {
    result |= DROP_VERYLATE;
  }
  return result;
}

void CDroppingStats::Reset()
{
  m_gain.clear();
  m_totalGain = 0;
}

void CDroppingStats::AddOutputDropGain(double pts, int frames)
{
  CDroppingStats::CGain gain;
  gain.frames = frames;
  gain.pts = pts;
  m_gain.push_back(gain);
  m_totalGain += frames;
}
