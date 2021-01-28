#include "AudioDeviceProvider.h"


CMMNotificationClient::CMMNotificationClient() :
    _cRef(1),
    _pEnumerator(NULL)
{
}

CMMNotificationClient::~CMMNotificationClient()
{
    SAFE_RELEASE(_pEnumerator)
}

// IUnknown methods -- AddRef, Release, and QueryInterface

ULONG STDMETHODCALLTYPE CMMNotificationClient::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE CMMNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::QueryInterface(
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

// Callback methods for device-event notifications.

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDefaultDeviceChanged(
    EDataFlow flow, ERole role,
    LPCWSTR pwstrDeviceId)
{
    string pszFlow = "?????";
    string pszRole = "?????";

    _PrintDeviceName(pwstrDeviceId);

    switch (flow)
    {
    case eRender:
        pszFlow = "eRender";
        break;
    case eCapture:
        pszFlow = "eCapture";
        break;
    }

    switch (role)
    {
    case eConsole:
        pszRole = "eConsole";
        break;
    case eMultimedia:
        pszRole = "eMultimedia";
        break;
    case eCommunications:
        pszRole = "eCommunications";
        break;
    }

    std::cout << "  -->New default device: flow = " << pszFlow << ", role = " << pszRole << std::endl;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    _PrintDeviceName(pwstrDeviceId);

    printf("  -->Added device\n");
    return S_OK;
};

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    _PrintDeviceName(pwstrDeviceId);

    printf("  -->Removed device\n");
    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceStateChanged(
    LPCWSTR pwstrDeviceId,
    DWORD dwNewState)
{
    string pszState = "?????";

    _PrintDeviceName(pwstrDeviceId);

    switch (dwNewState)
    {
    case DEVICE_STATE_ACTIVE:
        pszState = "ACTIVE";
        break;
    case DEVICE_STATE_DISABLED:
        pszState = "DISABLED";
        break;
    case DEVICE_STATE_NOTPRESENT:
        pszState = "NOTPRESENT";
        break;
    case DEVICE_STATE_UNPLUGGED:
        pszState = "UNPLUGGED";
        break;
    }

    cout << "  -->New device state is DEVICE_STATE_" << pszState << " (" << dwNewState << ")\n";

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnPropertyValueChanged(
    LPCWSTR pwstrDeviceId,
    const PROPERTYKEY key)
{
    _PrintDeviceName(pwstrDeviceId);

    printf("  -->Changed device property "
        "{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
        key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
        key.fmtid.Data4[0], key.fmtid.Data4[1],
        key.fmtid.Data4[2], key.fmtid.Data4[3],
        key.fmtid.Data4[4], key.fmtid.Data4[5],
        key.fmtid.Data4[6], key.fmtid.Data4[7],
        key.pid);
    return S_OK;
}

// Given an endpoint ID string, print the friendly device name.
HRESULT CMMNotificationClient::_PrintDeviceName(LPCWSTR pwstrId)
{
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;
    IPropertyStore* pProps = NULL;
    PROPVARIANT varString;

    CoInitialize(NULL);
    PropVariantInit(&varString);

    if (_pEnumerator == NULL)
    {
        // Get enumerator for audio endpoint devices.
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
            NULL, CLSCTX_INPROC_SERVER,
            __uuidof(IMMDeviceEnumerator),
            (void**)&_pEnumerator);
    }
    if (hr == S_OK)
    {
        hr = _pEnumerator->GetDevice(pwstrId, &pDevice);
    }
    if (hr == S_OK)
    {
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    }
    if (hr == S_OK)
    {
        // Get the endpoint device's friendly-name property.
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
    }
    printf("----------------------\nDevice name: \"%S\"\n"
        "  Endpoint ID string: \"%S\"\n",
        (hr == S_OK) ? varString.pwszVal : L"null device",
        (pwstrId != NULL) ? pwstrId : L"null ID");

    PropVariantClear(&varString);

    SAFE_RELEASE(pProps)
        SAFE_RELEASE(pDevice)
        CoUninitialize();
    return hr;
}