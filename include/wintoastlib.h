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
#include <memory>

typedef signed __int64 INT64, *PINT64;

namespace WinToastLib {

    class WinToastImpl;

    class IWinToastHandler {
    public:
        enum class WinToastDismissalReason {
            UserCanceled,
            ApplicationHidden,
            TimedOut,
        };

        virtual ~IWinToastHandler() = default;

        virtual void toastActivated() const = 0;

        virtual void toastActivated(int actionIndex) const = 0;

        virtual void toastDismissed(WinToastDismissalReason state) const = 0;

        virtual void toastFailed() const = 0;
    };

    class WinToastTemplate {
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


        WinToastTemplate(WinToastTemplateType type = WinToastTemplateType::ImageAndText02);

        ~WinToastTemplate();

        void setFirstLine(const std::wstring &text);

        void setSecondLine(const std::wstring &text);

        void setThirdLine(const std::wstring &text);

        void setTextField(const std::wstring &txt, TextField pos);

        void setAttributionText(const std::wstring &attributionText);

        void setImagePath(const std::wstring &imgPath);

        void setAudioPath(WinToastTemplate::AudioSystemFile audio);

        void setAudioPath(const std::wstring &audioPath);

        void setAudioOption(WinToastTemplate::AudioOption audioOption);

        void setDuration(Duration duration);

        void setExpiration(INT64 millisecondsFromNow);

        void setScenario(Scenario scenario);

        void addAction(const std::wstring &label);

        std::size_t textFieldsCount() const;

        std::size_t actionsCount() const;

        bool hasImage() const;

        const std::vector<std::wstring> &textFields() const;

        const std::wstring &textField(TextField pos) const;

        const std::wstring &actionLabel(std::size_t pos) const;

        const std::wstring &imagePath() const;

        const std::wstring &audioPath() const;

        const std::wstring &attributionText() const;

        const std::wstring &scenario() const;

        INT64 expiration() const;

        WinToastTemplateType type() const;

        WinToastTemplate::AudioOption audioOption() const;

        Duration duration() const;

    private:
        std::vector<std::wstring> _textFields{};
        std::vector<std::wstring> _actions{};
        std::wstring _imagePath{};
        std::wstring _audioPath{};
        std::wstring _attributionText{};
        std::wstring _scenario{L"Default"};
        INT64 _expiration{0};
        AudioOption _audioOption{WinToastTemplate::AudioOption::Default};
        WinToastTemplateType _type{WinToastTemplateType::Text01};
        Duration _duration{Duration::System};
    };

    class WinToast {
    public:
        enum class WinToastError {
            NoError = 0,
            NotInitialized,
            SystemNotSupported,
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
            SHORTCUT_COM_INIT_FAILURE = -3,
            SHORTCUT_CREATE_FAILED = -4
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

        static WinToast &instance();

        static bool isCompatible();

        static bool isSupportingModernFeatures();

        static std::wstring configureAUMI(const std::wstring &companyName,
                                          const std::wstring &productName,
                                          const std::wstring &subProduct = std::wstring(),
                                          const std::wstring &versionInformation = std::wstring());

        static const std::wstring &strerror(WinToastError error);

        bool initialize(WinToastError *error = nullptr);

        bool isInitialized() const;

        bool hideToast(INT64 id);

        INT64
        showToast(const WinToastTemplate &toast, IWinToastHandler *handler, WinToastError *error = nullptr);

        void clear();

        ShortcutResult createShortcut();

        const std::wstring &appName() const;

        const std::wstring &appUserModelId() const;

        void setAppUserModelId(const std::wstring &aumi);

        void setAppName(const std::wstring &appName);

        void setShortcutPolicy(ShortcutPolicy policy);

    private:
        static WinToastImpl &winToastImpl;
    };
}

#endif // WINTOASTLIB_H
