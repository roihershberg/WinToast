#include <Windows.h>

#include <iostream>
#include <string>     // std::string, std::stoi

#include "wintoastlib.h"

using namespace WinToastLib;

enum Results {
    ToastClicked,                    // user clicked on the toast
    ToastNotActivated,                // toast was not activated
    ToastFailed,                    // toast failed
    SystemNotSupported,                // system does not support toasts
    UnhandledOption,                // unhandled option
    MultipleTextNotSupported,        // multiple texts were provided
    InitializationFailure,            // toast notification manager initialization failure
    ToastNotLaunched                // toast could not be launched
};


#define COMMAND_ACTION      L"--action"
#define COMMAND_AUMI        L"--aumi"
#define COMMAND_APPNAME     L"--appname"
#define COMMAND_APPID       L"--appid"
#define COMMAND_EXPIREMS    L"--expirems"
#define COMMAND_TEXT        L"--text"
#define COMMAND_HELP        L"--help"
#define COMMAND_IMAGE       L"--image"
#define COMMAND_SHORTCUT    L"--only-create-shortcut"
#define COMMAND_AUDIOSTATE  L"--audio-state"
#define COMMAND_ATTRIBUTE   L"--attribute"

void print_help() {
    std::wcout << "WinToast Console Example [OPTIONS]" << std::endl;
    std::wcout << "\t" << COMMAND_ACTION << L" : Set the actions in buttons" << std::endl;
    std::wcout << "\t" << COMMAND_AUMI << L" : Set the App User Model Id" << std::endl;
    std::wcout << "\t" << COMMAND_APPNAME << L" : Set the default appname" << std::endl;
    std::wcout << "\t" << COMMAND_APPID << L" : Set the App Id" << std::endl;
    std::wcout << "\t" << COMMAND_EXPIREMS << L" : Set the default expiration time" << std::endl;
    std::wcout << "\t" << COMMAND_TEXT << L" : Set the text for the notifications" << std::endl;
    std::wcout << "\t" << COMMAND_IMAGE << L" : set the image path" << std::endl;
    std::wcout << "\t" << COMMAND_ATTRIBUTE << L" : set the attribute for the notification" << std::endl;
    std::wcout << "\t" << COMMAND_SHORTCUT << L" : create the shortcut for the app" << std::endl;
    std::wcout << "\t" << COMMAND_AUDIOSTATE << L" : set the audio state: Default = 0, Silent = 1, Loop = 2"
               << std::endl;
    std::wcout << "\t" << COMMAND_HELP << L" : Print the help description" << std::endl;
}


int wmain(int argc, LPWSTR *argv) {
    if (argc == 1) {
        print_help();
        return 0;
    }

    if (!WinToast::isCompatible()) {
        std::wcerr << L"Error, your system in not supported!" << std::endl;
        return Results::SystemNotSupported;
    }

    std::wstring appName = L"Console WinToast Example",
            appUserModelID = L"WinToast Console Example",
            text,
            imagePath,
            attribute = L"default";
    std::vector<std::wstring> actions;
    INT64 expiration = 0;


    bool onlyCreateShortcut = false;
    WinToastNotificationBuilder::AudioOption audioOption = WinToastNotificationBuilder::AudioOption::Default;

    int i;
    for (i = 1; i < argc; i++)
        if (!wcscmp(COMMAND_IMAGE, argv[i]))
            imagePath = argv[++i];
        else if (!wcscmp(COMMAND_ACTION, argv[i]))
            actions.emplace_back(argv[++i]);
        else if (!wcscmp(COMMAND_EXPIREMS, argv[i]))
            expiration = wcstol(argv[++i], nullptr, 10);
        else if (!wcscmp(COMMAND_APPNAME, argv[i]))
            appName = argv[++i];
        else if (!wcscmp(COMMAND_AUMI, argv[i]) || !wcscmp(COMMAND_APPID, argv[i]))
            appUserModelID = argv[++i];
        else if (!wcscmp(COMMAND_TEXT, argv[i]))
            text = argv[++i];
        else if (!wcscmp(COMMAND_ATTRIBUTE, argv[i]))
            attribute = argv[++i];
        else if (!wcscmp(COMMAND_SHORTCUT, argv[i]))
            onlyCreateShortcut = true;
        else if (!wcscmp(COMMAND_AUDIOSTATE, argv[i]))
            audioOption = static_cast<WinToastNotificationBuilder::AudioOption>(std::stoi(argv[++i]));
        else if (!wcscmp(COMMAND_HELP, argv[i])) {
            print_help();
            return 0;
        } else {
            std::wcerr << L"Option not recognized: " << argv[i] << std::endl;
            return Results::UnhandledOption;
        }

    WinToast::setAppName(appName);
    WinToast::setAppUserModelId(appUserModelID);
    WinToast::setOnActivated(
            [](const WinToastArguments &arguments, const std::map<std::wstring, std::wstring> &userData) {
                if (arguments.contains(L"actionId")) {
                    std::wcout << L"The user clicked on action #" + arguments.get(L"actionId") << std::endl;
                    WinToast::uninstall();
                    exit(16);
                } else {
                    std::wcout << L"The user clicked on the toast" << std::endl;
                    WinToast::uninstall();
                    exit(0);
                }
            });

    if (onlyCreateShortcut) {
        if (!imagePath.empty() || !text.empty() || !actions.empty() || expiration) {
            std::wcerr << L"--only-create-shortcut does not accept images/text/actions/expiration" << std::endl;
            return 9;
        }
        WinToast::ShortcutResult result = WinToast::createShortcut();
        return (int) result ? 16 + (int) result : 0;
    }

    if (text.empty())
        text = L"Hello, world!";

    if (!WinToast::initialize()) {
        std::wcerr << L"Error, your system in not compatible!" << std::endl;
        return Results::InitializationFailure;
    }

    bool withImage = !(imagePath.empty());
    WinToastNotificationBuilder templ(withImage ? WinToastNotificationBuilder::WinToastTemplateType::ImageAndText02
                                                : WinToastNotificationBuilder::WinToastTemplateType::Text02);
    templ.setTextField(text, WinToastNotificationBuilder::TextField::FirstLine)
            .setAudioOption(audioOption)
            .setAttributionText(attribute);

    for (auto const &action: actions)
        templ.addAction(action);
    if (expiration)
        templ.setExpiration(expiration);
    if (withImage)
        templ.setImagePath(imagePath);

    if (WinToast::showToast(templ) < 0) {
        std::wcerr << L"Could not launch your toast notification!";
        return Results::ToastFailed;
    }

    // Give the handler a chance for 15 seconds (or the expiration plus 1 second)
    Sleep(expiration ? (DWORD) expiration + 1000 : 15000);

    WinToast::uninstall();

    return 2;
}
