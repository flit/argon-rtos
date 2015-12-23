/*
 * Copyright (c) 2015 Immo Software
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its contributors may
 *   be used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_AUDIO_RAMP_H_)
#define _AUDIO_RAMP_H_

#include "audio_filter.h"
#include <stdint.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Audio ramp generator.
 */
class AudioRamp : public AudioFilter
{
public:
    //! Options for the shape of the ramp.
    enum CurveType
    {
        kLinear,    //!< Simple, straight line.
        kCubic      //!< Cubic curve.
    };

    AudioRamp(float begin = 0.0, float end = 1.0);
    virtual ~AudioRamp() {}

    virtual void set_length_in_seconds(float seconds);
    virtual void set_length_in_samples(uint32_t samples);

    float get_length_in_seconds() { return m_lengthInSeconds; }
    uint32_t get_length_in_samples() { return m_lengthInSamples; }

    virtual void set_begin_value(float beginValue);
    virtual void set_end_value(float endValue);

    //! Sets the curve type. You must call this before setting the
    //! begin or end values, or the length, because it does not recompute
    //! the slope itself.
    void set_curve_type(CurveType theType) { m_curveType = theType; }

    uint32_t get_remaining_samples() { return m_lengthInSamples - m_currentSample; }

    virtual void reset();
    virtual float next();

    //! Returns true if the number of samples returned from next() is
    //! equal to or greater than the length of the ramp in samples.
    virtual bool is_finished() { return m_currentSample >= m_lengthInSamples; }

    virtual void process(float * samples, uint32_t count);

protected:
    float m_lengthInSeconds;
    uint32_t m_lengthInSamples;
    float m_beginValue;
    float m_endValue;
    uint32_t m_currentSample;
    float m_slope;
    CurveType m_curveType;
    float m_y1;
    float m_y2;

    void recalculate_slope();

};

#endif // _AUDIO_RAMP_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
