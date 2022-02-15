//
// Created by roihe on 2/13/2022.
//

#ifndef WINTOAST_MACROS_H
#define WINTOAST_MACROS_H

#ifdef NDEBUG
#define DEBUG_MSG(str)
#define DEBUG_ERR(str)
#else
#define DEBUG_MSG(str) std::wcout << str << std::endl
#define DEBUG_ERR(str) std::wcerr << str << std::endl
#endif

#define catchAndLogHresult_2(execute, logPrefix) \
try {                                                     \
    execute                                               \
} catch (winrt::hresult_error const &ex) {                \
    DEBUG_ERR(logPrefix << ex.message().c_str());         \
}
#define catchAndLogHresult_3(execute, logPrefix, onError) \
try {                                                     \
    execute                                               \
} catch (winrt::hresult_error const &ex) {                \
    DEBUG_ERR(logPrefix << ex.message().c_str());         \
    onError                                               \
}

#define FUNC_CHOOSER(_f1, _f2, _f3, _f4, ...) _f4
#define FUNC_RECOMPOSER(argsWithParentheses) FUNC_CHOOSER argsWithParentheses
#define CHOOSE_FROM_ARG_COUNT(...) FUNC_RECOMPOSER((__VA_ARGS__, catchAndLogHresult_3, catchAndLogHresult_2, MISSING_PARAMETERS, MISSING_PARAMETERS))
#define MACRO_CHOOSER(...) CHOOSE_FROM_ARG_COUNT(__VA_ARGS__)
#define catchAndLogHresult(...) MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#endif //WINTOAST_MACROS_H
