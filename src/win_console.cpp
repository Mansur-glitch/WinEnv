#include "win_console.hpp"

#include "common.hpp"

namespace winenv {
std::unique_ptr<WinConsole> WinConsole::s_instance{};

HWND WinConsole::get_attached_console_hwnd() {
  // Основано на
  // https://learn.microsoft.com/en-us/troubleshoot/windows-server/performance/obtain-console-window-handle
  wchar_t old_con_title[g_max_file_path];
  if (!GetConsoleTitleW(old_con_title, g_max_file_path)) {
    throw WinError("Failed to get console window title", GetLastError());
  }
  // Временный заголовок - "узкая" строка. Вызовы winapi с суфиксом A
  char temp_con_title_n[g_max_file_path];
  get_unique_window_title(temp_con_title_n, g_max_file_path);
  // Временно меняем заголовок т.к. начальный заголовок может быть неуникальным
  if (!SetConsoleTitleA(temp_con_title_n)) {
    throw WinError("Failed to set temporary console window title",
                   GetLastError());
  };

  Sleep(40); // Время на смену заголовка
  HWND hwnd = FindWindowA(NULL, temp_con_title_n);
  if (hwnd == 0) {
    throw WinError("Failed to find console window by title", GetLastError());
  }
  if (!SetConsoleTitleW(old_con_title)) {
    throw WinError("Failed to reset original console window title",
                   GetLastError());
  }
  return (hwnd);
}

WinConsole::WinConsole(std::wstring_view search_console_title) {
  if (search_console_title.empty()) {
    m_hwnd = get_attached_console_hwnd();
  } else {
    m_hwnd = FindWindowW(nullptr, search_console_title.data());
    if (!m_hwnd) {
      throw WinError(std::string{"Failed to find console window with title: "} +
                         narrow_string(search_console_title),
                     GetLastError());
    }
  }

  m_console_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  ZeroMemory(&m_screen_buf_info, sizeof(m_screen_buf_info));
  m_screen_buf_info.cbSize = sizeof(m_screen_buf_info);
  if (!GetConsoleScreenBufferInfoEx(m_console_out_handle, &m_screen_buf_info)) {
    throw WinError("Failed to get console buffer info");
  };
}

const std::unique_ptr<WinConsole> &WinConsole::get() { return s_instance; }

void WinConsole::allocate(std::wstring_view search_console_title) {
  if (s_instance != nullptr) {
    throw std::runtime_error("WinConsole has already been created");
  } else {
    if (!AllocConsole()) {
      throw WinError("Failed to allocate console", GetLastError());
    }

    // Подключаем потоки стандартной библиотеки C++ к консоли
    FILE *fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    setvbuf(stderr, NULL, _IONBF, 0);
    freopen_s(&fp, "CONIN$", "r", stdin);
    setvbuf(stdin, NULL, _IONBF, 0);

    s_instance =
        std::unique_ptr<WinConsole>(new WinConsole(search_console_title));
  }
}

void WinConsole::create_from_attached(std::wstring_view search_console_title) {
  if (s_instance != nullptr) {
    throw std::runtime_error("WinConsole has already been created");
  } else {
    s_instance =
        std::unique_ptr<WinConsole>(new WinConsole(search_console_title));
  }
}

void WinConsole::set_title(std::wstring_view title) {
  if (!SetConsoleTitleW(title.data())) {
    throw WinError("Failed to set console window title", GetLastError());
  };
}

void WinConsole::free() {
  if (!FreeConsole()) {
    throw WinError("Failed to free console", GetLastError());
  }
  s_instance.release();
}

void WinConsole::resize_chrs(short width, short height, short buf_width,
                             short buf_height) {
  static constexpr short n_console_buf_rows = 500;

  // Нужно убедится, что буфер консоли больше размера отображаемого символьного
  // окна
  if (buf_width == 0) {
    buf_width = width;
    buf_height = height + n_console_buf_rows;
  } else if (buf_height == 0) {
    if (buf_width < width) {
      throw std::runtime_error("Failed to resize console. Buffer size must be "
                               "greater than window size");
    }
    buf_height = height + n_console_buf_rows;
  } else if (buf_width < width || buf_height < height) {
    throw std::runtime_error("Failed to resize console. Buffer size must be "
                             "greater than window size");
  }

  SMALL_RECT null_rect{0, 0, 0, 0};
  // Уменьшаем до нуля размер символьного окна (ConsoleWindow), чтобы
  // произвольно менять размер буфера консоли
  if (!SetConsoleWindowInfo(m_console_out_handle, true, &null_rect)) {
    throw WinError("Failed to set ConsoleWindow to null rect", GetLastError());
  }
  if (!SetConsoleScreenBufferSize(m_console_out_handle,
                                  {buf_width, buf_height})) {
    throw WinError("Failed to set console number columns and rows",
                   GetLastError());
  }
  CONSOLE_SCREEN_BUFFER_INFO screen_buf_info{};
  if (!GetConsoleScreenBufferInfo(m_console_out_handle, &screen_buf_info)) {
    throw WinError("Failed to get console screen buffer info", GetLastError());
  }
  // Отображаемая часть буфера консоли в 2д координатах
  SMALL_RECT con_win_rc = {0, 0, 0, 0};
  // Запрашиваемый размер окна может не помещаться на экране
  con_win_rc.Right = static_cast<short>(
      std::min(width - 1, screen_buf_info.dwMaximumWindowSize.X - 1));
  con_win_rc.Bottom = static_cast<short>(
      std::min(height - 1, screen_buf_info.dwMaximumWindowSize.Y - 1));
  if (!SetConsoleWindowInfo(m_console_out_handle, true, &con_win_rc)) {
    throw WinError("Failed to set console number columns and rows",
                   GetLastError());
  }
  // Обновляем информацию о буфере консоли для корректной работы зависящих
  // методов
  m_screen_buf_info.dwSize = {buf_width, buf_height};
  m_screen_buf_info.dwMaximumWindowSize = screen_buf_info.dwMaximumWindowSize;
  m_screen_buf_info.srWindow = con_win_rc;
}

void WinConsole::resize_pxls(int width, int height) {
  if (!SetWindowPos(m_hwnd, HWND_TOP, 0, 0, width, height,
                    SWP_ASYNCWINDOWPOS | SWP_NOMOVE)) {
    throw WinError("Failed to resize console window", GetLastError());
  }
}

void WinConsole::set_position(int left, int top) {
  if (!SetWindowPos(m_hwnd, HWND_TOP, left, top, 0, 0,
                    SWP_ASYNCWINDOWPOS | SWP_NOSIZE)) {
    throw WinError("Failed to set console window position", GetLastError());
  }
}

void WinConsole::set_color(ConsoleColor foreground, ConsoleColor background) {
  if (!SetConsoleTextAttribute(
          m_console_out_handle,
          console_text_attribute(foreground, background))) {
    throw WinError("Failed to set console color", GetLastError());
  }
}

void WinConsole::set_term_color_table(const RgbColor new_table[n_term_colors]) {
  for (size_t i = 0; i < n_term_colors; ++i) {
    m_screen_buf_info.ColorTable[i] = new_table[i];
  }

  if (!SetConsoleScreenBufferInfoEx(m_console_out_handle, &m_screen_buf_info)) {
    throw WinError("Failed to set console color table", GetLastError());
  }
}

void WinConsole::set_font(std::string_view font_name, short font_height) {
  CONSOLE_FONT_INFOEX font_info;
  ZeroMemory(&font_info, sizeof(font_info));
  font_info.cbSize = sizeof(font_info);

  std::copy(font_name.begin(), font_name.end(), font_info.FaceName);
  font_info.dwFontSize = {0,
                          font_height}; // Ширина будет подобрана автоматически
  if (!SetCurrentConsoleFontEx(m_console_out_handle, false, &font_info)) {
    throw WinError("Failed to set console font", GetLastError());
  }
}

void WinConsole::configure_use_startup_info(const STARTUPINFOW &start_info) {
  if (start_info.dwFlags & STARTF_USECOUNTCHARS) {
    resize_chrs(start_info.dwXCountChars, start_info.dwYCountChars);
  }
  if (start_info.dwFlags & STARTF_USESIZE) {
    resize_pxls(start_info.dwXSize, start_info.dwYSize);
  }
  if (start_info.dwFlags & STARTF_USEPOSITION) {
    set_position(start_info.dwX, start_info.dwY);
  }
  if (start_info.dwFlags & STARTF_USEFILLATTRIBUTE) {
    if (!SetConsoleTextAttribute(m_console_out_handle,
                                 start_info.dwFillAttribute)) {
      throw WinError("Failed to configure console use startup info",
                     GetLastError());
    }
  }
}

} // namespace winenv
