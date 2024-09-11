#pragma once
#include "color.hpp"
#include "event_driven.hpp"
#include "special_windows.hpp"

namespace winenv {
struct LogWindowFields {
  LogWindowFields(std::string text = {}, std::string font_name = {},
                  size_t font_size = {});
  ~LogWindowFields();
  LogWindowFields(const LogWindowFields &other) = delete;
  LogWindowFields &operator=(const LogWindowFields &other) = delete;
  LogWindowFields(LogWindowFields &&other) = delete;
  LogWindowFields &operator=(LogWindowFields &&other) = delete;
  void calculate_position(std::string_view text);

  std::string m_text;
  int m_border_width{3};
  int m_x{0}, m_y{0};
  int m_width{0}, m_height{0};
  size_t m_font_size{14};
  std::string m_font_name;
  HFONT m_font{nullptr};
};

class LogWindow : private LogWindowFields,
                  virtual public EventDriven,
                  public BorderlessWindow,
                  public HiddenWindow {
public:
  LogWindow(HINSTANCE app_hinstance, EventDispatcher &event_dispatcher,
            WinWindow::Constructor wnd_config = {}, std::string text = {},
            std::string font_name = {}, size_t font_size = {});
  void print(std::string_view text);

private:
  LRESULT paint(const MSG &msg);

  RgbColor m_fg_color{235, 207, 52};
  RgbColor m_bg_color{44, 42, 46};
  RgbColor m_brdr_color{29, 72, 242};
};

} // namespace winenv