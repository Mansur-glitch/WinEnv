#pragma once
#include "event_driven.hpp"
#include "win_window.hpp"

namespace winenv {
class BorderlessWindow : virtual public EventDriven, virtual public WinWindow {
public:
  BorderlessWindow(HINSTANCE app_hinstance, EventDispatcher &event_dispatcher,
                   WinWindow::Constructor wnd_config = {});

protected:
  BorderlessWindow();
  virtual LRESULT nccalcsize(const MSG &msg);
  virtual LRESULT nchittest(const MSG &msg);
};

class HiddenWindow : virtual public EventDriven, virtual public WinWindow {
public:
  HiddenWindow(HINSTANCE app_hinstance, EventDispatcher &event_dispatcher,
               bool is_shown_initialy = false,
               WinWindow::Constructor wnd_config = {});

  bool is_shown() const noexcept;
  void show(bool f_show);
  void show_for(UINT milliseconds);

protected:
  HiddenWindow(bool is_shown_initialy = false);

private:
  LRESULT timer(const MSG &msg);
  UINT_PTR m_timer_id{0};
  bool mf_shown{false};
  bool mf_active_timer{false};
};

} // namespace winenv
