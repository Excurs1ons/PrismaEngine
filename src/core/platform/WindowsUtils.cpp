#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(WIN64)
#include "WindowsUtils.h"
#include "Logger.h"
#include <Windows.h>
#include <iostream>

DirectoryWatcher::DirectoryWatcher() : m_running(false), m_hDir(INVALID_HANDLE_VALUE), m_stopEvent(NULL) {}

DirectoryWatcher::~DirectoryWatcher() {
    Stop();
}

bool DirectoryWatcher::Start(const std::wstring& directory, FileChangeCallback callback) {
    if (m_running)
        return false;

    m_directory = directory;
    m_callback  = callback;
    m_running   = true;

    // 创建手动重置事件
    m_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_stopEvent == nullptr) {
        return false;
}

    m_watchThread = std::thread(&DirectoryWatcher::WatchLoop, this);
    return true;
}

bool DirectoryWatcher::Start(const std::filesystem::path& directory, FileChangeCallback callback) {
    return Start(directory.wstring(), callback);
}

void DirectoryWatcher::Stop() {
    if (!m_running)
        return;

    m_running = false;
    if (m_stopEvent) {
        SetEvent((HANDLE)m_stopEvent);
    }

    if (m_watchThread.joinable()) {
        m_watchThread.join();
    }

    if (m_stopEvent) {
        CloseHandle((HANDLE)m_stopEvent);
        m_stopEvent = NULL;
    }
}

void DirectoryWatcher::WatchLoop() {
    m_hDir = CreateFileW(m_directory.c_str(),
                         FILE_LIST_DIRECTORY,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                         NULL);

    if (m_hDir == INVALID_HANDLE_VALUE) {
        LOG_ERROR("WindowsUtils", L"无法打开路径: {0}", m_directory);
        return;
    }

    char buffer[4096];
    OVERLAPPED overlapped = {0};
    overlapped.hEvent     = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!overlapped.hEvent) {
        CloseHandle((HANDLE)m_hDir);
        return;
    }

    HANDLE events[2] = {(HANDLE)m_stopEvent, overlapped.hEvent};

    while (m_running) {
        DWORD bytesReturned = 0;
        BOOL result         = ReadDirectoryChangesW((HANDLE)m_hDir,
                                            buffer,
                                            sizeof(buffer),
                                            TRUE,  // 递归子目录
                                            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                FILE_NOTIFY_CHANGE_SIZE,
                                            &bytesReturned,
                                            &overlapped,
                                            NULL);

        if (!result) {
            if (GetLastError() != ERROR_IO_PENDING) {
                LOG_ERROR("WindowsUtils", "ReadDirectoryChangesW 失败");
                break;
            }
        }

        DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0) {
            // 收到停止信号
            break;
        } else if (waitResult == WAIT_OBJECT_0 + 1) {
            // I/O 完成
            if (GetOverlappedResult((HANDLE)m_hDir, &overlapped, &bytesReturned, FALSE)) {
                if (bytesReturned == 0) {
                    // 这种情况可能发生吗？通常意味着溢出或错误
                } else {
                    FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer;
                    do {
                        std::wstring filename(info->FileName, info->FileName + info->FileNameLength / 2);
                        FileAction action = FileAction::Unknown;

                        switch (info->Action) {
                            case FILE_ACTION_MODIFIED:
                                action = FileAction::Modified;
                                break;
                            case FILE_ACTION_ADDED:
                                action = FileAction::Added;
                                break;
                            case FILE_ACTION_REMOVED:
                                action = FileAction::Removed;
                                break;
                            case FILE_ACTION_RENAMED_OLD_NAME:
                                action = FileAction::RenamedOldName;
                                break;
                            case FILE_ACTION_RENAMED_NEW_NAME:
                                action = FileAction::RenamedNewName;
                                break;
                        }

                        if (m_callback) {
                            LOG_DEBUG("WindowsUtils",
                                      "文件 {0} 发生了 {1} 事件",
                                      Logger::WStringToString(filename),
                                      static_cast<int>(action));
                            m_callback(filename, action);
                        }

                        if (info->NextEntryOffset == 0)
                            break;
                        info = (FILE_NOTIFY_INFORMATION*)((char*)info + info->NextEntryOffset);

                    } while (true);
                }

                // 重置事件以便下次使用
                ResetEvent(overlapped.hEvent);
            }
        } else {
            // 等待失败
            break;
        }
    }

    CancelIo((HANDLE)m_hDir);
    CloseHandle(overlapped.hEvent);
    CloseHandle((HANDLE)m_hDir);
    m_hDir = INVALID_HANDLE_VALUE;
}
#endif