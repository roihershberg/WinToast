/* * Copyright (c) 2022 Roee Hershberg <roihershberg@protonmail.com>
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

#include <vector>

using namespace WinToastLib;

inline void replaceAll(std::wstring &str, const std::wstring &from, const std::wstring &to) {
    std::size_t startPos = 0;

    while ((startPos = str.find(from, startPos)) != std::wstring::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length(); // Handles case where 'from' is a substring of 'to'
    }
}

inline std::vector<std::wstring> splitString(std::wstring str, wchar_t delim) {
    std::vector<std::wstring> res;
    std::size_t pos;

    while ((pos = str.find(delim)) != std::wstring::npos) {
        res.push_back(str.substr(0, pos));
        str.erase(0, pos + 1);
    }
    res.push_back(str);

    return res;
}

std::wstring encode(const std::wstring &str) {
    std::wstring encodedString{str};

    replaceAll(encodedString, L"%", L"%25");
    replaceAll(encodedString, L";", L"%3B");
    replaceAll(encodedString, L"=", L"%3D");
    replaceAll(encodedString, L"\"", L"%22");
    replaceAll(encodedString, L"'", L"%27");
    replaceAll(encodedString, L"<", L"%3C");
    replaceAll(encodedString, L">", L"%3E");
    replaceAll(encodedString, L"&", L"%26");

    return encodedString;
}

std::wstring decode(const std::wstring &str) {
    std::wstring decodedString{str};

    replaceAll(decodedString, L"%3B", L";");
    replaceAll(decodedString, L"%3D", L"=");
    replaceAll(decodedString, L"%22", L"\"");
    replaceAll(decodedString, L"%27", L"'");
    replaceAll(decodedString, L"%3C", L"<");
    replaceAll(decodedString, L"%3E", L">");
    replaceAll(decodedString, L"%26", L"&");
    replaceAll(decodedString, L"%25", L"%");

    return decodedString;
}

inline std::wstring encodePair(const std::wstring &key, const std::wstring &value) {
    if (value.empty())
        return encode(key);
    else
        return encode(key) + L'=' + encode(value);
}

WinToastArguments::WinToastArguments(const std::wstring &arguments) {
    parse(arguments);
}

void WinToastArguments::parse(const std::wstring &arguments) {
    mPairs.clear();

    if (arguments.find_first_not_of(' ') != std::wstring::npos) {
        for (const std::wstring &pair: splitString(arguments, ';')) {
            std::wstring key, value;
            std::size_t indexOfEquals = pair.find('=');

            if (indexOfEquals == std::wstring::npos) {
                key = decode(pair);
            } else {
                key = decode(pair.substr(0, indexOfEquals));
                value = decode(pair.substr(indexOfEquals + 1));
            }

            mPairs[key] = value;
        }
    }
}

std::wstring WinToastArguments::toString() const {
    if (mPairs.empty()) return {};

    std::wstring serializedString;
    auto it = mPairs.cbegin();

    for (; it != --mPairs.cend(); ++it) {
        serializedString += encodePair(it->first, it->second) + L';';
    }
    serializedString += encodePair(it->first, it->second);

    return serializedString;
}

void WinToastArguments::add(const std::wstring &key, const std::wstring &value) {
    mPairs[key] = value;
}

bool WinToastArguments::remove(const std::wstring &key) noexcept {
    return mPairs.erase(key);
}

std::wstring WinToastArguments::get(const std::wstring &key) const {
    return mPairs.at(key);
}

bool WinToastArguments::empty() const noexcept {
    return mPairs.empty();
}

bool WinToastArguments::contains(const std::wstring &key) const {
    return mPairs.count(key) > 0;
}

std::map<std::wstring, std::wstring>::size_type WinToastArguments::size() const noexcept {
    return mPairs.size();
}

std::map<std::wstring, std::wstring>::iterator WinToastArguments::begin() noexcept {
    return mPairs.begin();
}

std::map<std::wstring, std::wstring>::const_iterator WinToastArguments::cbegin() const noexcept {
    return mPairs.cbegin();
}

std::map<std::wstring, std::wstring>::iterator WinToastArguments::end() noexcept {
    return mPairs.end();
}

std::map<std::wstring, std::wstring>::const_iterator WinToastArguments::cend() const noexcept {
    return mPairs.cend();
}

std::wstring &WinToastArguments::operator[](const std::wstring &key) {
    return mPairs[key];
}
