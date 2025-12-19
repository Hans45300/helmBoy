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

#include "formant_manager.h"

/**
 * @file formant_manager.cpp
 * @brief Manages multiple band filters to produce vocal-like formant regions.
 *
 * The manager keeps a set of biquads tuned to formant frequencies and exposes
 * a simple API to set vowel positions and bandwidths. Designed for efficient
 * real-time updates.
 */

#include "biquad_filter.h"
#include "operators.h"

namespace mopo {

  FormantManager::FormantManager(int num_formants) : ProcessorRouter(0, 0) {
    Bypass* audio_input = new Bypass();
    cr::Bypass* reset_input = new cr::Bypass();

    registerInput(audio_input->input(), static_cast<int>(Inputs::Audio));
    registerInput(reset_input->input(), static_cast<int>(Inputs::Reset));

    addProcessor(audio_input);
    addProcessor(reset_input);

    VariableAdd* total = new VariableAdd(num_formants);
    for (int i = 0; i < num_formants; ++i) {
      BiquadFilter* formant = new BiquadFilter();
  formant->plug(audio_input, static_cast<int>(mopo::BiquadFilter::Inputs::Audio));
  formant->plug(reset_input, static_cast<int>(mopo::BiquadFilter::Inputs::Reset));
      formants_.push_back(formant);

      addProcessor(formant);
      total->plugNext(formant);
    }

    addProcessor(total);
    registerOutput(total->output());
  }

  std::complex<mopo_float> FormantManager::getResponse(mopo_float frequency) {
    std::complex<mopo_float> total;
    for (int i = 0; i < formants_.size(); ++i)
      total += formants_[i]->getResponse(frequency);

    return total;
  }
} // namespace mopo
