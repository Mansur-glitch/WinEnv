#pragma once
#include "color.hpp"
#include "config.hpp"
#include "log_window.hpp"

namespace winenv {
// Основной класс. Должен быть создан в одном экземпляре
class RootApp : public HasEventDispatcher, public EventDriven {
  class FileDropWnd : public BorderlessWindow, public HiddenWindow {
  public:
    inline FileDropWnd(HINSTANCE app_hinstance,
                       EventDispatcher &event_dispatcher,
                       WinWindow::Constructor wnd_config = {}) {
      delayed_construction(wnd_config.set_wnd_exstyle(WS_EX_ACCEPTFILES));
      construct(app_hinstance, event_dispatcher);
    }
  };

public:
  // Данные, которые передаются дочернему процессу через аргумент командной
  // строки
  struct CmdLineArg {
    RgbColor term_colors[16]{};
    char font_name[32]{};
    size_t font_size{};
  };
  // Подготовка к основному циклу программы.
  // Создает окно. Считывает параметры из .json файла.
  // Добавляет обработчики клавиш и сообщений. Регистрирует шрифты
  RootApp(HINSTANCE app_hinstance);
  ~RootApp();
  RootApp(const RootApp &other) = delete;
  RootApp &operator=(const RootApp &other) = delete;
  RootApp(RootApp &&other) noexcept = delete;
  RootApp &operator=(RootApp &&other) noexcept = delete;
  // Вход в основной цикл работы. Обрабатывает сообщения пока окно не закрыто
  void run();

private:
  // По параметрам из .json файла обновляет переменную среды PATH.
  // Устанавливает переменные среды XDG_HOME
  void configure_env();
  // Добавляет обработку сочетаний клавиш
  std::string configure_hotkeys();
  void add_con_font_to_registry(std::string font_name);
  // Создает дочерний процесс с настроенной консолью, в которой запускается
  // указанная команда
  void create_child_console(std::wstring_view launch_command);
  LRESULT spawn_cmd_khandler(const MSG &msg);
  LRESULT show_file_drop_khandler(const MSG &msg);
  // Вызывает завершение работы программы
  LRESULT exit_khandler(const MSG &msg);
  LRESULT browser_khandler(const MSG &msg);
  LRESULT file_drop_msg_handler(const MSG &msg);
  LRESULT log_wnd_2clk_handler(const MSG &msg);
  LRESULT paint_file_wnd(const MSG &msg);

  AppConfig m_config{};
  bool mf_found_fonts{false};
  bool mf_added_fonts{false};
  HINSTANCE m_hinstance{nullptr};
  LogWindow m_log_wnd;
  FileDropWnd m_file_wnd;
  Path m_programm_path;
  Path m_cmd_launch_dir;

  static constexpr const char *log_text_top =
      "Info\n\n";
  static constexpr const char *log_text_bottom =
      "\nDouble click to close window";
};

} // namespace winenv
