/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <libplacebo/options.h>



class PlOptionsWrapper
{

public:
  PlOptionsWrapper()
  {
    m_placeboOptions = pl_options_alloc(NULL);
    resetPlOptions(DEFAULT);
  }
  
  ~PlOptionsWrapper() 
  {
    if (m_placeboOptions)
      pl_options_free(&m_placeboOptions);
	m_placeboOptions = nullptr;
  }

  PlOptionsWrapper(const PlOptionsWrapper& other) 
  {
    m_placeboOptions = pl_options_alloc(NULL);
    if (m_placeboOptions && other.m_placeboOptions) 
    {
      //copy it
      //std::memcpy(m_placeboOptions, other.m_placeboOptions, sizeof(pl_options_t));
      *m_placeboOptions = *other.m_placeboOptions;
    }
  }

  PlOptionsWrapper& operator=(const PlOptionsWrapper& other)
  {
    if (this != &other) { // Avoid self-assignment
      if (m_placeboOptions)
      {
        pl_options_free(&m_placeboOptions); // Free existing memory
      }
      m_placeboOptions = pl_options_alloc(NULL);
      if (m_placeboOptions && other.m_placeboOptions) 
      {
        //shallow copy
        *m_placeboOptions = *other.m_placeboOptions;
        //deep copy
        m_placeboOptions->params.deband_params = other.m_placeboOptions->params.deband_params == NULL ? NULL : &m_placeboOptions->deband_params;
        m_placeboOptions->params.sigmoid_params = other.m_placeboOptions->params.sigmoid_params == NULL ? NULL : &m_placeboOptions->sigmoid_params;
        m_placeboOptions->params.color_adjustment = other.m_placeboOptions->params.color_adjustment == NULL ? NULL : &m_placeboOptions->color_adjustment;
        m_placeboOptions->params.peak_detect_params = other.m_placeboOptions->params.peak_detect_params == NULL ? NULL : &m_placeboOptions->peak_detect_params;
        m_placeboOptions->params.color_map_params = other.m_placeboOptions->params.color_map_params == NULL ? NULL : &m_placeboOptions->color_map_params;
        m_placeboOptions->params.dither_params = other.m_placeboOptions->params.dither_params == NULL ? NULL : &m_placeboOptions->dither_params;
        m_placeboOptions->params.icc_params = other.m_placeboOptions->params.icc_params == NULL ? NULL : &m_placeboOptions->icc_params;
        m_placeboOptions->params.cone_params = other.m_placeboOptions->params.cone_params == NULL ? NULL : &m_placeboOptions->cone_params;
        m_placeboOptions->params.blend_params = other.m_placeboOptions->params.blend_params == NULL ? NULL : &m_placeboOptions->blend_params;
        m_placeboOptions->params.deinterlace_params = other.m_placeboOptions->params.deinterlace_params == NULL ? NULL : &m_placeboOptions->deinterlace_params;
        m_placeboOptions->params.distort_params = other.m_placeboOptions->params.distort_params == NULL ? NULL : &m_placeboOptions->distort_params;
        //cl other fields like lut or priv???? Maybe also initialize before copying
      }
    }
    return *this;
  }
  
  void DeepCopy(const PlOptionsWrapper& other)
  {
    if (this != &other) { // Avoid self-assignment
      if (m_placeboOptions && other.m_placeboOptions)
      {
        //shallow copy
        *m_placeboOptions = *other.m_placeboOptions;
        //deep copy
        m_placeboOptions->params.deband_params = other.m_placeboOptions->params.deband_params == NULL ? NULL : &m_placeboOptions->deband_params;
        m_placeboOptions->params.sigmoid_params = other.m_placeboOptions->params.sigmoid_params == NULL ? NULL : &m_placeboOptions->sigmoid_params;
        m_placeboOptions->params.color_adjustment = other.m_placeboOptions->params.color_adjustment == NULL ? NULL : &m_placeboOptions->color_adjustment;
        m_placeboOptions->params.peak_detect_params = other.m_placeboOptions->params.peak_detect_params == NULL ? NULL : &m_placeboOptions->peak_detect_params;
        m_placeboOptions->params.color_map_params = other.m_placeboOptions->params.color_map_params == NULL ? NULL : &m_placeboOptions->color_map_params;
        m_placeboOptions->params.dither_params = other.m_placeboOptions->params.dither_params == NULL ? NULL : &m_placeboOptions->dither_params;
        m_placeboOptions->params.icc_params = other.m_placeboOptions->params.icc_params == NULL ? NULL : &m_placeboOptions->icc_params;
        m_placeboOptions->params.cone_params = other.m_placeboOptions->params.cone_params == NULL ? NULL : &m_placeboOptions->cone_params;
        m_placeboOptions->params.blend_params = other.m_placeboOptions->params.blend_params == NULL ? NULL : &m_placeboOptions->blend_params;
        m_placeboOptions->params.deinterlace_params = other.m_placeboOptions->params.deinterlace_params == NULL ? NULL : &m_placeboOptions->deinterlace_params;
        m_placeboOptions->params.distort_params = other.m_placeboOptions->params.distort_params == NULL ? NULL : &m_placeboOptions->distort_params;
        //cl other fields like lut or priv???? Maybe also initialize before copying
      }
    }
  }

  
  pl_options  getPlOptions() const { return m_placeboOptions; }

  enum reset_type 
  {
    DEFAULT,
    FAST,
    HIGH_QUALITY
  };

  void  resetPlOptions(reset_type type = DEFAULT) const 
  {
    switch (type) 
    {
      case DEFAULT:
        pl_options_reset(m_placeboOptions, &pl_render_default_params);
        break;
      case FAST:
        pl_options_reset(m_placeboOptions, &pl_render_fast_params);
        break;
      case HIGH_QUALITY:
        pl_options_reset(m_placeboOptions, &pl_render_high_quality_params);
        break;
    }
    // Change some default options for Kodi
    // enable deinterlacing by default
    m_placeboOptions->params.deinterlace_params = &m_placeboOptions->deinterlace_params;
  }


  bool operator!=(const PlOptionsWrapper& right) const;

private:
  pl_options m_placeboOptions;


};
