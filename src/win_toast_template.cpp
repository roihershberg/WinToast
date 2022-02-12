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

#include <cassert>

using namespace WinToastLib;

WinToastTemplate::WinToastTemplate(WinToastTemplateType type) : _type(type) {
    static constexpr std::size_t TextFieldsCount[] = {1, 2, 2, 3, 1, 2, 2, 3};
    _textFields = std::vector<std::wstring>(TextFieldsCount[(int) type], L"");
}

WinToastTemplate::~WinToastTemplate() {
    _textFields.clear();
}

void WinToastTemplate::setTextField(const std::wstring &txt, WinToastTemplate::TextField pos) {
    const auto position = static_cast<std::size_t>(pos);
    assert(position < _textFields.size());
    _textFields[position] = txt;
}

void WinToastTemplate::setImagePath(const std::wstring &imgPath) {
    _imagePath = imgPath;
}

void WinToastTemplate::setAudioPath(const std::wstring &audioPath) {
    _audioPath = audioPath;
}

void WinToastTemplate::setAudioPath(AudioSystemFile file) {
    static const std::unordered_map<AudioSystemFile, std::wstring> Files = {
            {AudioSystemFile::DefaultSound, L"ms-winsoundevent:Notification.Default"},
            {AudioSystemFile::IM,           L"ms-winsoundevent:Notification.IM"},
            {AudioSystemFile::Mail,         L"ms-winsoundevent:Notification.Mail"},
            {AudioSystemFile::Reminder,     L"ms-winsoundevent:Notification.Reminder"},
            {AudioSystemFile::SMS,          L"ms-winsoundevent:Notification.SMS"},
            {AudioSystemFile::Alarm,        L"ms-winsoundevent:Notification.Looping.Alarm"},
            {AudioSystemFile::Alarm2,       L"ms-winsoundevent:Notification.Looping.Alarm2"},
            {AudioSystemFile::Alarm3,       L"ms-winsoundevent:Notification.Looping.Alarm3"},
            {AudioSystemFile::Alarm4,       L"ms-winsoundevent:Notification.Looping.Alarm4"},
            {AudioSystemFile::Alarm5,       L"ms-winsoundevent:Notification.Looping.Alarm5"},
            {AudioSystemFile::Alarm6,       L"ms-winsoundevent:Notification.Looping.Alarm6"},
            {AudioSystemFile::Alarm7,       L"ms-winsoundevent:Notification.Looping.Alarm7"},
            {AudioSystemFile::Alarm8,       L"ms-winsoundevent:Notification.Looping.Alarm8"},
            {AudioSystemFile::Alarm9,       L"ms-winsoundevent:Notification.Looping.Alarm9"},
            {AudioSystemFile::Alarm10,      L"ms-winsoundevent:Notification.Looping.Alarm10"},
            {AudioSystemFile::Call,         L"ms-winsoundevent:Notification.Looping.Call"},
            {AudioSystemFile::Call1,        L"ms-winsoundevent:Notification.Looping.Call1"},
            {AudioSystemFile::Call2,        L"ms-winsoundevent:Notification.Looping.Call2"},
            {AudioSystemFile::Call3,        L"ms-winsoundevent:Notification.Looping.Call3"},
            {AudioSystemFile::Call4,        L"ms-winsoundevent:Notification.Looping.Call4"},
            {AudioSystemFile::Call5,        L"ms-winsoundevent:Notification.Looping.Call5"},
            {AudioSystemFile::Call6,        L"ms-winsoundevent:Notification.Looping.Call6"},
            {AudioSystemFile::Call7,        L"ms-winsoundevent:Notification.Looping.Call7"},
            {AudioSystemFile::Call8,        L"ms-winsoundevent:Notification.Looping.Call8"},
            {AudioSystemFile::Call9,        L"ms-winsoundevent:Notification.Looping.Call9"},
            {AudioSystemFile::Call10,       L"ms-winsoundevent:Notification.Looping.Call10"},
    };
    const auto iter = Files.find(file);
    assert(iter != Files.end());
    _audioPath = iter->second;
}

void WinToastTemplate::setAudioOption(WinToastTemplate::AudioOption audioOption) {
    _audioOption = audioOption;
}

void WinToastTemplate::setFirstLine(const std::wstring &text) {
    setTextField(text, WinToastTemplate::TextField::FirstLine);
}

void WinToastTemplate::setSecondLine(const std::wstring &text) {
    setTextField(text, WinToastTemplate::TextField::SecondLine);
}

void WinToastTemplate::setThirdLine(const std::wstring &text) {
    setTextField(text, WinToastTemplate::TextField::ThirdLine);
}

void WinToastTemplate::setDuration(Duration duration) {
    _duration = duration;
}

void WinToastTemplate::setExpiration(INT64 millisecondsFromNow) {
    _expiration = millisecondsFromNow;
}

void WinToastLib::WinToastTemplate::setScenario(Scenario scenario) {
    switch (scenario) {
        case Scenario::Default:
            _scenario = L"Default";
            break;
        case Scenario::Alarm:
            _scenario = L"Alarm";
            break;
        case Scenario::IncomingCall:
            _scenario = L"IncomingCall";
            break;
        case Scenario::Reminder:
            _scenario = L"Reminder";
            break;
    }
}

void WinToastTemplate::setAttributionText(const std::wstring &attributionText) {
    _attributionText = attributionText;
}

void WinToastTemplate::addAction(const std::wstring &label) {
    _actions.push_back(label);
}

std::size_t WinToastTemplate::textFieldsCount() const {
    return _textFields.size();
}

std::size_t WinToastTemplate::actionsCount() const {
    return _actions.size();
}

bool WinToastTemplate::hasImage() const {
    return _type < WinToastTemplateType::Text01;
}

const std::vector<std::wstring> &WinToastTemplate::textFields() const {
    return _textFields;
}

const std::wstring &WinToastTemplate::textField(_In_ TextField pos) const {
    const auto position = static_cast<std::size_t>(pos);
    assert(position < _textFields.size());
    return _textFields[position];
}

const std::wstring &WinToastTemplate::actionLabel(_In_ std::size_t position) const {
    assert(position < _actions.size());
    return _actions[position];
}

const std::wstring &WinToastTemplate::imagePath() const {
    return _imagePath;
}

const std::wstring &WinToastTemplate::audioPath() const {
    return _audioPath;
}

const std::wstring &WinToastTemplate::attributionText() const {
    return _attributionText;
}

const std::wstring &WinToastLib::WinToastTemplate::scenario() const {
    return _scenario;
}

INT64 WinToastTemplate::expiration() const {
    return _expiration;
}

WinToastTemplate::WinToastTemplateType WinToastTemplate::type() const {
    return _type;
}

WinToastTemplate::AudioOption WinToastTemplate::audioOption() const {
    return _audioOption;
}

WinToastTemplate::Duration WinToastTemplate::duration() const {
    return _duration;
}
