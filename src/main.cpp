#include "font.hpp"
#include "root_app.hpp"
#include "win_console.hpp"
#include "win_proc.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

std::ostream* winenv::g_logger {nullptr};
std::mt19937 winenv::g_rand_gen(std::random_device{}());

using namespace winenv;

int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance,
               PWSTR raw_cmdline, int show_flag)
{
  std::wstring_view cmdline {raw_cmdline};
  setlocale(LC_ALL, "Russian");

  std::ofstream log_file;
  std::stringstream log_str;
  if (cmdline.empty()) {
      log_file.open("log.txt");

    if (! log_file.good()) {
      MessageBoxA(nullptr, "Couldn't open log.txt\n Process is probably already started\nUse HK_EXIT to finish it", "Error", MB_OK | MB_ICONERROR);
      return 0;
    } else {
      g_logger = &log_file;
    }
  } else {
    g_logger = &log_str;
  }
  try {
  STARTUPINFOW start_info;
  ZeroMemory(&start_info, sizeof(start_info));
  start_info.cb = sizeof(start_info);
  GetStartupInfoW(&start_info);
  // Недопустимы аргументы командной строки для главного процесса
  if (! cmdline.empty()) {
    // Дочерний процесс с правами администратора для
    // установки переменной реестра, с дополнительным консольным шрифтом
  if(cmdline.substr(0, 4) == L"font") {
      DWORD ret_code {};
      HKEY hkey = open_con_font_hkey(ret_code, KEY_WRITE);
      if (ret_code != ERROR_SUCCESS) {
        throw WinError("Failed to open registry for write", ret_code);
      }
      std::string font_name = narrow_string({cmdline.substr(5)});
      ret_code = RegSetValueExA( hkey,  g_confont_reg_valname, 0, REG_SZ,
                  reinterpret_cast<BYTE*>(font_name.data()), font_name.size() + 1);
      RegCloseKey(hkey);
      if (ret_code != ERROR_SUCCESS) {
        throw WinError("Failed to write console font registry value", ret_code);
      } else {
        MessageBoxA(nullptr, "Registry value with specified console font was successfully set", "INFO", MB_OK | MB_ICONINFORMATION);
        return 0;
      }
    // Дочерний процесс консоли распознаем по флагу, по факту,
    // не влиящему на процесс, т.к. приложение оконное
    } else if (! (start_info.dwFlags & CREATE_NEW_CONSOLE)) {
      throw std::runtime_error("Not allowed to pass command line arguments");
    }
  }
  if (cmdline.empty()) {
    RootApp root_app(hinstance);
    root_app.run();
  } else {
    WinConsole::allocate(start_info.lpTitle);
    WinConsole::set_title(L"C-M-D");

    auto [cmd_arg0, arg0_end]= cmd_arg_to<RootApp::CmdLineArg>(cmdline);
    bool is_font_avlbl = cmd_arg0.font_name[0] != '\0';
//    *g_logger<<"Font: "<<cmd_arg0.font_name<<" available="<<std::boolalpha<<is_font_avlbl<<std::noboolalpha<<'\n';
    if (is_font_avlbl) {
      WinConsole::get()->set_font(cmd_arg0.font_name, cmd_arg0.font_size);
    }
    WinConsole::get()->set_term_color_table(cmd_arg0.term_colors);
    WinConsole::get()->configure_use_startup_info(start_info);
    WinProcess::Constructor cmd_proc_ctor;
    if (arg0_end != cmdline.size()) {
       std::wstring_view launch_command = cmdline.substr(arg0_end + 1);
       cmd_proc_ctor.set_command_line_arguments( {launch_command.data()} );
    } 
    std::wstring cmd_path_str = get_cmd_path().wstring();
    cmd_proc_ctor.create(cmd_path_str, {});
  }
  } catch (WinError& err) {
    *g_logger<<err.what()<<'\n';
  } catch (std::runtime_error& err) {
    *g_logger<<"[Runtime error] "<<err.what()<<'\n';
  } catch (std::exception& ex) {
    *g_logger<<"[Standard exception] "<<ex.what()<<'\n';  
  } catch (...) {
    *g_logger<<"[Unknown exception]\n";
  }

  std::string str {std::move(log_str).str()};
  if (! str.empty()) {
    MessageBoxA(nullptr, str.c_str(), "Error", MB_OK | MB_ICONERROR);
  }
  return 0;
}
