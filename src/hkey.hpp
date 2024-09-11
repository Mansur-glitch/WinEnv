#pragma once
#include "utils.hpp"

#include <string_view>

namespace winenv {

// Cочетание клавиш для привязки к заданным обработчикам
// event_dispatcher.add_key_handling(Hotkey('A', Hotkey::Modifier::alt),
// handler)); Удобнее пользоваться литералом
// event_dispatcher.add_key_handling("alt A"_hk, handler));
class Hotkey {
public:
  // Клавиши "модификаторы"
  // Возможно комбинировать: Modifier::alt | Modifier::shift
  enum class Modifier : UINT {
    none = 0,
    alt = MOD_ALT,
    ctrl = MOD_CONTROL,
    shift = MOD_SHIFT,
    win = MOD_WIN,
    norepeat = MOD_NOREPEAT // Зажатие клавиши - это одно нажатие
  };

  constexpr Hotkey(char key_code, Modifier modifiers = Modifier::none)
      : m_key_code{static_cast<UINT>(key_code)}, m_modifiers{modifiers} {
    if (!validate_key_code(key_code)) {
      throw std::runtime_error(
          "Hotkey c-tor acceptable key codes are chars from [A, Z]");
    }
  }

  constexpr UINT get_key_code() const noexcept { return m_key_code; }

  constexpr Modifier get_modifiers() const noexcept { return m_modifiers; }

  // Тип для получения строкового представления сочетания клавиш
  struct CString {
    // Длина получаемой строки не больше чем длина предопределнной строки
    static constexpr size_t max_size = sizeof("nr ctrl shift alt win A");
    char m_value[max_size]{};
  };

  constexpr CString to_cstring() const noexcept;

  inline std::string to_stdstring() const { return {to_cstring().m_value}; }

  friend constexpr Hotkey operator""_hk(const char *str, size_t sz);

private:
  struct ModifierWithBriefName {
    std::string_view m_brief{};
    Modifier m_modifier{};
  };
  // Для вывода из литерала и преобразования в строку названий модификаторов
  static constexpr ModifierWithBriefName mods_brief_names[] = {
      {"alt", Hotkey::Modifier::alt},     {"ctrl", Hotkey::Modifier::ctrl},
      {"shift", Hotkey::Modifier::shift}, {"win", Hotkey::Modifier::win},
      {"nr", Hotkey::Modifier::norepeat},
  };

  // Является ли код клавиши допустимым
  static constexpr bool validate_key_code(int code) {
    // Windows коды клавиш анг. букв совпадают с кодами ASCII
    return ('A' <= code && code <= 'Z');
  }

  UINT m_key_code;
  Modifier m_modifiers;
};

constexpr Hotkey::Modifier operator|(Hotkey::Modifier left,
                                     Hotkey::Modifier right) noexcept {
  return static_cast<Hotkey::Modifier>(static_cast<UINT>(left) |
                                       static_cast<UINT>(right));
}

constexpr Hotkey::Modifier operator&(Hotkey::Modifier left,
                                     Hotkey::Modifier right) noexcept {
  return static_cast<Hotkey::Modifier>(static_cast<UINT>(left) &
                                       static_cast<UINT>(right));
}

constexpr bool operator==(Hotkey left, Hotkey right) noexcept {
  return left.get_key_code() == right.get_key_code() &&
         left.get_modifiers() == right.get_modifiers();
}

constexpr bool operator!=(Hotkey left, Hotkey right) noexcept {
  return !(left == right);
}

constexpr Hotkey::CString Hotkey::to_cstring() const noexcept {
  constexpr char delimeter = ' ';

  CString str{};
  size_t i = 0;
  for (auto &mod : mods_brief_names) {
    // Включает ли комбинация модификатор данный модификатор
    if ((m_modifiers & mod.m_modifier) != Modifier::none) {
      copy_from_string_view(mod.m_brief, str.m_value + i);
      i += mod.m_brief.size();
      str.m_value[i] = delimeter;
      ++i;
    }
  }
  str.m_value[i] = static_cast<char>(m_key_code);
  return str;
}

// Удобное представление комбинаций клавиш.
// Коды клавиш - только заглавные буквы.
// Например:
// auto hotkey = "nr ctrl win alt A"_hk;
constexpr Hotkey operator""_hk(const char *str, size_t sz) {
  constexpr char delimeter{' '};

  Hotkey::Modifier key_modifiers{};
  char key_code{0};
  std::string_view strv{str};

  size_t token_pos{0};
  while (token_pos < sz) {
    size_t new_delim_pos = strv.find(delimeter, token_pos);
    // Нет больше разделителей
    if (new_delim_pos == std::string_view::npos) {
      new_delim_pos = sz;
    }
    std::string_view token = strv.substr(token_pos, new_delim_pos - token_pos);

    if (token.empty()) { // Вся строка - символы разделители
      throw WrongLiteralFormat("Literal contains only whitespaces");
    } else if (token.size() == 1) { // Код клавиши ?
      key_code = token[0];
      // Допустим только один код клавиши в самом конце строки
      if (new_delim_pos != sz) {
        throw WrongLiteralFormat("Key code character must be last");
      }
    } else { // Модификатор ?
      bool is_valid_mod = false;
      for (auto &mod : Hotkey::mods_brief_names) {
        if (token == mod.m_brief) {
          // Допустимо только одно вхождение каждого модификатора
          if ((key_modifiers & mod.m_modifier) != Hotkey::Modifier::none) {
            throw WrongLiteralFormat("Modifier duplicate met");
          } else {
            key_modifiers = key_modifiers | mod.m_modifier;
            is_valid_mod = true;
            break;
          }
        }
      }
      if (!is_valid_mod) {
        throw WrongLiteralFormat("Unknown modifier");
      }
    }
    token_pos = new_delim_pos + 1;
  } // while(...)
  return Hotkey{key_code, key_modifiers};
}

} // namespace winenv

namespace std {

// Для работы "unordered" конейнеров
template <> struct hash<winenv::Hotkey> {
  size_t operator()(const winenv::Hotkey &hk) const noexcept {
    std::size_t seed = 0;
    winenv::boost_hash_combine(seed, hk.get_key_code());
    winenv::boost_hash_combine(seed, hk.get_modifiers());
    return seed;
  }
};

} // namespace std