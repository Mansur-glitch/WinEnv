#include "font.hpp"
#include "utils.hpp"

#include "winreg.h"
//#define _WIN32_WINNT_VISTA

namespace {
// Обратный вызов при нахождении подходящего шрифта.
// Последний аргумент - произвольный; используем как указатель  
int font_search_success(const LOGFONTA * font_criteria,
 const TEXTMETRICA* metric, DWORD font_type, LPARAM success_flag_ptr)
{
  *reinterpret_cast<bool*>(success_flag_ptr) = true;
  return 0; // Прекращаем дальнейший поиск
}
}

namespace winenv
{

bool is_font_available(std::string_view font_name)
  {
    // Контекст рисования рабочего стола
    HDC global_dc = GetDC(NULL);
    LOGFONTA font_criteria;
    ZeroMemory(&font_criteria, sizeof(font_criteria));
    // Три параметра используемых при поиске шрифта
    font_criteria.lfCharSet = DEFAULT_CHARSET; // Все наборы символов
    // Отсеиваем по имени
    std::copy(font_name.begin(), font_name.end(), font_criteria.lfFaceName);
    // Из MSDN: Must be set to zero for all language versions of the operating system.
    font_criteria.lfPitchAndFamily = 0;

    bool success_flag {false};
    EnumFontFamiliesExA(global_dc, &font_criteria,
     font_search_success, reinterpret_cast<LPARAM>(&success_flag), {});
    return success_flag;
}


HKEY open_con_font_hkey(DWORD& res_code, REGSAM access_rights)
{
  static constexpr const char* subkey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont";
  HKEY  hkey {nullptr};
  res_code = RegOpenKeyExA(
          HKEY_LOCAL_MACHINE,
          subkey,
          0,
          access_rights,
          &hkey);
  return hkey;
}


std::string read_registry_string(HKEY hkey, const char* value_name, DWORD& ret_code)
{
  static constexpr DWORD buffer_size {1024};
  char buf[buffer_size] {};
  DWORD data_size {buffer_size};

  ret_code = RegGetValueA(
              hkey,
              0,
              value_name,
              2, // PRF_RT_REG_SZ
              0,
              buf,
              &data_size);

  return {buf};
}

} // namespace winenv
