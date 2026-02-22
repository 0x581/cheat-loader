#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <wincrypt.h>
#include <shellapi.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

namespace Crypto
{
    // AES-256 Key (32 bytes) - In production, get this from KeyAuth after license validation
    // This is a placeholder - replace with your own key or fetch from server
    constexpr unsigned char DEFAULT_KEY[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

    // Simple XOR-based encryption for basic protection
    // For stronger protection, use Windows CryptoAPI with AES
    inline std::vector<unsigned char> XorEncryptDecrypt(
        const std::vector<unsigned char> &data,
        const unsigned char *key,
        size_t keyLen)
    {
        std::vector<unsigned char> result(data.size());
        for (size_t i = 0; i < data.size(); i++)
        {
            result[i] = data[i] ^ key[i % keyLen];
        }
        return result;
    }

    // Read file into byte vector
    inline std::vector<unsigned char> ReadFileBytes(const std::string &path)
    {
        std::vector<unsigned char> data;

        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
            return data;

        DWORD fileSize = GetFileSize(hFile, NULL);
        if (fileSize == INVALID_FILE_SIZE)
        {
            CloseHandle(hFile);
            return data;
        }

        data.resize(fileSize);
        DWORD bytesRead = 0;
        ::ReadFile(hFile, data.data(), fileSize, &bytesRead, NULL);
        CloseHandle(hFile);

        if (bytesRead != fileSize)
            data.clear();

        return data;
    }

    // Write byte vector to file
    inline bool WriteFileBytes(const std::string &path, const std::vector<unsigned char> &data)
    {
        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0,
                                   NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        DWORD bytesWritten = 0;
        BOOL success = ::WriteFile(hFile, data.data(), (DWORD)data.size(), &bytesWritten, NULL);
        CloseHandle(hFile);

        return success && bytesWritten == data.size();
    }

    // Generate random filename
    inline std::string GenerateRandomFilename()
    {
        char tempPath[MAX_PATH];
        char tempFile[MAX_PATH];

        GetTempPathA(MAX_PATH, tempPath);
        GetTempFileNameA(tempPath, "app", 0, tempFile);

        // Change extension to .exe
        std::string result(tempFile);
        size_t dotPos = result.rfind('.');
        if (dotPos != std::string::npos)
            result = result.substr(0, dotPos) + ".exe";
        else
            result += ".exe";

        return result;
    }

    // Decrypt encrypted exe, write to temp, execute, delete
    // Returns error message or empty string on success
    inline std::string DecryptAndExecuteWithError(const std::string &encryptedPath,
                                                  const unsigned char *key = DEFAULT_KEY,
                                                  size_t keyLen = 32)
    {
        // Read encrypted file
        std::vector<unsigned char> encryptedData = ReadFileBytes(encryptedPath);
        if (encryptedData.empty())
            return "Cannot read file: " + encryptedPath;

        // Decrypt
        std::vector<unsigned char> decryptedData = XorEncryptDecrypt(encryptedData, key, keyLen);

        // Verify PE header (MZ signature)
        if (decryptedData.size() < 2 || decryptedData[0] != 'M' || decryptedData[1] != 'Z')
            return "Invalid PE header after decryption (wrong key?)";

        // Write to temp file
        std::string tempPath = GenerateRandomFilename();
        if (!WriteFileBytes(tempPath, decryptedData))
            return "Cannot write temp file: " + tempPath;

        // Use ShellExecuteEx for GUI app support
        SHELLEXECUTEINFOA sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = NULL;
        sei.lpFile = tempPath.c_str();
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteExA(&sei))
        {
            DWORD err = GetLastError();
            DeleteFileA(tempPath.c_str());
            return "ShellExecuteEx failed, error: " + std::to_string(err);
        }

        // hInstApp > 32 means success
        if ((intptr_t)sei.hInstApp <= 32)
        {
            DeleteFileA(tempPath.c_str());
            return "ShellExecute error code: " + std::to_string((intptr_t)sei.hInstApp);
        }

        // Wait briefly for process to initialize
        if (sei.hProcess)
        {
            WaitForInputIdle(sei.hProcess, 2000);
            CloseHandle(sei.hProcess);
        }

        // Don't delete - Windows cleans up %TEMP% eventually
        return ""; // Success
    }

    // Legacy wrapper
    inline bool DecryptAndExecute(const std::string &encryptedPath,
                                  const unsigned char *key = DEFAULT_KEY,
                                  size_t keyLen = 32)
    {
        return DecryptAndExecuteWithError(encryptedPath, key, keyLen).empty();
    }
}
