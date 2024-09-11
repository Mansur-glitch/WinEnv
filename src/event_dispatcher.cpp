#include "event_dispatcher.hpp"
#include "utils.hpp"
#include "win_window.hpp"

#include <memory>

namespace {
// Вспомогательная функция. Вызывает обработчики, привязанные к сочетанию
// клавиши или к сообщению
template <class Iter>
LRESULT
call_handlers_in_range(Iter beg, Iter end,
                       const std::vector<winenv::EventHandler> &handlers,
                       const MSG &msg) {
  LRESULT lres{};
  while (beg != end) {
    winenv::EventHandler handler = handlers[beg->second];
    // Обработчик мождет быть убран методом remove_handler
    if (handler.is_alive()) {
      // Перезаписываем, если несколько!
      lres = handler(msg);
    }
    ++beg;
  }
  return lres;
}
} // namespace

namespace winenv {
bool EventHandler::is_alive() const noexcept {
  if (mpf_alive == nullptr) {
    return false;
  }
  return *mpf_alive;
}

LRESULT EventHandler::operator()(const MSG &msg) const {
  return (*mp_handler)(msg);
}

EventHandlerOwner::EventHandlerOwner(HandlerFunctor functor)
    : mp_handler{new std::function<LRESULT(const MSG &)>(std::move(functor))},
      mpf_alive{new bool(true)} {}

EventHandlerOwner::EventHandlerOwner()
    : mp_handler{new HandlerFunctor()}, mpf_alive{new bool(false)} {}

EventHandlerOwner::~EventHandlerOwner() {
  if (mpf_alive != nullptr) {
    *mpf_alive = false;
  }
}

EventHandlerOwner::EventHandlerOwner(EventHandlerOwner &&other)
    : mp_handler{std::move(other.mp_handler)},
      mpf_alive{std::move(other.mpf_alive)} {}

EventHandlerOwner &EventHandlerOwner::operator=(EventHandlerOwner &&other) {
  if (mpf_alive != nullptr) {
    *mpf_alive = false;
  }
  mpf_alive = std::move(other.mpf_alive);
  mp_handler = std::move(other.mp_handler);
  return *this;
}

EventHandler EventHandlerOwner::get() const {
  EventHandler handler{};
  handler.mp_handler = mp_handler;
  handler.mpf_alive = mpf_alive;
  return handler;
}

void EventHandlerOwner::set(HandlerFunctor functor) {
  if (mpf_alive == nullptr) {
    mp_handler.reset(new HandlerFunctor(std::move(functor)));
    mpf_alive.reset(new bool(true));
  } else {
    *mp_handler = std::move(functor);
    *mpf_alive = true;
  }
}

void EventHandlerOwner::set_alive(bool f_alive) {
  if (mpf_alive != nullptr) {
    *mpf_alive = f_alive;
  } else if (f_alive == true) {
    throw std::runtime_error("Failed to set_alive to EventHandlerOwner. It was "
                             "empty (default constructed)");
  }
}

// Регистрирует новое сочетание клавиш и возвращает идентификатор.
// Если сочетание уже было зарегистрировано функцией,
// просто вернет его идентификатор.
HotkeyId get_registered_key_id(Hotkey hk) {
  static std::unordered_map<Hotkey, HotkeyId> s_registered_keys{};
  // Удаляет зарегистрированные в системе сочетания клавиш
  // вызовом деструктора для единственного статического экземпляра
  struct Deleter {
    ~Deleter() {
      for (auto &[_, id] : s_registered_keys) {
        UnregisterHotKey(NULL, id);
      }
    }
  } const static deleter{};

  if (auto iter = s_registered_keys.find(hk); iter != s_registered_keys.end()) {
    return iter->second; // Id уже зарегистрированного сочетания клавиш
  } else {
    HotkeyId new_key_id = static_cast<HotkeyId>(s_registered_keys.size() + 1);
    if (RegisterHotKey(NULL, new_key_id, static_cast<UINT>(hk.get_modifiers()),
                       hk.get_key_code())) {
      s_registered_keys.insert({hk, new_key_id});
      return new_key_id;
    } else {
      throw WinError("Couldn't register <" + hk.to_stdstring() +
                         "> key combination",
                     GetLastError());
    }
  }
}

void EventDispatcher::add_hotkey_handling(Hotkey hk, EventHandler handler) {
  HotkeyId key_id = get_registered_key_id(hk);
  m_handlers.push_back(handler);
  m_key_bindings.insert({key_id, m_handlers.size() - 1});
}

void EventDispatcher::add_message_handling(UINT message_code,
                                           EventHandler handler) {
  m_handlers.push_back(handler);
  AddressedMessage addr_msg{message_code, WinWindow::window_id_thread};
  m_msg_bindings.insert({addr_msg, m_handlers.size() - 1});
}

void EventDispatcher::add_message_handling(int window_id, UINT message_code,
                                           EventHandler handler) {
  m_handlers.push_back(handler);
  AddressedMessage addr_msg{message_code, window_id};
  m_msg_bindings.insert({addr_msg, m_handlers.size() - 1});
}

void EventDispatcher::dispatch(std::pair<UINT, UINT> msg_filter,
                               HWND wnd_filter) {
  MSG msg = {};
  ZeroMemory(&msg, sizeof(msg));
  // Извлекает полученные на данный момент сообщения, удовлетворяющие заданные
  // параметры
  while (PeekMessageA(&msg, wnd_filter, msg_filter.first, msg_filter.second,
                      PM_REMOVE) != 0) {
    if (msg.message == WM_HOTKEY) {
      HotkeyId key_id = msg.wParam;
      // Возможно несколько обработчиков относится к одному сочетанию клавиш
      auto [iter, end_iter] = m_key_bindings.equal_range(key_id);
      call_handlers_in_range(iter, end_iter, m_handlers, msg);
    } else if (msg.hwnd ==
               nullptr) { // Сообщение адресовано потоку, а не конкретному окну
      UINT message_id = msg.message;
      // Возможно несколько обработчиков относится к одному идентификатору
      // сообщения
      auto [iter, end_iter] =
          m_msg_bindings.equal_range({message_id, WinWindow::window_id_thread});
      if (iter != end_iter) {
        call_handlers_in_range(iter, end_iter, m_handlers, msg);
      }
    }
    TranslateMessage(
        &msg); // Для обработки нажатий клавиш как символьных сообщений
    DispatchMessageA(
        &msg); // Отправляет на обработку оконной процедуре window_procedure
  }
}

LRESULT EventDispatcher::window_procedure(HWND hwnd, UINT message_id,
                                          WPARAM wparam, LPARAM lparam,
                                          int window_id) {
  MSG msg{};
  msg.message = message_id;
  msg.hwnd = hwnd;
  msg.lParam = lparam;
  msg.wParam = wparam;
  msg.time = 0;
  msg.pt = {-1, -1};

  // сообщение адресовано потоку и уже обработано
  if (hwnd == nullptr) {
    // Стандартная обработка сообщений
    return DefWindowProcA(hwnd, message_id, wparam, lparam);
  }
  auto [iter, end_iter] = m_msg_bindings.equal_range({message_id, window_id});

  LRESULT lres{};
  bool found_wnd_handler{false};
  if (iter != end_iter) {
    lres = call_handlers_in_range(iter, end_iter, m_handlers, msg);
    found_wnd_handler = true;
  }
  // Ищем универсальные обработчики, относящиеся ко всем окнам
  auto [iter1, end_iter1] =
      m_msg_bindings.equal_range({message_id, WinWindow::window_id_thread});
  if (iter1 != end_iter1) {
    LRESULT lres1 = call_handlers_in_range(iter1, end_iter1, m_handlers, msg);
    if (!found_wnd_handler) {
      return lres1;
    } else {
      return lres;
    }
  } else if (!found_wnd_handler) {
    // Стандартная обработка сообщений
    return DefWindowProcA(hwnd, message_id, wparam, lparam);
  } else {
    return lres;
  }
}
} // namespace winenv