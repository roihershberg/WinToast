/* * Copyright (c) 2022 Roee Hershberg <roihershberg@protonmail.com>, 2016-2021 Mohammed Boujemaoui <mohabouje@gmail.com>
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

#include <ShObjIdl.h>
#include <strsafe.h>
#include <Psapi.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>

#include <iostream>
#include <memory>
#include <cassert>
#include <array>
#include <string_view>
#include <functional>

#include "dll_importer.h"

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")
#pragma comment(lib, "windowsapp")

#ifdef NDEBUG
#define DEBUG_MSG(str)
#define DEBUG_ERR(str)
#else
#define DEBUG_MSG(str) std::wcout << str << std::endl
#define DEBUG_ERR(str) std::wcerr << str << std::endl
#endif

#define DEFAULT_SHELL_LINKS_PATH    L"\\Microsoft\\Windows\\Start Menu\\Programs\\"
#define DEFAULT_LINK_FORMAT            L".lnk"
#define STATUS_SUCCESS (0x00000000)

#define catchAndLogHresult_2(execute, logPrefix) \
try {                                                     \
    execute                                               \
} catch (winrt::hresult_error const &ex) {                \
    DEBUG_ERR(logPrefix << ex.message().c_str());         \
}
#define catchAndLogHresult_3(execute, logPrefix, onError) \
try {                                                     \
    execute                                               \
} catch (winrt::hresult_error const &ex) {                \
    DEBUG_ERR(logPrefix << ex.message().c_str());         \
    onError                                               \
}

#define FUNC_CHOOSER(_f1, _f2, _f3, _f4, ...) _f4
#define FUNC_RECOMPOSER(argsWithParentheses) FUNC_CHOOSER argsWithParentheses
#define CHOOSE_FROM_ARG_COUNT(...) FUNC_RECOMPOSER((__VA_ARGS__, catchAndLogHresult_3, catchAndLogHresult_2, MISSING_PARAMETERS, MISSING_PARAMETERS))
#define MACRO_CHOOSER(...) CHOOSE_FROM_ARG_COUNT(__VA_ARGS__)
#define catchAndLogHresult(...) MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

using namespace WinToastLib;
using namespace winrt::Windows::UI::Notifications;
using namespace winrt::Windows::Data::Xml::Dom;

inline IWinToastHandler::WinToastDismissalReason getWinToastDismissalReason(const ToastDismissalReason reason) {
    switch (reason) {
        case ToastDismissalReason::UserCanceled:
            return IWinToastHandler::WinToastDismissalReason::UserCanceled;
        case ToastDismissalReason::ApplicationHidden:
            return IWinToastHandler::WinToastDismissalReason::ApplicationHidden;
        case ToastDismissalReason::TimedOut:
        default:
            return IWinToastHandler::WinToastDismissalReason::TimedOut;
    }
}

inline ToastTemplateType getToastTemplateType(const WinToastTemplate::WinToastTemplateType type) {
    switch (type) {
        case WinToastTemplate::WinToastTemplateType::ImageAndText01:
            return ToastTemplateType::ToastImageAndText01;
        case WinToastTemplate::WinToastTemplateType::ImageAndText02:
            return ToastTemplateType::ToastImageAndText02;
        case WinToastTemplate::WinToastTemplateType::ImageAndText03:
            return ToastTemplateType::ToastImageAndText03;
        case WinToastTemplate::WinToastTemplateType::ImageAndText04:
            return ToastTemplateType::ToastImageAndText04;
        case WinToastTemplate::WinToastTemplateType::Text01:
            return ToastTemplateType::ToastText01;
        case WinToastTemplate::WinToastTemplateType::Text02:
            return ToastTemplateType::ToastText02;
        case WinToastTemplate::WinToastTemplateType::Text03:
            return ToastTemplateType::ToastText03;
        case WinToastTemplate::WinToastTemplateType::Text04:
            return ToastTemplateType::ToastText04;
        default:
            winrt::throw_hresult(E_INVALIDARG);
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

    inline INT64 fileTimeNow() {
        FILETIME now;
        GetSystemTimeAsFileTime(&now);
        return ((((INT64) now.dwHighDateTime) << 32) | now.dwLowDateTime);
    }

    inline void defaultExecutablePath(_In_ WCHAR *path, _In_ DWORD nSize = MAX_PATH) {
        DWORD written = GetModuleFileNameExW(GetCurrentProcess(), nullptr, path, nSize);
        DEBUG_MSG("Default executable path: " << path);
        if (!written)
            throw winrt::hresult_error(E_FAIL, L"GetModuleFileNameExW failed for getting the executable path");
    }

    inline void defaultShellLinksDirectory(_In_ WCHAR *path, _In_ DWORD nSize = MAX_PATH) {
        DWORD written = GetEnvironmentVariableW(L"APPDATA", path, nSize);
        if (!written)
            throw winrt::hresult_error(E_FAIL, L"GetEnvironmentVariableW for APPDATA env var failed");

        errno_t result = wcscat_s(path, nSize, DEFAULT_SHELL_LINKS_PATH);
        if (result)
            throw winrt::hresult_error(E_FAIL,
                                       L"wcscat_s failed for appending the default shell links path to the APPDATA path");

        DEBUG_MSG("Default shell link path: " << path);
    }

    inline void defaultShellLinkPath(const std::wstring &appname, _In_ WCHAR *path, _In_ DWORD nSize = MAX_PATH) {
        const std::wstring appLink(appname + DEFAULT_LINK_FORMAT);

        defaultShellLinksDirectory(path, nSize);
        errno_t result = wcscat_s(path, nSize, appLink.c_str());
        if (result)
            throw winrt::hresult_error(E_FAIL,
                                       L"wcscat_s failed for appending the app link file name "
                                       L"to the default shell links path");

        DEBUG_MSG("Default shell link file path: " << path);
    }

    inline void
    setEventHandlers(_In_ ToastNotification notification, _In_ const std::shared_ptr<IWinToastHandler> &eventHandler,
                     _In_ INT64 expirationTime) {
        notification.Activated(
                [eventHandler](ToastNotification, const winrt::Windows::Foundation::IInspectable inspectable) {
                    auto activatedEventArgs = winrt::unbox_value<ToastActivatedEventArgs>(inspectable);
                    std::wstring arguments{to_hstring(activatedEventArgs.Arguments())};
                    eventHandler->toastActivated(WinToastArguments(arguments));
                });

        notification.Dismissed(
                [eventHandler, expirationTime](ToastNotification,
                                               const winrt::Windows::Foundation::IInspectable inspectable) {
                    auto dismissedEventArgs = winrt::unbox_value<ToastDismissedEventArgs>(inspectable);
                    ToastDismissalReason reason = dismissedEventArgs.Reason();
                    if (reason == ToastDismissalReason::UserCanceled && expirationTime &&
                        fileTimeNow() >= expirationTime)
                        reason = ToastDismissalReason::TimedOut;
                    eventHandler->toastDismissed(getWinToastDismissalReason(reason));
                });

        notification.Failed([eventHandler](auto &&...) {
            eventHandler->toastFailed();
        });
    }

    inline XmlElement
    createElement(_In_ XmlDocument xml, _In_ const std::wstring &root_node, _In_ const std::wstring &element_name) {
        IXmlNode root = xml.SelectSingleNode(L"//" + root_node + L"[1]");
        XmlElement element = xml.CreateElement(element_name);
        root.AppendChild(element);

        return element;
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
    DEBUG_MSG("App User Model Id: " << _aumi.c_str());
}

void WinToastImpl::setShortcutPolicy(_In_ WinToast::ShortcutPolicy shortcutPolicy) {
    _shortcutPolicy = shortcutPolicy;
}

bool WinToastImpl::isCompatible() {
    return DllImporter::initialize() && DllImporter::SetCurrentProcessExplicitAppUserModelID
           && DllImporter::PropVariantToString;
}

bool WinToastImpl::isSupportingModernFeatures() {
    constexpr auto MinimumSupportedVersion = 6;
    return Util::getRealOSVersion().dwMajorVersion > MinimumSupportedVersion;
}

std::wstring WinToastImpl::configureAUMI(_In_ const std::wstring &companyName,
                                         _In_ const std::wstring &productName,
                                         _In_ const std::wstring &subProduct,
                                         _In_ const std::wstring &versionInformation) {
    std::wstring aumi{companyName};
    aumi += L"." + productName;
    if (!subProduct.empty()) {
        aumi += L"." + subProduct;
        if (!versionInformation.empty()) {
            aumi += L"." + versionInformation;
        }
    }

    if (aumi.length() > SCHAR_MAX) {
        DEBUG_ERR("Error: max size allowed for AUMI: 128 characters.");
    }
    return aumi;
}

WinToast::ShortcutResult WinToastImpl::createShortcut() {
    if (_aumi.empty() || _appName.empty()) {
        DEBUG_ERR(L"Error: App User Model Id or Appname is empty!");
        return WinToast::ShortcutResult::SHORTCUT_MISSING_PARAMETERS;
    }

    if (!isCompatible()) {
        DEBUG_ERR(L"Your OS is not compatible with this library! =(");
        return WinToast::ShortcutResult::SHORTCUT_INCOMPATIBLE_OS;
    }

    bool wasChanged;
    catchAndLogHresult(
            {
                validateShellLinkHelper(wasChanged);
                return wasChanged ? WinToast::ShortcutResult::SHORTCUT_WAS_CHANGED
                                  : WinToast::ShortcutResult::SHORTCUT_UNCHANGED;
            },
            "Error in validateShellLinkHelper: "
    )

    catchAndLogHresult(
            {
                createShellLinkHelper();
                return WinToast::ShortcutResult::SHORTCUT_WAS_CREATED;
            },
            "Error in createShellLinkHelper: ",
            { return WinToast::ShortcutResult::SHORTCUT_CREATE_FAILED; }
    )
}

bool WinToastImpl::initialize(_Out_opt_ WinToast::WinToastError *error) {
    _isInitialized = false;
    setError(error, WinToast::WinToastError::NoError);

    if (!isCompatible()) {
        setError(error, WinToast::WinToastError::SystemNotSupported);
        DEBUG_ERR(L"Error: system not supported.");
        return false;
    }

    if (_aumi.empty() || _appName.empty()) {
        setError(error, WinToast::WinToastError::InvalidParameters);
        DEBUG_ERR(L"Error while initializing, did you set up a valid AUMI and App name?");
        return false;
    }

    if (!_hasCoInitialized) {
        catchAndLogHresult(
                {
                    winrt::init_apartment();
                    _hasCoInitialized = true;
                },
                "Error while trying to initialize the apartment: ",
                {
                    setError(error, WinToast::WinToastError::ApartmentInitError);
                    return false;
                }
        )
    }

    if (_shortcutPolicy != WinToast::ShortcutPolicy::SHORTCUT_POLICY_IGNORE) {
        if ((int) createShortcut() < 0) {
            setError(error, WinToast::WinToastError::ShellLinkNotCreated);
            DEBUG_ERR(L"Error while attaching the AUMI to the current proccess =(");
            return false;
        }
    }

    if (FAILED(DllImporter::SetCurrentProcessExplicitAppUserModelID(_aumi.c_str()))) {
        setError(error, WinToast::WinToastError::InvalidAppUserModelID);
        DEBUG_ERR(L"Error while attaching the AUMI to the current proccess =(");
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


void WinToastImpl::validateShellLinkHelper(_Out_ bool &wasChanged) {
    WCHAR path[MAX_PATH] = {L'\0'};
    Util::defaultShellLinkPath(_appName, path);
    // Check if the file exist
    DWORD attr = GetFileAttributesW(path);
    if (attr >= 0xFFFFFFF) {
        throw winrt::hresult_error(E_FAIL,
                                   L"Error, shell link not found. Try to create a new one in: " + std::wstring(path));
    }

    // Let's load the file as shell link to validate.
    // - Create a shell link
    // - Create a persistant file
    // - Load the path as data for the persistant file
    // - Read the property AUMI and validate with the current
    // - Review if AUMI is equal.

    winrt::com_ptr<IShellLink> shellLink;
    winrt::check_hresult(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink)));

    winrt::com_ptr<IPersistFile> persistFile = shellLink.as<IPersistFile>();
    winrt::check_hresult(persistFile->Load(path, STGM_READWRITE));

    winrt::com_ptr<IPropertyStore> propertyStore = shellLink.as<IPropertyStore>();
    PROPVARIANT appIdPropVar;
    winrt::check_hresult(propertyStore->GetValue(PKEY_AppUserModel_ID, &appIdPropVar));
    WCHAR AUMI[MAX_PATH];
    winrt::check_hresult(DllImporter::PropVariantToString(appIdPropVar, AUMI, MAX_PATH));
    PropVariantClear(&appIdPropVar);

    wasChanged = false;
    if (_aumi != AUMI) {
        if (_shortcutPolicy == WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE) {
            // AUMI Changed for the same app, let's update the current value! =)
            wasChanged = true;
            winrt::check_hresult(InitPropVariantFromString(_aumi.c_str(), &appIdPropVar));
            winrt::check_hresult(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
            winrt::check_hresult(propertyStore->Commit());
            winrt::check_hresult(persistFile->IsDirty());
            winrt::check_hresult(persistFile->Save(path, TRUE));
        } else {
            // Not allowed to touch the shortcut to fix the AUMI
            throw winrt::hresult_error(E_FAIL,
                                       L"AUMI in shortcut is different from the configured AUMI. "
                                       "The shortcut policy is not allowing to fix the shortcut.");
        }
    }
}


void WinToastImpl::createShellLinkHelper() {
    if (_shortcutPolicy != WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE) {
        throw winrt::hresult_error(E_FAIL, L"Configured shortcut policy is not allowing to create shortcuts.");
    }

    WCHAR exePath[MAX_PATH]{L'\0'};
    WCHAR slPath[MAX_PATH]{L'\0'};
    Util::defaultShellLinkPath(_appName, slPath);
    Util::defaultExecutablePath(exePath);

    winrt::com_ptr<IShellLinkW> shellLink;
    winrt::check_hresult(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink)));
    winrt::check_hresult(shellLink->SetPath(exePath));
    winrt::check_hresult(shellLink->SetArguments(L""));
    winrt::check_hresult(shellLink->SetWorkingDirectory(exePath));

    winrt::com_ptr<IPropertyStore> propertyStore = shellLink.as<IPropertyStore>();
    PROPVARIANT appIdPropVar;
    winrt::check_hresult(InitPropVariantFromString(_aumi.c_str(), &appIdPropVar));
    winrt::check_hresult(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
    PropVariantClear(&appIdPropVar);
    winrt::check_hresult(propertyStore->Commit());

    winrt::com_ptr<IPersistFile> persistFile = shellLink.as<IPersistFile>();
    winrt::check_hresult(persistFile->Save(slPath, TRUE));
}

INT64 WinToastImpl::showToast(_In_ const WinToastTemplate &toast, _In_  IWinToastHandler *handler, _Out_
                              WinToast::WinToastError *error) {
    setError(error, WinToast::WinToastError::NoError);
    INT64 id = 0;
    if (!isInitialized()) {
        setError(error, WinToast::WinToastError::NotInitialized);
        DEBUG_ERR("Error when launching the toast. WinToast is not initialized.");
        return -1;
    }
    if (!handler) {
        setError(error, WinToast::WinToastError::InvalidHandler);
        DEBUG_ERR("Error when launching the toast. Handler cannot be nullptr.");
        return -1;
    }

    ToastNotifier notifier{nullptr};
    catchAndLogHresult(
            { notifier = ToastNotificationManager::CreateToastNotifier(_aumi); },
            "Error in showToast while trying to create a notifier: ",
            {
                setError(error, WinToast::WinToastError::UnknownError);
                return -1;
            }
    )
    XmlDocument xmlDocument{nullptr};
    catchAndLogHresult(
            {
                xmlDocument = ToastNotificationManager::GetTemplateContent(
                        getToastTemplateType(toast.type())
                );
            },
            "Error in showToast while getting template content: ",
            {
                setError(error, WinToast::WinToastError::UnknownError);
                return -1;
            }
    )
    catchAndLogHresult(
            {
                XmlNodeList textFields = xmlDocument.GetElementsByTagName(L"text");

                for (UINT32 i = 0, fieldsCount = static_cast<UINT32>(toast.textFieldsCount()); i < fieldsCount; i++) {
                    textFields.Item(i).InnerText(toast.textField(WinToastTemplate::TextField(i)));
                }
            },
            "Error in showToast while setting text fields: ",
            {
                setError(error, WinToast::WinToastError::UnknownError);
                return -1;
            }
    )

    // Modern feature are supported Windows > Windows 10
    if (isSupportingModernFeatures()) {

        // Note that we do this *after* using toast.textFieldsCount() to
        // iterate/fill the template's text fields, since we're adding yet another text field.
        if (!toast.attributionText().empty()) {
            catchAndLogHresult(
                    { setAttributionTextFieldHelper(xmlDocument, toast.attributionText()); },
                    "Error in setAttributionTextFieldHelper: ",
                    {
                        setError(error, WinToast::WinToastError::UnknownError);
                        return -1;
                    }
            )
        }

        catchAndLogHresult(
                {
                    for (std::size_t i = 0, actionsCount = toast.actionsCount(); i < actionsCount; i++) {
                        WinToastArguments winToastArguments;

                        winToastArguments[L"actionId"] = std::to_wstring(i);
                        addActionHelper(xmlDocument, toast.actionLabel(i), winToastArguments.toString());
                    }
                },
                "Error in addActionHelper: ",
                {
                    setError(error, WinToast::WinToastError::UnknownError);
                    return -1;
                }
        )

        if (toast.audioPath().empty() && toast.audioOption() == WinToastTemplate::AudioOption::Default) {
            catchAndLogHresult(
                    { setAudioFieldHelper(xmlDocument, toast.audioPath(), toast.audioOption()); },
                    "Error in setAudioFieldHelper: ",
                    {
                        setError(error, WinToast::WinToastError::UnknownError);
                        return -1;
                    }
            )
        }

        auto toastElement = xmlDocument.SelectSingleNode(L"//toast[1]").as<XmlElement>();

        if (toast.duration() != WinToastTemplate::Duration::System) {
            catchAndLogHresult(
                    {
                        toastElement.SetAttribute(L"duration",
                                                  (toast.duration() == WinToastTemplate::Duration::Short) ? L"short"
                                                                                                          : L"long");
                    },
                    "Error in showToast while setting duration: ",
                    {
                        setError(error, WinToast::WinToastError::UnknownError);
                        return -1;
                    }
            )
        }

        catchAndLogHresult(
                { toastElement.SetAttribute(L"scenario", toast.scenario()); },
                "Error in showToast while setting scenario: ",
                {
                    setError(error, WinToast::WinToastError::UnknownError);
                    return -1;
                }
        )

    } else {
        DEBUG_MSG("Modern features (Actions/Sounds/Attributes) not supported in this os version");
    }

    if (toast.hasImage()) {
        catchAndLogHresult(
                { setImageFieldHelper(xmlDocument, toast.imagePath()); },
                "Error in setImageFieldHelper: ",
                {
                    setError(error, WinToast::WinToastError::UnknownError);
                    return -1;
                }
        )
    }

    ToastNotification notification{nullptr};
    INT64 expiration = 0, relativeExpiration = 0;
    catchAndLogHresult(
            {
                notification = xmlDocument;
                relativeExpiration = toast.expiration();
                if (relativeExpiration > 0) {
                    winrt::Windows::Foundation::DateTime expirationDateTime{
                            winrt::Windows::Foundation::TimeSpan(Util::fileTimeNow() + relativeExpiration * 10000)};
                    expiration = expirationDateTime.time_since_epoch().count();
                    notification.ExpirationTime(expirationDateTime);
                }
            },
            "Error in showToast while trying to construct the notification: ",
            {
                setError(error, WinToast::WinToastError::UnknownError);
                return -1;
            }
    )

    catchAndLogHresult(
            {
                Util::setEventHandlers(notification, std::shared_ptr<IWinToastHandler>(handler), expiration);
            },
            "Error in Util::setEventHandlers: ",
            {
                setError(error, WinToast::WinToastError::InvalidHandler);
                return -1;
            }
    )

    GUID guid;
    catchAndLogHresult(
            {
                winrt::check_hresult(CoCreateGuid(&guid));
            },
            "Error in CoCreateGuid: ",
            {
                setError(error, WinToast::WinToastError::UnknownError);
                return -1;
            }
    )

    id = guid.Data1;
    _buffer.insert(std::pair(id, notification));
    DEBUG_MSG("xml: " << xmlDocument.GetXml().c_str());
    catchAndLogHresult(
            {
                notifier.Show(notification);
            },
            "Error when showing notification: ",
            {
                setError(error, WinToast::WinToastError::NotDisplayed);
                return -1;
            }
    )

    return id;
}

bool WinToastImpl::hideToast(_In_ INT64 id) {
    if (!_isInitialized) {
        DEBUG_ERR("Error when hiding the toast. WinToast is not initialized.");
        return false;
    }

    if (_buffer.find(id) != _buffer.end()) {
        catchAndLogHresult(
                {
                    ToastNotifier notifier = ToastNotificationManager::CreateToastNotifier(_aumi);
                    notifier.Hide(_buffer.at(id));
                },
                "Error when hiding the toast: ",
                { return false; }
        )
        _buffer.erase(id);
        return true;
    }
    return false;
}

void WinToastImpl::clear() {
    catchAndLogHresult(
            {
                ToastNotifier notifier = ToastNotificationManager::CreateToastNotifier(_aumi);
                for (auto it = _buffer.begin(); it != _buffer.end(); ++it) {
                    notifier.Hide(it->second);
                }
            },
            "Error when clearing toasts: "
    )
    _buffer.clear();
}

//
// Available as of Windows 10 Anniversary Update
// Ref: https://docs.microsoft.com/en-us/windows/uwp/design/shell/tiles-and-notifications/adaptive-interactive-toasts
//
// NOTE: This will add a new text field, so be aware when iterating over
//       the toast's text fields or getting a count of them.
//
void WinToastImpl::setAttributionTextFieldHelper(_In_ XmlDocument xml, _In_ const std::wstring &text) {
    XmlElement attributionElement = Util::createElement(xml, L"binding", L"text");
    attributionElement.SetAttribute(L"placement", L"attribution");
    attributionElement.InnerText(text);
}

void WinToastImpl::setImageFieldHelper(_In_ XmlDocument xml, _In_ const std::wstring &path) {
    assert(path.size() < MAX_PATH);

    wchar_t imagePath[MAX_PATH] = L"file:///";
    winrt::check_hresult(StringCchCatW(imagePath, MAX_PATH, path.c_str()));
    XmlElement imageElement = xml.SelectSingleNode(L"//image[1]").as<XmlElement>();
    imageElement.SetAttribute(L"src", imagePath);
}

void WinToastImpl::setAudioFieldHelper(_In_ XmlDocument xml, _In_ const std::wstring &path, _In_opt_
                                       WinToastTemplate::AudioOption option) {
    Util::createElement(xml, L"toast", L"audio");

    XmlElement audioElement = xml.SelectSingleNode(L"//audio[1]").as<XmlElement>();

    if (!path.empty()) {
        audioElement.SetAttribute(L"src", path);
    }

    switch (option) {
        case WinToastTemplate::AudioOption::Loop:
            audioElement.SetAttribute(L"loop", L"true");
            break;
        case WinToastTemplate::AudioOption::Silent:
            audioElement.SetAttribute(L"silent", L"true");
        default:
            break;
    }
}

void WinToastImpl::addActionHelper(_In_ XmlDocument xml, _In_ const std::wstring &content, _In_
                                   const std::wstring &arguments) {
    XmlElement actionsElement = xml.SelectSingleNode(L"//actions[1]").as<XmlElement>();

    if (!actionsElement) {
        XmlElement toastElement = xml.SelectSingleNode(L"//toast[1]").as<XmlElement>();
        toastElement.SetAttribute(L"template", L"ToastGeneric");
        toastElement.SetAttribute(L"duration", L"long");

        actionsElement = xml.CreateElement(L"actions");
        toastElement.AppendChild(actionsElement);
    }

    XmlElement actionElement = xml.CreateElement(L"action");
    actionElement.SetAttribute(L"content", content);
    actionElement.SetAttribute(L"arguments", arguments);
    actionsElement.AppendChild(actionElement);
}

void WinToastImpl::setError(_Out_opt_ WinToast::WinToastError *error, _In_ WinToast::WinToastError value) {
    if (error) {
        *error = value;
    }
}
