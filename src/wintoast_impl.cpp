/* * Copyright (C) 2016-2019 Mohammed Boujemaoui <mohabouje@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "wintoast_impl.h"

#include <wrl/event.h>
#include <strsafe.h>
#include <Psapi.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>

#include <iostream>
#include <memory>
#include <cassert>
#include <array>

#include "dll_importer.h"
#include "wintoast_string_wrapper.h"
#include "internal_date_time.h"

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")

#ifdef NDEBUG
#define DEBUG_MSG(str)
#else
#define DEBUG_MSG(str) std::wcout << str << std::endl
#endif

#define DEFAULT_SHELL_LINKS_PATH    L"\\Microsoft\\Windows\\Start Menu\\Programs\\"
#define DEFAULT_LINK_FORMAT            L".lnk"
#define STATUS_SUCCESS (0x00000000)

using namespace WinToastLib;


inline IWinToastHandler::WinToastDismissalReason getWinToastDismissalReason(const ToastDismissalReason reason) {
    switch (reason) {
        case ToastDismissalReason::ToastDismissalReason_UserCanceled:
            return IWinToastHandler::WinToastDismissalReason::UserCanceled;
        case ToastDismissalReason::ToastDismissalReason_ApplicationHidden:
            return IWinToastHandler::WinToastDismissalReason::ApplicationHidden;
        case ToastDismissalReason::ToastDismissalReason_TimedOut:
        default:
            return IWinToastHandler::WinToastDismissalReason::TimedOut;
    }
}

inline ToastTemplateType getToastTemplateType(const WinToastTemplate::WinToastTemplateType type) {
    switch (type) {
        case WinToastTemplate::WinToastTemplateType::ImageAndText01:
            return ToastTemplateType::ToastTemplateType_ToastImageAndText01;
        case WinToastTemplate::WinToastTemplateType::ImageAndText02:
            return ToastTemplateType::ToastTemplateType_ToastImageAndText02;
        case WinToastTemplate::WinToastTemplateType::ImageAndText03:
            return ToastTemplateType::ToastTemplateType_ToastImageAndText03;
        case WinToastTemplate::WinToastTemplateType::ImageAndText04:
            return ToastTemplateType::ToastTemplateType_ToastImageAndText04;
        case WinToastTemplate::WinToastTemplateType::Text01:
            return ToastTemplateType::ToastTemplateType_ToastText01;
        case WinToastTemplate::WinToastTemplateType::Text02:
            return ToastTemplateType::ToastTemplateType_ToastText02;
        case WinToastTemplate::WinToastTemplateType::Text03:
            return ToastTemplateType::ToastTemplateType_ToastText03;
        case WinToastTemplate::WinToastTemplateType::Text04:
        default:
            return ToastTemplateType::ToastTemplateType_ToastText04;
    }
}

// Quickstart: Handling toast activations from Win32 apps in Windows 10
// https://blogs.msdn.microsoft.com/tiles_and_toasts/2015/10/16/quickstart-handling-toast-activations-from-win32-apps-in-windows-10/
namespace Util {

    typedef LONG NTSTATUS, *PNTSTATUS;

    typedef NTSTATUS(WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    inline RTL_OSVERSIONINFOW getRealOSVersion() {
        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            RtlGetVersionPtr fxPtr = (RtlGetVersionPtr) ::GetProcAddress(hMod, "RtlGetVersion");
            if (fxPtr != nullptr) {
                RTL_OSVERSIONINFOW rovi = {0};
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (STATUS_SUCCESS == fxPtr(&rovi)) {
                    return rovi;
                }
            }
        }
        RTL_OSVERSIONINFOW rovi = {0};
        return rovi;
    }

    inline HRESULT defaultExecutablePath(_In_ WCHAR *path, _In_ DWORD nSize = MAX_PATH) {
        DWORD written = GetModuleFileNameExW(GetCurrentProcess(), nullptr, path, nSize);
        DEBUG_MSG("Default executable path: " << path);
        return (written > 0) ? S_OK : E_FAIL;
    }


    inline HRESULT defaultShellLinksDirectory(_In_ WCHAR *path, _In_ DWORD nSize = MAX_PATH) {
        DWORD written = GetEnvironmentVariableW(L"APPDATA", path, nSize);
        HRESULT hr = written > 0 ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr)) {
            errno_t result = wcscat_s(path, nSize, DEFAULT_SHELL_LINKS_PATH);
            hr = (result == 0) ? S_OK : E_INVALIDARG;
            DEBUG_MSG("Default shell link path: " << path);
        }
        return hr;
    }

    inline HRESULT defaultShellLinkPath(const std::wstring &appname, _In_ WCHAR *path, _In_ DWORD nSize = MAX_PATH) {
        HRESULT hr = defaultShellLinksDirectory(path, nSize);
        if (SUCCEEDED(hr)) {
            const std::wstring appLink(appname + DEFAULT_LINK_FORMAT);
            errno_t result = wcscat_s(path, nSize, appLink.c_str());
            hr = (result == 0) ? S_OK : E_INVALIDARG;
            DEBUG_MSG("Default shell link file path: " << path);
        }
        return hr;
    }


    inline PCWSTR AsString(ComPtr<IXmlDocument> &xmlDocument) {
        HSTRING xml;
        ComPtr<IXmlNodeSerializer> ser;
        HRESULT hr = xmlDocument.As<IXmlNodeSerializer>(&ser);
        hr = ser->GetXml(&xml);
        if (SUCCEEDED(hr))
            return DllImporter::WindowsGetStringRawBuffer(xml, nullptr);
        return nullptr;
    }

    inline PCWSTR AsString(HSTRING hstring) {
        return DllImporter::WindowsGetStringRawBuffer(hstring, nullptr);
    }

    inline HRESULT setNodeStringValue(const std::wstring &string, IXmlNode *node, IXmlDocument *xml) {
        ComPtr<IXmlText> textNode;
        HRESULT hr = xml->CreateTextNode(WinToastStringWrapper(string).Get(), &textNode);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> stringNode;
            hr = textNode.As(&stringNode);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> appendedChild;
                hr = node->AppendChild(stringNode.Get(), &appendedChild);
            }
        }
        return hr;
    }

    inline HRESULT
    setEventHandlers(_In_ IToastNotification *notification, _In_ const std::shared_ptr<IWinToastHandler> &eventHandler,
                     _In_ INT64 expirationTime) {
        EventRegistrationToken activatedToken, dismissedToken, failedToken;
        HRESULT hr = notification->add_Activated(
                Callback<Implements<RuntimeClassFlags<ClassicCom>,
                        ITypedEventHandler<ToastNotification *, IInspectable * >>>(
                        [eventHandler](IToastNotification *, IInspectable *inspectable) {
                            IToastActivatedEventArgs *activatedEventArgs;
                            HRESULT hr = inspectable->QueryInterface(&activatedEventArgs);
                            if (SUCCEEDED(hr)) {
                                HSTRING argumentsHandle;
                                hr = activatedEventArgs->get_Arguments(&argumentsHandle);
                                if (SUCCEEDED(hr)) {
                                    PCWSTR arguments = Util::AsString(argumentsHandle);
                                    if (arguments && *arguments) {
                                        eventHandler->toastActivated(static_cast<int>(wcstol(arguments, nullptr, 10)));
                                        return S_OK;
                                    }
                                }
                            }
                            eventHandler->toastActivated();
                            return S_OK;
                        }).Get(), &activatedToken);

        if (SUCCEEDED(hr)) {
            hr = notification->add_Dismissed(Callback<Implements<RuntimeClassFlags<ClassicCom>,
                    ITypedEventHandler<ToastNotification *, ToastDismissedEventArgs * >>>(
                    [eventHandler, expirationTime](IToastNotification *, IToastDismissedEventArgs *e) {
                        ToastDismissalReason reason;
                        if (SUCCEEDED(e->get_Reason(&reason))) {
                            if (reason == ToastDismissalReason_UserCanceled && expirationTime &&
                                InternalDateTime::Now() >= expirationTime)
                                reason = ToastDismissalReason_TimedOut;
                            eventHandler->toastDismissed(getWinToastDismissalReason(reason));
                        }
                        return S_OK;
                    }).Get(), &dismissedToken);
            if (SUCCEEDED(hr)) {
                hr = notification->add_Failed(Callback<Implements<RuntimeClassFlags<ClassicCom>,
                        ITypedEventHandler<ToastNotification *, ToastFailedEventArgs * >>>(
                        [eventHandler](IToastNotification *, IToastFailedEventArgs *) {
                            eventHandler->toastFailed();
                            return S_OK;
                        }).Get(), &failedToken);
            }
        }
        return hr;
    }

    inline HRESULT addAttribute(_In_ IXmlDocument *xml, const std::wstring &name, IXmlNamedNodeMap *attributeMap) {
        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> attribute;
        HRESULT hr = xml->CreateAttribute(WinToastStringWrapper(name).Get(), &attribute);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> node;
            hr = attribute.As(&node);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> pNode;
                hr = attributeMap->SetNamedItem(node.Get(), &pNode);
            }
        }
        return hr;
    }

    inline HRESULT
    createElement(_In_ IXmlDocument *xml, _In_ const std::wstring &root_node, _In_ const std::wstring &element_name,
                  _In_ const std::vector<std::wstring> &attribute_names) {
        ComPtr<IXmlNodeList> rootList;
        HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(root_node).Get(), &rootList);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> root;
            hr = rootList->Item(0, &root);
            if (SUCCEEDED(hr)) {
                ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> element;
                hr = xml->CreateElement(WinToastStringWrapper(element_name).Get(), &element);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> nodeTmp;
                    hr = element.As(&nodeTmp);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IXmlNode> node;
                        hr = root->AppendChild(nodeTmp.Get(), &node);
                        if (SUCCEEDED(hr)) {
                            ComPtr<IXmlNamedNodeMap> attributes;
                            hr = node->get_Attributes(&attributes);
                            if (SUCCEEDED(hr)) {
                                for (const auto &it: attribute_names) {
                                    hr = addAttribute(xml, it, attributes.Get());
                                }
                            }
                        }
                    }
                }
            }
        }
        return hr;
    }
}

WinToastImpl &WinToastImpl::instance() {
    static WinToastImpl instance;
    return instance;
}

WinToastImpl::WinToastImpl() :
        _isInitialized(false),
        _hasCoInitialized(false) {
    if (!isCompatible()) {
        DEBUG_MSG(L"Warning: Your system is not compatible with this library ");
    }
}

WinToastImpl::~WinToastImpl() {
    if (_hasCoInitialized) {
        CoUninitialize();
    }
}

void WinToastImpl::setAppName(_In_ const std::wstring &appName) {
    _appName = appName;
}


void WinToastImpl::setAppUserModelId(_In_ const std::wstring &aumi) {
    _aumi = aumi;
    DEBUG_MSG(L"Default App User Model Id: " << _aumi.c_str());
}

void WinToastImpl::setShortcutPolicy(_In_ WinToast::ShortcutPolicy shortcutPolicy) {
    _shortcutPolicy = shortcutPolicy;
}

bool WinToastImpl::isCompatible() {
    DllImporter::initialize();
    return !((DllImporter::SetCurrentProcessExplicitAppUserModelID == nullptr)
             || (DllImporter::PropVariantToString == nullptr)
             || (DllImporter::RoGetActivationFactory == nullptr)
             || (DllImporter::WindowsCreateStringReference == nullptr)
             || (DllImporter::WindowsDeleteString == nullptr));
}

bool WinToastImpl::isSupportingModernFeatures() {
    constexpr auto MinimumSupportedVersion = 6;
    return Util::getRealOSVersion().dwMajorVersion > MinimumSupportedVersion;

}

std::wstring WinToastImpl::configureAUMI(_In_ const std::wstring &companyName,
                                         _In_ const std::wstring &productName,
                                         _In_ const std::wstring &subProduct,
                                         _In_ const std::wstring &versionInformation) {
    std::wstring aumi = companyName;
    aumi += L"." + productName;
    if (subProduct.length() > 0) {
        aumi += L"." + subProduct;
        if (versionInformation.length() > 0) {
            aumi += L"." + versionInformation;
        }
    }

    if (aumi.length() > SCHAR_MAX) {
        DEBUG_MSG("Error: max size allowed for AUMI: 128 characters.");
    }
    return aumi;
}

enum WinToast::ShortcutResult WinToastImpl::createShortcut() {
    if (_aumi.empty() || _appName.empty()) {
        DEBUG_MSG(L"Error: App User Model Id or Appname is empty!");
        return WinToast::ShortcutResult::SHORTCUT_MISSING_PARAMETERS;
    }

    if (!isCompatible()) {
        DEBUG_MSG(L"Your OS is not compatible with this library! =(");
        return WinToast::ShortcutResult::SHORTCUT_INCOMPATIBLE_OS;
    }

    if (!_hasCoInitialized) {
        HRESULT initHr = CoInitializeEx(nullptr, COINIT::COINIT_MULTITHREADED);
        if (initHr != RPC_E_CHANGED_MODE) {
            if (FAILED(initHr) && initHr != S_FALSE) {
                DEBUG_MSG(L"Error on COM library initialization!");
                return WinToast::ShortcutResult::SHORTCUT_COM_INIT_FAILURE;
            } else {
                _hasCoInitialized = true;
            }
        }
    }

    bool wasChanged;
    HRESULT hr = validateShellLinkHelper(wasChanged);
    if (SUCCEEDED(hr))
        return wasChanged ? WinToast::ShortcutResult::SHORTCUT_WAS_CHANGED
                          : WinToast::ShortcutResult::SHORTCUT_UNCHANGED;

    hr = createShellLinkHelper();
    return SUCCEEDED(hr) ? WinToast::ShortcutResult::SHORTCUT_WAS_CREATED
                         : WinToast::ShortcutResult::SHORTCUT_CREATE_FAILED;
}

bool WinToastImpl::initialize(_Out_opt_ WinToast::WinToastError *error) {
    _isInitialized = false;
    setError(error, WinToast::WinToastError::NoError);

    if (!isCompatible()) {
        setError(error, WinToast::WinToastError::SystemNotSupported);
        DEBUG_MSG(L"Error: system not supported.");
        return false;
    }


    if (_aumi.empty() || _appName.empty()) {
        setError(error, WinToast::WinToastError::InvalidParameters);
        DEBUG_MSG(L"Error while initializing, did you set up a valid AUMI and App name?");
        return false;
    }

    if (_shortcutPolicy != WinToast::ShortcutPolicy::SHORTCUT_POLICY_IGNORE) {
        if ((int) createShortcut() < 0) {
            setError(error, WinToast::WinToastError::ShellLinkNotCreated);
            DEBUG_MSG(L"Error while attaching the AUMI to the current proccess =(");
            return false;
        }
    }

    if (FAILED(DllImporter::SetCurrentProcessExplicitAppUserModelID(_aumi.c_str()))) {
        setError(error, WinToast::WinToastError::InvalidAppUserModelID);
        DEBUG_MSG(L"Error while attaching the AUMI to the current proccess =(");
        return false;
    }

    _isInitialized = true;
    return _isInitialized;
}

bool WinToastImpl::isInitialized() const {
    return _isInitialized;
}

const std::wstring &WinToastImpl::appName() const {
    return _appName;
}

const std::wstring &WinToastImpl::appUserModelId() const {
    return _aumi;
}


HRESULT WinToastImpl::validateShellLinkHelper(_Out_ bool &wasChanged) {
    WCHAR path[MAX_PATH] = {L'\0'};
    Util::defaultShellLinkPath(_appName, path);
    // Check if the file exist
    DWORD attr = GetFileAttributesW(path);
    if (attr >= 0xFFFFFFF) {
        DEBUG_MSG("Error, shell link not found. Try to create a new one in: " << path);
        return E_FAIL;
    }

    // Let's load the file as shell link to validate.
    // - Create a shell link
    // - Create a persistant file
    // - Load the path as data for the persistant file
    // - Read the property AUMI and validate with the current
    // - Review if AUMI is equal.
    ComPtr<IShellLink> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (SUCCEEDED(hr)) {
        ComPtr<IPersistFile> persistFile;
        hr = shellLink.As(&persistFile);
        if (SUCCEEDED(hr)) {
            hr = persistFile->Load(path, STGM_READWRITE);
            if (SUCCEEDED(hr)) {
                ComPtr<IPropertyStore> propertyStore;
                hr = shellLink.As(&propertyStore);
                if (SUCCEEDED(hr)) {
                    PROPVARIANT appIdPropVar;
                    hr = propertyStore->GetValue(PKEY_AppUserModel_ID, &appIdPropVar);
                    if (SUCCEEDED(hr)) {
                        WCHAR AUMI[MAX_PATH];
                        hr = DllImporter::PropVariantToString(appIdPropVar, AUMI, MAX_PATH);
                        wasChanged = false;
                        if (FAILED(hr) || _aumi != AUMI) {
                            if (_shortcutPolicy == WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE) {
                                // AUMI Changed for the same app, let's update the current value! =)
                                wasChanged = true;
                                PropVariantClear(&appIdPropVar);
                                hr = InitPropVariantFromString(_aumi.c_str(), &appIdPropVar);
                                if (SUCCEEDED(hr)) {
                                    hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
                                    if (SUCCEEDED(hr)) {
                                        hr = propertyStore->Commit();
                                        if (SUCCEEDED(hr) && SUCCEEDED(persistFile->IsDirty())) {
                                            hr = persistFile->Save(path, TRUE);
                                        }
                                    }
                                }
                            } else {
                                // Not allowed to touch the shortcut to fix the AUMI
                                hr = E_FAIL;
                            }
                        }
                        PropVariantClear(&appIdPropVar);
                    }
                }
            }
        }
    }
    return hr;
}


HRESULT WinToastImpl::createShellLinkHelper() {
    if (_shortcutPolicy != WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE) {
        return E_FAIL;
    }

    WCHAR exePath[MAX_PATH]{L'\0'};
    WCHAR slPath[MAX_PATH]{L'\0'};
    Util::defaultShellLinkPath(_appName, slPath);
    Util::defaultExecutablePath(exePath);
    ComPtr<IShellLinkW> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (SUCCEEDED(hr)) {
        hr = shellLink->SetPath(exePath);
        if (SUCCEEDED(hr)) {
            hr = shellLink->SetArguments(L"");
            if (SUCCEEDED(hr)) {
                hr = shellLink->SetWorkingDirectory(exePath);
                if (SUCCEEDED(hr)) {
                    ComPtr<IPropertyStore> propertyStore;
                    hr = shellLink.As(&propertyStore);
                    if (SUCCEEDED(hr)) {
                        PROPVARIANT appIdPropVar;
                        hr = InitPropVariantFromString(_aumi.c_str(), &appIdPropVar);
                        if (SUCCEEDED(hr)) {
                            hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
                            if (SUCCEEDED(hr)) {
                                hr = propertyStore->Commit();
                                if (SUCCEEDED(hr)) {
                                    ComPtr<IPersistFile> persistFile;
                                    hr = shellLink.As(&persistFile);
                                    if (SUCCEEDED(hr)) {
                                        hr = persistFile->Save(slPath, TRUE);
                                    }
                                }
                            }
                            PropVariantClear(&appIdPropVar);
                        }
                    }
                }
            }
        }
    }
    return hr;
}

INT64 WinToastImpl::showToast(_In_ const WinToastTemplate &toast, _In_  IWinToastHandler *handler, _Out_
                              WinToast::WinToastError *error) {
    setError(error, WinToast::WinToastError::NoError);
    INT64 id = -1;
    if (!isInitialized()) {
        setError(error, WinToast::WinToastError::NotInitialized);
        DEBUG_MSG("Error when launching the toast. WinToast is not initialized.");
        return id;
    }
    if (!handler) {
        setError(error, WinToast::WinToastError::InvalidHandler);
        DEBUG_MSG("Error when launching the toast. Handler cannot be nullptr.");
        return id;
    }

    ComPtr<IToastNotificationManagerStatics> notificationManager;
    HRESULT hr = DllImporter::Wrap_GetActivationFactory(
            WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            &notificationManager);
    if (SUCCEEDED(hr)) {
        ComPtr<IToastNotifier> notifier;
        hr = notificationManager->CreateToastNotifierWithId(WinToastStringWrapper(_aumi).Get(), &notifier);
        if (SUCCEEDED(hr)) {
            ComPtr<IToastNotificationFactory> notificationFactory;
            hr = DllImporter::Wrap_GetActivationFactory(
                    WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
                    &notificationFactory);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlDocument> xmlDocument;
                hr = notificationManager->GetTemplateContent(getToastTemplateType(toast.type()), &xmlDocument);
                if (SUCCEEDED(hr)) {
                    for (UINT32 i = 0, fieldsCount = static_cast<UINT32>(toast.textFieldsCount());
                         i < fieldsCount && SUCCEEDED(hr); i++) {
                        hr = setTextFieldHelper(xmlDocument.Get(), toast.textField(WinToastTemplate::TextField(i)), i);
                    }

                    // Modern feature are supported Windows > Windows 10
                    if (SUCCEEDED(hr) && isSupportingModernFeatures()) {

                        // Note that we do this *after* using toast.textFieldsCount() to
                        // iterate/fill the template's text fields, since we're adding yet another text field.
                        if (SUCCEEDED(hr)
                            && !toast.attributionText().empty()) {
                            hr = setAttributionTextFieldHelper(xmlDocument.Get(), toast.attributionText());
                        }

                        std::array<WCHAR, 12> buf{};
                        for (std::size_t i = 0, actionsCount = toast.actionsCount();
                             i < actionsCount && SUCCEEDED(hr); i++) {
                            _snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%zd", i);
                            hr = addActionHelper(xmlDocument.Get(), toast.actionLabel(i), buf.data());
                        }

                        if (SUCCEEDED(hr)) {
                            hr = (toast.audioPath().empty() &&
                                  toast.audioOption() == WinToastTemplate::AudioOption::Default)
                                 ? hr : setAudioFieldHelper(xmlDocument.Get(), toast.audioPath(), toast.audioOption());
                        }

                        if (SUCCEEDED(hr) && toast.duration() != WinToastTemplate::Duration::System) {
                            hr = addDurationHelper(xmlDocument.Get(),
                                                   (toast.duration() == WinToastTemplate::Duration::Short) ? L"short"
                                                                                                           : L"long");
                        }

                        if (SUCCEEDED(hr)) {
                            hr = addScenarioHelper(xmlDocument.Get(), toast.scenario());
                        }

                    } else {
                        DEBUG_MSG("Modern features (Actions/Sounds/Attributes) not supported in this os version");
                    }

                    if (SUCCEEDED(hr)) {
                        hr = toast.hasImage() ? setImageFieldHelper(xmlDocument.Get(), toast.imagePath()) : hr;
                        if (SUCCEEDED(hr)) {
                            ComPtr<IToastNotification> notification;
                            hr = notificationFactory->CreateToastNotification(xmlDocument.Get(), &notification);
                            if (SUCCEEDED(hr)) {
                                INT64 expiration = 0, relativeExpiration = toast.expiration();
                                if (relativeExpiration > 0) {
                                    InternalDateTime expirationDateTime(relativeExpiration);
                                    expiration = expirationDateTime;
                                    hr = notification->put_ExpirationTime(&expirationDateTime);
                                }

                                if (SUCCEEDED(hr)) {
                                    hr = Util::setEventHandlers(notification.Get(),
                                                                std::shared_ptr<IWinToastHandler>(handler), expiration);
                                    if (FAILED(hr)) {
                                        setError(error, WinToast::WinToastError::InvalidHandler);
                                    }
                                }

                                if (SUCCEEDED(hr)) {
                                    GUID guid;
                                    hr = CoCreateGuid(&guid);
                                    if (SUCCEEDED(hr)) {
                                        id = guid.Data1;
                                        _buffer[id] = notification;
                                        DEBUG_MSG("xml: " << Util::AsString(xmlDocument));
                                        hr = notifier->Show(notification.Get());
                                        if (FAILED(hr)) {
                                            setError(error, WinToast::WinToastError::NotDisplayed);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return FAILED(hr) ? -1 : id;
}

ComPtr<IToastNotifier> WinToastImpl::notifier(_In_ bool *succeded) const {
    ComPtr<IToastNotificationManagerStatics> notificationManager;
    ComPtr<IToastNotifier> notifier;
    HRESULT hr = DllImporter::Wrap_GetActivationFactory(
            WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            &notificationManager);
    if (SUCCEEDED(hr)) {
        hr = notificationManager->CreateToastNotifierWithId(WinToastStringWrapper(_aumi).Get(), &notifier);
    }
    *succeded = SUCCEEDED(hr);
    return notifier;
}

bool WinToastImpl::hideToast(_In_ INT64 id) {
    if (!isInitialized()) {
        DEBUG_MSG("Error when hiding the toast. WinToast is not initialized.");
        return false;
    }

    if (_buffer.find(id) != _buffer.end()) {
        auto succeded = false;
        auto notify = notifier(&succeded);
        if (succeded) {
            auto result = notify->Hide(_buffer[id].Get());
            _buffer.erase(id);
            return SUCCEEDED(result);
        }
    }
    return false;
}

void WinToastImpl::clear() {
    auto succeded = false;
    auto notify = notifier(&succeded);
    if (succeded) {
        auto end = _buffer.end();
        for (auto it = _buffer.begin(); it != end; ++it) {
            notify->Hide(it->second.Get());
        }
        _buffer.clear();
    }
}

//
// Available as of Windows 10 Anniversary Update
// Ref: https://docs.microsoft.com/en-us/windows/uwp/design/shell/tiles-and-notifications/adaptive-interactive-toasts
//
// NOTE: This will add a new text field, so be aware when iterating over
//       the toast's text fields or getting a count of them.
//
HRESULT WinToastImpl::setAttributionTextFieldHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &text) {
    Util::createElement(xml, L"binding", L"text", {L"placement"});
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"text").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        UINT32 nodeListLength;
        hr = nodeList->get_Length(&nodeListLength);
        if (SUCCEEDED(hr)) {
            for (UINT32 i = 0; i < nodeListLength; i++) {
                ComPtr<IXmlNode> textNode;
                hr = nodeList->Item(i, &textNode);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNamedNodeMap> attributes;
                    hr = textNode->get_Attributes(&attributes);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IXmlNode> editedNode;
                        if (SUCCEEDED(hr)) {
                            hr = attributes->GetNamedItem(WinToastStringWrapper(L"placement").Get(), &editedNode);
                            if (FAILED(hr) || !editedNode) {
                                continue;
                            }
                            hr = Util::setNodeStringValue(L"attribution", editedNode.Get(), xml);
                            if (SUCCEEDED(hr)) {
                                return setTextFieldHelper(xml, text, i);
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT WinToastImpl::addDurationHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &duration) {
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"toast").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        UINT32 length;
        hr = nodeList->get_Length(&length);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> toastNode;
            hr = nodeList->Item(0, &toastNode);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlElement> toastElement;
                hr = toastNode.As(&toastElement);
                if (SUCCEEDED(hr)) {
                    hr = toastElement->SetAttribute(WinToastStringWrapper(L"duration").Get(),
                                                    WinToastStringWrapper(duration).Get());
                }
            }
        }
    }
    return hr;
}

HRESULT WinToastImpl::addScenarioHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &scenario) {
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"toast").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        UINT32 length;
        hr = nodeList->get_Length(&length);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> toastNode;
            hr = nodeList->Item(0, &toastNode);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlElement> toastElement;
                hr = toastNode.As(&toastElement);
                if (SUCCEEDED(hr)) {
                    hr = toastElement->SetAttribute(WinToastStringWrapper(L"scenario").Get(),
                                                    WinToastStringWrapper(scenario).Get());
                }
            }
        }
    }
    return hr;
}

HRESULT WinToastImpl::setTextFieldHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &text, _In_ UINT32 pos) {
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"text").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> node;
        hr = nodeList->Item(pos, &node);
        if (SUCCEEDED(hr)) {
            hr = Util::setNodeStringValue(text, node.Get(), xml);
        }
    }
    return hr;
}


HRESULT WinToastImpl::setImageFieldHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &path) {
    assert(path.size() < MAX_PATH);

    wchar_t imagePath[MAX_PATH] = L"file:///";
    HRESULT hr = StringCchCatW(imagePath, MAX_PATH, path.c_str());
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNodeList> nodeList;
        hr = xml->GetElementsByTagName(WinToastStringWrapper(L"image").Get(), &nodeList);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> node;
            hr = nodeList->Item(0, &node);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNamedNodeMap> attributes;
                hr = node->get_Attributes(&attributes);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> editedNode;
                    hr = attributes->GetNamedItem(WinToastStringWrapper(L"src").Get(), &editedNode);
                    if (SUCCEEDED(hr)) {
                        Util::setNodeStringValue(imagePath, editedNode.Get(), xml);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT WinToastImpl::setAudioFieldHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &path, _In_opt_
                                          WinToastTemplate::AudioOption option) {
    std::vector<std::wstring> attrs;
    if (!path.empty()) attrs.emplace_back(L"src");
    if (option == WinToastTemplate::AudioOption::Loop) attrs.emplace_back(L"loop");
    if (option == WinToastTemplate::AudioOption::Silent) attrs.emplace_back(L"silent");
    Util::createElement(xml, L"toast", L"audio", attrs);

    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"audio").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> node;
        hr = nodeList->Item(0, &node);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNamedNodeMap> attributes;
            hr = node->get_Attributes(&attributes);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> editedNode;
                if (!path.empty()) {
                    if (SUCCEEDED(hr)) {
                        hr = attributes->GetNamedItem(WinToastStringWrapper(L"src").Get(), &editedNode);
                        if (SUCCEEDED(hr)) {
                            hr = Util::setNodeStringValue(path, editedNode.Get(), xml);
                        }
                    }
                }

                if (SUCCEEDED(hr)) {
                    switch (option) {
                        case WinToastTemplate::AudioOption::Loop:
                            hr = attributes->GetNamedItem(WinToastStringWrapper(L"loop").Get(), &editedNode);
                            if (SUCCEEDED(hr)) {
                                hr = Util::setNodeStringValue(L"true", editedNode.Get(), xml);
                            }
                            break;
                        case WinToastTemplate::AudioOption::Silent:
                            hr = attributes->GetNamedItem(WinToastStringWrapper(L"silent").Get(), &editedNode);
                            if (SUCCEEDED(hr)) {
                                hr = Util::setNodeStringValue(L"true", editedNode.Get(), xml);
                            }
                        default:
                            break;
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT WinToastImpl::addActionHelper(_In_ IXmlDocument *xml, _In_ const std::wstring &content, _In_
                                      const std::wstring &arguments) {
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"actions").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        UINT32 length;
        hr = nodeList->get_Length(&length);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> actionsNode;
            if (length > 0) {
                hr = nodeList->Item(0, &actionsNode);
            } else {
                hr = xml->GetElementsByTagName(WinToastStringWrapper(L"toast").Get(), &nodeList);
                if (SUCCEEDED(hr)) {
                    hr = nodeList->get_Length(&length);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IXmlNode> toastNode;
                        hr = nodeList->Item(0, &toastNode);
                        if (SUCCEEDED(hr)) {
                            ComPtr<IXmlElement> toastElement;
                            hr = toastNode.As(&toastElement);
                            if (SUCCEEDED(hr))
                                hr = toastElement->SetAttribute(WinToastStringWrapper(L"template").Get(),
                                                                WinToastStringWrapper(L"ToastGeneric").Get());
                            if (SUCCEEDED(hr))
                                hr = toastElement->SetAttribute(WinToastStringWrapper(L"duration").Get(),
                                                                WinToastStringWrapper(L"long").Get());
                            if (SUCCEEDED(hr)) {
                                ComPtr<IXmlElement> actionsElement;
                                hr = xml->CreateElement(WinToastStringWrapper(L"actions").Get(), &actionsElement);
                                if (SUCCEEDED(hr)) {
                                    hr = actionsElement.As(&actionsNode);
                                    if (SUCCEEDED(hr)) {
                                        ComPtr<IXmlNode> appendedChild;
                                        hr = toastNode->AppendChild(actionsNode.Get(), &appendedChild);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlElement> actionElement;
                hr = xml->CreateElement(WinToastStringWrapper(L"action").Get(), &actionElement);
                if (SUCCEEDED(hr))
                    hr = actionElement->SetAttribute(WinToastStringWrapper(L"content").Get(),
                                                     WinToastStringWrapper(content).Get());
                if (SUCCEEDED(hr))
                    hr = actionElement->SetAttribute(WinToastStringWrapper(L"arguments").Get(),
                                                     WinToastStringWrapper(arguments).Get());
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> actionNode;
                    hr = actionElement.As(&actionNode);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IXmlNode> appendedChild;
                        hr = actionsNode->AppendChild(actionNode.Get(), &appendedChild);
                    }
                }
            }
        }
    }
    return hr;
}

void WinToastImpl::setError(_Out_opt_ WinToast::WinToastError *error, _In_ WinToast::WinToastError value) {
    if (error) {
        *error = value;
    }
}
