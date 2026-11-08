#include "chorus.h"

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

namespace mopo {
	namespace {
		float boundedChorusValue(float value, float minimum, float maximum, float fallback) {
			return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
		}
	}

	struct Chorus::State {
		juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
		juce::dsp::Oscillator<float> left_lfo { [](float phase) { return std::sin(phase); } };
		juce::dsp::Oscillator<float> right_lfo { [](float phase) { return std::cos(phase); } };
		juce::SmoothedValue<float> mix;
		juce::SmoothedValue<float> feedback;
		juce::SmoothedValue<float> width;
		float last_left = 0.0f;
		float last_right = 0.0f;
	};

	Chorus::Chorus() : Processor(static_cast<int>(Inputs::NumInputs), 2),
											 state_(std::make_unique<State>()) {
		setSampleRate(sample_rate_);
	}

	Chorus::Chorus(const Chorus& other) : Processor(other), state_(std::make_unique<State>()) {
		setSampleRate(sample_rate_);
	}

	Chorus::~Chorus() = default;

	void Chorus::setSampleRate(int sample_rate) {
		if (prepared_ && sample_rate == sample_rate_)
			return;
		Processor::setSampleRate(sample_rate);
		prepared_ = false;
		if (sample_rate <= 0)
			return;
		try {
			state_->delay.setMaximumDelayInSamples(static_cast<int>(std::ceil(sample_rate * 0.111)) + 2);
			const juce::dsp::ProcessSpec spec { static_cast<double>(sample_rate), MAX_BUFFER_SIZE, 2 };
			state_->delay.prepare(spec);
			state_->left_lfo.prepare(spec);
			state_->right_lfo.prepare(spec);
			state_->mix.reset(sample_rate, 0.02);
			state_->feedback.reset(sample_rate, 0.02);
			state_->width.reset(sample_rate, 0.02);
			reset();
			prepared_ = true;
		}
		catch (...) {
			prepared_ = false;
		}
	}

	void Chorus::reset() {
		state_->delay.reset();
		state_->left_lfo.reset();
		state_->right_lfo.reset();
		state_->mix.setCurrentAndTargetValue(0.0f);
		state_->feedback.setCurrentAndTargetValue(0.0f);
		state_->width.setCurrentAndTargetValue(1.0f);
		state_->last_left = state_->last_right = 0.0f;
	}

	void Chorus::process() {
		if (buffer_size_ <= 0 || buffer_size_ > MAX_BUFFER_SIZE)
			return;
		MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));
		const auto* audio = input(static_cast<int>(Inputs::Audio))->source->buffer;
		auto* left = output(0)->buffer;
		auto* right = output(1)->buffer;
		if (!prepared_) {
			for (int sample = 0; sample < buffer_size_; ++sample)
				left[sample] = right[sample] = std::isfinite(audio[sample]) ? audio[sample] : 0.0f;
			return;
		}

		const auto parameter = [this](Inputs index, float minimum, float maximum, float fallback) {
			return boundedChorusValue(input(static_cast<int>(index))->at(0), minimum, maximum, fallback);
		};
		const float rate = parameter(Inputs::Rate, 0.05f, 10.0f, 0.8f);
		const float depth = parameter(Inputs::Depth, 0.0f, 1.0f, 0.25f);
		const float centre_delay = parameter(Inputs::Delay, 1.0f, 100.0f, 8.0f);
		const float excursion = std::min(10.0f * depth, centre_delay - 1.0f);
		const float samples_per_ms = sample_rate_ * 0.001f;
		const bool on = parameter(Inputs::On, 0.0f, 1.0f, 0.0f) >= 0.5f;
		state_->mix.setTargetValue(on ? parameter(Inputs::Mix, 0.0f, 1.0f, 0.5f) : 0.0f);
		state_->feedback.setTargetValue(parameter(Inputs::Feedback, -0.95f, 0.95f, 0.0f));
		state_->width.setTargetValue(parameter(Inputs::StereoWidth, 0.0f, 1.0f, 1.0f));
		state_->left_lfo.setFrequency(rate);
		state_->right_lfo.setFrequency(rate);

		for (int sample = 0; sample < buffer_size_; ++sample) {
			const float dry = std::isfinite(audio[sample]) ? audio[sample] : 0.0f;
			const float feedback = state_->feedback.getNextValue();
			const float left_delay = (centre_delay + excursion * state_->left_lfo.processSample(0.0f)) * samples_per_ms;
			const float right_delay = (centre_delay + excursion * state_->right_lfo.processSample(0.0f)) * samples_per_ms;
			const float left_input = dry + feedback * state_->last_left;
			const float right_input = dry + feedback * state_->last_right;
			state_->delay.pushSample(0, std::isfinite(left_input) ? left_input : 0.0f);
			state_->delay.pushSample(1, std::isfinite(right_input) ? right_input : 0.0f);
			state_->last_left = state_->delay.popSample(0, left_delay);
			state_->last_right = state_->delay.popSample(1, right_delay);
			if (!std::isfinite(state_->last_left) || !std::isfinite(state_->last_right)) {
				state_->delay.reset();
				state_->last_left = state_->last_right = 0.0f;
			}
			const float wet_mid = 0.5f * state_->last_left + 0.5f * state_->last_right;
			const float wet_side = (0.5f * state_->last_left - 0.5f * state_->last_right) * state_->width.getNextValue();
			const float mix = state_->mix.getNextValue();
			const float result_left = dry + mix * (wet_mid + wet_side - dry);
			const float result_right = dry + mix * (wet_mid - wet_side - dry);
			left[sample] = std::isfinite(result_left) ? result_left : dry;
			right[sample] = std::isfinite(result_right) ? result_right : dry;
		}
	}
}
