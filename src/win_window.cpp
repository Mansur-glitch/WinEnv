#include "win_window.hpp"

#include <functional>

namespace winenv
{


WinWindow WinWindow::Constructor::create(HINSTANCE app_hinstance, EventDispatcher& event_dispatcher)
{
    //std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 0xffff);

    *g_logger<<"Window with id " + std::to_string(s_last_used_id + 1) + " is being created"<<std::endl;
    // Необходимо уникальное название
    std::string class_name = std::to_string(s_last_used_id + 1) + "_unique";
     *g_logger<<"Window class with id " + std::to_string(s_last_used_id + 1) + " name got"<<std::endl;
    WNDCLASSA wc;
    wc.style         = m_clss_style;
    wc.cbClsExtra    = sizeof(LONG_PTR) * 2;
    wc.cbWndExtra    = 0; 
    wc.lpszClassName = class_name.c_str();
    wc.hInstance     = app_hinstance;
    wc.hbrBackground = nullptr;
    wc.lpszMenuName  = nullptr;
    wc.lpfnWndProc   = procedure_wrapper;
    wc.hCursor       = nullptr;
    wc.hIcon         = nullptr;
    int class_id = ++s_last_used_id;

    if (! RegisterClassA(&wc)){
        --class_id;
        *g_logger<<"Window class with id " + std::to_string(class_id) + " reg failed"<<std::endl;
       throw WinError("Window class registration failed", GetLastError());
    }
    *g_logger<<"Window class with id " + std::to_string(class_id) + " registered"<<std::endl;
    {
        HWND class_wnd = CreateWindowExA(0, wc.lpszClassName, "",
         0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, app_hinstance, nullptr);
        if (! class_wnd) {
            throw WinError("Failed to create class window", GetLastError());
        }
        *g_logger<<"Window class with id " + std::to_string(class_id) + " temp wnd created"<<std::endl;
        SetClassLongPtrA(class_wnd, 0, reinterpret_cast<LONG_PTR>(&event_dispatcher));
        SetClassLongPtrA(class_wnd, sizeof(LONG_PTR), class_id);
        *g_logger<<"Window class with id " + std::to_string(class_id) + " temp before destroy"<<std::endl;
        DestroyWindow(class_wnd);
    }
    WinWindow wnd;
    wnd.m_id = class_id;
    wnd.m_dispatcher = &event_dispatcher;
    wnd.mpf_quit = new std::atomic_bool(false);
    std::atomic_bool* quit_flag_lambda = wnd.mpf_quit;
    // Добавляем обработчик для оповещения системы о разрушении окна
    // Важно! Обработчик не должен выбрасывать исключения, т.к.
    // будем ожидать срабатывание обработчика в деструкторе
    auto destroy_handler = [quit_flag_lambda](const MSG& msg) noexcept -> LRESULT
    {
        PostQuitMessage(0);
        quit_flag_lambda->store(true);
        return 0;
    };
    wnd.m_destroy_handler = EventHandlerOwner(destroy_handler);
    event_dispatcher.add_message_handling(class_id, WM_DESTROY, wnd.m_destroy_handler.get());
    for (auto [msg_code, handler] : m_msg_handlers) {
        event_dispatcher.add_message_handling(class_id, msg_code, handler);
    }
    *g_logger<<"Window with id " + std::to_string(class_id) + " added destroy handler"<<std::endl;

    wnd.m_hwnd = CreateWindowExA(m_wnd_exstyle, wc.lpszClassName, "Class",
                m_wnd_style,
                m_x, m_y, m_width, m_height,
                nullptr, nullptr, app_hinstance, nullptr);
    if (wnd.m_hwnd == nullptr) {
        delete wnd.mpf_quit;
        wnd.mpf_quit = nullptr;
        throw WinError("Window creation failed", GetLastError());
    }
    *g_logger<<"Window with id " + std::to_string(class_id) + " returned from createwindowexa()"<<std::endl;

    ShowWindow(wnd.m_hwnd, m_show_flag);
    UpdateWindow(wnd.m_hwnd);

    *g_logger<<"Window with id " + std::to_string(wnd.m_id) + " created"<<std::endl;
    return wnd;
}

WinWindow::Constructor& WinWindow::Constructor::set_show_flag(int flag, MixBehavior b)
{
    mix_fields(m_show_flag, m_param_mixb.show, flag, b);
    return *this;
}

WinWindow::Constructor& WinWindow::Constructor::set_clss_style(UINT style, MixBehavior b)
{
    mix_flags(m_clss_style, m_param_mixb.class_style, style, b);
    return *this;
}

WinWindow::Constructor& WinWindow::Constructor::set_wnd_style(DWORD style, MixBehavior b)
{
    mix_flags(m_wnd_style, m_param_mixb.wnd_style, style, b);
    return *this;
}

WinWindow::Constructor& WinWindow::Constructor::set_wnd_exstyle(DWORD style, MixBehavior b)
{
    mix_flags(m_wnd_exstyle, m_param_mixb.wnd_exstyle, style, b);
    return *this;
}

WinWindow::Constructor& WinWindow::Constructor::set_position(int x, int y, MixBehavior b)
{
    std::pair<int&, int&> m_p {m_x, m_y}, o_p {x, y};
    mix_fields(m_p, m_param_mixb.position, o_p, b);
    return *this;
}

WinWindow::Constructor& WinWindow::Constructor::set_size(int width, int height, MixBehavior b)
{
    std::pair<int&, int&> m_p {m_width, m_height}, o_p {width, height};
    mix_fields(m_p, m_param_mixb.size, o_p, b);
    return *this;
}

WinWindow::Constructor& WinWindow::Constructor::add_message_handling(UINT message_code, EventHandler handler, MixBehavior b)
{
    if (m_param_mixb.handlers == MixBehavior::not_set || b == MixBehavior::rewrite) {
        m_param_mixb.handlers = b;
        m_msg_handlers.clear();
        m_msg_handlers.push_back({message_code, handler});
    } else if (m_param_mixb.handlers == MixBehavior::combine && b == MixBehavior::combine) {
        m_msg_handlers.push_back({message_code, handler});
    }
    return *this;
}

WinWindow::Constructor &WinWindow::Constructor::mix_with(Constructor other)
{
    if (other.m_param_mixb.show != MixBehavior::not_set) {
        set_show_flag(other.m_show_flag, other.m_param_mixb.show);
    }
    if (other.m_param_mixb.class_style != MixBehavior::not_set) {
        set_clss_style(other.m_clss_style, other.m_param_mixb.class_style);
    }
    if (other.m_param_mixb.wnd_style != MixBehavior::not_set) {
        set_wnd_style(other.m_wnd_style, other.m_param_mixb.wnd_style);
    }
    if (other.m_param_mixb.wnd_exstyle != MixBehavior::not_set) {
        set_wnd_exstyle(other.m_wnd_exstyle, other.m_param_mixb.wnd_exstyle);
    }
    if (other.m_param_mixb.position != MixBehavior::not_set) {
        set_position(other.m_x, other.m_y, other.m_param_mixb.position);
    }
    if (other.m_param_mixb.size != MixBehavior::not_set) {
        set_size(other.m_width, other.m_height, other.m_param_mixb.size);
    }
    
    if (m_param_mixb.handlers == MixBehavior::not_set ||
        other.m_param_mixb.handlers == MixBehavior::rewrite) {
        m_param_mixb.handlers = other.m_param_mixb.handlers;
        m_msg_handlers = std::move(other.m_msg_handlers);
    } else if (m_param_mixb.handlers == MixBehavior::combine &&
               other.m_param_mixb.handlers == MixBehavior::combine) {
        m_msg_handlers.insert(
            m_msg_handlers.end(),
            std::make_move_iterator(other.m_msg_handlers.begin()),
            std::make_move_iterator(other.m_msg_handlers.end())
        );
    }
    return *this;
}

void WinWindow::delayed_construction(Constructor config)
{
    if(mp_config == nullptr) {
        mp_config.reset(new Constructor(std::move(config)));
    } else {
        mp_config->mix_with(std::move(config));
    }
}

void WinWindow::construct(HINSTANCE app_hinstance, EventDispatcher& event_dispatcher)
{
    (*this) = mp_config->create(app_hinstance, event_dispatcher);
}

WinWindow::WinWindow() {}

void WinWindow::destroy()
{
    if (m_hwnd) {
        if (! mpf_quit->load()) {
            DestroyWindow(m_hwnd);
            // Нужно гарантировать обработку закрытия окна!
            while(! mpf_quit->load()) {
                m_dispatcher->dispatch({WM_DESTROY, WM_DESTROY}, m_hwnd);
            }
        }
        m_dispatcher = nullptr;

        delete mpf_quit;
        mpf_quit = nullptr;

        m_id = 0;
        m_hwnd = nullptr;
    }
}

WinWindow::~WinWindow()
{
    *g_logger<<"Window with id " + std::to_string(m_id) + " is being destruct"<<std::endl;
    destroy();
    *g_logger<<"Window with id " + std::to_string(m_id) + " destructed"<<std::endl;
}

WinWindow::WinWindow(WinWindow &&other) noexcept
: m_hwnd{other.m_hwnd},
  m_dispatcher{other.m_dispatcher},
  m_id{other.m_id},
  mpf_quit{other.mpf_quit},
  m_destroy_handler{std::move(other.m_destroy_handler)},
  mp_config{std::move(other.mp_config)}
{
    other.m_hwnd = nullptr;
    other.m_dispatcher = nullptr;
    other.m_id = 0;
    other.mpf_quit = nullptr;
}

WinWindow &WinWindow::operator=(WinWindow &&other) noexcept
{
    destroy();
    m_hwnd = other.m_hwnd;                       other.m_hwnd = nullptr;
    m_dispatcher = other.m_dispatcher;           other.m_dispatcher = nullptr;
    m_id = other.m_id;                           other.m_id = 0;
    mpf_quit = other.mpf_quit;                   other.mpf_quit = nullptr;
    m_destroy_handler = std::move(other.m_destroy_handler);
    mp_config = std::move(other.mp_config);
    return *this;
}

int WinWindow::get_id() const noexcept
{
  return m_id;
}

bool WinWindow::is_quit() const noexcept
{
    return mpf_quit->load();
}

HWND WinWindow::get_hwnd() noexcept
{
  return m_hwnd;
}

LRESULT WinWindow::procedure_wrapper(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

    if (! hwnd) {
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }
    // Получаем прикрепленную к классу окна информацию
    ULONG_PTR user_data0 = GetClassLongPtrA(hwnd, 0);
    if (! user_data0) {
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }
    EventDispatcher* dispatcher = reinterpret_cast<EventDispatcher*>(user_data0);
    ULONG_PTR user_data1 = GetClassLongPtrA(hwnd, sizeof(ULONG_PTR));
    int window_id = static_cast<int>(user_data1);
    if (window_id == 0) {
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }
    return dispatcher->window_procedure(hwnd, message, wparam, lparam, window_id);
}

int WinWindow::s_last_used_id {window_id_thread};

} // namespas windows_wrappers

