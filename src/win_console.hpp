#pragma once
#include "color.hpp"

#include <memory>

namespace winenv {
// Управляет консолью windows. Позволяет настроить внешний вид.
// Создает новую консоль или подключается к автоматически созданной.
// Одиночка
class WinConsole {
public:
  static constexpr size_t n_term_colors = 16;

  static const std::unique_ptr<WinConsole> &get();
  // Аллоцирует новую консоль. Создает экземпляр класса.
  // Окно созданной консоли будет найдено по переданной строке заголовка.
  // Пользователь должен гарантировать уникальность заголовка окна в системе.
  // Если заголовок не задан, поиск окна займет время, поток заблокируется.
  // Если экземпляр класса уже существует или
  // приложение уже подключено к консоли, выбросит исключение
  static void allocate(std::wstring_view search_console_title = {});
  // Находит подключенную консоль. Создает экземпляр класса.
  // Окно консоли будет найдено по переданной строке заголовка.
  // Пользователь должен гарантировать уникальность заголовка окна в системе.
  // Если заголовок не задан, поиск окна займет время, поток заблокируется.
  // Если экземпляр класса уже существует, выбросит исключение
  static void create_from_attached(std::wstring_view search_console_title = {});
  // Устанавливает заголовок окна консоли,
  // независимо от того, создан экземпляр класса или нет
  static void set_title(std::wstring_view title);
  // Отключает программу от окна консоли,
  // независимо от того, создан экземпляр класса или нет
  static void free();

  void resize_chrs(short width, short height, short buf_width = 0,
                   short buf_height = 0);
  void resize_pxls(int width, int height);
  void set_position(int left, int top);
  void set_color(ConsoleColor foreground, ConsoleColor background);
  void set_term_color_table(const RgbColor new_table[n_term_colors]);
  void set_font(std::string_view font_name, short font_height);
  // Настраивает внешний вид по информации переданной родительским процессом.
  // Позволяет в дочернем процессе оконной программы получить консоль того же
  // вида, как если бы программа была консольной
  void configure_use_startup_info(const STARTUPINFOW &start_info);

  WinConsole(const WinConsole &other) = delete;
  WinConsole &operator=(const WinConsole &other) = delete;
  WinConsole(WinConsole &&other) noexcept = delete;
  WinConsole &operator=(WinConsole &&other) noexcept = delete;

private:
  // Минимальное время выполнения 40 мс, т.к. содержит вызов Sleep(40)
  static HWND get_attached_console_hwnd();
  WinConsole(std::wstring_view search_console_title = {});

  static std::unique_ptr<WinConsole> s_instance;
  HANDLE m_console_out_handle;
  CONSOLE_SCREEN_BUFFER_INFOEX m_screen_buf_info;
  HWND m_hwnd;
};
} // namespace winenv
