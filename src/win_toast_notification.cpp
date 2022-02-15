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

#include <winrt/Windows.Data.Xml.Dom.h>

#include <iostream>
#include <memory>
#include <string_view>

#include "macros.h"

#pragma comment(lib, "windowsapp")

using namespace WinToastLib;
using namespace winrt::Windows::Data::Xml::Dom;

class WinToastNotification::Impl {
public:
    explicit Impl(const std::wstring &xml) {
        setXmlHelper(winrt::hstring(xml));
    }

    Impl(const Impl &o) {
        setXmlHelper(o.xmlDocument.GetXml());
    }

    Impl &operator=(const Impl &o) {
        setXmlHelper(o.xmlDocument.GetXml());
    }

    [[nodiscard]] std::wstring getXml() const {
        catchAndLogHresult(
                {
                    return std::wstring(xmlDocument.GetXml());
                },
                "Error when trying to get XML for the notification: ",
                { return {}; }
        )
    }

    bool setXml(const std::wstring &xml) {
        return setXmlHelper(winrt::hstring(xml));
    }

    void show() const {

    }

private:
    XmlDocument xmlDocument;

    bool setXmlHelper(const winrt::hstring &xml) {
        catchAndLogHresult(
                {
                    xmlDocument.LoadXml(xml);
                    return true;
                },
                "Error when trying to load XML for the notification: ",
                { return false; }
        )
    }
};

WinToastNotification::WinToastNotification(const std::wstring &xml) : _impl(std::make_unique<Impl>(xml)) {}

WinToastNotification::~WinToastNotification() = default;

WinToastNotification::WinToastNotification(WinToastNotification &&o) noexcept = default;

WinToastNotification &WinToastNotification::operator=(WinToastNotification &&o) noexcept = default;

WinToastNotification::WinToastNotification(const WinToastNotification &o)
        : _impl(std::make_unique<Impl>(*o.d_func())) {}

WinToastNotification &WinToastNotification::operator=(const WinToastNotification &o) {
    if (this != &o) {
        _impl = std::make_unique<Impl>(*o.d_func());
    }
    return *this;
}

[[nodiscard]] std::wstring WinToastNotification::getXml() const { return d_func()->getXml(); }

bool WinToastNotification::setXml(const std::wstring &xml) { return d_func()->setXml(xml); }

void WinToastNotification::show() const { d_func()->show(); }
