#pragma once

#include <functional>
#include <endpointvolume.h>
#include <iostream>

#include "common.h"


typedef std::function<void(const VolumeInfo& volumeInfo)> VolumeChangeCallback;

class VolumeNotificationProvider : public IAudioEndpointVolumeCallback
{
public:
    ~VolumeNotificationProvider(void) = default;
    VolumeNotificationProvider(void);

    SubscriptionId subscribe_volume_changes(VolumeChangeCallback callback);
    void unsubscribe(SubscriptionId id);

    STDMETHODIMP_(ULONG)AddRef() override;
    STDMETHODIMP_(ULONG)Release() override;
    STDMETHODIMP QueryInterface(REFIID IID, void** ReturnValue) override;
    STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA notification) override;

private:
    LONG m_RefCount;
    std::unordered_map<SubscriptionId, VolumeChangeCallback> volumeSubscriptionMap;
};

