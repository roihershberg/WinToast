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

#include <unordered_map>
#include <cassert>

#include "wintoast_impl.h"

namespace WinToastLib::WinToast {

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
        WinToastImpl::setAppName(appName);
    }

    void setAppUserModelId(const std::wstring &aumi) {
        WinToastImpl::setAppUserModelId(aumi);
    }

    void setIconPath(const std::wstring &iconPath) {
        WinToastImpl::setIconPath(iconPath);
    }

    void setIconBackgroundColor(const std::wstring &iconBackgroundColor) {
        WinToastImpl::setIconBackgroundColor(iconBackgroundColor);
    }

    void setShortcutPolicy(ShortcutPolicy shortcutPolicy) {
        WinToastImpl::setShortcutPolicy(shortcutPolicy);
    }

    void setOnActivated(
            const std::function<void(const WinToastArguments &,
                                     const std::map<std::wstring, std::wstring> &)> &callback) {
        WinToastImpl::setOnActivated(callback);
    }

    bool isCompatible() {
        return WinToastImpl::isCompatible();
    }

    bool isSupportingModernFeatures() {
        return WinToastImpl::isSupportingModernFeatures();
    }

    std::wstring configureAUMI(const std::wstring &companyName,
                               const std::wstring &productName,
                               const std::wstring &subProduct,
                               const std::wstring &versionInformation) {
        return WinToastImpl::configureAUMI(companyName, productName, subProduct, versionInformation);
    }

    ShortcutResult createShortcut() {
        return WinToastImpl::createShortcut();
    }

    bool initialize(WinToastError *error) {
        return WinToastImpl::initialize(error);
    }

    void uninstall() {
        WinToastImpl::uninstall();
    }

    bool isInitialized() {
        return WinToastImpl::isInitialized();
    }

    const std::wstring &appName() {
        return WinToastImpl::appName();
    }

    const std::wstring &appUserModelId() {
        return WinToastImpl::appUserModelId();
    }

    const std::wstring &iconPath() {
        return WinToastImpl::iconPath();
    }

    const std::wstring &iconBackgroundColor() {
        return WinToastImpl::iconBackgroundColor();
    }

    INT64 showToast(const WinToastTemplate &toast, WinToastError *error) {
        return WinToastImpl::showToast(toast, error);
    }

    bool hideToast(INT64 id) {
        return WinToastImpl::hideToast(id);
    }

    void clear() {
        WinToastImpl::clear();
    }
}
