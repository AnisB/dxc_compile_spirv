// System includes
#include <stdint.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <dxcapi.h>
#include <fstream>
#include <vector>

#ifndef _WIN32
int MultiByteToWideChar(uint32_t /*CodePage*/, uint32_t /*dwFlags*/, const char* lpMultiByteStr, int cbMultiByte, wchar_t* lpWideCharStr, int cchWideChar)
{
    if (cbMultiByte == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // if cbMultiByte is -1, it indicates that lpMultiByteStr is null-terminated
    // and the entire string should be processed.
    if (cbMultiByte == -1) {
        for (cbMultiByte = 0; lpMultiByteStr[cbMultiByte] != '\0'; ++cbMultiByte)
            ;
        // Add 1 for the null-terminating character.
        ++cbMultiByte;
    }
    // If zero is given as the destination size, this function should
    // return the required size (including the null-terminating character).
    // This is the behavior of mbstowcs when the target is null.
    if (cchWideChar == 0) {
        lpWideCharStr = nullptr;
    }
    else if (cchWideChar < cbMultiByte) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    size_t rv;
    const char* prevLocale = setlocale(LC_ALL, nullptr);
    setlocale(LC_ALL, "en_US.UTF-8");
    if (lpMultiByteStr[cbMultiByte - 1] != '\0') {
        char* srcStr = (char*)malloc((cbMultiByte + 1) * sizeof(char));
        strncpy(srcStr, lpMultiByteStr, cbMultiByte);
        srcStr[cbMultiByte] = '\0';
        rv = mbstowcs(lpWideCharStr, srcStr, cchWideChar);
        free(srcStr);
    }
    else {
        rv = mbstowcs(lpWideCharStr, lpMultiByteStr, cchWideChar);
    }

    if (prevLocale)
        setlocale(LC_ALL, prevLocale);

    if (rv == (size_t)cbMultiByte)
        return rv;
    return rv + 1; // mbstowcs excludes the terminating character
}
#endif

// Function that compiles a given shader to spirv
bool compile_shader_to_spirv(const char* fileName, std::vector<uint32_t>& spirvCode)
{
    // Initialize DXC
    IDxcCompiler3* compiler;
    IDxcUtils* utils;
    IDxcIncludeHandler* includeHandler;
    IDxcResult* result;

    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

    utils->CreateDefaultIncludeHandler(&includeHandler);

    // Load shader source file
    std::ifstream shaderFile(fileName, std::ios::binary | std::ios::ate);
    if (!shaderFile)
    {
        printf("Failed to open shader file: %s\n", fileName);
        return false;
    }
    std::streamsize size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    shaderFile.read(buffer.data(), size);

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = buffer.data();
    sourceBuffer.Size = buffer.size();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    // Convert the file name to wide
    wchar_t wFilename[100];
    MultiByteToWideChar(CP_UTF8, 0, fileName, -1, wFilename, 100);

    // Set compiler arguments
    std::vector<LPCWSTR> arguments = {
        wFilename,         // Input file
        L"-T", L"cs_6_6",           // Target profile
        L"-E", L"main",             // Entry point
        L"-spirv",                  // Output SPIR-V
        L"-fvk-use-dx-layout",      // Vulkan-compatible layout
    };

    // Compile
    HRESULT hr = compiler->Compile(
        &sourceBuffer,
        arguments.data(),
        static_cast<uint32_t>(arguments.size()),
        includeHandler,
        IID_PPV_ARGS(&result)
    );

    // Check result
    IDxcBlobUtf8* errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0)
    {
        printf("Compiler errors:%s\n", (const char*)errors->GetStringPointer());
        return false;
    }

    HRESULT status;
    result->GetStatus(&status);
    if (FAILED(status))
    {
        printf("Failed to compile shader.\n");
        return false;
    }

    // Save SPIR-V binary
    IDxcBlob* spirv;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);

    // Allocate space in the buffer
    spirvCode.resize(spirv->GetBufferSize() / sizeof(uint32_t));
    memcpy(spirvCode.data(), reinterpret_cast<const char*>(spirv->GetBufferPointer()), spirv->GetBufferSize());

    // Release everything
    spirv->Release();
    if (errors)
        errors->Release();
    compiler->Release();
    utils->Release();
    includeHandler->Release();
    result->Release();

    return true;
}

int main(int argc, char** argv)
{
    // Target file
    std::string fileLocation = SHADER_SOURCE_DIR;
    fileLocation += "/RayQuery.compute";

    // Compile and get the spirv code
    std::vector<uint32_t> spirvCode;
    bool compilationSuccess = compile_shader_to_spirv(fileLocation.c_str(), spirvCode);
    if (compilationSuccess)
#ifndef _WIN32
        printf("Compiled SPIRV on LINUX using DXC\n");
#else
        printf("Compiled SPIRV on Windows using DXC\n");
#endif

	return 0;
}