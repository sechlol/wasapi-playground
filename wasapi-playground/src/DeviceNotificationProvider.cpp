#include <cstdlib>

#include "DeviceNotificationProvider.h"
#include "common.h"

DeviceEvent state_to_device_event(DWORD state) {
    switch (state)
    {
    case DEVICE_STATE_ACTIVE:
        return DeviceEvent::Enabled;
    case DEVICE_STATE_DISABLED:
        return DeviceEvent::Disabled;
    case DEVICE_STATE_NOTPRESENT:
        return DeviceEvent::NotPresent;
    case DEVICE_STATE_UNPLUGGED:
        return DeviceEvent::Disconnected;
    default:
        return DeviceEvent::Unknown;
    }

}
DeviceNotificationProvider::DeviceNotificationProvider() : notificationClient(std::make_shared<NotificationClient>(this))
{
    enumerator.register_notifications(notificationClient.get());
}

DeviceNotificationProvider::~DeviceNotificationProvider()
{
    enumerator.unregister_notifications(notificationClient.get());
}

SubscriptionId DeviceNotificationProvider::subscribe_to_global_events(GlobalDeviceEventCallback callback)
{
    SubscriptionId newId = rand();
    globalSubscriptionMap.insert({ newId, callback });
    return newId;
}

void DeviceNotificationProvider::unsubscribe(SubscriptionId id)
{
    // this implementation needs to be optimized
    globalSubscriptionMap.erase(id);
    deviceSubscriptionMap.erase(id);
}

SubscriptionId DeviceNotificationProvider::subscribe_to_device_events(std::string deviceId, DeviceEventCallback callback)
{
    SubscriptionId newId = rand();

    // Inserts subscription ID into subscription map
    auto callbackList = deviceEventsMap.find(deviceId);
    if (callbackList == deviceEventsMap.end()) {
        deviceEventsMap.insert({deviceId,  std::vector<SubscriptionId>{ newId } });
    }
    else {
        deviceEventsMap[deviceId].push_back(newId);
    }

    deviceSubscriptionMap.insert({ newId, callback });

    return newId;
}

void DeviceNotificationProvider::notify_change(std::string deviceId, DeviceEvent newEvent)
{
    // Notify all subscribers of this specific device ID
    if (auto entry = deviceEventsMap.find(deviceId); entry != deviceEventsMap.end()) {
        for (auto const& subscriptionId : entry->second)
        {
            if(auto callback = deviceSubscriptionMap.find(subscriptionId); callback != deviceSubscriptionMap.end())
                callback->second(newEvent);
        }
    }

    // Notify all global subscribers
    for (auto it = globalSubscriptionMap.begin(); it != globalSubscriptionMap.end(); ++it)
    {
        it->second(deviceId, newEvent);
    }
}

DeviceNotificationProvider::NotificationClient::NotificationClient(DeviceNotificationProvider* parentRef) : 
    _cRef(1),
    parent(parentRef)
{}

HRESULT STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
{
    if (pwstrDeviceId != nullptr) {
        parent->notify_change(
            LPCWSTR_to_string(pwstrDeviceId),
            DeviceEvent::PropertyChanged);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    parent->notify_change(
        LPCWSTR_to_string(pwstrDeviceId),
        DeviceEvent::Connected);

    return S_OK;
};

HRESULT STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    parent->notify_change(
        LPCWSTR_to_string(pwstrDeviceId),
        DeviceEvent::Disconnected);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    parent->notify_change(
        LPCWSTR_to_string(pwstrDeviceId),
        state_to_device_event(dwNewState));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    parent->notify_change(
        LPCWSTR_to_string(pwstrDeviceId),
        DeviceEvent::PropertyChanged);

    /*printf("  -->Changed device property "
        "{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
        key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
        key.fmtid.Data4[0], key.fmtid.Data4[1],
        key.fmtid.Data4[2], key.fmtid.Data4[3],
        key.fmtid.Data4[4], key.fmtid.Data4[5],
        key.fmtid.Data4[6], key.fmtid.Data4[7],
        key.pid);*/

    return S_OK;
}


ULONG STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationProvider::NotificationClient::QueryInterface(
    REFIID riid, VOID** ppvInterface)
{
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    }
    else
    {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}