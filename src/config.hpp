#pragma once
#include "color.hpp"
#include "common.hpp"
#include "hkey.hpp"

#include <boost/json.hpp>

#include <fstream>

namespace winenv {
// Считывает файл в json объект
boost::json::value parse_config(std::string_view file_name);
// Параметры приложения, хранящиеся в .json файле
struct AppConfig {
  AppConfig() = default;
  AppConfig(std::string_view file_name);

  Path root_offset;
  Path apps_dir;
  Path apps_data;
  Path xdg_home;
  std::vector<Path> apps_bin_paths;
  std::vector<RgbColor> term_color_table;
  ConsoleColor foreground;
  ConsoleColor background;
  std::string font_name;
  size_t font_size;
  size_t columns;
  size_t rows;
  Hotkey spawn_cmd_hk{'A'};
  Hotkey launch_browser_hk{'B'};
  Hotkey file_pick_hk{'C'};
  Hotkey exit_hk{'D'};
};

} // namespace winenv

// Вспомогательные функции для приведения json объектов к типу AppConfig
namespace boost::json {
winenv::Path tag_invoke(value_to_tag<winenv::Path>, value const &jv);
winenv::ConsoleColor tag_invoke(value_to_tag<winenv::ConsoleColor>,
                                value const &jv);
winenv::RgbColor tag_invoke(value_to_tag<winenv::RgbColor>, value const &jv);
winenv::Hotkey tag_invoke(value_to_tag<winenv::Hotkey>, value const &jv);
// Для приведения к типу AppConfig. Определяет названия соответсвующих
// параметров в .json файле
winenv::AppConfig tag_invoke(boost::json::value_to_tag<winenv::AppConfig>,
                             boost::json::value const &jv);
} // namespace boost::json
