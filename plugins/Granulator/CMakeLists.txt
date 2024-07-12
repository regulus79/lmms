INCLUDE(BuildPlugin)

BUILD_PLUGIN(granulator Granulator.cpp Granulator.h GranulatorView.cpp GranulatorView.h GranulatorWaveView.cpp GranulatorWaveView.h MOCFILES Granulator.h GranulatorView.h GranulatorWaveView.h EMBEDDED_RESOURCES *.png)
