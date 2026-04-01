#pragma once

#include <Arduino.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "LittleFS.h"
#include "secure_pak.h"
#include "splash.h"

extern UiU8g2 u8g2;
extern bool pageDirty;

#ifndef CFG_FS_GIF_DIR
#define CFG_FS_GIF_DIR "/bin"
#endif

#ifndef CFG_FS_PLAYLIST_FILE
#define CFG_FS_PLAYLIST_FILE CFG_FS_GIF_DIR "/gif_playlist.txt"
#endif

#ifndef CFG_FS_GIF_SETTINGS_FILE
#define CFG_FS_GIF_SETTINGS_FILE CFG_FS_GIF_DIR "/setting_gif.json"
#endif

struct SdGifEntry
{
    const char *path = "";
    bool ok = false;
};

struct BootBinMeta
{
    uint32_t dataOffset = 0;
    uint16_t frameSize = 0;
    uint16_t w = 128;
    uint16_t h = 64;
    uint16_t frameCount = 0;
    uint16_t defaultDelay = 40;
    bool hasDelayField = false;
};

struct BinMeta
{
    uint16_t w = 0;
    uint16_t h = 0;
    uint16_t frameCount = 0;
    uint16_t defaultDelay = 40;
    uint16_t frameSize = 0;
    uint32_t dataOffset = 0;
    bool hasDelayField = false;
};

struct FsFile
{
    bool isSecure = false;
    File f;
    SecurePakFile sp;
};

struct FsGifMeta
{
    char path[64];
    uint16_t w = 0;
    uint16_t h = 0;
    uint16_t frameCount = 0;
    uint16_t defaultDelay = 40;
    uint16_t frameSize = 0;
    uint32_t dataOffset = 0;
    uint16_t minDelay = 0;
    bool hasDelayField = false;
    bool ok = false;
};

struct FsMinDelayEntry
{
    char bin[64];
    uint16_t minDelay = 0;
    bool ok = false;
};

bool fsMounted = false;
bool sdMounted = false;
bool sdPresent = false;
bool g_fwUiDirty = false;
bool g_isPlaying = false;

static constexpr int16_t SD_ICON_X_TARGET = 116;
static constexpr int16_t SD_ICON_X_FROM_RIGHT = 138;

File sdCurFile;
uint16_t sdCurFrameCount = 0;
uint16_t sdCurW = 128;
uint16_t sdCurH = 64;
const uint8_t *sdFrameBuf = splash_f0000;

uint8_t gifSource = SRC_FLASH;
uint8_t animIdx = 0;
uint16_t frameIdx = 0;
uint32_t nextFrameMs = 0;

uint16_t fsCurW = 128;
uint16_t fsCurH = 64;
uint8_t fsFrameBuf[BUF_MAX] = {0};

uint16_t sdCurAnimIdx = 0;
uint8_t sdGifCount = 0;
SdGifEntry sdGifs[1] = {};

FsGifMeta fsGifs[GIF_MAX];
uint8_t PLAY_N_FLASH = 0;

FsFile fsCurFile;
uint16_t fsCurFrameCount = 0;
uint16_t fsCurFrameSize = BUF_MAX;
uint16_t fsCurDefaultDelay = 40;
uint16_t fsCurMinDelay = 0;
bool fsCurHasDelay = false;
uint8_t fsCurAnimIdx = 0;

FsMinDelayEntry fsMinDelayMap[GIF_MAX];
uint8_t fsMinDelayN = 0;
bool fsMinDelayLoaded = false;

bool fsScanDone = false;
bool fsScanStarted = false;

uint8_t gifOrder[GIF_MAX];
uint8_t gifOrderPos = 0;
uint8_t gifOrderN = 0;
bool gifOrderValid = false;

MENU setting_menu_gif[] = {
    {"[ GIF Setting ]"},
    {"+ GIF Shuffle"},
    {"+ GIF Debug"},
    {"~ GIF Delay"},
    {"[ GIF Playlist ]"},
};
const uint8_t SET_GIF_N = sizeof(setting_menu_gif) / sizeof(setting_menu_gif[0]);

MENU setting_menu_gifplays[GIFPLAYS_BASE + GIF_MAX];
char setting_gifplays_label[GIF_MAX][28];
uint8_t playsMenuSource = SRC_FLASH;
uint8_t playsMenuCount = 0;
bool playsRefreshPending = false;
bool gifplaysSwitchAnim = false;

static inline const char *sd_basename(const char *path)
{
    if (!path)
        return "";
    const char *slash = std::strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static inline void sd_strip_ext(char *text)
{
    if (!text)
        return;
    char *dot = std::strrchr(text, '.');
    if (dot)
        *dot = 0;
}

static inline void trim_line(char *text)
{
    if (!text)
        return;

    for (size_t i = 0; text[i]; ++i)
    {
        if (text[i] == '#')
        {
            text[i] = 0;
            break;
        }
        if (text[i] == '/' && text[i + 1] == '/')
        {
            text[i] = 0;
            break;
        }
    }

    int n = static_cast<int>(std::strlen(text));
    while (n > 0)
    {
        const char c = text[n - 1];
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
        {
            text[n - 1] = 0;
            --n;
        }
        else
        {
            break;
        }
    }

    int start = 0;
    while (text[start] == ' ' || text[start] == '\t')
        ++start;
    if (start > 0)
        std::memmove(text, text + start, std::strlen(text + start) + 1);
}

static inline bool make_fullpath(char *out, size_t outSize, const char *baseDir, const char *line)
{
    if (!out || outSize < 4 || !line || !line[0])
        return false;

    out[0] = 0;
    while (*line == ' ' || *line == '\t')
        ++line;

    if (line[0] == '/')
    {
        std::snprintf(out, outSize, "%s", line);
        return true;
    }

    if (std::strncmp(line, "bin/", 4) == 0 || std::strncmp(line, "BIN/", 4) == 0)
    {
        std::snprintf(out, outSize, "/%s", line);
        return true;
    }

    std::snprintf(out, outSize, "%s/%s", baseDir, line);
    return true;
}

static inline uint16_t rd16(File &f)
{
    uint8_t b[2] = {0, 0};
    return f.read(b, 2) == 2 ? static_cast<uint16_t>(b[0] | (b[1] << 8)) : 0;
}

static inline bool parse_bin_header_common(size_t fileSize, const uint8_t hdr[18], BinMeta &meta)
{
    if (std::memcmp(hdr, "U8G2GIFB", 8) == 0)
    {
        const uint8_t ver = hdr[8];
        const uint8_t flags = hdr[9];
        if (ver != 1)
            return false;

        const uint16_t w = static_cast<uint16_t>(hdr[10] | (hdr[11] << 8));
        const uint16_t h = static_cast<uint16_t>(hdr[12] | (hdr[13] << 8));
        const uint16_t fc = static_cast<uint16_t>(hdr[14] | (hdr[15] << 8));
        const uint16_t defd = static_cast<uint16_t>(hdr[16] | (hdr[17] << 8));
        const uint16_t frameSize = static_cast<uint16_t>(((w + 7) / 8) * h);

        if (frameSize == 0 || frameSize > BUF_MAX || fc == 0)
            return false;

        meta.w = w;
        meta.h = h;
        meta.frameCount = fc;
        meta.defaultDelay = defd ? defd : 40;
        meta.frameSize = frameSize;
        meta.dataOffset = 18;
        meta.hasDelayField = (flags & 0x01) != 0;
        return true;
    }

    if (fileSize > 0 && (fileSize % 1024u) == 0)
    {
        size_t frames = fileSize / 1024u;
        if (frames > 65535u)
            frames = 65535u;

        meta.w = 128;
        meta.h = 64;
        meta.frameSize = 1024;
        meta.frameCount = static_cast<uint16_t>(frames);
        meta.defaultDelay = 40;
        meta.dataOffset = 0;
        meta.hasDelayField = false;
        return true;
    }

    return false;
}

static inline bool parse_boot_bin_header(File &f, BootBinMeta &meta)
{
    const size_t sz = f.size();
    if (sz < 18 || !f.seek(0))
        return false;

    uint8_t hdr[18];
    if (f.read(hdr, sizeof(hdr)) != sizeof(hdr))
        return false;

    BinMeta m;
    if (!parse_bin_header_common(sz, hdr, m))
        return false;

    meta.dataOffset = m.dataOffset;
    meta.frameSize = m.frameSize;
    meta.w = m.w;
    meta.h = m.h;
    meta.frameCount = m.frameCount;
    meta.defaultDelay = m.defaultDelay;
    meta.hasDelayField = m.hasDelayField;
    return true;
}

static inline void fsfile_close(FsFile &ff)
{
    if (ff.isSecure)
        securepak_file_close(ff.sp);
    else if (ff.f)
        ff.f.close();
    ff.isSecure = false;
}

static inline bool fsfile_is_open(const FsFile &ff)
{
    return ff.isSecure ? ff.sp.open : static_cast<bool>(ff.f);
}

static inline bool fsfile_open(FsFile &ff, const char *path)
{
    fsfile_close(ff);

    if (securepak_is_mounted() && securepak_find(path))
    {
        ff.isSecure = true;
        return securepak_file_open(ff.sp, path);
    }

    ff.f = LittleFS.open(path, FILE_READ);
    ff.isSecure = false;
    return static_cast<bool>(ff.f);
}

static inline bool fsfile_seek(FsFile &ff, uint32_t pos)
{
    return ff.isSecure ? securepak_file_seek(ff.sp, pos) : ff.f.seek(pos);
}

static inline size_t fsfile_read(FsFile &ff, uint8_t *buf, size_t len)
{
    return ff.isSecure ? securepak_file_read(ff.sp, buf, len) : ff.f.read(buf, len);
}

static inline bool fsfile_available(FsFile &ff)
{
    return ff.isSecure ? securepak_file_available(ff.sp) : ff.f.available();
}

static inline uint32_t fsfile_size(FsFile &ff)
{
    return ff.isSecure ? securepak_file_size(ff.sp) : static_cast<uint32_t>(ff.f.size());
}

static inline size_t fsfile_read_line(FsFile &ff, char *buf, size_t bufSize)
{
    if (!buf || bufSize == 0)
        return 0;

    size_t n = 0;
    while (fsfile_available(ff) && n < bufSize - 1)
    {
        uint8_t c = 0;
        if (fsfile_read(ff, &c, 1) != 1)
            break;
        if (c == '\r')
            continue;
        if (c == '\n')
            break;
        buf[n++] = static_cast<char>(c);
    }
    buf[n] = 0;
    return n;
}

static inline bool read_u16_fs(FsFile &f, uint16_t &out)
{
    uint8_t b[2];
    if (fsfile_read(f, b, 2) != 2)
        return false;
    out = static_cast<uint16_t>(b[0] | (b[1] << 8));
    return true;
}

static inline bool parse_bin_header_fs(FsFile &f, BinMeta &meta)
{
    if (fsfile_size(f) < 18 || !fsfile_seek(f, 0))
        return false;

    uint8_t hdr[18];
    if (fsfile_read(f, hdr, sizeof(hdr)) != sizeof(hdr))
        return false;

    return parse_bin_header_common(fsfile_size(f), hdr, meta);
}

static inline void fs_min_delay_clear()
{
    fsMinDelayN = 0;
    fsMinDelayLoaded = false;
    for (auto &entry : fsMinDelayMap)
    {
        entry.bin[0] = 0;
        entry.minDelay = 0;
        entry.ok = false;
    }
}

static inline void fs_min_delay_add(const char *binName, uint16_t minDelay)
{
    if (!binName || !binName[0] || fsMinDelayN >= GIF_MAX)
        return;

    char norm[64];
    std::snprintf(norm, sizeof(norm), "%s", sd_basename(binName));
    trim_line(norm);
    if (!norm[0])
        return;

    auto &entry = fsMinDelayMap[fsMinDelayN++];
    std::snprintf(entry.bin, sizeof(entry.bin), "%s", norm);
    entry.minDelay = minDelay;
    entry.ok = true;
}

static inline bool fs_name_eq_ci(const char *lhs, const char *rhs)
{
    if (!lhs || !rhs)
        return false;

    while (*lhs && *rhs)
    {
        const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(*lhs)));
        const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(*rhs)));
        if (a != b)
            return false;
        ++lhs;
        ++rhs;
    }
    return *lhs == 0 && *rhs == 0;
}

static inline uint16_t fs_min_delay_lookup(const char *binBase)
{
    if (!binBase || !binBase[0])
        return 0;

    char norm[64];
    std::snprintf(norm, sizeof(norm), "%s", sd_basename(binBase));
    trim_line(norm);
    if (!norm[0])
        return 0;

    for (uint8_t i = 0; i < fsMinDelayN; ++i)
    {
        if (fsMinDelayMap[i].ok && fs_name_eq_ci(fsMinDelayMap[i].bin, norm))
            return fsMinDelayMap[i].minDelay;
    }
    return 0;
}

static inline bool fs_load_min_delay_json()
{
    fs_min_delay_clear();

    FsFile f;
    bool opened = false;

    if (fsfile_open(f, CFG_FS_GIF_SETTINGS_FILE))
        opened = true;

    if (!opened)
        return false;

    const uint32_t size = fsfile_size(f);
    if (size == 0 || size > 20000u)
    {
        fsfile_close(f);
        return false;
    }

    std::vector<char> buf(size + 1, 0);
    const size_t readLen = fsfile_read(f, reinterpret_cast<uint8_t *>(buf.data()), size);
    fsfile_close(f);
    buf[readLen] = 0;

    char curBin[64] = {0};
    bool haveBin = false;
    bool haveDelay = false;
    uint16_t curDelay = 0;
    const char *p = buf.data();

    while (*p)
    {
        const char *binKey = std::strstr(p, "\"bin\"");
        const char *delayKey = std::strstr(p, "\"min_delay_ms\"");
        if (!binKey && !delayKey)
            break;

        if (binKey && (!delayKey || binKey < delayKey))
        {
            const char *colon = std::strchr(binKey, ':');
            if (colon)
            {
                const char *q1 = std::strchr(colon, '"');
                if (q1)
                {
                    const char *q2 = std::strchr(q1 + 1, '"');
                    if (q2)
                    {
                        const size_t len = std::min<size_t>(static_cast<size_t>(q2 - (q1 + 1)), sizeof(curBin) - 1);
                        std::memcpy(curBin, q1 + 1, len);
                        curBin[len] = 0;
                        haveBin = true;
                    }
                }
            }
            p = binKey + 5;
            continue;
        }

        if (delayKey)
        {
            const char *colon = std::strchr(delayKey, ':');
            const char *q = colon ? colon + 1 : delayKey;
            while (*q && (*q < '0' || *q > '9'))
                ++q;
            if (*q)
            {
                curDelay = static_cast<uint16_t>(std::strtoul(q, nullptr, 10));
                haveDelay = true;
            }
            p = delayKey + 14;
        }

        if (haveBin && haveDelay)
        {
            fs_min_delay_add(curBin, curDelay);
            haveBin = false;
            haveDelay = false;
            curBin[0] = 0;
            curDelay = 0;
        }
    }

    fsMinDelayLoaded = true;
    return fsMinDelayN > 0;
}

static inline void fs_clear_list()
{
    PLAY_N_FLASH = 0;
    for (auto &gif : fsGifs)
    {
        gif.path[0] = 0;
        gif.ok = false;
    }
}

static inline void fs_sort_list_natural()
{
    if (PLAY_N_FLASH <= 1)
        return;

    std::sort(fsGifs, fsGifs + PLAY_N_FLASH, [](const FsGifMeta &lhs, const FsGifMeta &rhs) {
        const char *a = sd_basename(lhs.path);
        const char *b = sd_basename(rhs.path);

        const auto firstNumber = [](const char *text) -> uint32_t {
            while (*text && (*text < '0' || *text > '9'))
                ++text;
            if (!*text)
                return 0xFFFFFFFFu;

            uint32_t value = 0;
            while (*text >= '0' && *text <= '9')
            {
                value = value * 10u + static_cast<uint32_t>(*text - '0');
                ++text;
            }
            return value;
        };

        const uint32_t na = firstNumber(a);
        const uint32_t nb = firstNumber(b);
        if (na != nb)
        {
            if (na == 0xFFFFFFFFu)
                return false;
            if (nb == 0xFFFFFFFFu)
                return true;
            return na < nb;
        }

        std::string sa(a);
        std::string sb(b);
        std::transform(sa.begin(), sa.end(), sa.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(sb.begin(), sb.end(), sb.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return sa < sb;
    });
}

static inline bool fs_scan_start()
{
    fs_clear_list();
    fs_load_min_delay_json();
    fsScanDone = false;
    fsScanStarted = true;
    return true;
}

static inline bool fs_scan_step(uint8_t maxItems)
{
    if (fsScanDone)
        return true;
    if (!fsScanStarted)
        fs_scan_start();

    uint16_t total = securepak_count();
    (void)maxItems;

    if (securepak_is_mounted() && total > 0)
    {
        for (uint16_t i = 0; i < total && PLAY_N_FLASH < GIF_MAX; ++i)
        {
            const SecurePakEntry *entry = securepak_entry(i);
            if (!entry || !entry->ok)
                continue;

            const char *name = entry->name;
            if (!(std::strncmp(name, "bin/", 4) == 0 || std::strncmp(name, "/bin/", 5) == 0))
                continue;

            const char *dot = std::strrchr(name, '.');
            if (!dot)
                continue;

            char ext[5] = {0};
            std::strncpy(ext, dot, 4);
            for (char &c : ext)
            {
                if (!c)
                    break;
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (std::strcmp(ext, ".bin") != 0)
                continue;

            char fullPath[64];
            if (name[0] == '/')
                std::snprintf(fullPath, sizeof(fullPath), "%s", name);
            else
                std::snprintf(fullPath, sizeof(fullPath), "/%s", name);

            FsFile f;
            if (!fsfile_open(f, fullPath))
                continue;

            BinMeta meta;
            const bool ok = parse_bin_header_fs(f, meta);
            fsfile_close(f);
            if (!ok)
                continue;

            auto &gif = fsGifs[PLAY_N_FLASH++];
            std::memset(&gif, 0, sizeof(gif));
            std::snprintf(gif.path, sizeof(gif.path), "%s", fullPath);
            gif.w = meta.w;
            gif.h = meta.h;
            gif.frameCount = meta.frameCount;
            gif.defaultDelay = meta.defaultDelay;
            gif.frameSize = meta.frameSize;
            gif.dataOffset = meta.dataOffset;
            gif.hasDelayField = meta.hasDelayField;
            gif.minDelay = fs_min_delay_lookup(sd_basename(fullPath));
            gif.ok = true;
        }
    }

    if (PLAY_N_FLASH == 0 && LittleFS.exists(CFG_FS_GIF_DIR))
    {
        File dir = LittleFS.open(CFG_FS_GIF_DIR);
        while (dir && dir.isDirectory() && PLAY_N_FLASH < GIF_MAX)
        {
            File f = dir.openNextFile();
            if (!f)
                break;
            if (f.isDirectory())
                continue;

            const char *name = sd_basename(f.name());
            const char *dot = std::strrchr(name, '.');
            if (!dot || std::strcmp(dot, ".bin") != 0)
            {
                f.close();
                continue;
            }

            BootBinMeta bootMeta;
            if (!parse_boot_bin_header(f, bootMeta))
            {
                f.close();
                continue;
            }

            auto &gif = fsGifs[PLAY_N_FLASH++];
            std::memset(&gif, 0, sizeof(gif));
            std::snprintf(gif.path, sizeof(gif.path), "%s", f.name());
            gif.w = bootMeta.w;
            gif.h = bootMeta.h;
            gif.frameCount = bootMeta.frameCount;
            gif.defaultDelay = bootMeta.defaultDelay;
            gif.frameSize = bootMeta.frameSize;
            gif.dataOffset = bootMeta.dataOffset;
            gif.hasDelayField = bootMeta.hasDelayField;
            gif.minDelay = fs_min_delay_lookup(sd_basename(gif.path));
            gif.ok = true;
            f.close();
        }
        dir.close();
    }

    fs_sort_list_natural();
    fsScanDone = true;
    return true;
}

static inline bool fs_scan_is_done()
{
    return fsScanDone;
}

static inline void fs_scan_finish(uint8_t step = 0)
{
    (void)step;
    (void)fs_scan_step(GIF_MAX);
}

static inline bool fs_mount_try()
{
    return LittleFS.begin(true);
}

static inline void fs_close_current()
{
    fsfile_close(fsCurFile);
}

static inline bool fs_open_anim(uint8_t idx)
{
    fs_close_current();
    if (!fsMounted || idx >= PLAY_N_FLASH || !fsGifs[idx].ok)
        return false;

    if (!fsfile_open(fsCurFile, fsGifs[idx].path))
        return false;

    fsCurAnimIdx = idx;
    fsCurW = fsGifs[idx].w;
    fsCurH = fsGifs[idx].h;
    fsCurFrameCount = fsGifs[idx].frameCount;
    fsCurDefaultDelay = fsGifs[idx].defaultDelay ? fsGifs[idx].defaultDelay : 40;
    fsCurFrameSize = fsGifs[idx].frameSize ? fsGifs[idx].frameSize : BUF_MAX;
    fsCurHasDelay = fsGifs[idx].hasDelayField;
    if (!fsMinDelayLoaded)
        fs_load_min_delay_json();
    fsCurMinDelay = fs_min_delay_lookup(sd_basename(fsGifs[idx].path));
    return true;
}

static inline uint16_t fs_load_frame(uint16_t fidx)
{
    if (!fsfile_is_open(fsCurFile) || fsCurFrameCount == 0)
        return 40;

    fidx = static_cast<uint16_t>(fidx % fsCurFrameCount);
    uint32_t off = fsGifs[fsCurAnimIdx].dataOffset;
    if (fsCurHasDelay)
        off += static_cast<uint32_t>(fidx) * static_cast<uint32_t>(2 + fsCurFrameSize);
    else
        off += static_cast<uint32_t>(fidx) * static_cast<uint32_t>(fsCurFrameSize);

    if (!fsfile_seek(fsCurFile, off))
        return fsCurDefaultDelay;

    uint16_t delayMs = 0;
    if (fsCurHasDelay && !read_u16_fs(fsCurFile, delayMs))
        delayMs = 0;

    const int readLen = static_cast<int>(fsfile_read(fsCurFile, fsFrameBuf, fsCurFrameSize));
    if (readLen != fsCurFrameSize)
    {
        std::memset(fsFrameBuf, 0, sizeof(fsFrameBuf));
        return fsCurDefaultDelay;
    }

    if (delayMs == 0)
        delayMs = fsCurDefaultDelay;
    if (fsCurMinDelay > 0 && delayMs < fsCurMinDelay)
        delayMs = fsCurMinDelay;
    return delayMs;
}

static inline uint32_t gif_switch_extra_ms()
{
    return static_cast<uint32_t>(ui_param[GIF_DELAY]) * 1000UL;
}

static inline uint8_t total_count_src(uint8_t src)
{
    return (src == SRC_SD) ? sdGifCount : PLAY_N_FLASH;
}

static inline uint8_t total_count()
{
    return total_count_src(gifSource);
}

static inline uint8_t *mask_src(uint8_t src)
{
    return (src == SRC_SD) ? gifMaskSd : gifMaskFlash;
}

static inline uint8_t *cur_mask()
{
    return mask_src(gifSource);
}

static inline uint8_t mask_next_enabled(const uint8_t *mask, uint8_t n, uint8_t start)
{
    if (n == 0)
        return 0;
    if (n > GIF_MAX)
        n = GIF_MAX;

    start = static_cast<uint8_t>(start % n);
    for (uint8_t off = 0; off < n; ++off)
    {
        const uint8_t idx = static_cast<uint8_t>((start + off) % n);
        if (mask_get(mask, idx))
            return idx;
    }
    return 0;
}

static inline void gif_invalidate_shuffle()
{
    gifOrderValid = false;
    gifOrderPos = 0;
    gifOrderN = 0;
}

static inline void gif_shuffle_reset(uint8_t avoidIdx, const uint8_t *mask, uint8_t n)
{
    if (n == 0)
    {
        gif_invalidate_shuffle();
        return;
    }

    uint8_t tmp[GIF_MAX];
    uint8_t count = 0;
    for (uint8_t i = 0; i < n; ++i)
    {
        if (mask_get(mask, i))
            tmp[count++] = i;
    }

    if (count == 0)
    {
        gif_invalidate_shuffle();
        return;
    }

    gifOrderN = count;
    for (uint8_t i = 0; i < count; ++i)
        gifOrder[i] = tmp[i];

    for (int i = gifOrderN - 1; i > 0; --i)
    {
        const uint8_t j = static_cast<uint8_t>(random(static_cast<long>(i) + 1));
        std::swap(gifOrder[i], gifOrder[j]);
    }

    if (gifOrderN > 1 && gifOrder[0] == avoidIdx)
        std::swap(gifOrder[0], gifOrder[1]);

    gifOrderPos = 0;
    gifOrderValid = true;
}

static inline uint8_t gif_next_anim(uint8_t currentIdx)
{
    uint8_t n = total_count();
    if (n == 0)
        return 0;

    uint8_t *mask = cur_mask();
    if (!mask_any_on(mask, n))
        return currentIdx;
    if (n == 1)
        return mask_next_enabled(mask, n, 0);

    if (!ui_param[GIF_RANDOM])
        return mask_next_enabled(mask, n, static_cast<uint8_t>((currentIdx + 1) % n));

    if (!gifOrderValid || gifOrderPos >= gifOrderN)
        gif_shuffle_reset(currentIdx, mask, n);
    if (!gifOrderValid || gifOrderN == 0)
        return currentIdx;

    uint8_t nextIdx = gifOrder[gifOrderPos++];
    if (nextIdx == currentIdx && gifOrderN > 1)
        nextIdx = gifOrder[gifOrderPos++ % gifOrderN];
    return nextIdx;
}

static inline void build_gifplays_menu(uint8_t src)
{
    setting_menu_gifplays[0].m_select = "[ GIF Playlist ]";
    setting_menu_gifplays[1].m_select = "--------------------------";
    setting_menu_gifplays[2].m_select = "= GIF Select All";
    setting_menu_gifplays[3].m_select = "= GIF Diselect All";
    setting_menu_gifplays[4].m_select = "--------------------------";

    playsMenuSource = src;
    playsMenuCount = 0;

    if (src != SRC_FLASH)
        return;

    const uint8_t n = std::min<uint8_t>(PLAY_N_FLASH, GIF_MAX);
    playsMenuCount = n;
    for (uint8_t i = 0; i < n; ++i)
    {
        char tmp[28];
        std::snprintf(tmp, sizeof(tmp), "%s", sd_basename(fsGifs[i].path));
        sd_strip_ext(tmp);
        std::snprintf(setting_gifplays_label[i], sizeof(setting_gifplays_label[i]), "%s", tmp[0] ? tmp : "GIF");
        setting_menu_gifplays[GIFPLAYS_BASE + i].m_select = setting_gifplays_label[i];
    }
}

static inline uint8_t sd_count_ready_enabled(uint8_t n)
{
    return n;
}

static inline void sd_poll()
{
}

static inline void sd_switch_to_flash()
{
    gifSource = SRC_FLASH;
    if (PLAY_N_FLASH == 0)
    {
        std::memcpy(fsFrameBuf, splash_f0000, sizeof(fsFrameBuf));
        return;
    }

    const uint8_t *mask = gifMaskFlash;
    if (animIdx >= PLAY_N_FLASH || !mask_get(mask, animIdx))
        animIdx = mask_next_enabled(mask, PLAY_N_FLASH, 0);
    frameIdx = 0;
    if (fs_open_anim(animIdx))
        nextFrameMs = millis() + fs_load_frame(frameIdx);
}

static inline void sd_switch_to_sd()
{
    gifSource = SRC_FLASH;
}

static inline void gif_apply_selection_change()
{
    if (gifSource != SRC_FLASH || PLAY_N_FLASH == 0)
    {
        std::memcpy(fsFrameBuf, splash_f0000, sizeof(fsFrameBuf));
        return;
    }

    const uint8_t *mask = gifMaskFlash;
    if (animIdx >= PLAY_N_FLASH || !mask_get(mask, animIdx))
        animIdx = mask_next_enabled(mask, PLAY_N_FLASH, 0);
    frameIdx = 0;
    if (fs_open_anim(animIdx))
        nextFrameMs = millis() + fs_load_frame(0);
    pageDirty = true;
}

static inline void dasai_boot_mount_scan()
{
    if (!fsMounted)
        fsMounted = fs_mount_try();

    if (fsMounted && !securepak_is_mounted())
        securepak_mount();

    fs_scan_finish();
    if (PLAY_N_FLASH > 0)
    {
        gifSource = SRC_FLASH;
        gif_apply_selection_change();
    }
    else
    {
        std::memcpy(fsFrameBuf, splash_f0000, sizeof(fsFrameBuf));
    }
}

static inline void updateGifNonBlocking(uint32_t nowMs)
{
    if (gifSource != SRC_FLASH || !fsMounted || PLAY_N_FLASH == 0)
        return;

    uint8_t n = total_count();
    if (n == 0)
        return;

    uint8_t *mask = cur_mask();
    if (!mask_any_on(mask, n))
    {
        nextFrameMs = nowMs + 1000u;
        return;
    }

    if (animIdx >= n || !mask_get(mask, animIdx))
    {
        animIdx = mask_next_enabled(mask, n, 0);
        frameIdx = 0;
        if (fs_open_anim(animIdx))
            nextFrameMs = nowMs + fs_load_frame(0);
        pageDirty = true;
        return;
    }

    if (!time_due(nowMs, nextFrameMs))
        return;

    if (!fsfile_is_open(fsCurFile) || fsCurAnimIdx != animIdx)
    {
        if (!fs_open_anim(animIdx))
            return;
        frameIdx = 0;
        nextFrameMs = nowMs + fs_load_frame(0);
        pageDirty = true;
        return;
    }

    uint16_t nextFrame = static_cast<uint16_t>(frameIdx + 1);
    uint8_t nextAnim = animIdx;
    bool wrapped = false;

    if (nextFrame >= fsCurFrameCount)
    {
        wrapped = true;
        nextFrame = 0;
        nextAnim = gif_next_anim(animIdx);
        if (!fs_open_anim(nextAnim))
            return;
    }

    const uint16_t delayMs = fs_load_frame(nextFrame);
    animIdx = nextAnim;
    frameIdx = nextFrame;
    nextFrameMs = nowMs + delayMs + (wrapped ? gif_switch_extra_ms() : 0u);
    pageDirty = true;
}

static inline bool fs_scan_bin_files()
{
    fs_scan_finish();
    return PLAY_N_FLASH > 0;
}

static inline bool sd_scan_bin_files()
{
    return false;
}

static inline bool sd_scan_start_incremental()
{
    return false;
}

static inline bool sd_scan_step_incremental(uint8_t maxItems)
{
    (void)maxItems;
    return true;
}

static inline bool sd_scan_incremental_is_done()
{
    return true;
}

static inline bool sd_scan_bin_files_quick(uint8_t maxItems)
{
    (void)maxItems;
    return false;
}

static inline bool sd_persist_count_stable()
{
    return true;
}

static inline void sd_mutex_init()
{
}

static inline bool sd_cd_present_raw()
{
    return false;
}

static inline bool sd_mount_try()
{
    return false;
}

static inline void sd_set_mounted(bool mounted, const char *reason)
{
    (void)reason;
    sdMounted = mounted;
}

static inline void ui_tick_gifplays_rebuild()
{
}

static inline void play_u8g2gif_bin(const char *path, uint16_t loops, uint16_t frameDelayMs)
{
    if (!path || loops == 0)
        return;

    FsFile f;
    if (!fsfile_open(f, path))
        return;

    BinMeta meta;
    if (!parse_bin_header_fs(f, meta))
    {
        fsfile_close(f);
        return;
    }

    std::vector<uint8_t> frame(meta.frameSize);
    for (uint16_t loop = 0; loop < loops; ++loop)
    {
        for (uint16_t idx = 0; idx < meta.frameCount; ++idx)
        {
            uint32_t off = meta.dataOffset;
            if (meta.hasDelayField)
                off += static_cast<uint32_t>(idx) * static_cast<uint32_t>(meta.frameSize + 2);
            else
                off += static_cast<uint32_t>(idx) * static_cast<uint32_t>(meta.frameSize);

            if (!fsfile_seek(f, off))
                break;

            uint16_t delayMs = meta.defaultDelay;
            if (meta.hasDelayField)
            {
                uint16_t d = 0;
                if (read_u16_fs(f, d) && d > 0)
                    delayMs = d;
            }
            if (frameDelayMs > 0)
                delayMs = frameDelayMs;

            if (fsfile_read(f, frame.data(), frame.size()) != frame.size())
                break;

            const int16_t x = std::max<int16_t>(0, static_cast<int16_t>((DISP_W - static_cast<int16_t>(meta.w)) / 2));
            const int16_t y = std::max<int16_t>(0, static_cast<int16_t>((DISP_H - static_cast<int16_t>(meta.h)) / 2));

            u8g2.clearBuffer();
            u8g2.drawXBMP(x, y, meta.w, meta.h, frame.data());
            u8g2.sendBuffer();
            delay(delayMs);
        }
    }

    fsfile_close(f);
}

static inline void sdToast_prepare_for_slide(uint32_t nowMs)
{
    (void)nowMs;
}

static inline bool sdToast_tick(uint32_t nowMs)
{
    (void)nowMs;
    return false;
}

static inline void sdToast_draw(uint32_t nowMs)
{
    (void)nowMs;
}

static inline bool sd_status_icon_tick(uint32_t nowMs)
{
    (void)nowMs;
    return false;
}

static inline void sd_status_icon_draw(uint32_t nowMs, bool forceDraw, int16_t xOffset = 0)
{
    (void)nowMs;
    (void)forceDraw;
    (void)xOffset;
}
