#include <iostream>
#include <sstream>
#include <bitset>
#include <Audioclient.h>

#include "log.h"
#include "DeviceEnumerator.h"

using std::cout;
using std::endl;

std::string indent(unsigned short level) {
    std::stringstream ss;
    for (size_t i = 0; i < level; i++)
        ss << "\t";
    ss << " - ";
    return ss.str();
}

void log_all_devices_list(const AudioDeviceList& allDevices) {
    cout << "Listing INPUT devices:" << endl;
    for (size_t i = 0; i < allDevices.inputDevices.size(); i++)
    {
        auto& device = allDevices.inputDevices[i];
        cout << "\t - " << i << " " << device.friendlyName << " - " << device.id << endl;
    }

    cout << endl << "Listing OUTPUT devices:" << endl;
    for (size_t i = 0; i < allDevices.outputDevices.size(); i++)
    {
        auto& device = allDevices.outputDevices[i];
        cout << "\t - " << i << " " << device.friendlyName << " - " << device.id << endl;
    }
}

void log_wave_format(const WAVEFORMATEX& format, unsigned int level) {
    std::string formatTag;
    if (format.wFormatTag == WAVE_FORMAT_PCM)
        formatTag = "PCM";
    else if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        formatTag = "EXTENSIBLE";
    else
        formatTag = std::to_string(format.wFormatTag);

    cout << indent(level) << "tag: " << formatTag << endl;
    cout << indent(level) << "Channels: " << format.nChannels << endl;
    cout << indent(level) << "Samples per seconds: " << format.nSamplesPerSec << endl;
    cout << indent(level) << "Bits per Sample: " << format.wBitsPerSample << endl;
    cout << indent(level) << "Avg bytes per second: " << format.nAvgBytesPerSec << endl;
    cout << indent(level) << "Block alignment: " << format.nBlockAlign << endl;
}

void log_wave_format_ex(const WAVEFORMATEXTENSIBLE& format, unsigned int level) {
    log_wave_format(format.Format, level);

    std::string formatTag;
    if (format.SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
        formatTag = "PCM";
    else if (format.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        formatTag = "IEEE_FLOAT";
    else if (format.SubFormat == KSDATAFORMAT_SUBTYPE_DRM)
        formatTag = "DRM";
    else
        formatTag = "??? (check Ksmedia.h for a list of formats) ";

    cout << indent(level) << "samples in one block of audio data: " << format.Samples.wSamplesPerBlock << endl;
    cout << indent(level) << "Bits of precision in the signal: " << format.Samples.wValidBitsPerSample << endl;
    cout << indent(level) << "Channel mask: " << std::bitset<sizeof(DWORD) * 8>(format.dwChannelMask) << endl;
    cout << indent(level) << "tag: " << formatTag << endl;
}

void log_extended_info_1(const AudioInfo1& info, unsigned short level) {
    cout << indent(level) << "Default device buffer period: " << info.defaultDevicePeriod / 10000 << "ms" << endl;
    cout << indent(level) << "Minimum device buffer period: " << info.minDevicePeriod / 10000 << "ms" << endl;
    //cout << indent(level) << "Latency: " << info.latency << endl;
    //cout << indent(level) << "Buffer size in frames: " << info.bufferSizeInFrames << endl;
    //cout << indent(level) << "Buffer size in bytes: " << info.bufferSizeInFrames * waveFormat->nBlockAlign * waveFormat->nChannels

    if (info.extendedStreamFormat.has_value()) {
        cout << indent(level) << "PROPERTIES FROM WAVEFORMATEXTENSIBLE" << endl;
        log_wave_format_ex(info.extendedStreamFormat.value(), level+1);
    }
    else if (info.streamFormat.has_value()) {
        cout << indent(level) << "PROPERTIES FROM WAVEFORMATEX" << endl;
        log_wave_format(info.streamFormat.value(), level+1);
    }
}

void log_extended_info_2(const AudioInfo2& info, unsigned short level) {
    long min = info.minSupportedBufferDuration / 10000;
    long max = info.maxSupportedBufferDuration / 10000;
    cout << indent(level) << "Is offload capable: " << info.isOffloadCapable << endl;
    cout << indent(level) << "Supported buffer durations [min, max]: ["<< min << "ms, " << max << "ms]" << endl;
}

void log_extended_info_3(const AudioInfo3& info, unsigned short level) {
    cout << indent(level) << "Default period for each frame: " << info.defaultPeriodInFrames << endl;
    cout << indent(level) << "Fundamental frame period: " << info.fundamentalPeriodInFrames << endl;
    cout << indent(level) << "[min, max] periods for each frame: [" << info.minPeriodInFrames << ", " << info.maxPeriodInFrames << "]" << endl;
    cout << indent(level) << "Current shared-mode period for each frame: " << info.currentSharedModePeriodInFrames << endl;
}

void log_device_details(const AudioDeviceDetails& deviceInfo)
{
    auto summary = deviceInfo.summary.value();
    cout << "*** DEVICE " << summary.friendlyName << endl;
    cout << indent(0) << "id: " << summary.id << endl;
    cout << indent(0) << "direction: " << (summary.direction == EDataFlow::eCapture ? "input" : "output") << endl;

    if (deviceInfo.extendedInfo1.has_value()) {
        cout << indent(0) << "PROPERTIES FROM AudioDevice 1" << endl;
        log_extended_info_1(deviceInfo.extendedInfo1.value(), 1);
    }
    
    if (deviceInfo.extendedInfo2.has_value()) {
        cout << indent(0) << "PROPERTIES FROM AudioDevice 2" << endl;
        log_extended_info_2(deviceInfo.extendedInfo2.value(), 1);
    }

    if (deviceInfo.extendedInfo3.has_value()) {
        cout << indent(0) << "PROPERTIES FROM AudioDevice 3" << endl;
        log_extended_info_3(deviceInfo.extendedInfo3.value(), 1);
    }

    cout << "**********" << endl << endl;
}

