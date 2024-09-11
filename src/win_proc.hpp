#pragma once
#include "color.hpp"

#include <string>

namespace winenv {
void run_as_admin(const std::wstring &file_path, const std::wstring &cmd_args);

// Обёртка процесса windows. Позволяет создать дочерний процесс
// с необходимыми параметрами, дождаться завершения процесса.
// Использованы широкие строки для большей переносимости.
// Пример. Создание процесса командной строки:
// WinProcess proc = WinProcess::Constructor()
//                  .set_command_line_arguments(L"_ echo hello world")
//                  .create(L"C:\\WINDOWS\\system32\\cmd.exe", L"hello world");
class WinProcess {
public:
  // Инкапсулирующая фабрика
  class Constructor {
  public:
    Constructor();
    // Информация требуемая для создания процесса.
    // Если exe_path пустая строка, нужно определить аргументы
    // командной строки set_command_line_arguments(...).
    // Второй аргумент не влияет на вид пользовательских окон
    WinProcess create(std::wstring_view exe_path, std::wstring console_title);
    // Аргументы отделены пробелами. Первый аргумент - название программы,
    // аргумент не записывается в строку LPSTR szCmdLine функции WinMain.
    // Если exe_path == {}, тогда первый аргумент используется как путь к .exe
    // файлу
    Constructor &set_command_line_arguments(std::wstring cmd_args);
    // Если не вызывать, то наследует от родительского процесса
    Constructor &set_environment_variables(std::wstring env);
    // Если не вызывать, то наследует от родительского процесса
    Constructor &set_startup_directory(std::wstring_view dir);
    // Допустимые флаги:
    // https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags
    Constructor &add_startup_flags(int flags) noexcept;
    Constructor &set_window_position(DWORD x, DWORD y) noexcept;
    // Применяется при первом вызове CreateWindow()
    Constructor &set_window_size_pxl(DWORD width, DWORD height) noexcept;
    // Не применяют оконные программы
    Constructor &set_console_size_chr(DWORD width, DWORD height) noexcept;
    Constructor &set_console_color(ConsoleColor foreground,
                                   ConsoleColor background) noexcept;
    Constructor &set_window_show_parameter(int show_parameter) noexcept;

  private:
    STARTUPINFOW m;
    // Опционально. Нужны неконстантные указатели. Храним копии
    std::wstring m_cmd_args, m_env_var;
    // Опционально. Подойдет константный указаетель
    std::wstring_view m_start_dir;
    int m_startup_flags{};
  };
  // Не завершает процесс, только освобождает дескрипторы
  ~WinProcess();
  // Не создает новой процесс, используется как "пустой" объект
  WinProcess();
  WinProcess(const WinProcess &other) = delete;
  WinProcess(WinProcess &&other);
  WinProcess &operator=(const WinProcess &other) = delete;
  WinProcess &operator=(WinProcess &&other);
  // Ждем завершение процесса заданное время.
  // Если процесс завершился, возвращаем true иначе - false.
  // Допустимы особые значения времени 0 и INFINITE
  bool wait_for(DWORD milliseconds);
  HANDLE get_handle() noexcept;
  DWORD get_id() const noexcept;
  HANDLE get_thread_handle() noexcept;
  DWORD get_thread_id() const noexcept;

private:
  PROCESS_INFORMATION m_info{};
};

} // namespace winenv
