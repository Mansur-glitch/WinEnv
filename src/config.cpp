#include "config.hpp"

namespace winenv
{
boost::json::value parse_config(std::string_view file_name)
{
  std::ifstream config_file(file_name.data());
  if (! config_file.good()) {
    throw std::runtime_error("Failed to open config file");
  }
  boost::json::parse_options opts;
  opts.allow_comments = true;
  return boost::json::parse(config_file, {}, opts);
}

AppConfig::AppConfig(std::string_view file_name)
{
  *this =  boost::json::value_to<AppConfig>(parse_config(file_name));
}
} // namespace winenv

namespace boost::json
{
winenv::Path tag_invoke(value_to_tag<winenv::Path>, value const& jv)
{
  return jv.as_string().c_str();
}

winenv::ConsoleColor tag_invoke(value_to_tag<winenv::ConsoleColor>, value const& jv)
{
  // Используем as_int64, а не as_uint64 т.к. для парсера числа меньше INT64_MAX имеют этот тип
  return static_cast<winenv::ConsoleColor>(jv.as_int64());
}

winenv::RgbColor tag_invoke(value_to_tag<winenv::RgbColor>, value const& jv)
{
  string jstring = jv.as_string();
  return winenv::operator""_rgb(jstring.c_str(), jstring.size()); 
}

winenv::Hotkey tag_invoke(value_to_tag<winenv::Hotkey>, value const& jv)
{
  string jstring = jv.as_string();
  return winenv::operator""_hk(jstring.c_str(), jstring.size()); 
}

winenv::AppConfig tag_invoke(boost::json::value_to_tag<winenv::AppConfig>, boost::json::value const& jv)
{
  using namespace winenv;
  using boost::json::value_to;

  boost::json::object jobj = jv.as_object();
  AppConfig c;

  c.root_offset = value_to<Path>(jobj["ROOT_OFFSET"]);
  c.apps_dir = value_to<Path>(jobj["APPS_DIR"]);
  c.apps_data = value_to<Path>(jobj["APPS_DATA"]);
  c.xdg_home = value_to<Path>(jobj["XDGHOME"]);
  c.apps_bin_paths = value_to<std::vector<Path>>(jobj["APPS_BIN_PATHS"]);
  c.term_color_table = value_to<std::vector<RgbColor>>(jobj["TERM_COLOR_TABLE"]);
  c.foreground = value_to<ConsoleColor>(jobj["COLOR_FG"]);
  c.background = value_to<ConsoleColor>(jobj["COLOR_BG"]);
  c.font_name = jobj["FONT_NAME"].as_string();
  c.font_size = jobj["FONT_SIZE"].as_int64();
  c.columns = jobj["COLUMNS"].as_int64();
  c.rows = jobj["ROWS"].as_int64();
  c.spawn_cmd_hk = value_to<Hotkey>(jobj["HK_SPAWN_CMD"]);
  c.launch_browser_hk = value_to<Hotkey>(jobj["HK_LAUNCH_BROWSER"]);
  c.file_pick_hk = value_to<Hotkey>(jobj["HK_FILE_PICK"]);
  c.exit_hk = value_to<Hotkey>(jobj["HK_EXIT"]);

  return c;
}
} // namespace boost::json