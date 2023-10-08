# SiTraNo
SiTraNo $^+$ : An Audio Application For Signal Decomposition and Time-Scale Modification

I made this app for my summer project at the Acoutics Lab at Aalto University.

This demo app can be used for audio signal processing-related courses. It uses the 2-stage STN decomposition to separate the audio signal's sines, transients, and noise components.

The imported audio file (.wav, .mp3, and .flac) will be trimmed to 20 seconds if it is longer. This is done because the code was not optimized and could take an excessive amount of time to process long audio files.


JUCE v6 or v7 can be used to build the app. A pre-built app for Windows is also available in the Release section.
