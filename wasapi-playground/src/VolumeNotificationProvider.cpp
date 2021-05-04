#include "VolumeNotificationProvider.h"

VolumeNotificationProvider::VolumeNotificationProvider(void) : m_RefCount(1)
{
}

SubscriptionId VolumeNotificationProvider::subscribe_volume_changes(VolumeChangeCallback callback){
    auto newId = new_subscription_id();
    volumeSubscriptionMap.insert({ newId, callback });
    return newId;
}

void VolumeNotificationProvider::unsubscribe(SubscriptionId id){
    volumeSubscriptionMap.erase(id);
}


STDMETHODIMP_(ULONG) VolumeNotificationProvider::AddRef() { 
    return InterlockedIncrement(&m_RefCount); 
}

STDMETHODIMP_(ULONG) VolumeNotificationProvider::Release()
{
    LONG ref = InterlockedDecrement(&m_RefCount);
    if (ref == 0)
        delete this;
    return ref;
}

STDMETHODIMP VolumeNotificationProvider::QueryInterface(REFIID IID, void** ReturnValue)
{
    if (IID == IID_IUnknown || IID == __uuidof(IAudioEndpointVolumeCallback))
    {
        *ReturnValue = reinterpret_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }
    *ReturnValue = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP VolumeNotificationProvider::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA notification)
{
    VolumeInfo info = { (bool)notification ->bMuted, notification->fMasterVolume };
    for (auto it = volumeSubscriptionMap.begin(); it != volumeSubscriptionMap.end(); ++it)
        it->second(info);

    return S_OK;
}