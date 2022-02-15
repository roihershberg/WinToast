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

#ifndef WINTOAST_WINTOAST_IMPL_H
#define WINTOAST_WINTOAST_IMPL_H

#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.UI.Notifications.h>

#include <map>
#include <functional>

#include "wintoastlib.h"

namespace WinToastLib {

    class WinToastImpl {
    public:
        [[nodiscard]] static bool isCompatible();

        [[nodiscard]] static bool isSupportingModernFeatures();

        [[nodiscard]] static std::wstring configureAUMI(_In_ const std::wstring &companyName,
                                                        _In_ const std::wstring &productName,
                                                        _In_ const std::wstring &subProduct = std::wstring(),
                                                        _In_ const std::wstring &versionInformation = std::wstring());

        static bool initialize(_Out_opt_ WinToast::WinToastError *error = nullptr);

        static void uninstall();

        [[nodiscard]] static bool isInitialized();

        static bool hideToast(_In_ INT64 id);

        static INT64 showToast(_In_ const WinToastTemplate &toast, _Out_opt_ WinToast::WinToastError *error = nullptr);

        static void clear();

        static WinToast::ShortcutResult createShortcut();

        [[nodiscard]] static const std::wstring &appName();

        [[nodiscard]] static const std::wstring &appUserModelId();

        [[nodiscard]] static const std::wstring &iconPath();

        [[nodiscard]] static const std::wstring &iconBackgroundColor();

        static void setAppUserModelId(_In_ const std::wstring &aumi);

        static void setAppName(_In_ const std::wstring &appName);

        static void setIconPath(_In_ const std::wstring &iconPath);

        static void setIconBackgroundColor(_In_ const std::wstring &iconBackgroundColor);

        static void setShortcutPolicy(_In_ WinToast::ShortcutPolicy policy);

        static void setOnActivated(
                const std::function<void(const WinToastArguments &,
                                         const std::map<std::wstring, std::wstring> &)> &callback);

    private:
        struct callback;
        struct callback_factory;

        static bool _isInitialized;
        static bool _hasCoInitialized;
        static WinToast::ShortcutPolicy _shortcutPolicy;
        static std::wstring _appName;
        static std::wstring _aumi;
        static std::wstring _clsid;
        static std::wstring _iconPath;
        static std::wstring _iconBackgroundColor;
        static std::map<INT64, winrt::Windows::UI::Notifications::ToastNotification> _buffer;
        static std::function<void(const WinToastArguments &,
                                  const std::map<std::wstring, std::wstring> &)> _onActivated;

        static void createAndRegisterActivator();

        static void validateShellLinkHelper(_Out_ bool &wasChanged);

        static void createShellLinkHelper();

        static void
        setImageFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &path);

        static void
        setAudioFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &path,
                            _In_opt_
                            WinToastTemplate::AudioOption option = WinToastTemplate::AudioOption::Default);

        static void setAttributionTextFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_
                                                  const std::wstring &text);

        static void
        addActionHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &action, _In_
                        const std::wstring &arguments);
    };
}

#endif //WINTOAST_WINTOAST_IMPL_H
