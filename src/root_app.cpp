#include "root_app.hpp"
#include "font.hpp"
#include "win_proc.hpp"

#include "log_window.hpp"


namespace {
bool add_font(std::string_view font_name)
{
  return winenv::manage_font_resouces<winenv::FontAction::add>
    (std::filesystem::current_path() / "fonts") > 0;
}
}

namespace winenv
{
RootApp::RootApp(HINSTANCE app_hinstance) 
: 
  m_config{"config.json"},
  mf_found_fonts{is_font_available(m_config.font_name)},
  mf_added_fonts{!mf_found_fonts && add_font(m_config.font_name)},
  m_hinstance{app_hinstance},
  m_log_wnd{
    m_hinstance, m_dispatcher,
    WinWindow::Constructor()
      .set_wnd_exstyle(WS_EX_TOOLWINDOW)
      .set_clss_style(CS_DBLCLKS)
      .add_message_handling(WM_LBUTTONDBLCLK, method_handle(&RootApp::log_wnd_2clk_handler))
      .add_message_handling(WM_NCLBUTTONDBLCLK, method_handle(&RootApp::log_wnd_2clk_handler)),
    "",
    mf_found_fonts || mf_added_fonts ? m_config.font_name : "",
    m_config.font_size * 15 / 10
  },
  m_file_wnd{
    m_hinstance, m_dispatcher,
    WinWindow::Constructor()
    .set_wnd_exstyle(WS_EX_TOOLWINDOW)
    .set_size(200, 200)
    .add_message_handling(WM_DROPFILES, method_handle(&RootApp::file_drop_msg_handler))
    .add_message_handling(WM_PAINT, method_handle(&RootApp::paint_file_wnd))
  },
  m_programm_path{get_programm_path()}
{
  EventDriven::reasign_owner(this);
  configure_env();
  std::string warning_str;
  
  warning_str += configure_hotkeys();
  add_con_font_to_registry(m_config.font_name);

  if (!warning_str.empty()) {
    warning_str = log_text_top + warning_str + log_text_bottom;
    m_log_wnd.print(warning_str);
    m_log_wnd.show(true);
  } else {
    m_log_wnd.print("Hello!");
    m_log_wnd.show_for(1'000);
  }
}

RootApp::~RootApp()
{
  *g_logger<<"Root app is being destructed"<<std::endl;
  if (mf_added_fonts) {
    manage_font_resouces<FontAction::remove>(std::filesystem::current_path() / "fonts");      
  } 
  *g_logger<<"Root app was destructed"<<std::endl;
}

void RootApp::run()
{
  try {
    while (! m_file_wnd.is_quit()) {
      m_dispatcher.dispatch();
      Sleep(50);
    }
    m_log_wnd.print("Exiting...");
    m_log_wnd.show_for(1'000);
    while (m_log_wnd.is_shown()) {
      m_dispatcher.dispatch({0, 0}, m_log_wnd.get_hwnd());
    }
  } catch (std::exception& ex) {
    std::string log_msg = std::string(log_text_top) + ex.what() + log_text_bottom;
    m_log_wnd.print(log_msg);
    m_log_wnd.show(true);
    while (m_log_wnd.is_shown()) {
      m_dispatcher.dispatch({0, 0}, m_log_wnd.get_hwnd());
    }
    throw ex;
  }

}

void RootApp::configure_env()
{
  using std::filesystem::canonical;
  Path abs_root_path = canonical(m_programm_path / m_config.root_offset);
  std::string str_root_path = abs_root_path.string();
  set_env_variable("flash_root", str_root_path);
  m_cmd_launch_dir = abs_root_path; // Запускаем cmd в корне
  Path abs_apps_dir = canonical(abs_root_path / m_config.apps_dir);
  Path abs_apps_data = canonical(abs_root_path / m_config.apps_data);
  Path abs_xdg_home = canonical(abs_root_path / m_config.xdg_home);
  // Переменные, используемые linux приложениями как директории хранения данных
  std::string str_xdg_home = abs_xdg_home.string();
  set_env_variable("XDG_CONFIG_HOME", str_xdg_home);
  set_env_variable("XDG_DATA_HOME", str_xdg_home);
  set_env_variable("XDG_RUNTIME_HOME", str_xdg_home);
  set_env_variable("XDG_STATE_HOME", str_xdg_home);

  // Объединяем пути к приложениям
  std::string new_path {};
  for (auto& bin_path : m_config.apps_bin_paths) {
    new_path += canonical(abs_apps_dir / bin_path).string() + ';';
  }
  new_path += get_env_variable("PATH");
  set_env_variable("PATH", new_path);
}

std::string RootApp::configure_hotkeys()
{ 
    std::string log_msg;
    // Совершает две попытки добавить обработку сочетания клавиш.
    // Меняет комбинацию клавиш после первой неудачной попытки 
    auto add_key_handling_with_backup = [this, &log_msg](Hotkey hk, auto handler) -> void
    {
      static constexpr Hotkey::Modifier backup_modifier = 
      Hotkey::Modifier::ctrl |
      Hotkey::Modifier::win |
      Hotkey::Modifier::alt |
      Hotkey::Modifier::norepeat;
      try {
        m_dispatcher.add_hotkey_handling(hk, handler);
      } catch (WinError err) {
        char keycode = static_cast<char>(hk.get_key_code());
        try {
          Hotkey backup_hk {keycode, backup_modifier}; 
          m_dispatcher.add_hotkey_handling(backup_hk, handler);
          log_msg += "Failed to register \"" + hk.to_stdstring()+ "\" hotkey. \"" 
          + backup_hk.to_stdstring() +"\" used instead.\n";
        } catch (...) { // Не удалось во второй раз
          throw err;
        }
      }
    };
    add_key_handling_with_backup(m_config.exit_hk, method_handle(&RootApp::exit_khandler));
    add_key_handling_with_backup(m_config.spawn_cmd_hk, method_handle(&RootApp::spawn_cmd_khandler));
    add_key_handling_with_backup(m_config.file_pick_hk, method_handle(&RootApp::show_file_drop_khandler));
    add_key_handling_with_backup("nr alt T"_hk, method_handle(&RootApp::test_khandler));
    return log_msg;
}

void RootApp::add_con_font_to_registry(std::string font_name)
{

  DWORD ret_code {};
  HKEY hkey = open_con_font_hkey(ret_code, KEY_READ);
  if (ret_code != ERROR_SUCCESS) {
    throw WinError("Failed to open registry for read", ret_code);
  }
  
  std::string prev_font = read_registry_string(hkey, g_confont_reg_valname, ret_code); 
  RegCloseKey(hkey);
  if (ret_code == ERROR_SUCCESS && prev_font == font_name) {
    return;
  } else if (ret_code != ERROR_SUCCESS && ret_code != ERROR_FILE_NOT_FOUND) {
    throw WinError("Failed to read console font registry key", ret_code);
  }

  hkey = open_con_font_hkey(ret_code, KEY_WRITE);
  if (ret_code != ERROR_SUCCESS) {
    std::string warning_msg = "Failed to open console font registry key for write\n"; 
    warning_msg += "Error code: ";
    warning_msg += std::to_string(ret_code);
    warning_msg += "\nTrying elevate privileges..."; 
    m_log_wnd.print(warning_msg);
    m_log_wnd.show(true);
    while (m_log_wnd.is_shown()) {
      m_dispatcher.dispatch({0, 0}, m_log_wnd.get_hwnd());
    }
    std::wstring cmd_arg = L"font " + widen_string(font_name);
    run_as_admin(m_programm_path.wstring(), cmd_arg);
    return;
  } 

  ret_code = RegSetValueEx( hkey, g_confont_reg_valname, 0, REG_SZ,
              reinterpret_cast<BYTE*>(font_name.data()), font_name.size() + 1);
  RegCloseKey(hkey);
  if (ret_code != ERROR_SUCCESS) {
    throw WinError("Failed to write console font registry value", ret_code);
  }
}

void RootApp::create_child_console(std::wstring_view launch_command)
{
  char title_narrow[g_max_file_path];
  // Уникальный заголовок, чтобы предотвратить ошибки при поиске окна консоли
  get_unique_window_title(title_narrow, g_max_file_path);
  std::wstring title_wide = widen_string(title_narrow);

  CmdLineArg console_app_args;
  std::copy(m_config.term_color_table.begin(),
            m_config.term_color_table.end(),
            console_app_args.term_colors);
  std::copy(m_config.font_name.begin(),
            m_config.font_name.end(),
            console_app_args.font_name);
  console_app_args.font_size = m_config.font_size;
  // В командной строку WinMain входит последовательность символов после первого пробела
  std::wstring cmd_args = L"_ " + cmd_arg_from(console_app_args);
  if (! launch_command.empty()) {
    cmd_args += L' ';
    cmd_args += launch_command.data();
  }
  std::wstring program_path_str {m_programm_path.wstring()};
  std::wstring launch_directory {m_cmd_launch_dir.wstring()};
  WinProcess proc = WinProcess::Constructor()
    .set_command_line_arguments(cmd_args.c_str())
    .set_startup_directory(launch_directory)
    .set_console_color(m_config.foreground, m_config.background)
    .set_window_position(0, 0)
    .set_console_size_chr(m_config.columns, m_config.rows)
    .add_startup_flags(CREATE_NEW_CONSOLE) // По флагу отличаем дочерний процесс
    .create(program_path_str, title_wide)
  ;
}

LRESULT RootApp::spawn_cmd_khandler(const MSG &msg)
{
  create_child_console(L"");
  return 0;
}

LRESULT RootApp::show_file_drop_khandler(const MSG& msg)
{
  m_file_wnd.show_for(10'000);
  return 0;
}

LRESULT RootApp::exit_khandler(const MSG& msg)
{
  DestroyWindow(m_file_wnd.get_hwnd());
  return 0;
}

LRESULT RootApp::test_khandler(const MSG &msg)
{
  if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    m_log_wnd.print("No text info available in the clipboard");
    m_log_wnd.show_for(1'000);
    return 0; 
  }
  if (!OpenClipboard(nullptr)) {
    m_log_wnd.print("Failed to open the clipboard");
    m_log_wnd.show_for(1'000);
    return 0; 
  }
  // https://www.google.com/search?q=
  std::wstring launch_command = L"/C start /B chrome.exe";
  LPWSTR clipboard_text {nullptr};
  HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT); 
  if (hglb != nullptr) { 
      clipboard_text = reinterpret_cast<LPWSTR>(GlobalLock(hglb)); 
      if (clipboard_text != nullptr) {
        launch_command += L' ';
        launch_command += clipboard_text;
        GlobalUnlock(hglb); 
      } 
  } 
  CloseClipboard(); 

  std::wstring program_path_str = get_cmd_path().wstring();
  std::wstring launch_directory {m_cmd_launch_dir.wstring()};
  WinProcess proc = WinProcess::Constructor()
    .set_command_line_arguments(launch_command.c_str())
    .set_startup_directory(launch_directory)
    .create(program_path_str, L"CHROMIUM")
  ;
  return 0;
}

LRESULT RootApp::file_drop_msg_handler(const MSG &msg)
{
  m_file_wnd.show(false);

	std::wstring file_name;
  file_name.resize(g_max_file_path);
  std::wstring launch_command {L"/K nvim "};
	HDROP hdrop = (HDROP)msg.wParam;
  // Получаем число переданных файлов через специальное значение параметра
	UINT n_files = DragQueryFileW(hdrop, 0xFFFFFFFF, file_name.data(), g_max_file_path);
	for(UINT i = 0; i < n_files; ++i)
	{
		DragQueryFileW(hdrop, i, file_name.data(), g_max_file_path);
    launch_command += file_name.c_str();
    launch_command += L' ';
	}
	DragFinish(hdrop);
  launch_command.resize(launch_command.size() - 1); // Отсекаем последний пробел
  create_child_console(launch_command);
  return 0;
}

LRESULT RootApp::log_wnd_2clk_handler(const MSG &msg)
{
  m_log_wnd.show(false);
  return 0;
}

LRESULT RootApp::paint_file_wnd(const MSG &msg)
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(m_file_wnd.get_hwnd(), &ps);
  int width = ps.rcPaint.right, height = ps.rcPaint.bottom;
  RECT draw_area {0, 0, width, height};
  HBRUSH bg_brush = CreateSolidBrush("60,60,60"_rgb);
  HPEN pen = CreatePen(PS_SOLID, 3, "210,210,210"_rgb);
  SelectObject(hdc, pen);
  FillRect(hdc, &draw_area, bg_brush);
  DrawEdge(hdc, &draw_area, BDR_SUNKENINNER, BF_RECT | BF_MONO);

  int smallest_side = std::min(width, height);
  if (smallest_side < 30) {
    DeleteObject(bg_brush);
    DeleteObject(pen);
    GetStockObject(DC_PEN);
    EndPaint(m_file_wnd.get_hwnd(), &ps);
    return 0;
  }
  
  int dp = smallest_side / 10;
  int cx = width / 2, cy = height / 2;
  std::array ico_vetrexes {
    POINT{cx - 3 * dp, cy - 4 * dp},
    POINT{cx + 1 * dp, cy - 4 * dp},
    POINT{cx + 1 * dp, cy - 2 * dp},
    POINT{cx + 3 * dp, cy - 2 * dp},
    POINT{cx + 3 * dp, cy + 4 * dp},
    POINT{cx - 3 * dp, cy + 4 * dp},
    POINT{cx - 3 * dp, cy - 4 * dp},

    POINT{cx + 1 * dp, cy - 4 * dp},
    POINT{cx + 3 * dp, cy - 2 * dp},

    POINT{cx - 2 * dp, cy - 1 * dp},
    POINT{cx + 2 * dp, cy - 1 * dp},

    POINT{cx - 2 * dp, cy},
    POINT{cx + 2 * dp, cy},

    POINT{cx - 2 * dp, cy + 1 * dp},
    POINT{cx, cy + 1 * dp},
  };
  std::array<DWORD, 5> connected_vertexes = {7, 2, 2, 2, 2};
  PolyPolyline(hdc, ico_vetrexes.data(), connected_vertexes.data(), connected_vertexes.size());

  DeleteObject(bg_brush);
  DeleteObject(pen);
  GetStockObject(DC_PEN);
  EndPaint(m_file_wnd.get_hwnd(), &ps);
  return 0;
}
} // namespace winenv
