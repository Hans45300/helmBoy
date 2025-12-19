# helmBoy

![helmBoy main Interface](./docs/helmBoy%20online%20manual/pictures/whole%20interface.png)

## helm... Boy

HelmBoy is a sequel to a historic virtual synthesizer, ![helm](https://tytel.org/helm/), and a fork of ![Bepzi's own fork](https://github.com/bepzi/helm) of the original one.

It began as a personal gambit, playing back with c++, JUCE, and thinking of what it would be to get Helm back nowadays. As it's now in a usable state, I have the pleasure to share it, hoping you will have fun with it, and as a beginning for new forks adventures, maybe.

## Changes

### Under the hood

* Code update, to build and compile under C++20 (compiled under C++14).
* Code refactoring, using C++17 and C++20 otimisations
* Use of submodules, to keep project up to date in the future:
  * ![JUCE framework](https://github.com/juce-framework/JUCE) from ![JUCE](https://juce.com/)
  * ![VST3 SDK](https://github.com/steinbergmedia/vst3sdk) from ![Steinberg](https://www.steinberg.net/)
  * ![xsimd](https://github.com/xtensor-stack/xsimd) from xtensor, to allow SIMD optimisation, preserving a good compatibility

### Synthesizer structure update

* Added waveforms display and possibility of direct choice (existed in code, but were disabled?)
* Added 2 mono LFO and 1 Poly LFO
* Extended step sequencer to a max of 64 steps.
* Hability to set individual step using direction arrows (left-right to change step, up-down to increase-decrease value, with .01 step, of .1 step using Ctrl ; also with 1/48 step using Shift, and 6/48 using Shift + Ctrl)
* Experimental midi 2.0 implementation (not tested)
* Phase Shifting on every waveforms (LFO and oscillators): like PWM for square, stretching the wave left or right depending on knob value
* Ring modulation
* Frequency modulation

### Miscellaneous

* logo and design changes (tastes and colours...)
* added a 250% size option to about window
* lots of templates patches added, to allow easy sound design beginning
* Full documentation:
  * ![code](./docs/Source%20Documentation/index.html)
  * ![user manual](./docs/helmBoy%20online%20manual/index.md)

### Known issues

* Some controls keep black when launching the synth. Mouse over it or displaying the about window get rid of it.
* Linux and MacOS compilations not tested at this time (play with it and let me know if it's OK!)
* Asio initialisation in standalone mode sometimes a bit quirky (JUCE has some difficulties with my Steinberg ur44c audio interface). First try misses, second one is the good one.

## The following softwares were used to achieve this result

* Microsoft Visual Studio 2026
* ![Inkscape](https://inkscape.org/)
* ![cMake](https://cmake.org/)
* ![Doxygen](https://www.doxygen.nl/)
* ![Graphviz Dot](https://graphviz.org/)
* ![Paint.NET](https://www.getpaint.net/)
* ![ImageMagick](https://imagemagick.org/)

Have fun!

Marc Scheffer, 2025 12 19
