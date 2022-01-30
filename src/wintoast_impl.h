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

#include "wintoastlib.h"

namespace WinToastLib {

    class WinToastImpl {
    public:
        WinToastImpl();

        virtual ~WinToastImpl();

        static WinToastImpl &instance();

        static bool isCompatible();

        static bool isSupportingModernFeatures();

        static std::wstring configureAUMI(_In_ const std::wstring &companyName,
                                          _In_ const std::wstring &productName,
                                          _In_ const std::wstring &subProduct = std::wstring(),
                                          _In_ const std::wstring &versionInformation = std::wstring());

        virtual bool initialize(_Out_opt_ WinToast::WinToastError *error = nullptr);

        virtual bool isInitialized() const;

        virtual bool hideToast(_In_ INT64 id);

        virtual INT64 showToast(_In_ const WinToastTemplate &toast, _In_ IWinToastHandler *handler, _Out_opt_
                                WinToast::WinToastError *error = nullptr);

        virtual void clear();

        virtual WinToast::ShortcutResult createShortcut();

        const std::wstring &appName() const;

        const std::wstring &appUserModelId() const;

        void setAppUserModelId(_In_ const std::wstring &aumi);

        void setAppName(_In_ const std::wstring &appName);

        void setShortcutPolicy(_In_ WinToast::ShortcutPolicy policy);

    protected:
        bool _isInitialized{false};
        bool _hasCoInitialized{false};
        WinToast::ShortcutPolicy _shortcutPolicy{WinToast::ShortcutPolicy::SHORTCUT_POLICY_REQUIRE_CREATE};
        std::wstring _appName{};
        std::wstring _aumi{};
        std::map<INT64, winrt::Windows::UI::Notifications::ToastNotification> _buffer{};

        void validateShellLinkHelper(_Out_ bool &wasChanged);

        void createShellLinkHelper();

        void setImageFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &path);

        void setAudioFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &path,
                                 _In_opt_
                                 WinToastTemplate::AudioOption option = WinToastTemplate::AudioOption::Default);

        void
        setTextFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &text, _In_
                           UINT32 pos);

        void setAttributionTextFieldHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_
                                           const std::wstring &text);

        void
        addActionHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &action, _In_
                        const std::wstring &arguments);

        void addDurationHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &duration);

        void addScenarioHelper(_In_ winrt::Windows::Data::Xml::Dom::XmlDocument xml, _In_ const std::wstring &scenario);

        void setError(_Out_opt_ WinToast::WinToastError *error, _In_ WinToast::WinToastError value);
    };
}

#endif //WINTOAST_WINTOAST_IMPL_H
