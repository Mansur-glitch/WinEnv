#pragma once
#include "common.hpp"

#include <windows.h>

#include <filesystem>
#include <string_view>

 
namespace winenv
{
  // Зарегистрирован ли шрифт в системе
  bool is_font_available(std::string_view font_name);

  enum class FontAction : bool
  {
    remove = 0,
    add = 1
  };

  // В зависимости от параметра шаблона регистрирует шрифты или удаляет шрифты из системы.
  // Обрабатывает ВСЕ файлы в указанной папке.
  // Возвращает число успешно зарегистрированных/удаленных файлов шрифтов
  template<FontAction action>
  size_t manage_font_resouces(const Path& fonts_directory)
  {
    size_t n_resources_succeeded {0};
    // Для каждого файла
    for (const auto & entry : std::filesystem::directory_iterator(fonts_directory)) {
      std::string file_str = entry.path().string();
      if constexpr (action == FontAction::add) {
        n_resources_succeeded += static_cast<bool>(AddFontResourceA(file_str.c_str()));
      } else {
        n_resources_succeeded += static_cast<bool>(RemoveFontResourceA(file_str.c_str()));
      }
    }
    if (n_resources_succeeded > 0) {
      // Оповещение всех оконных программ
      SendNotifyMessageA(HWND_BROADCAST, WM_FONTCHANGE, {}, {});
    }
    return n_resources_succeeded;
  }
  
  static constexpr const char* g_confont_reg_valname = "winenv";

  HKEY open_con_font_hkey(DWORD& res_code, REGSAM access_rights = KEY_ALL_ACCESS);
  std::string read_registry_string(HKEY hkey, const char* value_name, DWORD& ret_code);
} // namespace winenv
