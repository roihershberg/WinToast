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

#ifndef WINTOAST_DLL_IMPORTER_H
#define WINTOAST_DLL_IMPORTER_H

#include <Windows.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL;

namespace DllImporter {

    // Function load a function from library
    template<typename Function>
    HRESULT loadFunctionFromLibrary(HINSTANCE library, LPCSTR name, Function &func) {
        if (!library) {
            return E_INVALIDARG;
        }
        func = reinterpret_cast<Function>(GetProcAddress(library, name));
        return (func != nullptr) ? S_OK : E_FAIL;
    }

    typedef HRESULT(FAR STDAPICALLTYPE *f_SetCurrentProcessExplicitAppUserModelID)(__in PCWSTR AppID);

    typedef HRESULT(FAR STDAPICALLTYPE *f_PropVariantToString)(_In_ REFPROPVARIANT propvar, _Out_writes_(cch) PWSTR psz,
                                                               _In_ UINT cch);

    typedef HRESULT(FAR STDAPICALLTYPE *f_RoGetActivationFactory)(_In_ HSTRING activatableClassId, _In_ REFIID iid,
                                                                  _COM_Outptr_ void **factory);

    typedef HRESULT(FAR STDAPICALLTYPE *f_WindowsCreateStringReference)(_In_reads_opt_(length + 1) PCWSTR sourceString,
                                                                        UINT32 length, _Out_
                                                                        HSTRING_HEADER *hstringHeader,
                                                                        _Outptr_result_maybenull_ _Result_nullonfailure_
                                                                        HSTRING *string);

    typedef PCWSTR(FAR STDAPICALLTYPE *f_WindowsGetStringRawBuffer)(_In_ HSTRING string, _Out_opt_ UINT32 *length);

    typedef HRESULT(FAR STDAPICALLTYPE *f_WindowsDeleteString)(_In_opt_ HSTRING string);

    static f_SetCurrentProcessExplicitAppUserModelID SetCurrentProcessExplicitAppUserModelID;
    static f_PropVariantToString PropVariantToString;
    static f_RoGetActivationFactory RoGetActivationFactory;
    static f_WindowsCreateStringReference WindowsCreateStringReference;
    static f_WindowsGetStringRawBuffer WindowsGetStringRawBuffer;
    static f_WindowsDeleteString WindowsDeleteString;


    template<class T>
    _Check_return_ __inline HRESULT _1_GetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ T **factory) {
        return RoGetActivationFactory(activatableClassId, IID_INS_ARGS(factory));
    }

    template<typename T>
    inline HRESULT
    Wrap_GetActivationFactory(_In_ HSTRING activatableClassId, _Inout_ Details::ComPtrRef<T> factory) noexcept {
        return _1_GetActivationFactory(activatableClassId, factory.ReleaseAndGetAddressOf());
    }

    inline HRESULT initialize() {
        HINSTANCE LibShell32 = LoadLibraryW(L"SHELL32.DLL");
        HRESULT hr = loadFunctionFromLibrary(LibShell32, "SetCurrentProcessExplicitAppUserModelID",
                                             SetCurrentProcessExplicitAppUserModelID);
        if (SUCCEEDED(hr)) {
            HINSTANCE LibPropSys = LoadLibraryW(L"PROPSYS.DLL");
            hr = loadFunctionFromLibrary(LibPropSys, "PropVariantToString", PropVariantToString);
            if (SUCCEEDED(hr)) {
                HINSTANCE LibComBase = LoadLibraryW(L"COMBASE.DLL");
                const bool succeded =
                        SUCCEEDED(loadFunctionFromLibrary(LibComBase, "RoGetActivationFactory", RoGetActivationFactory))
                        && SUCCEEDED(loadFunctionFromLibrary(LibComBase, "WindowsCreateStringReference",
                                                             WindowsCreateStringReference))
                        && SUCCEEDED(loadFunctionFromLibrary(LibComBase, "WindowsGetStringRawBuffer",
                                                             WindowsGetStringRawBuffer))
                        && SUCCEEDED(loadFunctionFromLibrary(LibComBase, "WindowsDeleteString", WindowsDeleteString));
                return succeded ? S_OK : E_FAIL;
            }
        }
        return hr;
    }
}

#endif //WINTOAST_DLL_IMPORTER_H
