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
#if !defined(_SEQUENCER_H_)
#define _SEQUENCER_H_

#include <stdint.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Audio event sequencer.
 */
class Sequencer
{
public:
    class Event;
    class Track;

    Sequencer();
    ~Sequencer() {}

    void set_sample_rate(float rate) { m_sampleRate = rate; }
    void set_tempo(float tempo) { m_tempo = tempo; }

    void init();

    void add_track(Track * track);

protected:
    enum {
        kMaxTracks = 4,
    };

    float m_sampleRate;
    float m_tempo;
    float m_samplesPerBeat;
    Track * m_tracks[kMaxTracks];
    uint32_t m_trackCount;
};

/*!
 *
 */
class Sequencer::Event
{
public:
    Event(uint32_t timestamp=0) : m_timestamp(timestamp) {}

    uint32_t get_timestamp() const { return m_timestmap; }
    void set_timestamp(uint32_t timestamp) { m_timestamp = timestamp; }

    Event * get_next() { return m_next; }
    void set_next(Event * next) { m_next = next; }

protected:
    uint32_t m_timestamp;
    Event * m_next;
};

/*!
 *
 */
class Sequencer::Track
{
public:
    Track() {}

    void add_event(Sequencer::Event * event);

protected:
    Sequencer::Event * m_events[kMaxEvents];
};

#endif // _SEQUENCER_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
