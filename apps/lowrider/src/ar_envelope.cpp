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

#include "ar_envelope.h"
#include "arm_math.h"

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

AREnvelope::AREnvelope()
:   AudioFilter(),
    m_attack(),
    m_release(),
    m_peak(1.0f)
{
}

void AREnvelope::set_sample_rate(float rate)
{
    AudioFilter::set_sample_rate(rate);
    m_attack.set_sample_rate(rate);
    m_release.set_sample_rate(rate);
}

void AREnvelope::set_peak(float peak)
{
    m_peak = peak;
    m_attack.set_begin_value(0.0f);
    m_attack.set_end_value(peak);
    m_release.set_begin_value(peak);
    m_release.set_end_value(0.0f);
}

void AREnvelope::set_length_in_seconds(EnvelopeStage stage, float seconds)
{
    switch (stage)
    {
        case kAttack:
            m_attack.set_length_in_seconds(seconds);
            break;

        case kRelease:
            m_release.set_length_in_seconds(seconds);
            break;
    }
}

void AREnvelope::set_length_in_samples(EnvelopeStage stage, uint32_t samples)
{
    switch (stage)
    {
        case kAttack:
            m_attack.set_length_in_samples(samples);
            break;

        case kRelease:
            m_release.set_length_in_samples(samples);
            break;
    }
}

float AREnvelope::get_length_in_seconds(EnvelopeStage stage)
{
    switch (stage)
    {
        case kAttack:
            return m_attack.get_length_in_seconds();

        case kRelease:
            return m_release.get_length_in_seconds();
    }
    return 0.0f;
}

uint32_t AREnvelope::get_length_in_samples(EnvelopeStage stage)
{
    switch (stage)
    {
        case kAttack:
            return m_attack.get_length_in_samples();

        case kRelease:
            return m_release.get_length_in_samples();
    }
    return 0;
}

void AREnvelope::set_curve_type(EnvelopeStage stage, AudioRamp::CurveType theType)
{
    switch (stage)
    {
        case kAttack:
            m_attack.set_curve_type(theType);
            break;

        case kRelease:
            m_release.set_curve_type(theType);
            break;
    }
}

void AREnvelope::reset()
{
    set_peak(m_peak);
    m_attack.reset();
    m_release.reset();
}

float AREnvelope::next()
{
    if (m_attack.is_finished())
    {
        return m_release.next();
    }
    else
    {
        return m_attack.next();
    }
}

bool AREnvelope::is_finished()
{
    return m_attack.is_finished() && m_release.is_finished();
}

void AREnvelope::process(float * samples, uint32_t count)
{
    if (!m_attack.is_finished())
    {
        uint32_t attackCount = m_attack.get_remaining_samples();
        if (attackCount > count)
        {
            attackCount = count;
        }
        m_attack.process(samples, attackCount);

        if (attackCount < count)
        {
            m_release.process(samples + attackCount, count - attackCount);
        }
    }
    else
    {
//         uint32_t releaseCount = m_release.get_remaining_samples();
//         if (releaseCount > count)
//         {
//             releaseCount = count;
//         }
//         m_release.process(samples, releaseCount);
        m_release.process(samples, count);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
