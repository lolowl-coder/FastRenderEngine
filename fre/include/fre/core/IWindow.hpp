#pragma once

#include <stdint.h>

namespace fre
{
    class IWindow
    {
    public:
        struct Desc {
            uint32_t width = 1920;
            uint32_t height = 1080;
            const char* title = "App";
            bool external = false;
            void* externalHandle = nullptr;
        };

        virtual ~IWindow() = default;
        virtual void setPosition(const int width, const int height) = 0;
		virtual void onSizeChanged(const int width, const int height) = 0;
		virtual void onCustomMessage(const uint32_t messageId) = 0;
        virtual void pollEvents() = 0;
        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;
        virtual bool shouldClose() const = 0;
    };
}