#include "special_windows.hpp"

namespace winenv {
BorderlessWindow::BorderlessWindow(HINSTANCE app_hinstance,
                                   EventDispatcher &event_dispatcher,
                                   WinWindow::Constructor wnd_config)
    : WinWindow{
          wnd_config
              .add_message_handling(
                  WM_NCCALCSIZE, method_handle(&BorderlessWindow::nccalcsize))
              .add_message_handling(WM_NCHITTEST,
                                    method_handle(&BorderlessWindow::nchittest))
              .create(app_hinstance, event_dispatcher)} {
  EventDriven::reasign_owner(this);
}

BorderlessWindow::BorderlessWindow() {
  EventDriven::reasign_owner(this);
  delayed_construction(
      WinWindow::Constructor()
          .add_message_handling(WM_NCCALCSIZE,
                                method_handle(&BorderlessWindow::nccalcsize))
          .add_message_handling(WM_NCHITTEST,
                                method_handle(&BorderlessWindow::nchittest)));
}

LRESULT BorderlessWindow::nccalcsize(const MSG &msg) { return 0; }

LRESULT BorderlessWindow::nchittest(const MSG &msg) { return HTCAPTION; }

HiddenWindow::HiddenWindow(HINSTANCE app_hinstance,
                           EventDispatcher &event_dispatcher,
                           bool is_shown_initialy,
                           WinWindow::Constructor wnd_config)
    : m_timer_id{g_rand_gen() % 100'001}, mf_shown{is_shown_initialy},
      WinWindow{wnd_config.set_show_flag(is_shown_initialy ? SW_SHOW : SW_HIDE)
                    .add_message_handling(WM_TIMER,
                                          method_handle(&HiddenWindow::timer))
                    .create(app_hinstance, event_dispatcher)} {
  EventDriven::reasign_owner(this);
}

bool HiddenWindow::is_shown() const noexcept { return mf_shown; }

void HiddenWindow::show(bool f_show) {
  if (mf_shown != f_show) {
    int show_flag = f_show ? SW_SHOW : SW_HIDE;
    ShowWindow(m_hwnd, show_flag);
  }
  if (mf_active_timer) {
    KillTimer(m_hwnd, m_timer_id);
    mf_active_timer = false;
  }
  mf_shown = f_show;
}

void HiddenWindow::show_for(UINT milliseconds) {
  show(true);
  SetTimer(m_hwnd, m_timer_id, milliseconds, 0);
  mf_active_timer = true;
}

HiddenWindow::HiddenWindow(bool is_shown_initialy)
    : m_timer_id{g_rand_gen() % 100'001}, mf_shown{is_shown_initialy} {
  EventDriven::reasign_owner(this);
  delayed_construction(
      WinWindow::Constructor()
          .set_show_flag(is_shown_initialy ? SW_SHOW : SW_HIDE)
          .add_message_handling(WM_TIMER, method_handle(&HiddenWindow::timer)));
}

LRESULT HiddenWindow::timer(const MSG &msg) {

  if (mf_active_timer) {
    show(false);
    KillTimer(m_hwnd, m_timer_id);
    mf_active_timer = false;
  }
  return 0;
}

} // namespace winenv
