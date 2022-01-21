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

#ifndef WINTOAST_INTERNAL_DATE_TIME_H
#define WINTOAST_INTERNAL_DATE_TIME_H

#include <Windows.h>
#include <windows.devices.geolocation.h>

using namespace ABI::Windows::Foundation;

class InternalDateTime : public IReference<DateTime> {
public:
    static INT64 Now() {
        FILETIME now;
        GetSystemTimeAsFileTime(&now);
        return ((((INT64) now.dwHighDateTime) << 32) | now.dwLowDateTime);
    }

    explicit InternalDateTime(DateTime dateTime) : _dateTime(dateTime) {}

    explicit InternalDateTime(INT64 millisecondsFromNow) {
        _dateTime.UniversalTime = Now() + millisecondsFromNow * 10000;
    }

    virtual ~InternalDateTime() = default;

    operator INT64() const {
        return _dateTime.UniversalTime;
    }

    HRESULT STDMETHODCALLTYPE get_Value(DateTime *dateTime) override {
        *dateTime = _dateTime;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid, void **ppvObject) override {
        if (!ppvObject) {
            return E_POINTER;
        }
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IReference<DateTime>)) {
            *ppvObject = static_cast<IUnknown *>(static_cast<IReference<DateTime> *>(this));
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        return 1;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return 2;
    }

    HRESULT STDMETHODCALLTYPE GetIids(ULONG *, IID **) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING *) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel *) override {
        return E_NOTIMPL;
    }

protected:
    DateTime _dateTime;
};

#endif //WINTOAST_INTERNAL_DATE_TIME_H
