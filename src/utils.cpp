#include "utils.hpp"

#include <charconv>
#include <system_error>

namespace winenv
{
std::string WinError::get_as_string(long long int win_err_code)
{
    return {std::system_category().message(win_err_code)};
}

WinError::WinError(const std::string& str)
  :
    std::runtime_error("[Windows error] " + str +'\n'), m_err_code()
{}

WinError::WinError(const std::string& str, DWORD win_err_code)
  :
    std::runtime_error("[Windows error] " + str + "\n[Error code " +
      std::to_string(win_err_code) +" ] " + get_as_string(win_err_code) + '\n'),
    m_err_code(win_err_code)
{}

DWORD WinError::get_error_code() const noexcept
{
    return m_err_code;
}

WrongLiteralFormat::WrongLiteralFormat(const std::string& str)
: std::runtime_error(str)
{}

std::wstring widen_string(std::string_view narrow)
{
  size_t n_converted;
  std::wstring wide;
  wide.resize(narrow.size() + 1); // Место для конца строки
  mbstowcs_s(&n_converted, wide.data(), wide.size(), narrow.data(), narrow.size());
  wide.resize(n_converted);
  return wide;
}

std::string narrow_string(std::wstring_view wide)
{
  size_t n_converted;
  std::string narrow;
  narrow.resize((wide.size() + 1) * sizeof(wchar_t));
  wcstombs_s(&n_converted, narrow.data(), 
          narrow.size(), wide.data(), wide.size());
  narrow.resize(n_converted);
  return narrow;
}

void get_unique_window_title(char *str, size_t size)
{
    auto [last1, error_code1] = std::to_chars(str,
                                  str + size,
                                  GetCurrentProcessId());
    if (error_code1 != std::errc{}) {
      throw std::runtime_error("Failed to_chars(...) call");
    }
    *last1 = '_';
    auto [last2, error_code2] = std::to_chars(last1 + 1,
                                  str + size,
                                  GetTickCount());
    if (error_code2 != std::errc{}) {
      throw std::runtime_error("Failed to_chars(...) call");
    }
    *last2 = '\0';
}

Path get_programm_path()
{
    std::wstring path_str;
    path_str.resize(g_max_file_path);
    DWORD length = GetModuleFileNameW(nullptr, path_str.data(), g_max_file_path);
    path_str.resize(length);
    return {path_str};
}

Path get_cmd_path()
{
  return {get_env_variable("COMSPEC")};
}

std::string get_env_variable(std::string_view env_name)
{
    constexpr size_t supposed_variable_size {4096};
    std::string env_variable {};
    env_variable.resize(supposed_variable_size);

    // Возвращает 0 в случае ошибки или размер, необходимый для того, чтобы вместить строку.
    // Если входной буфер достаточного размера, записывает строку
    DWORD real_size = GetEnvironmentVariableA(env_name.data(), env_variable.data(), supposed_variable_size);
    if (real_size == 0) {
      throw WinError(std::string{"Failed to get \""} +  env_name.data() +"\" env variable", GetLastError());
    } else if (supposed_variable_size < real_size) {
      env_variable.resize(real_size);
      DWORD written = GetEnvironmentVariableA(env_name.data(), env_variable.data(), real_size);
      if (written == 0) {
        throw WinError(std::string{"Failed to get \""} +  env_name.data() +"\" env variable", GetLastError());
      }
    } else {
      env_variable.resize(real_size);
    }
    return env_variable;
}

void set_env_variable(std::string_view name, std::string_view value)
{
    if (! SetEnvironmentVariableA(name.data(),value.data())) {
      throw WinError(std::string{"Failed to set \""} +name.data() +"\" env variable", GetLastError()); 
    }
}

void invoke_message_box(std::string_view message)
{
  MessageBoxA(nullptr, message.data(), nullptr, MB_OK | MB_ICONINFORMATION);
}

} // namespace winenv
