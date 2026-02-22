// Encryptor Tool - Encrypts .exe files for the loader
// Usage: Encryptor.exe <input.exe> <output.enc>

#include <iostream>
#include <vector>
#include <string>
#include <windows.h>

// Same key as in crypto.hpp - MUST MATCH!
constexpr unsigned char DEFAULT_KEY[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

std::vector<unsigned char> ReadFile(const std::string &path)
{
    std::vector<unsigned char> data;

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: Cannot open file: " << path << std::endl;
        return data;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        return data;
    }

    data.resize(fileSize);
    DWORD bytesRead = 0;
    ReadFile(hFile, data.data(), fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    if (bytesRead != fileSize)
        data.clear();

    return data;
}

bool WriteFile(const std::string &path, const std::vector<unsigned char> &data)
{
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0,
                               NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: Cannot create file: " << path << std::endl;
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL success = ::WriteFile(hFile, data.data(), (DWORD)data.size(), &bytesWritten, NULL);
    CloseHandle(hFile);

    return success && bytesWritten == data.size();
}

std::vector<unsigned char> XorEncrypt(const std::vector<unsigned char> &data,
                                      const unsigned char *key, size_t keyLen)
{
    std::vector<unsigned char> result(data.size());
    for (size_t i = 0; i < data.size(); i++)
    {
        result[i] = data[i] ^ key[i % keyLen];
    }
    return result;
}

int main(int argc, char *argv[])
{
    std::cout << "=== EXE Encryptor Tool ===" << std::endl;

    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <input.exe> <output.enc>" << std::endl;
        std::cout << "Example: " << argv[0] << " MyApp.exe products/MyApp.enc" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    std::cout << "Input:  " << inputPath << std::endl;
    std::cout << "Output: " << outputPath << std::endl;

    // Read input file
    std::cout << "Reading file..." << std::endl;
    std::vector<unsigned char> data = ReadFile(inputPath);

    if (data.empty())
    {
        std::cerr << "Error: Failed to read input file or file is empty." << std::endl;
        return 1;
    }

    // Verify PE header
    if (data.size() < 2 || data[0] != 'M' || data[1] != 'Z')
    {
        std::cerr << "Warning: File does not appear to be a valid PE executable." << std::endl;
    }

    std::cout << "File size: " << data.size() << " bytes" << std::endl;

    // Encrypt
    std::cout << "Encrypting..." << std::endl;
    std::vector<unsigned char> encrypted = XorEncrypt(data, DEFAULT_KEY, 32);

    // Write output
    std::cout << "Writing encrypted file..." << std::endl;
    if (!WriteFile(outputPath, encrypted))
    {
        std::cerr << "Error: Failed to write output file." << std::endl;
        return 1;
    }

    std::cout << "Success! Encrypted file saved to: " << outputPath << std::endl;
    std::cout << "Place this file in your products folder." << std::endl;

    return 0;
}
