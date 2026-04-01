#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "littlefs_secret.h"

#ifndef CFG_SECURE_PAK_PATH
#define CFG_SECURE_PAK_PATH "/secure.pak"
#endif

#ifndef CFG_SECURE_PAK_VERIFY_HMAC
#define CFG_SECURE_PAK_VERIFY_HMAC 0
#endif

#ifndef CFG_SECURE_PAK_MAX_FILES
#define CFG_SECURE_PAK_MAX_FILES 120
#endif

#ifndef CFG_SECURE_PAK_MAX_NAME
#define CFG_SECURE_PAK_MAX_NAME 80
#endif

struct SecurePakEntry
{
    char name[CFG_SECURE_PAK_MAX_NAME];
    uint32_t size = 0;
    uint32_t off = 0;
    bool ok = false;
};

struct SecurePakFile
{
    bool open = false;
    uint32_t pos = 0;
    std::vector<uint8_t> data;
};

static SecurePakEntry g_secure_entries[CFG_SECURE_PAK_MAX_FILES];
static uint16_t g_secure_count = 0;
static bool g_secure_mounted = false;
static uint32_t g_secure_data_base = 0;
static std::array<uint8_t, 32> g_secure_keyEnc{};
static std::array<uint8_t, 32> g_secure_keyMac{};
static std::vector<uint8_t> g_secure_blob;

static inline uint16_t securepak_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t securepak_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void securepak_reset_state()
{
    g_secure_count = 0;
    g_secure_mounted = false;
    g_secure_data_base = 0;
    g_secure_blob.clear();
    g_secure_keyEnc.fill(0);
    g_secure_keyMac.fill(0);

    for (auto &entry : g_secure_entries)
    {
        entry.name[0] = 0;
        entry.size = 0;
        entry.off = 0;
        entry.ok = false;
    }
}

static inline bool securepak_is_mounted()
{
    return g_secure_mounted;
}

static inline uint16_t securepak_count()
{
    return g_secure_count;
}

static inline const SecurePakEntry *securepak_entry(uint16_t idx)
{
    if (idx >= g_secure_count)
        return nullptr;
    return &g_secure_entries[idx];
}

static inline bool securepak_pbkdf2(const char *password, const uint8_t salt[16], uint32_t iters, uint8_t out[64])
{
    if (!password || !password[0] || !out)
        return false;

    return PKCS5_PBKDF2_HMAC(
               password,
               -1,
               salt,
               16,
               static_cast<int>(iters),
               EVP_sha256(),
               64,
               out) == 1;
}

static inline bool securepak_hmac_sha256(const uint8_t *key,
                                         size_t keyLen,
                                         const uint8_t *data,
                                         size_t dataLen,
                                         uint8_t out[32])
{
    if (!key || !data || !out)
        return false;

    unsigned int outLen = 0;
    return HMAC(EVP_sha256(),
                key,
                static_cast<int>(keyLen),
                data,
                dataLen,
                out,
                &outLen) != nullptr &&
           outLen == 32;
}

static inline bool securepak_aes_ctr_crypt(const uint8_t *key,
                                           const uint8_t *iv,
                                           const uint8_t *in,
                                           size_t len,
                                           uint8_t *out)
{
    if (!key || !iv || !in || !out)
        return false;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return false;

    bool ok = false;
    int outLen1 = 0;
    int outLen2 = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr, key, iv) == 1 &&
        EVP_EncryptUpdate(ctx, out, &outLen1, in, static_cast<int>(len)) == 1 &&
        EVP_EncryptFinal_ex(ctx, out + outLen1, &outLen2) == 1 &&
        static_cast<size_t>(outLen1 + outLen2) == len)
    {
        ok = true;
    }

    EVP_CIPHER_CTX_free(ctx);
    return ok;
}

static inline bool securepak_file_iv(const uint8_t keyEnc[32], const char *name, uint8_t ivOut[16])
{
    if (!keyEnc || !name || !ivOut)
        return false;

    std::string payload = "FILE:";
    payload += name;

    uint8_t digest[32];
    if (!securepak_hmac_sha256(keyEnc,
                               32,
                               reinterpret_cast<const uint8_t *>(payload.data()),
                               payload.size(),
                               digest))
    {
        return false;
    }

    std::memcpy(ivOut, digest, 16);
    return true;
}

static inline const SecurePakEntry *securepak_find(const char *path)
{
    if (!path || !path[0])
        return nullptr;

    const char *needle = path;
    if (needle[0] == '/')
        needle++;

    for (uint16_t i = 0; i < g_secure_count; ++i)
    {
        if (!g_secure_entries[i].ok)
            continue;

        const char *name = g_secure_entries[i].name;
        if (std::strcmp(name, needle) == 0)
            return &g_secure_entries[i];
    }

    return nullptr;
}

static inline bool securepak_mount()
{
    securepak_reset_state();

    if (!LittleFS.exists(CFG_SECURE_PAK_PATH))
    {
        Serial.println("[SECUREPAK] secure.pak tidak ditemukan");
        return false;
    }

    File pak = LittleFS.open(CFG_SECURE_PAK_PATH, FILE_READ);
    if (!pak)
    {
        Serial.println("[SECUREPAK] gagal buka secure.pak");
        return false;
    }

    const size_t totalSize = pak.size();
    if (totalSize < (64u + 32u))
    {
        Serial.println("[SECUREPAK] file terlalu kecil");
        pak.close();
        return false;
    }

    g_secure_blob.resize(totalSize);
    if (pak.read(g_secure_blob.data(), totalSize) != totalSize)
    {
        Serial.println("[SECUREPAK] gagal baca secure.pak");
        pak.close();
        securepak_reset_state();
        return false;
    }
    pak.close();

    const uint8_t *hdr = g_secure_blob.data();
    static const uint8_t MAGIC[8] = {'S', 'Z', 'I', 'P', 'v', '1', 0, 0};
    if (std::memcmp(hdr, MAGIC, sizeof(MAGIC)) != 0 || hdr[8] != 1)
    {
        Serial.println("[SECUREPAK] magic/version tidak cocok");
        securepak_reset_state();
        return false;
    }

    const uint32_t iters = securepak_le32(&hdr[16]);
    const uint8_t *salt = &hdr[20];
    const uint8_t *indexIV = &hdr[36];
    const uint32_t fileCount = securepak_le32(&hdr[52]);
    const uint32_t indexLen = securepak_le32(&hdr[56]);
    const uint32_t dataBase = 64u + indexLen;

    if (fileCount > CFG_SECURE_PAK_MAX_FILES || dataBase + 32u > totalSize)
    {
        Serial.println("[SECUREPAK] header invalid");
        securepak_reset_state();
        return false;
    }

    uint8_t key64[64];
    if (!securepak_pbkdf2(LITTLEFS_SECURE_PIN, salt, iters, key64))
    {
        Serial.println("[SECUREPAK] PBKDF2 gagal");
        securepak_reset_state();
        return false;
    }

    std::memcpy(g_secure_keyEnc.data(), key64, 32);
    std::memcpy(g_secure_keyMac.data(), key64 + 32, 32);

#if CFG_SECURE_PAK_VERIFY_HMAC
    {
        uint8_t calcTag[32];
        const size_t blobNoTag = totalSize - 32u;
        if (!securepak_hmac_sha256(g_secure_keyMac.data(),
                                   g_secure_keyMac.size(),
                                   g_secure_blob.data(),
                                   blobNoTag,
                                   calcTag) ||
            std::memcmp(calcTag, g_secure_blob.data() + blobNoTag, 32) != 0)
        {
            Serial.println("[SECUREPAK] HMAC mismatch");
            securepak_reset_state();
            return false;
        }
    }
#endif

    std::vector<uint8_t> indexPlain(indexLen);
    if (!securepak_aes_ctr_crypt(g_secure_keyEnc.data(),
                                 indexIV,
                                 g_secure_blob.data() + 64u,
                                 indexLen,
                                 indexPlain.data()))
    {
        Serial.println("[SECUREPAK] decrypt index gagal");
        securepak_reset_state();
        return false;
    }

    const uint8_t *p = indexPlain.data();
    const uint8_t *end = p + indexPlain.size();
    for (uint32_t i = 0; i < fileCount; ++i)
    {
        if (p + 2 > end)
            break;

        const uint16_t nameLen = securepak_le16(p);
        p += 2;
        if (p + nameLen + 8 > end)
            break;

        SecurePakEntry &entry = g_secure_entries[g_secure_count];
        const size_t copyLen = std::min<size_t>(nameLen, CFG_SECURE_PAK_MAX_NAME - 1);
        std::memcpy(entry.name, p, copyLen);
        entry.name[copyLen] = 0;
        p += nameLen;

        entry.size = securepak_le32(p);
        p += 4;
        entry.off = securepak_le32(p);
        p += 4;
        entry.ok = true;
        ++g_secure_count;
    }

    g_secure_data_base = dataBase;
    g_secure_mounted = (g_secure_count > 0);

    if (g_secure_mounted)
        Serial.printf("[SECUREPAK] mounted %u files\n", static_cast<unsigned>(g_secure_count));
    else
        Serial.println("[SECUREPAK] tidak ada file valid");

    return g_secure_mounted;
}

static inline void securepak_file_close(SecurePakFile &f)
{
    f.open = false;
    f.pos = 0;
    f.data.clear();
}

static inline bool securepak_file_open(SecurePakFile &out, const char *path)
{
    securepak_file_close(out);
    if (!g_secure_mounted)
        return false;

    const SecurePakEntry *entry = securepak_find(path);
    if (!entry || !entry->ok)
        return false;

    const size_t dataStart = static_cast<size_t>(g_secure_data_base) + static_cast<size_t>(entry->off);
    const size_t dataEnd = dataStart + static_cast<size_t>(entry->size);
    if (dataEnd > g_secure_blob.size() - 32u)
        return false;

    uint8_t iv[16];
    if (!securepak_file_iv(g_secure_keyEnc.data(), entry->name, iv))
        return false;

    out.data.resize(entry->size);
    if (!securepak_aes_ctr_crypt(g_secure_keyEnc.data(),
                                 iv,
                                 g_secure_blob.data() + dataStart,
                                 entry->size,
                                 out.data.data()))
    {
        out.data.clear();
        return false;
    }

    out.open = true;
    out.pos = 0;
    return true;
}

static inline bool securepak_file_seek(SecurePakFile &f, uint32_t pos)
{
    if (!f.open || pos > f.data.size())
        return false;
    f.pos = pos;
    return true;
}

static inline uint32_t securepak_file_size(const SecurePakFile &f)
{
    return f.open ? static_cast<uint32_t>(f.data.size()) : 0;
}

static inline bool securepak_file_available(const SecurePakFile &f)
{
    return f.open && f.pos < f.data.size();
}

static inline size_t securepak_file_read(SecurePakFile &f, uint8_t *outBuf, size_t len)
{
    if (!f.open || !outBuf || len == 0 || f.pos >= f.data.size())
        return 0;

    const size_t remaining = f.data.size() - f.pos;
    const size_t chunk = std::min(len, remaining);
    std::memcpy(outBuf, f.data.data() + f.pos, chunk);
    f.pos += static_cast<uint32_t>(chunk);
    return chunk;
}
