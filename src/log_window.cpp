#include "log_window.hpp"

namespace {
RECT get_text_area(std::string_view text, HFONT font) {
  static constexpr size_t n_columns{80};
  HDC hdc = GetDC(nullptr);
  HFONT original_font{nullptr};
  if (font != nullptr) {
    original_font = (HFONT)SelectObject(hdc, font);
  }
  TEXTMETRIC txt_metric;
  GetTextMetrics(hdc, &txt_metric);
  int target_width = txt_metric.tmMaxCharWidth * n_columns;
  RECT text_area{0, 0, target_width, 0};
  int text_height =
      DrawTextA(hdc, text.data(), text.size(), &text_area, DT_CALCRECT);
  if (text_height == 0) {
    text_area = {0, 0, 0, 0};
  }
  if (font != nullptr) {
    SelectObject(hdc, original_font);
  }
  ReleaseDC(nullptr, hdc);

  return text_area;
}

RECT get_draw_area(RECT initial_area, int border_width) {
  int screen_width = GetSystemMetrics(SM_CXSCREEN);
  int screen_height = GetSystemMetrics(SM_CYSCREEN);
  initial_area.right += border_width * 2;
  initial_area.bottom += border_width * 2;
  initial_area.right =
      initial_area.right > screen_width ? screen_width : initial_area.right;
  initial_area.bottom =
      initial_area.bottom > screen_height ? screen_height : initial_area.bottom;
  int x = (screen_width - initial_area.right) / 2;
  int y = (screen_height - initial_area.bottom) / 2;
  initial_area.left = x;
  initial_area.top = y;
  initial_area.right += x;
  initial_area.bottom += y;
  return initial_area;
}
} // namespace

namespace winenv {

LogWindowFields::LogWindowFields(std::string text, std::string font_name,
                                 size_t font_size)
    : m_text{std::move(text)}, m_font_name{std::move(font_name)} {
  if (!m_font_name.empty()) {
    m_font = CreateFontA(font_size ? font_size : 14,
                         0, // автоматический подбор ширины
                         0,         // поворот строки
                         0,         // поворот символов
                         FW_NORMAL, // вес (жирность)
                         false,     // курсив
                         false,     // подчеркивание
                         false,     // выделение
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                         FIXED_PITCH |
                             FF_MODERN, // моноширинный шрифт предпочтительнее
                         m_font_name.c_str());
  }
  calculate_position(m_text);
}

LogWindowFields::~LogWindowFields() {
  if (m_font) {
    DeleteObject(m_font);
    m_font = nullptr;
  }
}

void LogWindowFields::calculate_position(std::string_view text) {
  RECT text_area = get_text_area(text, m_font);
  RECT draw_area = get_draw_area(text_area, m_border_width);
  m_x = draw_area.left;
  m_y = draw_area.top;
  m_width = draw_area.right - draw_area.left;
  m_height = draw_area.bottom - draw_area.top;
}

LogWindow::LogWindow(HINSTANCE app_hinstance, EventDispatcher &event_dispatcher,
                     WinWindow::Constructor wnd_config, std::string text,
                     std::string font_name, size_t font_size)
    : LogWindowFields{std::move(text), std::move(font_name), font_size} {
  EventDriven::reasign_owner(this);
  delayed_construction(
      wnd_config.set_wnd_exstyle(WS_EX_TOOLWINDOW)
          .set_position(m_x, m_y)
          .set_size(m_width, m_height)
          .add_message_handling(WM_PAINT, method_handle(&LogWindow::paint)));
  construct(app_hinstance, event_dispatcher);
}

void LogWindow::print(std::string_view text) {
  m_text = text.data();
  calculate_position(m_text);
  SetWindowPos(m_hwnd, HWND_TOP, m_x, m_y, m_width, m_height, 0);
  RECT redraw_area = {0, 0, m_width, m_height};
  InvalidateRect(m_hwnd, &redraw_area, false);
}

LRESULT LogWindow::paint(const MSG &msg) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(m_hwnd, &ps);
  SetTextColor(hdc, m_fg_color);
  SetBkColor(hdc, m_bg_color);
  RECT border_area{0, 0, ps.rcPaint.right, ps.rcPaint.bottom};
  HBRUSH text_bg_brush = CreateSolidBrush(m_bg_color);
  FillRect(hdc, &border_area, text_bg_brush);
  DeleteObject(text_bg_brush);
  HBRUSH border_brush = CreateSolidBrush(m_brdr_color);
  for (size_t i = 0; i < m_border_width; ++i) {
    border_area.left += i;
    border_area.top += i;
    border_area.right -= i;
    border_area.bottom -= i;
    FrameRect(hdc, &border_area, border_brush);
    border_area.left -= i;
    border_area.top -= i;
    border_area.right += i;
    border_area.bottom += i;
  }
  DeleteObject(border_brush);
  RECT text_area{m_border_width, m_border_width,
                 border_area.right - m_border_width,
                 border_area.bottom - m_border_width};
  HFONT original_font{nullptr};
  if (m_font != nullptr) {
    original_font = (HFONT)SelectObject(hdc, m_font);
  }
  DrawTextA(hdc, m_text.c_str(), m_text.size(), &text_area, 0);
  if (m_font != nullptr) {
    SelectObject(hdc, original_font);
  }
  EndPaint(m_hwnd, &ps);
  return 0;
}
} // namespace winenv
