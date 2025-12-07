//#pragma once
//#include "IPlatform.h"
//namespace Engine {
//    namespace Platform {
//
//
//        class PlatformAndroid :public IPlatform {
//        public:
//            virtual ~PlatformAndroid() = default;
//
//            // ------------------------------------------------------------
//            // ------------------------------------------------------------
//            virtual bool InitPlatform() override;
//            virtual void ShutdownPlatform() override;
//
//            // ------------------------------------------------------------
//            // ------------------------------------------------------------
//            virtual WindowHandle CreateWindow(const WindowDesc& desc) override;
//            virtual void DestroyWindow(WindowHandle window) override;
//
//            virtual void GetWindowSize(WindowHandle window, int& outW, int& outH) override;
//            virtual void SetWindowTitle(WindowHandle window, const char* title) override;
//
//            virtual void PumpEvents() override;
//            virtual bool ShouldClose(WindowHandle window) const override;
//
//            // ------------------------------------------------------------
//            // ------------------------------------------------------------
//            virtual uint64_t GetTimeMicroseconds() const override;
//            virtual double    GetTimeSeconds() const override;
//
//            // ------------------------------------------------------------
//            // ------------------------------------------------------------
//            virtual bool IsKeyDown(KeyCode key) const override;
//            virtual bool IsMouseButtonDown(MouseButton btn) const override;
//            virtual void GetMousePosition(float& x, float& y) const override;
//            virtual void SetMousePosition(float x, float y) override;
//            virtual void SetMouseLock(bool locked) override;
//
//            // ------------------------------------------------------------
//            // ------------------------------------------------------------
//            virtual bool   FileExists(const char* path) const override;
//            virtual size_t FileSize(const char* path) const override;
//            virtual size_t ReadFile(const char* path, void* dst, size_t maxBytes) const override;
//
//            virtual const char* GetExecutablePath() const override;
//            virtual const char* GetPersistentPath() const override;
//            virtual const char* GetTemporaryPath() const override;
//
//            // ------------------------------------------------------------
//            // ------------------------------------------------------------
//            virtual PlatformThreadHandle CreateThread(ThreadFunc entry, void* userData) override;
//            virtual void JoinThread(PlatformThreadHandle thread) override;
//
//            virtual PlatformMutexHandle CreateMutex() override;
//            virtual void DestroyMutex(PlatformMutexHandle mtx) override;
//            virtual void LockMutex(PlatformMutexHandle mtx) override;
//            virtual void UnlockMutex(PlatformMutexHandle mtx) override;
//
//            virtual void SleepMilliseconds(uint32_t ms) override;
//        };
//    }
//}
