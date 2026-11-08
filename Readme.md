# lkjb TinyQ

## Build steps:

- copy VST3 SDK and VST SDK files in according directories in wdl_ol/
- build TinyQ in TinyQ_wdl/TinyQ.sln

If only the VST3 SDK should be build remove the vst2 project from the qrange.sln solution.

The project contains solutions for Visual Studio 2022. There are no macOS/XCode project files as I don't have a Mac anymore to test it. But It shouldn't be to hard to get a current AI LLM to create a CMakeLists.txt from the Visual Studio solution and project files. 

The SimdBandfiler class used for the BandAnalyser uses Intel intrinsics. This might be a problem when compiling for ARM like Apple Silicon. Libraries like https://github.com/DLTcollab/sse2neon could help but this hasn't been tested.

## 3rd Party Libraries used with TinyQ

BassTame uses WDL / IPlug - Oli Larkin Edition. See wdl-ol/readme.txt for more information.

It also uses the ISC licensed juce_core module from JUCE 5.3.2.

Some filters are inspired from https://github.com/vinniefalco/DSPFilters with modifications
