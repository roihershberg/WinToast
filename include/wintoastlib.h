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

#ifndef WINTOASTLIB_H
#define WINTOASTLIB_H

#include <string>
#include <vector>
#include <map>
#include <functional>

#define TOAST_ACTIVATED_LAUNCH_ARG "-ToastActivated"

typedef signed __int64 INT64, *PINT64;

namespace WinToastLib {

    class WinToastArguments {
    public:
        WinToastArguments() = default;

        explicit WinToastArguments(const std::wstring &arguments);

        void parse(const std::wstring &arguments);

        [[nodiscard]] std::wstring toString() const;

        void add(const std::wstring &key, const std::wstring &value);

        bool remove(const std::wstring &key) noexcept;

        [[nodiscard]] std::wstring get(const std::wstring &key) const;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool contains(const std::wstring &key) const;

        [[nodiscard]] std::map<std::wstring, std::wstring>::size_type size() const noexcept;

        [[nodiscard]] std::map<std::wstring, std::wstring>::iterator begin() noexcept;

        [[nodiscard]] std::map<std::wstring, std::wstring>::const_iterator cbegin() const noexcept;

        [[nodiscard]] std::map<std::wstring, std::wstring>::iterator end() noexcept;

        [[nodiscard]] std::map<std::wstring, std::wstring>::const_iterator cend() const noexcept;

        std::wstring &operator[](const std::wstring &key);

    private:
        std::map<std::wstring, std::wstring> mPairs;
    };

    class WinToastNotification {
    public:
        explicit WinToastNotification(const std::wstring &xml);

        ~WinToastNotification();

        WinToastNotification(WinToastNotification &&o) noexcept;

        WinToastNotification &operator=(WinToastNotification &&o) noexcept;

        WinToastNotification(const WinToastNotification &o);

        WinToastNotification &operator=(const WinToastNotification &o);

        [[nodiscard]] std::wstring getXml() const;

        bool setXml(const std::wstring &xml);

        void show() const;

    private:
        class Impl;

        std::unique_ptr<Impl> _impl;

        [[nodiscard]] const Impl *d_func() const { return _impl.get(); }

        [[nodiscard]] Impl *d_func() { return _impl.get(); }
    };

    class WinToastNotificationBuilder {
    public:
        enum class Scenario {
            Default, Alarm, IncomingCall, Reminder
        };
        enum class Duration {
            System, Short, Long
        };
        enum class AudioOption {
            Default = 0, Silent, Loop
        };
        enum class TextField {
            FirstLine = 0, SecondLine, ThirdLine
        };
        enum class WinToastTemplateType {
            ImageAndText01,
            ImageAndText02,
            ImageAndText03,
            ImageAndText04,
            Text01,
            Text02,
            Text03,
            Text04,
        };

        enum class AudioSystemFile {
            DefaultSound,
            IM,
            Mail,
            Reminder,
            SMS,
            Alarm,
            Alarm2,
            Alarm3,
            Alarm4,
            Alarm5,
            Alarm6,
            Alarm7,
            Alarm8,
            Alarm9,
            Alarm10,
            Call,
            Call1,
            Call2,
            Call3,
            Call4,
            Call5,
            Call6,
            Call7,
            Call8,
            Call9,
            Call10,
        };


        explicit WinToastNotificationBuilder(WinToastTemplateType type = WinToastTemplateType::ImageAndText02);

        ~WinToastNotificationBuilder();

        WinToastNotificationBuilder(WinToastNotificationBuilder &&o) noexcept;

        WinToastNotificationBuilder &operator=(WinToastNotificationBuilder &&o) noexcept;

        WinToastNotificationBuilder(const WinToastNotificationBuilder &o);

        WinToastNotificationBuilder &operator=(const WinToastNotificationBuilder &o);

        WinToastNotificationBuilder &setFirstLine(const std::wstring &text);

        WinToastNotificationBuilder &setSecondLine(const std::wstring &text);

        WinToastNotificationBuilder &setThirdLine(const std::wstring &text);

        WinToastNotificationBuilder &setTextField(const std::wstring &text, TextField pos);

        WinToastNotificationBuilder &setAttributionText(const std::wstring &attributionText);

        WinToastNotificationBuilder &setImagePath(const std::wstring &imgPath);

        WinToastNotificationBuilder &setAudioPath(AudioSystemFile file);

        WinToastNotificationBuilder &setAudioPath(const std::wstring &audioPath);

        WinToastNotificationBuilder &setAudioOption(AudioOption audioOption);

        WinToastNotificationBuilder &setDuration(Duration duration);

        WinToastNotificationBuilder &setExpiration(INT64 millisecondsFromNow);

        WinToastNotificationBuilder &setScenario(Scenario scenario);

        WinToastNotificationBuilder &addAction(const std::wstring &label);

        [[nodiscard]] std::size_t getTextFieldsCount() const;

        [[nodiscard]] std::size_t getActionsCount() const;

        [[nodiscard]] bool hasImage() const;

        [[nodiscard]] const std::vector<std::wstring> &getTextFields() const;

        [[nodiscard]] const std::wstring &getTextField(TextField pos) const;

        [[nodiscard]] const std::wstring &getActionLabel(std::size_t pos) const;

        [[nodiscard]] const std::wstring &getImagePath() const;

        [[nodiscard]] const std::wstring &getAudioPath() const;

        [[nodiscard]] const std::wstring &getAttributionText() const;

        [[nodiscard]] const std::wstring &getScenario() const;

        [[nodiscard]] INT64 getExpiration() const;

        [[nodiscard]] WinToastTemplateType getType() const;

        [[nodiscard]] AudioOption getAudioOption() const;

        [[nodiscard]] Duration getDuration() const;

    private:
        class Impl;

        std::unique_ptr<Impl> _impl;

        [[nodiscard]] const Impl *d_func() const { return _impl.get(); }

        [[nodiscard]] Impl *d_func() { return _impl.get(); }
    };

    namespace WinToast {
        enum class WinToastError {
            NoError = 0,
            NotInitialized,
            SystemNotSupported,
            ApartmentInitError,
            ShellLinkNotCreated,
            InvalidAppUserModelID,
            InvalidParameters,
            InvalidHandler,
            NotDisplayed,
            UnknownError
        };

        enum class ShortcutResult {
            SHORTCUT_UNCHANGED = 0,
            SHORTCUT_WAS_CHANGED = 1,
            SHORTCUT_WAS_CREATED = 2,

            SHORTCUT_MISSING_PARAMETERS = -1,
            SHORTCUT_INCOMPATIBLE_OS = -2,
            SHORTCUT_CREATE_FAILED = -3
        };

        enum class ShortcutPolicy {
            /* Don't check, create, or modify a shortcut. */
            SHORTCUT_POLICY_IGNORE = 0,
            /* Require a shortcut with matching AUMI, don't create or modify an existing one. */
            SHORTCUT_POLICY_REQUIRE_NO_CREATE = 1,
            /* Require a shortcut with matching AUMI, create if missing, modify if not matching.
             * This is the default. */
            SHORTCUT_POLICY_REQUIRE_CREATE = 2,
        };

        [[nodiscard]] bool isCompatible();

        [[nodiscard]] bool isSupportingModernFeatures();

        [[nodiscard]] std::wstring configureAUMI(const std::wstring &companyName,
                                                 const std::wstring &productName,
                                                 const std::wstring &subProduct = std::wstring(),
                                                 const std::wstring &versionInformation = std::wstring());

        [[nodiscard]] const std::wstring &strerror(WinToastError error);

        bool initialize(WinToastError *error = nullptr);

        void uninstall();

        [[nodiscard]] bool isInitialized();

        bool hideToast(INT64 id);

        INT64
        showToast(const WinToastNotificationBuilder &toast, WinToastError *error = nullptr);

        void clear();

        ShortcutResult createShortcut();

        [[nodiscard]] const std::wstring &appName();

        [[nodiscard]] const std::wstring &appUserModelId();

        [[nodiscard]] const std::wstring &iconPath();

        [[nodiscard]] const std::wstring &iconBackgroundColor();

        void setAppUserModelId(const std::wstring &aumi);

        void setAppName(const std::wstring &appName);

        void setIconPath(const std::wstring &iconPath);

        void setIconBackgroundColor(const std::wstring &iconBackgroundColor);

        void setShortcutPolicy(ShortcutPolicy policy);

        void setOnActivated(
                const std::function<void(const WinToastArguments &,
                                         const std::map<std::wstring, std::wstring> &)> &callback);
    }
}

#endif // WINTOASTLIB_H
