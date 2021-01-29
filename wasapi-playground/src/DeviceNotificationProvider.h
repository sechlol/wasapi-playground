#pragma once
//class AudioDeviceProvider {};

#include <functional>
#include <MMDeviceAPI.h>

#include "DeviceEnumerator.h"

enum class DeviceEvent {
    Enabled = 0,
    Disabled = 1,
    Connected = 2, 
    Disconnected = 3,
    NotPresent = 4, //???
    PropertyChanged = 5,
    Unknown = 6
};

typedef unsigned int SubscriptionId;
typedef std::function<void(DeviceEvent deviceEvent)> DeviceEventCallback;
typedef std::function<void(std::string deviceId, DeviceEvent deviceEvent)> GlobalDeviceEventCallback;

class DeviceNotificationProvider {
public:
    DeviceNotificationProvider();
    ~DeviceNotificationProvider();

    SubscriptionId subscribe_to_device_events(std::string deviceId, DeviceEventCallback callback);
    SubscriptionId subscribe_to_global_events(GlobalDeviceEventCallback callback);

    void unsubscribe(SubscriptionId id);

private:
    void notify_change(std::string deviceId, DeviceEvent deviceEvent);

    DeviceEnumerator enumerator;
    std::unordered_map<std::string, std::vector<SubscriptionId>> deviceEventsMap;
    std::unordered_map<SubscriptionId, DeviceEventCallback> deviceSubscriptionMap;
    std::unordered_map<SubscriptionId, GlobalDeviceEventCallback> globalSubscriptionMap;

    
    class NotificationClient : public IMMNotificationClient
    {
    private:
        LONG _cRef;
        DeviceNotificationProvider* parent;

    public:
        NotificationClient(DeviceNotificationProvider* parentRef);

        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);

        // Callback methods for device-event notifications.
        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
    };

    std::shared_ptr<NotificationClient> notificationClient;
};


