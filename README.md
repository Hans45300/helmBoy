# helmBoy

helmBoy is a polyphonic software synthesizer derived from Helm. Its modular
`mopo` DSP engine is shared by the JUCE standalone application and audio plug-in.
The interface brings together oscillators, filters, envelopes, modulation
sources, the arpeggiator, sequencer, and effects.

## Documentation

- [User manual](docs/helmBoy%20online%20manual/index.md)
- [Software architecture](docs/Source%20Documentation/developer/architecture.md)
- [Parameters and state](docs/Source%20Documentation/developer/state-and-parameters.md)
- [MIDI and UMP](docs/Source%20Documentation/developer/midi.md)
- [GUI and threads](docs/Source%20Documentation/developer/gui-and-threads.md)
- [Development and targeted tests](docs/Source%20Documentation/developer/development-and-tests.md)
- [Generated Doxygen API reference](docs/Source%20Documentation/README.md)

## Building and Compiling HelmBoy

### Under Windows

  - Clone this repository.
  - Open an administrative PowerShell session and get to the cloned repo folder.
  - Run the following scripts from the root folder:
      ./utilities/Start-CodingSession.ps1
      ./utilities/buildAndCompile.ps1 -BuildMode All -Config Release

### under Linux

  - Clone this repository.
  - Open an administrative shell session. Depending on your linux flavor, you may have to prefix the following script with "sudo - " instead.
  - Get to the cloned repo folder
  - Run the following scripts from the root folder:
      ./utilities/Start-CodingSession.sh
      ./utilities/buildAndCompile.sh -BuildMode All -Config Release
