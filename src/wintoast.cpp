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

#include "wintoastlib.h"

#include <ShObjIdl.h>
#include <strsafe.h>
#include <Psapi.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>
#include <VersionHelpers.h>
#include <NotificationActivationCallback.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <map>
#include <unordered_map>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <array>
#include <string_view>

#include "macros.h"

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "propsys")
#pragma comment(lib, "user32")
#pragma comment(lib, "windowsapp")

#define DEFAULT_SHELL_LINKS_PATH    L"\\Microsoft\\Windows\\Start Menu\\Programs\\"
#define DEFAULT_LINK_FORMAT            L".lnk"
#define STATUS_SUCCESS (0x00000000)

using namespace WinToastLib;
using namespace winrt::Windows::UI::Notifications;
using namespace winrt::Windows::Data::Xml::Dom;

struct prop_variant : PROPVARIANT {
    prop_variant() noexcept: PROPVARIANT{} {
    }

    ~prop_variant() noexcept {
        clear();
    }

    void clear() noexcept {
        WINRT_VERIFY_(S_OK, PropVariantClear(this));
    }
};

namespace Util {

    typedef LONG NTSTATUS, *PNTSTATUS;

    typedef NTSTATUS(WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    inline RTL_OSVERSIONINFOW getRealOSVersion() {
        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            auto fxPtr = (RtlGetVersionPtr) ::GetProcAddress(hMod, "RtlGetVersion");
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

    inline ToastTemplateType getToastTemplateType(const WinToastNotificationBuilder::WinToastTemplateType type) {
        switch (type) {
            case WinToastNotificationBuilder::WinToastTemplateType::ImageAndText01:
                return ToastTemplateType::ToastImageAndText01;
            case WinToastNotificationBuilder::WinToastTemplateType::ImageAndText02:
                return ToastTemplateType::ToastImageAndText02;
            case WinToastNotificationBuilder::WinToastTemplateType::ImageAndText03:
                return ToastTemplateType::ToastImageAndText03;
            case WinToastNotificationBuilder::WinToastTemplateType::ImageAndText04:
                return ToastTemplateType::ToastImageAndText04;
            case WinToastNotificationBuilder::WinToastTemplateType::Text01:
                return ToastTemplateType::ToastText01;
            case WinToastNotificationBuilder::WinToastTemplateType::Text02:
                return ToastTemplateType::ToastText02;
            case WinToastNotificationBuilder::WinToastTemplateType::Text03:
                return ToastTemplateType::ToastText03;
            case WinToastNotificationBuilder::WinToastTemplateType::Text04:
                return ToastTemplateType::ToastText04;
            default:
                winrt::throw_hresult(E_INVALIDARG);
        }
    }

    std::wstring generateGuid(const std::wstring &name) {
        // From https://github.com/WindowsNotifications/desktop-toasts/blob/master/CPP-WINRT/DesktopToastsCppWinRtApp/DesktopNotificationManagerCompat.cpp
        wchar_t const *bytes = name.c_str();

        if (name.length() <= 16) {
            wchar_t guid[36];
            swprintf_s(
                    guid,
                    36,
                    L"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                    name[0], name[1], name[2], name[3], name[4], name[5], name[6], name[7], name[8], name[9], name[10],
                    name[11], name[12], name[13], name[14], name[15]);
            return guid;
        } else {
            std::size_t hash = std::hash<std::wstring>{}(name);

            // Only ever at most 20 chars long
            std::wstring hashStr = std::to_wstring(hash);

            wchar_t guid[37];
            for (int i = 0; i < 36; i++) {
                if (i == 8 || i == 13 || i == 18 || i == 23) {
                    guid[i] = '-';
                } else {
                    int strPos = i;
                    if (i > 23) {
                        strPos -= 4;
                    } else if (i > 18) {
                        strPos -= 3;
                    } else if (i > 13) {
                        strPos -= 2;
                    } else if (i > 8) {
                        strPos -= 1;
                    }

                    if (strPos < hashStr.length()) {
                        guid[i] = hashStr[strPos];
                    } else {
                        guid[i] = '0';
                    }
                }
            }

            guid[36] = '\0';

            return guid;
        }
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

    inline XmlElement
    createElement(_In_ XmlDocument xml, _In_ const std::wstring &root_node, _In_ const std::wstring &element_name) {
        IXmlNode root = xml.SelectSingleNode(L"//" + root_node + L"[1]");
        XmlElement element = xml.CreateElement(element_name);
        root.AppendChild(element);

        return element;
    }

    inline void setRegistryKeyValue(HKEY hKey, const std::wstring &subKey, const std::wstring &valueName,
                                    const std::wstring &value) {
        winrt::check_win32(::RegSetKeyValueW(
                hKey,
                subKey.c_str(),
                valueName.empty() ? nullptr : valueName.c_str(),
                REG_SZ,
                reinterpret_cast<const BYTE *>(value.c_str()),
                static_cast<DWORD>((value.length() + 1) * sizeof(WCHAR))));
    }

    inline void deleteRegistryKeyValue(HKEY hKey, const std::wstring &subKey, const std::wstring &valueName) {
        winrt::check_win32(::RegDeleteKeyValueW(
                hKey,
                subKey.c_str(),
                valueName.c_str()));
    }

    inline void deleteRegistryKey(HKEY hKey, const std::wstring &subKey) {
        winrt::check_win32(::RegDeleteKeyW(
                hKey,
                subKey.c_str()));
    }
}

namespace WinToastLib::WinToast {
    namespace {
        bool _isInitialized = false;
        bool _hasCoInitialized = false;
        WinToast::ShortcutPolicy _shortcutPolicy{WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE};
        std::wstring _appName{};
        std::wstring _aumi{};
        std::wstring _clsid{};
        std::wstring _iconPath{};
        std::wstring _iconBackgroundColor{};
        std::map<INT64, winrt::Windows::UI::Notifications::ToastNotification> _buffer{};
        std::function<void(const WinToastArguments &, const std::map<std::wstring, std::wstring> &)> _onActivated;
    }

    // https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/author-coclasses#implement-the-coclass-and-class-factory
    struct callback : winrt::implements<callback, INotificationActivationCallback> {
        HRESULT __stdcall Activate(
                LPCWSTR appUserModelId,
                LPCWSTR invokedArgs,
                [[maybe_unused]] NOTIFICATION_USER_INPUT_DATA const *data,
                [[maybe_unused]] ULONG dataCount) noexcept {
            if (_onActivated != nullptr) {
                try {
                    WinToastArguments arguments(invokedArgs);

                    std::map<std::wstring, std::wstring> userInput;

                    for (int i = 0; i < dataCount; i++) {
                        userInput[data[i].Key] = data[i].Value;
                    }

                    _onActivated(arguments, userInput);
                } catch (const winrt::hresult_error &ex) {
                    DEBUG_ERR("Error in Activate callback: " << ex.message().c_str());
                } catch (const std::exception &ex) {
                    DEBUG_ERR("Error in Activate callback: " << ex.what());
                }
            }

            return S_OK;
        }
    };

    struct callback_factory : winrt::implements<callback_factory, IClassFactory> {
        HRESULT __stdcall CreateInstance(
                IUnknown *outer,
                GUID const &iid,
                void **result) noexcept {
            *result = nullptr;

            if (outer) {
                return CLASS_E_NOAGGREGATION;
            }

            return winrt::make<callback>()->QueryInterface(iid, result);
        }

        HRESULT __stdcall LockServer(BOOL) noexcept {
            return S_OK;
        }
    };

    void setError(WinToast::WinToastError *error, WinToast::WinToastError value) {
        if (error) {
            *error = value;
        }
    }

    const std::wstring &strerror(WinToastError error) {
        static const std::unordered_map<WinToastError, std::wstring> Labels = {
                {WinToastError::NoError,               L"No error. The process was executed correctly"},
                {WinToastError::NotInitialized,        L"The library has not been initialized"},
                {WinToastError::SystemNotSupported,    L"The OS does not support WinToast"},
                {WinToastError::ApartmentInitError,    L"Failed to initialize apartment"},
                {WinToastError::ShellLinkNotCreated,   L"The library was not able to create a Shell Link for the app"},
                {WinToastError::InvalidAppUserModelID, L"The AUMI is not a valid one"},
                {WinToastError::InvalidParameters,     L"The parameters used to configure the library are not valid normally because an invalid AUMI or App Name"},
                {WinToastError::NotDisplayed,          L"The toast was created correctly but WinToast was not able to display the toast"},
                {WinToastError::UnknownError,          L"Unknown error"}
        };

        const auto iter = Labels.find(error);
        assert(iter != Labels.end());
        return iter->second;
    }

    void setAppName(const std::wstring &appName) {
        _appName = appName;
    }

    void setAppUserModelId(const std::wstring &aumi) {
        _aumi = aumi;
        DEBUG_MSG("App User Model Id: " << _aumi.c_str());
    }

    void setIconPath(const std::wstring &iconPath) {
        _iconPath = iconPath;
    }

    void setIconBackgroundColor(const std::wstring &iconBackgroundColor) {
        _iconBackgroundColor = iconBackgroundColor;
    }

    void setShortcutPolicy(ShortcutPolicy shortcutPolicy) {
        _shortcutPolicy = shortcutPolicy;
    }

    void setOnActivated(
            const std::function<void(const WinToastArguments &,
                                     const std::map<std::wstring, std::wstring> &)> &callback) {
        _onActivated = callback;
    }

    bool isCompatible() {
        return IsWindows8OrGreater();
    }

    bool isSupportingModernFeatures() {
        constexpr auto MinimumSupportedVersion = 6;
        return Util::getRealOSVersion().dwMajorVersion > MinimumSupportedVersion;
    }

    std::wstring configureAUMI(const std::wstring &companyName,
                               const std::wstring &productName,
                               const std::wstring &subProduct,
                               const std::wstring &versionInformation) {
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

    void validateShellLinkHelper(bool &wasChanged) {
        WCHAR path[MAX_PATH] = {L'\0'};
        Util::defaultShellLinkPath(_appName, path);
        // Check if the file exist
        DWORD attr = GetFileAttributesW(path);
        if (attr >= 0xFFFFFFF) {
            throw winrt::hresult_error(E_FAIL,
                                       L"Error, shell link not found. Try to create a new one in: " +
                                       std::wstring(path));
        }

        // Let's load the file as shell link to validate.
        // - Create a shell link
        // - Create a persistant file
        // - Load the path as data for the persistant file
        // - Read the property AUMI and validate with the current
        // - Review if AUMI is equal.

        winrt::com_ptr<IShellLink> shellLink;
        winrt::check_hresult(
                CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink)));

        auto persistFile = shellLink.as<IPersistFile>();
        winrt::check_hresult(persistFile->Load(path, STGM_READWRITE));

        auto propertyStore = shellLink.as<IPropertyStore>();
        prop_variant appIdPropVar;
        winrt::check_hresult(propertyStore->GetValue(PKEY_AppUserModel_ID, &appIdPropVar));
        WCHAR AUMI[MAX_PATH];
        winrt::check_hresult(PropVariantToString(appIdPropVar, AUMI, MAX_PATH));
        appIdPropVar.clear();

        wasChanged = false;
        if (_aumi != AUMI) {
            if (_shortcutPolicy == WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE) {
                // AUMI Changed for the same app, let's update the current value! =)
                wasChanged = true;
                winrt::check_hresult(InitPropVariantFromString(_aumi.c_str(), &appIdPropVar));
                winrt::check_hresult(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
                appIdPropVar.clear();
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

    void createShellLinkHelper() {
        if (_shortcutPolicy != WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE) {
            throw winrt::hresult_error(E_FAIL, L"Configured shortcut policy is not allowing to create shortcuts.");
        }

        WCHAR exePath[MAX_PATH]{L'\0'};
        WCHAR slPath[MAX_PATH]{L'\0'};
        Util::defaultShellLinkPath(_appName, slPath);
        Util::defaultExecutablePath(exePath);

        winrt::com_ptr<IShellLinkW> shellLink;
        winrt::check_hresult(
                CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink)));
        winrt::check_hresult(shellLink->SetPath(exePath));
        winrt::check_hresult(shellLink->SetArguments(L""));
        winrt::check_hresult(shellLink->SetWorkingDirectory(exePath));

        winrt::com_ptr<IPropertyStore> propertyStore = shellLink.as<IPropertyStore>();
        prop_variant appIdPropVar;
        winrt::check_hresult(InitPropVariantFromString(_aumi.c_str(), &appIdPropVar));
        winrt::check_hresult(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
        appIdPropVar.clear();
        winrt::check_hresult(propertyStore->Commit());

        winrt::com_ptr<IPersistFile> persistFile = shellLink.as<IPersistFile>();
        winrt::check_hresult(persistFile->Save(slPath, TRUE));
    }

    ShortcutResult createShortcut() {
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

    void createAndRegisterActivator() {
        // From https://github.com/WindowsNotifications/desktop-toasts/blob/master/CPP-WINRT/DesktopToastsCppWinRtApp/DesktopNotificationManagerCompat.cpp
        DWORD registration{};
        std::wstring clsidStr = Util::generateGuid(_aumi);
        GUID clsid;
        winrt::check_hresult(CLSIDFromString((L"{" + clsidStr + L"}").c_str(), &clsid));

        // Register callback
        auto result = CoRegisterClassObject(
                clsid,
                winrt::make<callback_factory>().get(),
                CLSCTX_LOCAL_SERVER,
                REGCLS_MULTIPLEUSE,
                &registration);

        // Create launch path + args
        // Include a flag so we know this was a toast activation and should wait for COM to process
        WCHAR exePath[MAX_PATH]{L'\0'};
        Util::defaultExecutablePath(exePath);
        std::string launchArg = TOAST_ACTIVATED_LAUNCH_ARG;
        std::wstring launchArgW(launchArg.begin(), launchArg.end());
        std::wstring launchStr = L"\"" + std::wstring(exePath) + L"\" " + launchArgW;

        // Update registry with activator
        std::wstring keyPath = LR"(SOFTWARE\Classes\CLSID\{)" + clsidStr + LR"(}\LocalServer32)";
        Util::setRegistryKeyValue(HKEY_CURRENT_USER, keyPath, L"", launchStr);
        _clsid = clsidStr;
    }

    bool initialize(WinToastError *error) {
        _isInitialized = true;
        setError(error, WinToast::WinToastError::NoError);

        if (!isCompatible()) {
            setError(error, WinToast::WinToastError::SystemNotSupported);
            DEBUG_ERR(L"Error: system not supported.");
            _isInitialized = false;
            return false;
        }

        if (_aumi.empty() || _appName.empty()) {
            setError(error, WinToast::WinToastError::InvalidParameters);
            DEBUG_ERR(L"Error while initializing, did you set up a valid AUMI and App name?");
            _isInitialized = false;
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
                        _isInitialized = false;
                        return false;
                    }
            )
        }

        if (_shortcutPolicy != WinToast::ShortcutPolicy::SHORTCUT_POLICY_IGNORE) {
            if ((int) createShortcut() < 0) {
                setError(error, WinToast::WinToastError::ShellLinkNotCreated);
                DEBUG_ERR(L"Error while attaching the AUMI to the current proccess =(");
                _isInitialized = false;
                return false;
            }
        }

        if (FAILED(SetCurrentProcessExplicitAppUserModelID(_aumi.c_str()))) {
            setError(error, WinToast::WinToastError::InvalidAppUserModelID);
            DEBUG_ERR(L"Error while attaching the AUMI to the current proccess =(");
            _isInitialized = false;
            return false;
        }

        catchAndLogHresult(
                {
                    createAndRegisterActivator();
                },
                "Error while trying to create and register Activator: ",
                {
                    setError(error, WinToast::WinToastError::UnknownError);
                    _isInitialized = false;
                    return false;
                }
        )

        catchAndLogHresult(
                {
                    std::wstring subKey = LR"(SOFTWARE\Classes\AppUserModelId\)" + _aumi;
                    Util::setRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"DisplayName", _appName);

                    if (!_iconPath.empty()) {
                        Util::setRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"IconUri", L"file:///" + _iconPath);
                    } else {
                        try {
                            Util::deleteRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"IconUri");
                        } catch (const winrt::hresult_error &ex) {
                            DEBUG_MSG(
                                    "Failed to delete IconUri registry key. Probably iconUri wasn't set before.\n\tError message: "
                                            << ex.message().c_str());
                        }
                    }

                    // Background color only appears in the settings page, format is
                    // hex without leading #, like "FFDDDDDD"
                    if (!_iconBackgroundColor.empty()) {
                        Util::setRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"IconBackgroundColor",
                                                  _iconBackgroundColor);
                    } else {
                        try {
                            Util::deleteRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"IconBackgroundColor");
                        } catch (const winrt::hresult_error &ex) {
                            DEBUG_MSG(
                                    "Failed to delete IconBackgroundColor registry key. Probably iconBackgroundColor wasn't set before.\n\tError message: "
                                            << ex.message().c_str());
                        }
                    }

                    Util::setRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"CustomActivator", L"{" + _clsid + L"}");
                },
                "Error while trying to set registry values: ",
                {
                    setError(error, WinToast::WinToastError::UnknownError);
                    _isInitialized = false;
                    return false;
                }
        )

        return true;
    }

    void uninstall() {
        // From https://github.com/WindowsNotifications/desktop-toasts/blob/master/CPP-WINRT/DesktopToastsCppWinRtApp/DesktopNotificationManagerCompat.cpp
        if (!_aumi.empty()) {
            try {
                // Remove all scheduled notifications (do this first before clearing current notifications)
                ToastNotifier notifier{nullptr};
                catchAndLogHresult(
                        { notifier = ToastNotificationManager::CreateToastNotifier(_aumi); },
                        "Error in showToast while trying to create a notifier: ",
                        { return; }
                )
                auto scheduledNotifications = notifier.GetScheduledToastNotifications();
                UINT vectorSize = scheduledNotifications.Size();
                for (UINT i = 0; i < vectorSize; i++) {
                    try {
                        notifier.RemoveFromSchedule(scheduledNotifications.GetAt(i));
                    }
                    catch (...) {}
                }

                // Clear all current notifications
                ToastNotificationManager::History().Clear(_aumi);

                std::wstring subKey = LR"(SOFTWARE\Classes\AppUserModelId\)" + _aumi;
                Util::deleteRegistryKey(HKEY_CURRENT_USER, subKey);
                if (!_clsid.empty()) {
                    std::wstring baseSubKey = LR"(SOFTWARE\Classes\CLSID\{)" + _clsid + L"}";
                    subKey = baseSubKey + LR"(\LocalServer32)";
                    Util::deleteRegistryKey(HKEY_CURRENT_USER, subKey);
                    Util::deleteRegistryKey(HKEY_CURRENT_USER, baseSubKey);
                }
            }
            catch (...) {}
        }
    }

    bool isInitialized() {
        return _isInitialized;
    }

    const std::wstring &appName() {
        return _appName;
    }

    const std::wstring &appUserModelId() {
        return _aumi;
    }

    const std::wstring &iconPath() {
        return _iconPath;
    }

    const std::wstring &iconBackgroundColor() {
        return _iconBackgroundColor;
    }

    //
    // Available as of Windows 10 Anniversary Update
    // Ref: https://docs.microsoft.com/en-us/windows/uwp/design/shell/tiles-and-notifications/adaptive-interactive-toasts
    //
    // NOTE: This will add a new text field, so be aware when iterating over
    //       the toast's text fields or getting a count of them.
    //
    void setAttributionTextFieldHelper(XmlDocument xml, const std::wstring &text) {
        XmlElement attributionElement = Util::createElement(xml, L"binding", L"text");
        attributionElement.SetAttribute(L"placement", L"attribution");
        attributionElement.InnerText(text);
    }

    void setImageFieldHelper(XmlDocument xml, const std::wstring &path) {
        assert(path.size() < MAX_PATH);

        wchar_t imagePath[MAX_PATH] = L"file:///";
        winrt::check_hresult(StringCchCatW(imagePath, MAX_PATH, path.c_str()));
        XmlElement imageElement = xml.SelectSingleNode(L"//image[1]").as<XmlElement>();
        imageElement.SetAttribute(L"src", imagePath);
    }

    void setAudioFieldHelper(XmlDocument xml, const std::wstring &path, WinToastNotificationBuilder::AudioOption option) {
        Util::createElement(xml, L"toast", L"audio");

        XmlElement audioElement = xml.SelectSingleNode(L"//audio[1]").as<XmlElement>();

        if (!path.empty()) {
            audioElement.SetAttribute(L"src", path);
        }

        switch (option) {
            case WinToastNotificationBuilder::AudioOption::Loop:
                audioElement.SetAttribute(L"loop", L"true");
                break;
            case WinToastNotificationBuilder::AudioOption::Silent:
                audioElement.SetAttribute(L"silent", L"true");
            default:
                break;
        }
    }

    void addActionHelper(XmlDocument xml, const std::wstring &content, const std::wstring &arguments) {
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

    INT64 showToast(const WinToastNotificationBuilder &toast, WinToastError *error) {
        setError(error, WinToast::WinToastError::NoError);
        INT64 id = 0;
        if (!isInitialized()) {
            setError(error, WinToast::WinToastError::NotInitialized);
            DEBUG_ERR("Error when launching the toast. WinToast is not initialized.");
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
                            Util::getToastTemplateType(toast.getType())
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

                    for (UINT32 i = 0, fieldsCount = static_cast<UINT32>(toast.getTextFieldsCount());
                         i < fieldsCount; i++) {
                        textFields.Item(i).InnerText(toast.getTextField(WinToastNotificationBuilder::TextField(i)));
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

            // Note that we do this *after* using toast.getTextFieldsCount() to
            // iterate/fill the template's text fields, since we're adding yet another text field.
            if (!toast.getAttributionText().empty()) {
                catchAndLogHresult(
                        { setAttributionTextFieldHelper(xmlDocument, toast.getAttributionText()); },
                        "Error in setAttributionTextFieldHelper: ",
                        {
                            setError(error, WinToast::WinToastError::UnknownError);
                            return -1;
                        }
                )
            }

            catchAndLogHresult(
                    {
                        for (std::size_t i = 0, actionsCount = toast.getActionsCount(); i < actionsCount; i++) {
                            WinToastArguments winToastArguments;

                            winToastArguments[L"actionId"] = std::to_wstring(i);
                            addActionHelper(xmlDocument, toast.getActionLabel(i), winToastArguments.toString());
                        }
                    },
                    "Error in addActionHelper: ",
                    {
                        setError(error, WinToast::WinToastError::UnknownError);
                        return -1;
                    }
            )

            if (toast.getAudioPath().empty() && toast.getAudioOption() == WinToastNotificationBuilder::AudioOption::Default) {
                catchAndLogHresult(
                        { setAudioFieldHelper(xmlDocument, toast.getAudioPath(), toast.getAudioOption()); },
                        "Error in setAudioFieldHelper: ",
                        {
                            setError(error, WinToast::WinToastError::UnknownError);
                            return -1;
                        }
                )
            }

            auto toastElement = xmlDocument.SelectSingleNode(L"//toast[1]").as<XmlElement>();

            if (toast.getDuration() != WinToastNotificationBuilder::Duration::System) {
                catchAndLogHresult(
                        {
                            toastElement.SetAttribute(L"duration",
                                                      (toast.getDuration() == WinToastNotificationBuilder::Duration::Short) ? L"short"
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
                    { toastElement.SetAttribute(L"scenario", toast.getScenario()); },
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
                    { setImageFieldHelper(xmlDocument, toast.getImagePath()); },
                    "Error in setImageFieldHelper: ",
                    {
                        setError(error, WinToast::WinToastError::UnknownError);
                        return -1;
                    }
            )
        }

        ToastNotification notification{nullptr};
        catchAndLogHresult(
                {
                    INT64 relativeExpiration = toast.getExpiration();
                    notification = xmlDocument;
                    if (relativeExpiration > 0) {
                        winrt::Windows::Foundation::DateTime expirationDateTime{
                                winrt::Windows::Foundation::TimeSpan(Util::fileTimeNow() + relativeExpiration * 10000)};
                        notification.ExpirationTime(expirationDateTime);
                    }
                },
                "Error in showToast while trying to construct the notification: ",
                {
                    setError(error, WinToast::WinToastError::UnknownError);
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

    bool hideToast(INT64 id) {
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

    void clear() {
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
}
