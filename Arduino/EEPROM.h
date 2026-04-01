#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#if defined(INPUT)
#pragma push_macro("INPUT")
#undef INPUT
#define WOUOUI_RESTORE_INPUT_MACRO
#endif
#include <windows.h>
#if defined(WOUOUI_RESTORE_INPUT_MACRO)
#pragma pop_macro("INPUT")
#undef WOUOUI_RESTORE_INPUT_MACRO
#endif
#endif

class EEPROMClass
{
public:
    bool begin(std::size_t size)
    {
        size_ = size;
        data_.assign(size, 0);
        load_from_disk();
        return true;
    }

    bool commit()
    {
        if (data_.empty())
            return false;

        std::error_code ec;
        const auto path = storage_path();
        if (path.empty())
            return false;

        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path(), ec);

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;

        out.write(reinterpret_cast<const char *>(data_.data()), static_cast<std::streamsize>(data_.size()));
        out.flush();
        return out.good();
    }

    void write(int address, int value)
    {
        if (address < 0 || static_cast<std::size_t>(address) >= data_.size())
        {
            return;
        }
        data_[static_cast<std::size_t>(address)] = static_cast<std::uint8_t>(value);
    }

    int read(int address) const
    {
        if (address < 0 || static_cast<std::size_t>(address) >= data_.size())
        {
            return 0;
        }
        return data_[static_cast<std::size_t>(address)];
    }

    template <typename T>
    void get(int address, T &value) const
    {
        std::memset(&value, 0, sizeof(T));
        if (address < 0)
        {
            return;
        }

        const auto pos = static_cast<std::size_t>(address);
        if (pos >= data_.size())
        {
            return;
        }

        const auto available = data_.size() - pos;
        const auto toCopy = (available < sizeof(T)) ? available : sizeof(T);
        std::memcpy(&value, data_.data() + pos, toCopy);
    }

    template <typename T>
    void put(int address, const T &value)
    {
        if (address < 0)
        {
            return;
        }

        const auto pos = static_cast<std::size_t>(address);
        if (pos + sizeof(T) > data_.size()) {
            data_.resize(pos + sizeof(T), 0);
        }

        std::memcpy(data_.data() + pos, &value, sizeof(T));
    }

private:
    static std::filesystem::path storage_path()
    {
        if (const char *overridePath = std::getenv("WOUOUI_EEPROM_FILE"))
        {
            return std::filesystem::path(overridePath);
        }

#if defined(_WIN32)
        char modulePath[MAX_PATH] = {0};
        const DWORD len = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            return std::filesystem::path(modulePath).parent_path() / "WouoUI.eeprom.bin";
        }
#endif

        return std::filesystem::current_path() / "WouoUI.eeprom.bin";
    }

    void load_from_disk()
    {
        const auto path = storage_path();
        if (path.empty())
            return;

        std::ifstream in(path, std::ios::binary);
        if (!in)
            return;

        in.read(reinterpret_cast<char *>(data_.data()), static_cast<std::streamsize>(data_.size()));
        const auto bytesRead = static_cast<std::size_t>(in.gcount());
        if (bytesRead < data_.size())
        {
            std::fill(data_.begin() + static_cast<std::ptrdiff_t>(bytesRead), data_.end(), 0);
        }
    }

    std::size_t size_ = 0;
    std::vector<std::uint8_t> data_;
};

inline EEPROMClass EEPROM;
