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
#include <memory>

using namespace WinToastLib;

class WinToastNotificationBuilder::Impl {
public:
    explicit Impl(
            WinToastNotificationBuilder::WinToastTemplateType type = WinToastNotificationBuilder::WinToastTemplateType::ImageAndText02) {
        static constexpr std::size_t TextFieldsCount[] = {1, 2, 2, 3, 1, 2, 2, 3};
        _textFields = std::vector<std::wstring>(TextFieldsCount[(int) type], L"");
        _type = type;
    }

    ~Impl() {
        _textFields.clear();
    }

    void setTextField(const std::wstring &txt, WinToastNotificationBuilder::TextField pos) {
        const auto position = static_cast<std::size_t>(pos);
        assert(position < _textFields.size());
        _textFields[position] = txt;
    }

    void setAttributionText(const std::wstring &attributionText) { _attributionText = attributionText; }

    void setImagePath(const std::wstring &imgPath) { _imagePath = imgPath; }

    void setAudioPath(const WinToastNotificationBuilder::AudioSystemFile file) {
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

    void setAudioPath(const std::wstring &audioPath) { _audioPath = audioPath; }

    void setAudioOption(const WinToastNotificationBuilder::AudioOption audioOption) { _audioOption = audioOption; }

    void setDuration(const WinToastNotificationBuilder::Duration duration) { _duration = duration; }

    void setExpiration(const INT64 millisecondsFromNow) { _expiration = millisecondsFromNow; }

    void setScenario(const WinToastNotificationBuilder::Scenario scenario) {
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

    void addAction(const std::wstring &label) { _actions.push_back(label); }

    [[nodiscard]] std::size_t getTextFieldsCount() const { return _textFields.size(); }

    [[nodiscard]] std::size_t getActionsCount() const { return _actions.size(); }

    [[nodiscard]] bool hasImage() const { return _type < WinToastTemplateType::Text01; }

    [[nodiscard]] const std::vector<std::wstring> &getTextFields() const { return _textFields; }

    [[nodiscard]] const std::wstring &getTextField(const WinToastNotificationBuilder::TextField pos) const {
        const auto position = static_cast<std::size_t>(pos);
        assert(position < _textFields.size());
        return _textFields[position];
    }

    [[nodiscard]] const std::wstring &getActionLabel(const std::size_t pos) const {
        assert(pos < _actions.size());
        return _actions[pos];
    }

    [[nodiscard]] const std::wstring &getImagePath() const { return _imagePath; }

    [[nodiscard]] const std::wstring &getAudioPath() const { return _audioPath; }

    [[nodiscard]] const std::wstring &getAttributionText() const { return _attributionText; }

    [[nodiscard]] const std::wstring &getScenario() const { return _scenario; }

    [[nodiscard]] INT64 getExpiration() const { return _expiration; }

    [[nodiscard]] WinToastNotificationBuilder::WinToastTemplateType getType() const { return _type; }

    [[nodiscard]] WinToastNotificationBuilder::AudioOption getAudioOption() const { return _audioOption; }

    [[nodiscard]] WinToastNotificationBuilder::Duration getDuration() const { return _duration; }

private:
    std::vector<std::wstring> _textFields{};
    std::vector<std::wstring> _actions{};
    std::wstring _imagePath{};
    std::wstring _audioPath{};
    std::wstring _attributionText{};
    std::wstring _scenario{L"Default"};
    INT64 _expiration{0};
    WinToastNotificationBuilder::AudioOption _audioOption{WinToastNotificationBuilder::AudioOption::Default};
    WinToastNotificationBuilder::WinToastTemplateType _type{WinToastNotificationBuilder::WinToastTemplateType::Text01};
    WinToastNotificationBuilder::Duration _duration{WinToastNotificationBuilder::Duration::System};
};

WinToastNotificationBuilder::WinToastNotificationBuilder(WinToastTemplateType type) : _impl(std::make_unique<Impl>()) {}

WinToastNotificationBuilder::~WinToastNotificationBuilder() = default;

WinToastNotificationBuilder::WinToastNotificationBuilder(WinToastNotificationBuilder &&o) noexcept = default;

WinToastNotificationBuilder &WinToastNotificationBuilder::operator=(WinToastNotificationBuilder &&o) noexcept = default;

WinToastNotificationBuilder::WinToastNotificationBuilder(const WinToastNotificationBuilder &o)
        : _impl(std::make_unique<Impl>(*o.d_func())) {}

WinToastNotificationBuilder &WinToastNotificationBuilder::operator=(const WinToastNotificationBuilder &o) {
    if (this != &o) {
        _impl = std::make_unique<Impl>(*o.d_func());
    }
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setTextField(const std::wstring &text, const TextField pos) {
    d_func()->setTextField(text, pos);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setImagePath(const std::wstring &imgPath) {
    d_func()->setImagePath(imgPath);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setAudioPath(const AudioSystemFile file) {
    d_func()->setAudioPath(file);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setAudioPath(const std::wstring &audioPath) {
    d_func()->setAudioPath(audioPath);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setAudioOption(const AudioOption audioOption) {
    d_func()->setAudioOption(audioOption);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setFirstLine(const std::wstring &text) {
    d_func()->setTextField(text, TextField::FirstLine);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setSecondLine(const std::wstring &text) {
    d_func()->setTextField(text, TextField::SecondLine);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setThirdLine(const std::wstring &text) {
    d_func()->setTextField(text, TextField::ThirdLine);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setDuration(const Duration duration) {
    d_func()->setDuration(duration);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setExpiration(const INT64 millisecondsFromNow) {
    d_func()->setExpiration(millisecondsFromNow);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setScenario(const Scenario scenario) {
    d_func()->setScenario(scenario);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::setAttributionText(const std::wstring &attributionText) {
    d_func()->setAttributionText(attributionText);
    return *this;
}

WinToastNotificationBuilder &WinToastNotificationBuilder::addAction(const std::wstring &label) {
    d_func()->addAction(label);
    return *this;
}

std::size_t WinToastNotificationBuilder::getTextFieldsCount() const { return d_func()->getTextFieldsCount(); }

std::size_t WinToastNotificationBuilder::getActionsCount() const { return d_func()->getActionsCount(); }

bool WinToastNotificationBuilder::hasImage() const { return d_func()->hasImage(); }

const std::vector<std::wstring> &WinToastNotificationBuilder::getTextFields() const {
    return d_func()->getTextFields();
}

const std::wstring &WinToastNotificationBuilder::getTextField(const TextField pos) const {
    return d_func()->getTextField(pos);
}

const std::wstring &WinToastNotificationBuilder::getActionLabel(const std::size_t pos) const {
    return d_func()->getActionLabel(pos);
}

const std::wstring &WinToastNotificationBuilder::getImagePath() const { return d_func()->getImagePath(); }

const std::wstring &WinToastNotificationBuilder::getAudioPath() const { return d_func()->getAudioPath(); }

const std::wstring &WinToastNotificationBuilder::getAttributionText() const { return d_func()->getAttributionText(); }

const std::wstring &WinToastLib::WinToastNotificationBuilder::getScenario() const { return d_func()->getScenario(); }

INT64 WinToastNotificationBuilder::getExpiration() const { return d_func()->getExpiration(); }

WinToastNotificationBuilder::WinToastTemplateType WinToastNotificationBuilder::getType() const {
    return d_func()->getType();
}

WinToastNotificationBuilder::AudioOption WinToastNotificationBuilder::getAudioOption() const {
    return d_func()->getAudioOption();
}

WinToastNotificationBuilder::Duration WinToastNotificationBuilder::getDuration() const {
    return d_func()->getDuration();
}
