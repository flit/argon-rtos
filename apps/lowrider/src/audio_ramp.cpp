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

#include "audio_ramp.h"
#include "arm_math.h"

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

AudioRamp::AudioRamp(float begin, float end)
:   AudioFilter(),
    m_lengthInSeconds(0),
    m_lengthInSamples(0),
    m_beginValue(begin),
    m_endValue(end),
    m_currentSample(0),
    m_slope(0.0f),
    m_curveType(kLinear),
    m_y1(0),
    m_y2(0)
{
}

void AudioRamp::set_length_in_seconds(float seconds)
{
    m_lengthInSeconds = seconds;
    m_lengthInSamples = unsigned(get_sample_rate() * seconds);
    recalculate_slope();
}

void AudioRamp::set_length_in_samples(uint32_t samples)
{
    m_lengthInSamples = samples;
    m_lengthInSeconds = samples / get_sample_rate();
    recalculate_slope();
}

void AudioRamp::set_begin_value(float beginValue)
{
    m_beginValue = beginValue;
    recalculate_slope();
}

void AudioRamp::set_end_value(float endValue)
{
    m_endValue = endValue;
    recalculate_slope();
}

//! Computes the values needed by next() and process() to return
//! intermediate ramp values.
void AudioRamp::recalculate_slope()
{
    switch (m_curveType)
    {
        case kLinear:
            m_slope = 1.0f / float(m_lengthInSamples) * (m_endValue - m_beginValue);
            break;

        case kCubic:
            m_y1 = pow(m_beginValue, 1.0f / 3.0f);
            m_y2 = pow(m_endValue, 1.0f / 3.0f);
            m_slope = (m_y2 - m_y1) / float(m_lengthInSamples);
            break;
    }
}

void AudioRamp::reset()
{
    m_currentSample = 0;
    recalculate_slope();
}

float AudioRamp::next()
{
    float result;
    process(&result, 1);
    return result;
}

void AudioRamp::process(float * samples, uint32_t count)
{
    float * value = samples;
    uint32_t realCount = count;
    if (m_currentSample + count > m_lengthInSamples)
    {
        realCount = m_lengthInSamples - m_currentSample;
    }
    uint32_t leftoverCount = count - realCount;

    uint32_t realCount4 = realCount / 4;
    uint32_t remainderCount4 = realCount % 4;

    switch (m_curveType)
    {
        case kLinear:
        {
            float beginValue = m_beginValue;
            float slope = m_slope;
            float currentSample = float(m_currentSample);

            while (realCount4--)
            {
                float i1 = currentSample * slope + beginValue;
                currentSample += 1.0f;
                float i2 = currentSample * slope + beginValue;
                currentSample += 1.0f;
                float i3 = currentSample * slope + beginValue;
                currentSample += 1.0f;
                float i4 = currentSample * slope + beginValue;
                currentSample += 1.0f;

                *value++ = i1;
                *value++ = i2;
                *value++ = i3;
                *value++ = i4;
            }

            while (remainderCount4--)
            {
                *value++ = beginValue + currentSample * slope;
                currentSample += 1.0f;
            }

            m_currentSample = uint32_t(currentSample);
            break;
        }

        case kCubic:
        {
            float grow = m_slope;
            float y1 = m_y1;

            while (realCount4--)
            {
                float y1_1 = y1 + grow;
                float y1_2 = y1 + grow * 2;
                float y1_3 = y1 + grow * 3;
                float y1_4 = y1 + grow * 4;

                *value++ = y1_1 * y1_1 * y1_1;
                *value++ = y1_2 * y1_2 * y1_2;
                *value++ = y1_3 * y1_3 * y1_3;
                *value++ = y1_4 * y1_4 * y1_4;

                y1 = y1_4;
            }

            while (remainderCount4--)
            {
                y1 += grow;
                *value++ = y1 * y1 * y1;
            }

            m_currentSample += realCount;
            m_y1 = y1;
            break;
        }
    }

    // fill the remaining samples with the end value
    if (leftoverCount > 0)
    {
        float endValue = m_endValue;
        while (leftoverCount--)
        {
            *value++ = endValue;
        }
    }

}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
