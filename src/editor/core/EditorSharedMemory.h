#pragma once

#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Prisma {

/* 编辑器高性能共享内存通道 */
class EditorSharedMemory {
public:
    EditorSharedMemory(const std::string& name, size_t size) : m_name(name), m_size(size) {
#if defined(_WIN32)
        m_hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, name.c_str());
        m_buffer = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
#else
        m_shmFd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        ftruncate(m_shmFd, size);
        m_buffer = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
#endif
    }

    ~EditorSharedMemory() {
        if (!m_buffer) return;
#if defined(_WIN32)
        UnmapViewOfFile(m_buffer);
        CloseHandle(m_hMapFile);
#else
        munmap(m_buffer, m_size);
        shm_unlink(m_name.c_str());
#endif
    }

    void* GetBuffer() { return m_buffer; }
    size_t GetSize() const { return m_size; }

private:
    std::string m_name;
    size_t m_size;
    void* m_buffer = nullptr;
#if defined(_WIN32)
    HANDLE m_hMapFile;
#else
    int m_shmFd;
#endif
};

} // namespace Prisma
