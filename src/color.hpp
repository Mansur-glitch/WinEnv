#pragma once
#include "utils.hpp"

namespace winenv {
// Цвета используемые в консоли windows
enum class ConsoleColor : unsigned char {
  black = 0b0000,
  blue = FOREGROUND_BLUE,
  green = FOREGROUND_GREEN,
  red = FOREGROUND_RED,
  cyan = blue | green,
  magenta = blue | red,
  yellow = green | red,
  white = blue | green | red,
  i_black = FOREGROUND_INTENSITY,
  i_blue = i_black | blue,
  i_green = i_black | green,
  i_red = i_black | red,
  i_cyan = i_black | cyan,
  i_magenta = i_black | magenta,
  i_yellow = i_black | yellow,
  i_white = i_black | white
};

// Для получения комбинации цвета текста и фона, используемой в консоли
constexpr DWORD console_text_attribute(ConsoleColor foreground,
                                       ConsoleColor background) {
  return static_cast<DWORD>(foreground) | static_cast<DWORD>(background) << 4;
}

// Представление цвета в трех каналах
class RgbColor {
public:
  using Channel = unsigned char;
  static constexpr size_t n_channels = 3;
  constexpr RgbColor(Channel red = 0, Channel green = 0,
                     Channel blue = 0) noexcept
      // В windows младший байт - синий, старший - красный, т.е. обратный
      // порядок
      : m_channels{blue, green, red} {}
  constexpr Channel &operator[](size_t channel_num) {
    // Учитывая обратный порядок
    return m_channels[n_channels - 1 - channel_num];
  }

  constexpr Channel operator[](size_t channel_num) const {
    // Учитывая обратный порядок
    return m_channels[n_channels - 1 - channel_num];
  }

  // Тип для получения строкового представления цвета
  struct CString {
    // Максимальная длина строки, выражающая RGB цвет
    static constexpr size_t max_size = sizeof("255,255,255");
    char m_value[max_size]{};
  };

  // Преобразование в Си-строку
  constexpr CString to_cstring() const noexcept {
    CString str{};
    size_t i = 0;
    for (size_t ch_num = 0; ch_num < 3; ++ch_num) {
      Channel channel_val = m_channels[ch_num];
      Channel hundreds = channel_val / 100;
      if (hundreds > 0) {
        str.m_value[i++] = '0' + hundreds;
      }
      channel_val %= 100;
      Channel dozens = channel_val / 10;
      if (dozens > 0) {
        str.m_value[i++] = '0' + dozens;
      }
      Channel units = channel_val % 10;
      str.m_value[i++] = '0' + units;
      str.m_value[i++] = ',';
    }
    return str;
  }

  inline std::string to_stdstring() const { return {to_cstring().m_value}; }

  // Неявное приведение для использования в вызовах winapi
  constexpr operator COLORREF() const noexcept {
    return m_channels[2] | m_channels[1] << 8 | m_channels[0] << 16;
  }

private:
  Channel m_channels[n_channels];
};

// Более короткий способ создания переменных RgbColor
// Например:
// RgbColor my_color = "255,67,67"_rgb;
constexpr RgbColor operator""_rgb(const char *str, size_t sz) {
  size_t i = 0;
  char cur_sy = '\0';
  RgbColor color{};
  unsigned int cur_channel_val = 0, channel_num = 0;
  do {
    cur_sy = str[i++];
    // Цифры формируют значение канала
    if (cur_sy >= '0' && cur_sy <= '9') {
      cur_channel_val = cur_channel_val * 10 + cur_sy - '0';
      // Запятые и символ конца строки ограничивают величины каналов
    } else if (cur_sy == ',' || cur_sy == '\0') {
      if (cur_channel_val > 255) {
        throw WrongLiteralFormat("Channel value must not be greater than 255");
      }
      color[channel_num] = cur_channel_val;
      ++channel_num;
      cur_channel_val = 0;
    } else { // Не принимаем никакие другие символы
      throw WrongLiteralFormat("Invalid character");
    }
  } while (cur_sy != '\0');
  return color;
}

} // namespace winenv
