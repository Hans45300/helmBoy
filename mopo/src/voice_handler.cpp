#include "voice_handler.h"

namespace mopo {

/**
 * @file voice_handler.cpp
 * @brief Implementation for managing voices (allocation, note on/off,
 * stealing policy).
 *
 * The VoiceHandler coordinates voice allocation and interfacing with the
 * higher-level synth logic. It ensures sample-accurate note scheduling.
 */

void Voice::activate(mopo::mopo_float note, mopo::mopo_float velocity,
                     mopo::mopo_float last_note, int note_pressed,
                     int sample, int channel) {
  state_.event = mopo::kVoiceOn;
  state_.note = note;
  state_.velocity = velocity;
  state_.last_note = last_note;
  state_.note_pressed = note_pressed;
  state_.channel = channel;
  event_sample_ = sample;
  key_state_ = KeyState::Held;
}

} // namespace mopo
// D�finitions manquantes pour mopo::Voice
#include "mopo.h"
namespace mopo {
void Voice::clearEvents() {
  this->event_sample_ = -1;
  this->aftertouch_sample_ = -1;
  this->channel_aftertouch_sample_ = -1;
}

bool Voice::hasNewAftertouch() {
  return this->aftertouch_sample_ >= 0;
}

void Voice::setAftertouch(mopo::mopo_float aftertouch, int sample) {
  this->aftertouch_ = aftertouch;
  this->aftertouch_sample_ = sample;
}

void Voice::setChannelAftertouch(mopo::mopo_float aftertouch, int sample) {
  this->channel_aftertouch_ = aftertouch;
  this->channel_aftertouch_sample_ = sample;
}

bool Voice::hasNewChannelAftertouch() {
  return this->channel_aftertouch_sample_ >= 0;
}

bool Voice::hasNewEvent() {
  return this->event_sample_ >= 0;
}

void Voice::kill(int sample) {
  this->state_.event = mopo::kVoiceKill;
  this->event_sample_ = sample;
  this->key_state_ = mopo::Voice::KeyState::Released;
}

void Voice::deactivate(int sample) {
  this->state_.event = mopo::kVoiceOff;
  this->event_sample_ = sample;
  this->key_state_ = mopo::Voice::KeyState::Released;
}

void Voice::sustain() {
  this->key_state_ = mopo::Voice::KeyState::Sustained;
}
} // namespace mopo
/* Copyright 2025 Marc Scheffer
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * mopo is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mopo.  If not, see <http://www.gnu.org/licenses/>.
 */


// Toujours inclure le header d'abord pour garantir la coh�rence des membres et de l'espace de noms
#include "voice_handler.h"
#include "utils.h"



mopo::Voice::Voice(mopo::Processor* processor)
  : event_sample_(-1),
    state_{},
    key_state_(KeyState::Released),
    aftertouch_sample_(-1),
    aftertouch_(0.0f),
    processor_(processor)
{
  state_.event = kVoiceOff;
  state_.note = 0.0f;
  state_.velocity = 0.0f;
  state_.last_note = 0.0f;
  state_.note_pressed = 0;
  state_.channel = 0;
}

 mopo::Voice::~Voice() {
  delete processor_;
}

 mopo::VoiceHandler::VoiceHandler(size_t polyphony)
  : mopo::ProcessorRouter(static_cast<int>(Inputs::NumInputs), 0),
    polyphony_(0),
    sustain_(false),
    legato_(false),
    voice_killer_(nullptr),
    last_played_note_(-1.0f),
    last_num_voices_(0)
{
  pressed_notes_.reserve(mopo::MIDI_SIZE);
  all_voices_.reserve(mopo::MAX_POLYPHONY);
  free_voices_.reserve(mopo::MAX_POLYPHONY);
  active_voices_.reserve(mopo::MAX_POLYPHONY);

  setPolyphony(polyphony);
  voice_router_.router(this);
  global_router_.router(this);
}

 mopo::VoiceHandler::~VoiceHandler() {
  this->voice_router_.destroy();
  this->global_router_.destroy();

  for (mopo::Voice* voice : this->all_voices_)
    delete voice;

  for (auto& output : this->accumulated_outputs_)
    delete output.second;

  for (auto& output : this->last_voice_outputs_)
    delete output.second;
}

void mopo::VoiceHandler::prepareVoiceTriggers(mopo::Voice* voice) {
  note_.clearTrigger();
  last_note_.clearTrigger();
  note_pressed_.clearTrigger();
  channel_.clearTrigger();
  velocity_.clearTrigger();
  voice_event_.clearTrigger();
  aftertouch_.clearTrigger();
  channel_aftertouch_.clearTrigger();
  channel_.buffer[0] = voice->state().channel;

  if (voice->hasNewEvent()) {
    voice_event_.trigger(voice->state().event, voice->event_sample());
    if (voice->state().event == kVoiceOn) {
      note_.trigger(voice->state().note, 0);
      last_note_.trigger(voice->state().last_note, 0);
      velocity_.trigger(voice->state().velocity, 0);
      note_pressed_.trigger(voice->state().note_pressed, 0);
      channel_.trigger(voice->state().channel, 0);
    }
  }

  if (voice->hasNewAftertouch())
    aftertouch_.trigger(voice->aftertouch(), voice->aftertouch_sample());
  if (voice->hasNewChannelAftertouch())
    channel_aftertouch_.trigger(voice->channelAftertouch(), voice->channelAftertouchSample());

  voice->clearEvents();
}

void mopo::VoiceHandler::processVoice(mopo::Voice* voice) {
  voice->processor()->process();
}

void mopo::VoiceHandler::clearAccumulatedOutputs() {
  for (auto& output : accumulated_outputs_)
  utils::zeroBuffer(std::span<mopo_float>(output.second->buffer, mopo::MAX_BUFFER_SIZE));
}

void mopo::VoiceHandler::clearNonaccumulatedOutputs() {
  for (auto& output : last_voice_outputs_)
  utils::zeroBuffer(std::span<mopo_float>(output.second->buffer, mopo::MAX_BUFFER_SIZE));
}

void mopo::VoiceHandler::accumulateOutputs() {
  for (auto& output : accumulated_outputs_) {
    int buffer_size = output.first->owner->getBufferSize();
    mopo::mopo_float* dest = output.second->buffer;
    const mopo::mopo_float* source = output.first->buffer;

    VECTORIZE_LOOP
    for (int i = 0; i < buffer_size; ++i)
      dest[i] += source[i];
  }
}

void mopo::VoiceHandler::writeNonaccumulatedOutputs() {
  for (auto& output : last_voice_outputs_) {
    int buffer_size = output.first->owner->getBufferSize();
    mopo::mopo_float* dest = output.second->buffer;
    const mopo::mopo_float* source = output.first->buffer;
    utils::copyBuffer(
      std::span<mopo_float>(dest, buffer_size),
      std::span<const mopo_float>(source, buffer_size)
    );
  }
}

bool mopo::VoiceHandler::shouldAccumulate(mopo::Output* output) {
  return !output->owner->isControlRate();
}

void mopo::VoiceHandler::process() {
  global_router_.process();

  int num_voices = static_cast<int>(active_voices_.size());
  if (num_voices == 0) {
    if (last_num_voices_) {
      clearNonaccumulatedOutputs();
      clearAccumulatedOutputs();
    }
    last_num_voices_ = num_voices;
    return;
  }

  int polyphony = static_cast<int>(input(static_cast<int>(Inputs::Polyphony))->at(0));
  setPolyphony(utils::iclamp(polyphony, 1, polyphony));
  clearAccumulatedOutputs();

  auto iter = active_voices_.begin();
  while (iter != active_voices_.end()) {
    mopo::Voice* voice = *iter;
    prepareVoiceTriggers(voice);
    processVoice(voice);
    accumulateOutputs();

    // Remove voice if the right processor has a full silent buffer.
    if (voice_killer_ && voice->state().event != mopo::kVoiceOn &&
  utils::isSilent(std::span<const mopo_float>(voice_killer_->buffer, buffer_size_))) {
      free_voices_.push_back(voice);
      iter = active_voices_.erase(iter);
    } else {
      ++iter;
    }
  }

  if (active_voices_.size())
    writeNonaccumulatedOutputs();

  last_num_voices_ = num_voices;
}

void mopo::VoiceHandler::setSampleRate(int sample_rate) {
  mopo::ProcessorRouter::setSampleRate(sample_rate);
  voice_router_.setSampleRate(sample_rate);
  global_router_.setSampleRate(sample_rate);
  for (auto* voice : all_voices_)
    voice->processor()->setSampleRate(sample_rate);
}

void mopo::VoiceHandler::setBufferSize(int buffer_size) {
  mopo::ProcessorRouter::setBufferSize(buffer_size);
  voice_router_.setBufferSize(buffer_size);
  global_router_.setBufferSize(buffer_size);
  for (auto* voice : all_voices_)
    voice->processor()->setBufferSize(buffer_size);
}

int mopo::VoiceHandler::getNumActiveVoices() {
  return static_cast<int>(active_voices_.size());
}

bool mopo::VoiceHandler::isNotePlaying(mopo::mopo_float note) {
  for (mopo::Voice* voice : active_voices_) {
    if (voice->state().note == note)
      return true;
  }
  return false;
}

void mopo::VoiceHandler::sustainOn() {
  sustain_ = true;
}

void mopo::VoiceHandler::sustainOff(int sample) {
  sustain_ = false;
  for (mopo::Voice* voice : active_voices_) {
    if (voice->key_state() == mopo::Voice::KeyState::Sustained)
      voice->deactivate(sample);
  }
}

void mopo::VoiceHandler::allNotesOff(int sample) {
  pressed_notes_.clear();
  for (mopo::Voice* voice : active_voices_)
    voice->deactivate(sample);
}

void mopo::VoiceHandler::resetState() {
  pressed_notes_.clear();
  active_voices_.clear();
  free_voices_.clear();
  sustain_ = false;
  last_played_note_ = -1.0f;
  last_num_voices_ = 0;
  for (mopo::Voice* voice : all_voices_) {
    voice->deactivate();
    voice->clearEvents();
    voice->processor()->resetState();
    free_voices_.push_back(voice);
  }
  global_router_.resetState();
  voice_router_.resetState();
  mopo::ProcessorRouter::resetState();
  clearAccumulatedOutputs();
  clearNonaccumulatedOutputs();
  for (mopo::Output* output : { &voice_event_, &note_, &last_note_, &note_pressed_,
                               &channel_, &velocity_, &aftertouch_, &channel_aftertouch_ }) {
    output->clearTrigger();
    output->clearBuffer();
  }
}

 mopo::Voice* mopo::VoiceHandler::grabVoice() {
  mopo::Voice* voice = nullptr;
  // First check free voices.
  if (free_voices_.size() &&
      (!legato_ || pressed_notes_.size() < polyphony_ || active_voices_.size() < polyphony_)) {
    voice = free_voices_.front();
    free_voices_.pop_front();
    return voice;
  }
  // Next check released voices.
  for (auto iter = active_voices_.begin(); iter != active_voices_.end(); ++iter) {
    voice = *iter;
    if (voice->key_state() == mopo::Voice::KeyState::Released) {
      active_voices_.erase(iter);
      return voice;
    }
  }
  // Then check sustained voices.
  for (auto iter = active_voices_.begin(); iter != active_voices_.end(); ++iter) {
    voice = *iter;
    if (voice->key_state() == mopo::Voice::KeyState::Sustained) {
      active_voices_.erase(iter);
      return voice;
    }
  }
  // If all are active just grab the oldest voice.
  MOPO_ASSERT(active_voices_.size());
  voice = active_voices_.front();
  active_voices_.pop_front();
  return voice;
}

mopo::Voice* mopo::VoiceHandler::getVoiceToKill() {
  int excess_voices = static_cast<int>(active_voices_.size()) - static_cast<int>(polyphony_);
  mopo::Voice* oldest_released = nullptr;
  mopo::Voice* oldest_sustained = nullptr;
  mopo::Voice* oldest_held = nullptr;
  for (auto iter = active_voices_.begin(); iter != active_voices_.end(); ++iter) {
    mopo::Voice* voice = *iter;
    if (voice->state().event == mopo::kVoiceKill)
      excess_voices--;
    else if (oldest_released == nullptr && voice->key_state() == mopo::Voice::KeyState::Released)
      oldest_released = voice;
    else if (oldest_sustained == nullptr && voice->key_state() == mopo::Voice::KeyState::Sustained)
      oldest_sustained = voice;
    else if (oldest_held == nullptr)
      oldest_held = voice;
  }
  // Return null if we've killed enough voices.
  if (excess_voices <= 0)
    return nullptr;
  // If there were any released notes kill the oldest.
  if (oldest_released)
    return oldest_released;
  // Then if there were any sustained notes kill the oldest.
  if (oldest_sustained)
    return oldest_sustained;
  // If all are active just grab the oldest held voice.
  if (oldest_held)
    return oldest_held;
  return nullptr;
}

void mopo::VoiceHandler::noteOn(mopo::mopo_float note, mopo::mopo_float velocity, int sample, int channel) {
  MOPO_ASSERT(sample >= 0 && sample < buffer_size_);
  MOPO_ASSERT(channel >= 0 && channel < mopo::NUM_MIDI_CHANNELS);
  mopo::Voice* voice = grabVoice();
  pressed_notes_.remove(note);
  pressed_notes_.push_front(note);
  if (last_played_note_ < 0)
    last_played_note_ = note;
  voice->activate(note, velocity, last_played_note_, static_cast<int>(pressed_notes_.size()), sample, channel);
  active_voices_.push_back(voice);
  last_played_note_ = note;
}

mopo::VoiceEvent mopo::VoiceHandler::noteOff(mopo::mopo_float note, int sample) {
  pressed_notes_.removeAll(note);
  mopo::VoiceEvent voice_event = mopo::kVoiceOff;
  for (mopo::Voice* voice : active_voices_) {
    if (voice->state().note == note) {
      if (sustain_)
        voice->sustain();
      else {
        if (polyphony_ <= pressed_notes_.size() && voice->state().event != mopo::kVoiceKill) {
          voice->kill();
          mopo::Voice* new_voice = grabVoice();
          active_voices_.push_back(new_voice);
          mopo::mopo_float old_note = pressed_notes_.back();
          pressed_notes_.pop_back();
          pressed_notes_.push_front(old_note);
          new_voice->activate(old_note, voice->state().velocity, last_played_note_,
                              static_cast<int>(pressed_notes_.size()) + 1, sample);
          last_played_note_ = old_note;
          voice_event = mopo::kVoiceReset;
        } else {
          voice->deactivate(sample);
        }
      }
    }
  }
  return voice_event;
}

void mopo::VoiceHandler::setAftertouch(mopo::mopo_float note, mopo::mopo_float aftertouch,
                                       int sample, int channel) {
  for (mopo::Voice* voice : active_voices_) {
    if (voice->state().note == note &&
        (channel < 0 || voice->state().channel == channel))
      voice->setAftertouch(aftertouch, sample);
  }
}

void mopo::VoiceHandler::setChannelAftertouch(int channel, mopo::mopo_float aftertouch, int sample) {
  for (mopo::Voice* voice : active_voices_) {
    if (voice->state().channel == channel)
      voice->setChannelAftertouch(aftertouch, sample);
  }
}

void mopo::VoiceHandler::setPolyphony(size_t polyphony) {
  while (all_voices_.size() < polyphony) {
    mopo::Voice* new_voice = createVoice();
    all_voices_.push_back(new_voice);
    active_voices_.push_back(new_voice);
  }
  int num_voices_to_kill = static_cast<int>(active_voices_.size()) - static_cast<int>(polyphony);
  for (int i = 0; i < num_voices_to_kill; ++i) {
    mopo::Voice* sacrifice = getVoiceToKill();
    if (sacrifice)
      sacrifice->kill();
  }
  polyphony_ = polyphony;
}

 mopo::mopo_float mopo::VoiceHandler::getLastActiveNote() const {
  if (active_voices_.size())
    return active_voices_.back()->state().note;
  return 0.0;
}

void mopo::VoiceHandler::addProcessor(mopo::Processor* processor) {
  processor->setBufferSize(getBufferSize());
  processor->setSampleRate(getSampleRate());
  voice_router_.addProcessor(processor);
}

void mopo::VoiceHandler::removeProcessor(const mopo::Processor* processor) {
  voice_router_.removeProcessor(processor);
}

void mopo::VoiceHandler::addGlobalProcessor(mopo::Processor* processor) {
  global_router_.addProcessor(processor);
}

void mopo::VoiceHandler::removeGlobalProcessor(mopo::Processor* processor) {
  global_router_.removeProcessor(processor);
}

 mopo::Output* mopo::VoiceHandler::registerOutput(mopo::Output* output) {
  mopo::Output* new_output = new mopo::Output();
  new_output->owner = this;
  mopo::ProcessorRouter::registerOutput(new_output);
  if (shouldAccumulate(output))
    accumulated_outputs_[output] = new_output;
  else
    last_voice_outputs_[output] = new_output;
  return new_output;
}

 mopo::Output* mopo::VoiceHandler::registerOutput(mopo::Output* output, int index) {
  MOPO_ASSERT(false);
  return output;
}

bool mopo::VoiceHandler::isPolyphonic(const mopo::Processor* processor) const {
  return processor == &voice_router_;
}

mopo::Voice* mopo::VoiceHandler::createVoice() {
  return new mopo::Voice(voice_router_.clone());
}
