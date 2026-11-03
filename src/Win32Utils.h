#pragma once

#include <Windows.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace win32
{
    inline std::string HResultMessage(HRESULT result)
    {
        wchar_t* wideMessage = nullptr;
        const DWORD flags =
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS;

        const DWORD length = FormatMessageW(
            flags,
            nullptr,
            static_cast<DWORD>(result),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<wchar_t*>(&wideMessage),
            0,
            nullptr);

        std::string message;
        if (length != 0 && wideMessage != nullptr)
        {
            const int utf8Length = WideCharToMultiByte(
                CP_UTF8,
                0,
                wideMessage,
                static_cast<int>(length),
                nullptr,
                0,
                nullptr,
                nullptr);

            if (utf8Length > 0)
            {
                message.resize(static_cast<std::size_t>(utf8Length));
                WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    wideMessage,
                    static_cast<int>(length),
                    message.data(),
                    utf8Length,
                    nullptr,
                    nullptr);
            }

            LocalFree(wideMessage);
        }

        while (!message.empty() &&
               (message.back() == '\r' || message.back() == '\n'))
        {
            message.pop_back();
        }

        return message;
    }

    [[noreturn]] inline void ThrowHResult(
        HRESULT result,
        std::string_view operation)
    {
        std::ostringstream stream;
        stream << operation << " failed with HRESULT 0x"
               << std::uppercase << std::hex << std::setw(8)
               << std::setfill('0')
               << static_cast<unsigned long>(result);

        const std::string systemMessage = HResultMessage(result);
        if (!systemMessage.empty())
        {
            stream << ": " << systemMessage;
        }

        throw std::runtime_error(stream.str());
    }

    inline void ThrowIfFailed(
        HRESULT result,
        std::string_view operation)
    {
        if (FAILED(result))
        {
            ThrowHResult(result, operation);
        }
    }

    [[noreturn]] inline void ThrowLastError(std::string_view operation)
    {
        ThrowHResult(
            HRESULT_FROM_WIN32(GetLastError()),
            operation);
    }
}
