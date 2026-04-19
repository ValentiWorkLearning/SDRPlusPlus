## Build for macos

```shell
brew install libusb fftw glfw airspy airspyhf portaudio hackrf rtl-sdr libbladerf codec2 zstd volk
pip3 install mako --break-system-packages

cmake -B build -S . -DOPT_BUILD_SOAPY_SOURCE=ON -DOPT_BUILD_PLUTOSDR_SOURCE=OFF -DOPT_BUILD_BLADERF_SOURCE=OFF -DOPT_BUILD_AUDIO_SOURCE=OFF -DOPT_BUILD_AUDIO_SINK=OFF -DOPT_BUILD_PORTAUDIO_SINK=ON -DOPT_BUILD_NEW_PORTAUDIO_SINK=ON -DOPT_BUILD_M17_DECODER=OFF -DUSE_BUNDLE_DEFAULTS=ON -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel=8

sh make_macos_bundle.sh ./build ./SDR++.app
```