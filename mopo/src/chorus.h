#pragma once
#ifndef MOPO_CHORUS_H
#define MOPO_CHORUS_H

#include "processor.h"
#include <memory>

namespace mopo {
	class Chorus : public Processor {
		public:
			enum class Inputs {
				Audio,
				On,
				Rate,
				Depth,
				Mix,
				Feedback,
				Delay,
				StereoWidth,
				NumInputs
			};

			Chorus();
			Chorus(const Chorus& other);
			~Chorus() override;
			Processor* clone() const override { return new Chorus(*this); }
			void process() override;
			void setSampleRate(int sample_rate) override;
			void reset();
			/** Reset prepared delay/LFO state without allocating. */
			void resetState() override { reset(); }

		private:
			struct State;
			std::unique_ptr<State> state_;
			bool prepared_ = false;
	};
}

#endif
