#pragma once

#include <Arduino.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#ifndef FILE_READ
#define FILE_READ "r"
#endif

#ifndef FILE_WRITE
#define FILE_WRITE "w"
#endif

namespace emulator_fs
{
namespace stdfs = std::filesystem;

static inline stdfs::path littlefs_root_path()
{
    static bool resolved = false;
    static stdfs::path cached;

    if (resolved)
        return cached;

    resolved = true;

    if (const char *overrideDir = std::getenv("WOUOUI_LITTLEFS_DIR"))
    {
        stdfs::path candidate = stdfs::path(overrideDir);
        if (stdfs::exists(candidate))
        {
            cached = candidate;
            return cached;
        }
    }

    const std::vector<stdfs::path> candidates = {
        stdfs::path("data") / "data-seeed_xiao_esp32s3",
        stdfs::path("..") / "data" / "data-seeed_xiao_esp32s3",
        stdfs::path("..") / ".." / "data" / "data-seeed_xiao_esp32s3",
        stdfs::path("..") / ".." / ".." / "data" / "data-seeed_xiao_esp32s3",
        stdfs::path("..") / ".." / ".." / ".." / "data" / "data-seeed_xiao_esp32s3",
    };

    for (const auto &candidate : candidates)
    {
        std::error_code ec;
        if (stdfs::exists(candidate, ec))
        {
            cached = stdfs::weakly_canonical(candidate, ec);
            if (!ec)
                return cached;
            cached = candidate;
            return cached;
        }
    }

    cached.clear();
    return cached;
}

static inline stdfs::path normalize_fs_path(const char *path)
{
    if (!path || !path[0] || std::string(path) == "/")
        return littlefs_root_path();

    std::string rel(path);
    std::replace(rel.begin(), rel.end(), '\\', '/');
    while (!rel.empty() && rel.front() == '/')
        rel.erase(rel.begin());

    stdfs::path root = littlefs_root_path();
    if (root.empty())
        return {};

    return root / stdfs::path(rel);
}

static inline std::string display_name_for(const stdfs::path &path)
{
    std::error_code ec;
    const stdfs::path root = littlefs_root_path();
    if (root.empty())
        return path.generic_string();

    stdfs::path rel = stdfs::relative(path, root, ec);
    if (ec)
        rel = path.filename();

    std::string out = "/" + rel.generic_string();
    if (out == "/.")
        out = "/";
    return out;
}
} // namespace emulator_fs

class File
{
public:
    File() = default;

    explicit File(const emulator_fs::stdfs::path &path, const char *mode = FILE_READ)
    {
        open(path, mode);
    }

    explicit operator bool() const
    {
        if (isDirectory_)
            return !dirPath_.empty();
        return stream_ && stream_->is_open();
    }

    bool isDirectory() const
    {
        return isDirectory_;
    }

    int read()
    {
        uint8_t value = 0;
        return read(&value, 1) == 1 ? value : -1;
    }

    size_t read(uint8_t *buffer, size_t len)
    {
        if (!buffer || len == 0 || !stream_ || !stream_->is_open() || isDirectory_)
            return 0;

        stream_->read(reinterpret_cast<char *>(buffer), static_cast<std::streamsize>(len));
        const size_t got = static_cast<size_t>(stream_->gcount());
        readPos_ += got;
        return got;
    }

    size_t readBytesUntil(char terminator, char *buffer, size_t len)
    {
        if (!buffer || len == 0)
            return 0;

        size_t n = 0;
        while (n < len)
        {
            const int ch = read();
            if (ch < 0)
                break;
            if (ch == terminator)
                break;
            buffer[n++] = static_cast<char>(ch);
        }
        return n;
    }

    size_t write(const uint8_t *buffer, size_t len)
    {
        if (!buffer || len == 0 || !stream_ || !stream_->is_open() || isDirectory_)
            return 0;

        stream_->write(reinterpret_cast<const char *>(buffer), static_cast<std::streamsize>(len));
        if (!stream_->good())
            return 0;

        std::streampos pos = stream_->tellp();
        if (pos >= 0)
            size_ = std::max<size_t>(size_, static_cast<size_t>(pos));
        return len;
    }

    size_t write(uint8_t value)
    {
        return write(&value, 1);
    }

    bool seek(uint32_t pos)
    {
        if (!stream_ || !stream_->is_open() || isDirectory_)
            return false;
        if (pos > size_)
            return false;

        stream_->clear();
        stream_->seekg(static_cast<std::streamoff>(pos), std::ios::beg);
        stream_->seekp(static_cast<std::streamoff>(pos), std::ios::beg);
        if (!stream_->good())
            return false;
        readPos_ = pos;
        return true;
    }

    size_t size() const
    {
        return size_;
    }

    int available() const
    {
        if (isDirectory_ || !stream_ || !stream_->is_open())
            return 0;
        return (readPos_ < size_) ? static_cast<int>(size_ - readPos_) : 0;
    }

    const char *name() const
    {
        return displayName_.c_str();
    }

    File openNextFile()
    {
        if (!isDirectory_)
            return File();

        while (dirIndex_ < dirEntries_.size())
        {
            const auto path = dirEntries_[dirIndex_++];
            std::error_code ec;
            if (!emulator_fs::stdfs::exists(path, ec) || ec)
                continue;

            if (emulator_fs::stdfs::is_directory(path, ec))
            {
                File child;
                child.openDirectory(path);
                return child;
            }

            return File(path, FILE_READ);
        }
        return File();
    }

    void close()
    {
        if (stream_ && stream_->is_open())
            stream_->close();
        stream_.reset();
        isDirectory_ = false;
        size_ = 0;
        readPos_ = 0;
        dirEntries_.clear();
        dirIndex_ = 0;
        dirPath_.clear();
        displayName_.clear();
    }

private:
    void open(const emulator_fs::stdfs::path &path, const char *mode)
    {
        close();

        const std::string modeStr = mode ? mode : FILE_READ;
        const bool wantsWrite = modeStr.find('w') != std::string::npos || modeStr.find('a') != std::string::npos || modeStr.find('+') != std::string::npos;

        std::error_code ec;
        if (!emulator_fs::stdfs::exists(path, ec) || ec)
        {
            if (!wantsWrite)
                return;
            if (const auto parent = path.parent_path(); !parent.empty())
                emulator_fs::stdfs::create_directories(parent, ec);
        }

        if (emulator_fs::stdfs::exists(path, ec) && !ec && emulator_fs::stdfs::is_directory(path, ec))
        {
            openDirectory(path);
            return;
        }

        std::ios::openmode flags = std::ios::binary;
        const bool wantsRead = modeStr.find('r') != std::string::npos || modeStr.find('+') != std::string::npos;

        if (wantsRead)
            flags |= std::ios::in;
        if (wantsWrite)
            flags |= std::ios::out;
        if (modeStr.find('w') != std::string::npos)
            flags |= std::ios::trunc;
        if (modeStr.find('a') != std::string::npos)
            flags |= std::ios::app;

        auto stream = std::make_shared<std::fstream>(path, flags);
        if (!stream->is_open())
            return;

        stream_ = stream;
        filePath_ = path;
        displayName_ = emulator_fs::display_name_for(path);

        const auto size64 = emulator_fs::stdfs::file_size(path, ec);
        size_ = ec ? 0u : static_cast<size_t>(size64);
        readPos_ = 0;
        if (!wantsRead && wantsWrite)
            readPos_ = size_;
    }

    void openDirectory(const emulator_fs::stdfs::path &path)
    {
        close();
        isDirectory_ = true;
        dirPath_ = path;
        displayName_ = emulator_fs::display_name_for(path);

        std::error_code ec;
        for (const auto &entry : emulator_fs::stdfs::directory_iterator(path, ec))
        {
            if (ec)
                break;
            dirEntries_.push_back(entry.path());
        }

        std::sort(dirEntries_.begin(), dirEntries_.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.filename().generic_string() < rhs.filename().generic_string();
        });
    }

    std::shared_ptr<std::fstream> stream_;
    emulator_fs::stdfs::path filePath_;
    emulator_fs::stdfs::path dirPath_;
    std::vector<emulator_fs::stdfs::path> dirEntries_;
    std::string displayName_;
    size_t size_ = 0;
    size_t readPos_ = 0;
    size_t dirIndex_ = 0;
    bool isDirectory_ = false;
};

namespace fs
{
class FS
{
public:
    bool begin(bool formatOnFail = false)
    {
        (void)formatOnFail;
        return !emulator_fs::littlefs_root_path().empty();
    }

    void end()
    {
    }

    bool exists(const char *path) const
    {
        const auto hostPath = emulator_fs::normalize_fs_path(path);
        if (hostPath.empty())
            return false;

        std::error_code ec;
        return emulator_fs::stdfs::exists(hostPath, ec) && !ec;
    }

    bool remove(const char *path) const
    {
        const auto hostPath = emulator_fs::normalize_fs_path(path);
        if (hostPath.empty())
            return false;

        std::error_code ec;
        return emulator_fs::stdfs::remove(hostPath, ec) && !ec;
    }

    File open(const char *path, const char *mode = FILE_READ) const
    {
        const auto hostPath = emulator_fs::normalize_fs_path(path);
        if (hostPath.empty())
            return File();
        return File(hostPath, mode);
    }
};
} // namespace fs
