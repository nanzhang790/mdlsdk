/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#include "utils.h"
#include <Windows.h>
#include <d3d12.h>
#include <iostream>

namespace
{
    void print(
        const std::string& prefix, 
        const std::string& message, 
        const std::string& file, 
        int line)
    {
        std::string m = prefix;
        m.append(message);
        if (!file.empty())
        {
            m.append("\n                file: ");
            m.append(file);
            m.append(", line: ");
            m.append(std::to_string(line));
            m.append("\n");
        }
        else
        {
            m.append("\n");
        }
        std::cerr << m.c_str();
        OutputDebugStringA(m.c_str());
    }

    std::string to_string(HRESULT error_code)
    {
        switch (error_code)
        {
            case DXGI_ERROR_INVALID_CALL:
                return "DXGI_ERROR_INVALID_CALL";
            case DXGI_ERROR_WAS_STILL_DRAWING:
                return "DXGI_ERROR_WAS_STILL_DRAWING";
            case DXGI_ERROR_DEVICE_REMOVED:
                return "DXGI_ERROR_DEVICE_REMOVED";
            case DXGI_ERROR_DEVICE_HUNG:
                return "DXGI_ERROR_DEVICE_HUNG";
            case DXGI_ERROR_DEVICE_RESET:
                return "DXGI_ERROR_DEVICE_RESET";
            case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
                return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
            case E_FAIL:
                return "E_FAIL";
            case E_INVALIDARG:
                return "E_INVALIDARG";
            case E_OUTOFMEMORY:
                return "E_OUTOFMEMORY";
            case E_NOTIMPL:
                return "E_NOTIMPL";
            case S_FALSE:
                return "S_FALSE";
            case S_OK:
                return "S_OK";
            default:
                return "";
        }
    }

    std::string print_nested_exceotion(const std::exception& e) 
    {
        std::string message = e.what();
        try {
            std::rethrow_if_nested(e);
        }
        catch (const std::exception& nested) {
            message += "\n               nested: " + print_nested_exceotion(nested);
        }
        return message;
    }

} // anonymous

namespace mdl_d3d12
{
    void log_info(const std::string& message, const std::string& file, int line)
    {
        print("[MDL_D3D12] [INFO]    ", message, file, line);
    }

    void log_warning(const std::string& message, const std::string& file, int line)
    {
        print("[MDL_D3D12] [WARNING] ", message, file, line);
    }

    void log_error(const std::string& message, const std::string& file, int line)
    {
        print("[MDL_D3D12] [ERROR]   ", message, file, line);
    }

    void log_error(const std::exception& exception, const std::string& file, int line)
    {
        print("[MDL_D3D12] [ERROR]   ", print_nested_exceotion(exception), file, line);
    }


    bool log_on_failure(
        HRESULT error_code, const std::string& message, const std::string& file, int line)
    {
        if (SUCCEEDED(error_code))
            return false;

        std::string readable_error = to_string(error_code);
        if (!readable_error.empty())
            readable_error += " (" + std::to_string(error_code) + ")";
        else
            readable_error = std::to_string(error_code);

        print("[MDL_D3D12] [FAILURE] ", 
              message + "\n                     return code: " + readable_error,
              file, line);
        return true;
    }

    void throw_on_failure(
        HRESULT error_code, const std::string& message, const std::string& file, int line)
    {
        if (log_on_failure(error_code, message, file, line))
            throw(message.c_str());
    }


    void set_debug_name(ID3D12Object* obj, const std::string& name)
    {
        obj->SetName(str_to_wstr(name).c_str());
    }


    Timing::Timing(std::string operation)
        : m_operation(operation)
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    Timing::~Timing()
    {
        auto stop = std::chrono::high_resolution_clock::now();
        double elapsed_seconds = (stop - m_start).count() * 1e-9;
        log_info("Finished '" + m_operation + "' after " + 
                 std::to_string(elapsed_seconds) + " seconds.");
    }
}
