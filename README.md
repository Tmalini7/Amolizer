# Amolizer

one knob vocal enhancer. drag the tongue, vocal sounds better. that's it

![Amolizer](docs/screenshot.png)

## what it does

the tongue is the main knob. turn it up and it does a bunch of stuff at once:

- low cut
- cuts the boxy mud around 300hz
- presence boost so the vocal comes forward
- air on top
- de-esser so the S's don't get harsh
- a bit of saturation
- compression (with makeup gain so it doesn't just get quieter)
- limiter at the end so it never clips

at 0% it does basically nothing, at 100% everything is fully on.

there's 4 modes:

- **Smooth** - polished, even, a little shine on top
- **Bright** - more forward and airy, extra de-essing
- **Warm** - thicker, more saturation, softer top end
- **Punchy** - squashed harder with strong mids, for busy mixes

plus an output knob to level match.

## download

grab the latest zip from [Releases](../../releases) (or from the Actions tab if there's no release yet).

**Windows:** unzip and put the `Amolizer.vst3` folder in `C:\Program Files\Common Files\VST3\`

**Mac:** unzip and put `Amolizer.vst3` in `~/Library/Audio/Plug-Ins/VST3/`. it's not signed so macOS will probably block it, run this once in terminal to fix:

```
xattr -cr ~/Library/Audio/Plug-Ins/VST3/Amolizer.vst3
```

then in FL go to Options > Manage plugins > Find installed plugins and it should show up.

## building it yourself

needs CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows, Xcode on Mac). JUCE gets downloaded automatically.

```
cmake -B build
cmake --build build --config Release
```

the plugin ends up in `build/Amolizer_artefacts/Release/VST3/`

## tweaking the sound

all the voicing is in the `kModes` table at the top of `Source/PluginProcessor.cpp`. every number is what you get at 100%, change it and rebuild.

## license

built with [JUCE](https://juce.com). free for personal use, check their license if you want to sell it.
