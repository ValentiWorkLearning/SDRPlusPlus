#include <imgui.h>
#include <module.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
#include <signal_path/sink.h>
#include <portaudio.h>
#include <dsp/buffer/packer.h>
#include <dsp/convert/stereo_to_mono.h>
#include <utils/flog.h>
#include <config.h>
#include <algorithm>
#include <core.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <cstring>

#define AUDIO_LATENCY 0.1

SDRPP_MOD_INFO{
    /* Name:            */ "new_portaudio_sink",
    /* Description:     */ "Audio sink module for SDR++",
    /* Author:          */ "Ryzerth;Maxime Biette",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ 1
};

ConfigManager config;

class AudioSink : SinkManager::Sink {
public:
    struct AudioDevice_t {
        const PaDeviceInfo* deviceInfo = nullptr;
        const PaHostApiInfo* hostApiInfo = nullptr;
        PaDeviceIndex id = paNoDevice;
        int defaultSrId = 0;
        PaStreamParameters outputParams{};
        std::vector<double> sampleRates;
        std::string sampleRatesTxt;
    };

    AudioSink(SinkManager::Stream* stream, std::string streamName) {
        _stream = stream;
        _streamName = std::move(streamName);

        // Create config if it doesn't exist
        config.acquire();
        if (!config.conf.contains(_streamName)) {
            config.conf[_streamName]["device"] = "";
            config.conf[_streamName]["devices"] = json::object();
        }
        std::string selected = config.conf[_streamName]["device"];
        config.release(true);

        // Register the play state handler
        playStateHandler.handler = playStateChangeHandler;
        playStateHandler.ctx = this;
        gui::mainWindow.onPlayStateChange.bindHandler(&playStateHandler);

        // Initialize DSP blocks
        packer.init(_stream->sinkOut, 1024);
        s2m.init(&packer.out);

        // Refresh devices and select the one from the config
        refreshDevices();
        selectDevByName(selected);
    }

    ~AudioSink() {
        stop();
        gui::mainWindow.onPlayStateChange.unbindHandler(&playStateHandler);
    }

    void start() {
        if (running || selectedDevName.empty())
            return;

        auto& dev = devices[deviceNames[devId]];

        const double sampleRate = dev.sampleRates[srId];
        const int blockSize = 2048;

        currentSampleRate = sampleRate;
        currentFrameCount = blockSize;

        _stream->setSampleRate(sampleRate);
        packer.setSampleCount(blockSize);

        stereo = dev.deviceInfo->maxOutputChannels > 1;

        resetStats();
        clearQueue();

        packer.out.clearReadStop();
        s2m.out.clearReadStop();

        packer.start();
        if (!stereo)
            s2m.start();

        workerRunning = true;
        workerThread = std::thread(&AudioSink::workerLoop, this);

        PaError err;

        if (stereo) {
            err = Pa_OpenStream(
                &devStream,
                nullptr,
                &dev.outputParams,
                sampleRate,
                blockSize,
                paNoFlag,
                _stereo_cb,
                this);
        }
        else {
            err = Pa_OpenStream(
                &devStream,
                nullptr,
                &dev.outputParams,
                sampleRate,
                blockSize,
                paNoFlag,
                _mono_cb,
                this);
        }

        if (err != paNoError) {
            flog::error("Pa_OpenStream failed: {}", Pa_GetErrorText(err));
            stopWorkerAndDsp();
            return;
        }

        // Start stream
        err = Pa_StartStream(devStream);
        if (err != paNoError) {
            flog::error("Pa_StartStream failed: {}", Pa_GetErrorText(err));
            Pa_CloseStream(devStream);
            devStream = nullptr;
            stopWorkerAndDsp();
            return;
        }

        flog::info("Audio started: {} Hz block={} stereo={}",
                   sampleRate, blockSize, stereo);

        running = true;
    }

    void stop() {
        if (!running && !workerRunning.load())
            return;

        running = false;

        if (devStream) {
            Pa_AbortStream(devStream);
            Pa_CloseStream(devStream);
            devStream = nullptr;
        }

        stopWorkerAndDsp();
    }

    void menuHandler() {
        float menuWidth = ImGui::GetContentRegionAvail().x;

        // Select device
        ImGui::SetNextItemWidth(menuWidth);
        if (ImGui::Combo("##audio_sink_dev_sel", &devId, deviceNamesTxt.c_str())) {
            selectDevByName(deviceNames[devId]);
            stop();
            start();
            if (selectedDevName != "") {
                config.acquire();
                config.conf[_streamName]["device"] = selectedDevName;
                config.release(true);
            }
        }

        // Select sample rate
        ImGui::SetNextItemWidth(menuWidth);
        if (ImGui::Combo("##audio_sink_sr_sel", &srId, selectedDev.sampleRatesTxt.c_str())) {
            stop();
            start();
            if (selectedDevName != "") {
                config.acquire();
                config.conf[_streamName]["devices"][selectedDevName] = selectedDev.sampleRates[srId];
                config.release(true);
            }
        }

        ImGui::Separator();
        ImGui::Text("Audio Diagnostics");

        ImGui::Text("Callbacks: %llu",
                    (unsigned long long)callbackCount.load());

        ImGui::Text("PA Underflows: %llu",
                    (unsigned long long)paUnderflows.load());

        ImGui::Text("PA Overflows: %llu",
                    (unsigned long long)paOverflows.load());

        ImGui::Text("Queue Underruns: %llu",
                    (unsigned long long)queueUnderruns.load());

        ImGui::Text("Queue Overruns: %llu",
                    (unsigned long long)queueOverruns.load());

        ImGui::Text("Queue Fill: %zu blocks", queueSize());

        ImGui::Text("Late Callbacks: %llu",
                    (unsigned long long)lateCallbacks.load());

        ImGui::Text("Last CB delta: %.2f ms",
                    lastCallbackDeltaMs.load());

        ImGui::Text("Max CB delta: %.2f ms",
                    maxCallbackDeltaMs.load());

        double expected =
            1000.0 * (double)currentFrameCount / currentSampleRate;

        ImGui::Text("Expected period: %.2f ms", expected);

        if (ImGui::Button("Reset Stats"))
            resetStats();
    }

    int devId = 0;
    int srId = 0;

private:
    struct AudioBlock {
        std::vector<float> data;
    };

    static void playStateChangeHandler(bool newState, void* ctx) {
        auto* self = static_cast<AudioSink*>(ctx);

        if (newState) {
            self->packer.out.clearReadStop();
            self->s2m.out.clearReadStop();
        }
        else {
            self->packer.out.stopReader();
            self->s2m.out.stopReader();
            self->clearQueue();
        }
    }

    void stopWorkerAndDsp() {
        workerRunning = false;

        packer.out.stopReader();
        s2m.out.stopReader();

        if (workerThread.joinable())
            workerThread.join();

        if (!stereo)
            s2m.stop();

        packer.stop();

        clearQueue();
    }

    void workerLoop() {
        while (workerRunning.load()) {

            if (!gui::mainWindow.isPlaying()) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(2));
                continue;
            }

            AudioBlock blk;

            if (stereo) {
                packer.out.read();

                if (!packer.out.readBuf) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1));
                    continue;
                }

                blk.data.resize(currentFrameCount * 2);

                std::memcpy(
                    blk.data.data(),
                    packer.out.readBuf,
                    blk.data.size() * sizeof(float));

                packer.out.flush();
            }
            else {
                s2m.out.read();

                if (!s2m.out.readBuf) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1));
                    continue;
                }

                blk.data.resize(currentFrameCount);

                std::memcpy(
                    blk.data.data(),
                    s2m.out.readBuf,
                    blk.data.size() * sizeof(float));

                s2m.out.flush();
            }

            {
                std::lock_guard<std::mutex> lock(queueMtx);
                audioQueue.push(std::move(blk));
            }
        }
    }

    static int _mono_cb(
        const void*,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo*,
        PaStreamCallbackFlags statusFlags,
        void* userData) {
        auto* self = static_cast<AudioSink*>(userData);

        self->updateCallbackTiming(frameCount);

        if (statusFlags & paOutputUnderflow)
            self->paUnderflows++;

        if (statusFlags & paOutputOverflow)
            self->paOverflows++;

        float* out = static_cast<float*>(output);

        self->popQueue(out, frameCount);

        return paContinue;
    }

    static int _stereo_cb(
        const void*,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo*,
        PaStreamCallbackFlags statusFlags,
        void* userData) {
        auto* self = static_cast<AudioSink*>(userData);

        self->updateCallbackTiming(frameCount);

        if (statusFlags & paOutputUnderflow)
            self->paUnderflows++;

        if (statusFlags & paOutputOverflow)
            self->paOverflows++;

        float* out = static_cast<float*>(output);

        self->popQueue(out, frameCount * 2);

        return paContinue;
    }

    void popQueue(float* dst, size_t samplesNeeded) {
        std::lock_guard<std::mutex> lock(queueMtx);

        if (audioQueue.empty()) {
            std::memset(dst, 0, samplesNeeded * sizeof(float));
            queueUnderruns++;
            return;
        }

        auto& blk = audioQueue.front();

        if (blk.data.size() < samplesNeeded) {
            std::memset(dst, 0, samplesNeeded * sizeof(float));
            queueUnderruns++;
            audioQueue.pop();
            return;
        }

        std::memcpy(
            dst,
            blk.data.data(),
            samplesNeeded * sizeof(float));

        audioQueue.pop();
    }

    void clearQueue() {
        std::lock_guard<std::mutex> lock(queueMtx);

        while (!audioQueue.empty())
            audioQueue.pop();
    }

    size_t queueSize() {
        std::lock_guard<std::mutex> lock(queueMtx);
        return audioQueue.size();
    }

    void resetStats() {
        paUnderflows = 0;
        paOverflows = 0;
        queueUnderruns = 0;
        queueOverruns = 0;
        lateCallbacks = 0;
        callbackCount = 0;
        lastCallbackDeltaMs = 0;
        maxCallbackDeltaMs = 0;
        cbHasLastTs = false;
    }

    void updateCallbackTiming(unsigned long frameCount) {
        using namespace std::chrono;

        auto now = steady_clock::now();

        if (cbHasLastTs) {
            double dtMs =
                duration_cast<
                    duration<double, std::milli>>(
                    now - cbLastTs)
                    .count();

            lastCallbackDeltaMs = dtMs;

            if (dtMs > maxCallbackDeltaMs.load())
                maxCallbackDeltaMs = dtMs;

            double expected =
                1000.0 *
                (double)frameCount /
                currentSampleRate;

            if (dtMs > expected * 1.5)
                lateCallbacks++;
        }

        cbLastTs = now;
        cbHasLastTs = true;
        callbackCount++;
    }

    void refreshDevices() {
        // Clear current list
        devices.clear();
        deviceNames.clear();
        deviceNamesTxt.clear();

        // Get number of devices
        int devCount = Pa_GetDeviceCount();
        constexpr size_t bufSize = 256;
        char buf[bufSize];

        for (int i = 0; i < devCount; i++) {
            AudioDevice_t dev;

            // Get device info
            dev.deviceInfo = Pa_GetDeviceInfo(i);
            if (!dev.deviceInfo || dev.deviceInfo->maxOutputChannels == 0){
                continue;
            }

            dev.hostApiInfo =
                Pa_GetHostApiInfo(dev.deviceInfo->hostApi);

            dev.id = i;

#ifdef _WIN32
            // On Windows, use only WASAPI
            if (dev.hostApiInfo->type == paMME || dev.hostApiInfo->type == paWDMKS) { continue; }
#endif
            // Zero out output params
            dev.outputParams.device = i;
            dev.outputParams.sampleFormat = paFloat32;
            dev.outputParams.channelCount =
                std::min<int>(
                    dev.deviceInfo->maxOutputChannels,
                    2);

            dev.outputParams.suggestedLatency =
                std::min<PaTime>(
                    AUDIO_LATENCY,
                    dev.deviceInfo->defaultLowOutputLatency);

            dev.outputParams.hostApiSpecificStreamInfo =
                nullptr;

            for (int sr = 12000; sr <= 192000; sr += 12000) {
                if (Pa_IsFormatSupported(
                        nullptr,
                        &dev.outputParams,
                        sr) == paFormatIsSupported)
                    dev.sampleRates.push_back(sr);
            }

            // If no sample rates are supported, cancel adding device
            if (dev.sampleRates.empty()) {
                continue;
            }

            int idx = 0;
            for (auto sr : dev.sampleRates) {
                std::snprintf(buf,
                              sizeof(buf),
                              "%d",
                              (int)sr);

                dev.sampleRatesTxt += buf;
                dev.sampleRatesTxt += '\0';

                if (sr == 48000)
                    dev.defaultSrId = idx;

                idx++;
            }

            std::snprintf(
                buf,
                sizeof(buf),
                "[%s] %s",
                dev.hostApiInfo->name,
                dev.deviceInfo->name);

            devices[buf] = dev;
            deviceNames.push_back(buf);
            deviceNamesTxt += buf;
            deviceNamesTxt += '\0';
        }
    }

    void selectDefault() {
        if (devices.empty()) {
            selectedDevName.clear();
            return;
        }

        // If default not found, select first
        selectDevByName(deviceNames[0]);
    }

    void selectDevByName(std::string name) {
        auto it =
            std::find(deviceNames.begin(),
                      deviceNames.end(),
                      name);

        if (it == deviceNames.end()) {
            selectDefault();
            return;
        }

        // Load the device name, device descriptor and device ID
        selectedDevName = name;
        selectedDev = devices[name];
        devId =
            (int)std::distance(
                deviceNames.begin(),
                it);

        srId = selectedDev.defaultSrId;
    }

private:
    std::string _streamName;
    bool running = false;
    bool stereo = false;

    std::map<std::string, AudioDevice_t> devices;
    std::vector<std::string> deviceNames;
    std::string deviceNamesTxt;

    AudioDevice_t selectedDev;
    std::string selectedDevName;

    SinkManager::Stream* _stream = nullptr;

    dsp::buffer::Packer<dsp::stereo_t> packer;
    dsp::convert::StereoToMono s2m;

    PaStream* devStream = nullptr;
    EventHandler<bool> playStateHandler;

    std::thread workerThread;
    std::atomic<bool> workerRunning{ false };

    std::queue<AudioBlock> audioQueue;
    std::mutex queueMtx;
    std::atomic<uint64_t> paUnderflows{ 0 };
    std::atomic<uint64_t> paOverflows{ 0 };
    std::atomic<uint64_t> queueUnderruns{ 0 };
    std::atomic<uint64_t> queueOverruns{ 0 };
    std::atomic<uint64_t> lateCallbacks{ 0 };
    std::atomic<uint64_t> callbackCount{ 0 };

    std::atomic<double> lastCallbackDeltaMs{ 0 };
    std::atomic<double> maxCallbackDeltaMs{ 0 };

    double currentSampleRate = 48000.0;
    unsigned long currentFrameCount = 2048;

    std::chrono::steady_clock::time_point cbLastTs{};
    bool cbHasLastTs = false;
};

class AudioSinkModule : public ModuleManager::Instance {
public:
    AudioSinkModule(std::string name) {
        this->name = std::move(name);

        provider.create = create_sink;
        provider.ctx = this;

        Pa_Initialize();

        sigpath::sinkManager.registerSinkProvider(
            "New Audio",
            provider);
    }

    ~AudioSinkModule() {
        sigpath::sinkManager.unregisterSinkProvider(
            "New Audio");

        Pa_Terminate();
    }

    void postInit() {}
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool isEnabled() { return enabled; }

private:
    static SinkManager::Sink* create_sink(
        SinkManager::Stream* stream,
        std::string streamName,
        void*) {
        return (SinkManager::Sink*)new AudioSink(
            stream,
            std::move(streamName));
    }

    std::string name;
    bool enabled = true;
    SinkManager::SinkProvider provider;
};

MOD_EXPORT void _INIT_() {
    config.setPath(
        core::args["root"].s() +
        "/new_audio_sink_config.json");

    config.load(json::object());
    config.enableAutoSave();
}

MOD_EXPORT void* _CREATE_INSTANCE_(std::string name) {
    return new AudioSinkModule(std::move(name));
}

MOD_EXPORT void _DELETE_INSTANCE_(void* instance) {
    delete (AudioSinkModule*)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}