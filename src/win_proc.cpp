#include "win_proc.hpp"

namespace winenv {

void run_as_admin(const std::wstring &file_path, const std::wstring &cmd_args) {
  HINSTANCE res = ShellExecuteW(nullptr, L"runas", file_path.c_str(),
                                cmd_args.c_str(), nullptr, SW_SHOWNORMAL);
  INT_PTR ret_code = reinterpret_cast<INT_PTR>(res);
  if (ret_code <= 32) {
    throw WinError("Failed to run as admin", ret_code);
  }
}

using Constructor = WinProcess::Constructor;
Constructor::Constructor() {
  ZeroMemory(&m, sizeof(m));
  m.cb = sizeof(m); // Обязательно заполнить !
}

WinProcess Constructor::create(std::wstring_view exe_path,
                               std::wstring console_title) {
  WinProcess process{};
  m.lpTitle = console_title.data();
  ZeroMemory(&process.m_info, sizeof(process));
  BOOL rs = CreateProcessW(
      exe_path.empty() ? nullptr : exe_path.data(),
      m_cmd_args.empty() ? nullptr : m_cmd_args.data(), nullptr, nullptr,
      false, // Не меняем параметры безопасности
      m_startup_flags ? m_startup_flags : NORMAL_PRIORITY_CLASS,
      m_env_var.empty() ? nullptr : reinterpret_cast<void *>(m_env_var.data()),
      m_start_dir.empty() ? nullptr : m_start_dir.data(), &m, &process.m_info);
  if (rs == 0) {
    // Чтобы вывести заголовок процесса в сообщение ошибки, нужно преобразовать
    // широкую строку в однобайтную строку.
    std::string narrow_title = narrow_string(console_title);
    throw WinError("Failed to create WinProcess with title: " + narrow_title,
                   GetLastError());
  }
  return process;
}

Constructor &Constructor::set_command_line_arguments(std::wstring cmd_args) {
  m_cmd_args = std::move(cmd_args);
  return *this;
}

Constructor &Constructor::set_environment_variables(std::wstring env) {
  m_env_var = std::move(env);
  return *this;
}

Constructor &Constructor::set_startup_directory(std::wstring_view dir) {
  m_start_dir = dir;
  return *this;
}

Constructor &Constructor::add_startup_flags(int flags) noexcept {
  m_startup_flags |= flags;
  return *this;
}

Constructor &Constructor::set_window_position(DWORD x, DWORD y) noexcept {
  m.dwX = x;
  m.dwY = y;
  m.dwFlags |= STARTF_USEPOSITION;
  return *this;
}

Constructor &Constructor::set_window_size_pxl(DWORD width,
                                              DWORD height) noexcept {
  m.dwXSize = width;
  m.dwYSize = height;
  m.dwFlags |= STARTF_USESIZE;
  return *this;
}

Constructor &Constructor::set_console_size_chr(DWORD width,
                                               DWORD height) noexcept {
  m.dwXCountChars = width;
  m.dwYCountChars = height;
  m.dwFlags |= STARTF_USECOUNTCHARS;
  return *this;
}

Constructor &Constructor::set_console_color(ConsoleColor foreground,
                                            ConsoleColor background) noexcept {
  m.dwFillAttribute = console_text_attribute(foreground, background);
  m.dwFlags |= STARTF_USEFILLATTRIBUTE;
  return *this;
}

Constructor &
Constructor::set_window_show_parameter(int show_parameter) noexcept {
  m.wShowWindow = show_parameter;
  m.dwFlags |= STARTF_USESHOWWINDOW;
  return *this;
}

WinProcess::~WinProcess() {
  if (m_info.hProcess != nullptr) {
    CloseHandle(m_info.hProcess);
    CloseHandle(m_info.hThread);
  }
}

WinProcess::WinProcess() : m_info{} { ZeroMemory(&m_info, sizeof(m_info)); }

WinProcess::WinProcess(WinProcess &&other) {
  this->m_info = other.m_info;
  ZeroMemory(&other.m_info, sizeof(other.m_info));
}

WinProcess &WinProcess::operator=(WinProcess &&other) {
  if (m_info.hProcess != nullptr) {
    CloseHandle(m_info.hProcess);
    CloseHandle(m_info.hThread);
  }
  this->m_info = other.m_info;
  ZeroMemory(&other.m_info, sizeof(other.m_info));
  return *this;
}

bool WinProcess::wait_for(DWORD milliseconds) {
  if (!m_info.hProcess) {
    throw std::runtime_error("Uninitialized WinProcess instance");
  }
  DWORD result = WaitForSingleObject(m_info.hProcess, milliseconds);
  if (result == WAIT_OBJECT_0) {
    return true;
  }
  if (result == WAIT_TIMEOUT) {
    return false;
  };
  throw WinError("Failed while waiting WinProcess", GetLastError());
}

HANDLE WinProcess::get_handle() noexcept { return m_info.hProcess; }

DWORD WinProcess::get_id() const noexcept { return m_info.dwProcessId; }

HANDLE WinProcess::get_thread_handle() noexcept { return m_info.hThread; }

DWORD WinProcess::get_thread_id() const noexcept { return m_info.dwThreadId; }

} // namespace winenv
