#pragma once
#include "common.hpp"
#include "event_dispatcher.hpp"

#include <atomic>

namespace winenv
{
// Обёртка окна windows. Позволяет настроить новое окно,
// делигирует обработку сообщений окна объекту класса EventDispatcher. 
// Пример:
// auto wnd = WinWindow::Constructor(hinstance, dispatcher)
//                                  .set_show_flag(SW_SHOWMINIMIZED);
class WinWindow
{
public:
    // Класс строитель. Контролирует создание окна
    class Constructor
    {
        public:
        // Поведение при объединении двух конфигураций 
        enum class MixBehavior : unsigned char
        {
            not_set = 0, // параметр не задан
            combine = 1, // параметры не стираются, а дополяются, если это возможно (Исходные данные в приоритете).
            rewrite = 2  // исходные параметры стираются в пользу переданных
        };
        // Параметр app_hinstance приравнивают к аргументу полученному в WinMain.
        // Вызываются неконстантные методы event_dispatcher
        WinWindow create(HINSTANCE app_hinstance, EventDispatcher& event_dispatcher);
        // https://learn.microsoft.com/ru-ru/windows/win32/api/winuser/nf-winuser-showwindow
        Constructor& set_show_flag(int flag, MixBehavior b = MixBehavior::combine);
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
        Constructor& set_clss_style(UINT style, MixBehavior b = MixBehavior::combine);
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
        Constructor& set_wnd_style(DWORD style, MixBehavior b = MixBehavior::combine);
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles?source=recommendations
        Constructor& set_wnd_exstyle(DWORD style, MixBehavior b = MixBehavior::combine);
        // В координатах экрана
        Constructor& set_position(int x, int y, MixBehavior b = MixBehavior::rewrite);
        // Размер в пикселях
        Constructor& set_size(int width, int height, MixBehavior b = MixBehavior::rewrite);
        // Позволяет обработать сообщения, возникающие, в том числе, при создании окна (WM_CREATE)
        Constructor& add_message_handling(UINT message_code, EventHandler handler, MixBehavior b = MixBehavior::combine);

        // Объединяет параметры создания
        Constructor& mix_with(Constructor other);

        private:
        template<class Ty>
        void mix_flags(Ty& my_flag, MixBehavior& my_b, Ty other_flag, MixBehavior other_b)
        {
            if(my_b == MixBehavior::not_set || other_b == MixBehavior::rewrite) {
                my_b = other_b;
                my_flag = other_flag;
            } else if (my_b == MixBehavior::combine && other_b == MixBehavior::combine) {
                my_flag |= other_flag;
            }
        }
        template<class Ty>
        void mix_fields(Ty& my_field, MixBehavior& my_b, Ty other_field, MixBehavior other_b)
        {
            if(my_b == MixBehavior::not_set || other_b == MixBehavior::rewrite) {
                my_b = other_b;
                my_field = other_field;
            }
        }
        struct MixParams {
            MixBehavior show;
            MixBehavior class_style;
            MixBehavior wnd_style;
            MixBehavior wnd_exstyle;
            MixBehavior position;
            MixBehavior size;
            MixBehavior handlers;
        } m_param_mixb;
        int m_show_flag = SW_SHOWNORMAL;
        UINT m_clss_style = CS_HREDRAW | CS_VREDRAW;
        DWORD m_wnd_style = WS_OVERLAPPEDWINDOW;
        DWORD m_wnd_exstyle = 0;
        int m_x = CW_USEDEFAULT;
        int m_y = CW_USEDEFAULT;
        int m_width = CW_USEDEFAULT;
        int m_height = CW_USEDEFAULT;
        std::vector<std::pair<UINT, EventHandler>> m_msg_handlers;
    };
    // Функция указывается при регистрации класса окна.
    // Вызовает обработку связанного event_dispatcher'а
    static LRESULT CALLBACK procedure_wrapper(HWND, UINT, WPARAM, LPARAM);

    virtual ~WinWindow();
    WinWindow(const WinWindow& other) = delete;
    WinWindow(WinWindow&& other) noexcept;
    WinWindow& operator=(const WinWindow& other) = delete;
    WinWindow& operator=(WinWindow&& other) noexcept;
    int get_id() const noexcept;
    bool is_quit() const noexcept;
    HWND get_hwnd() noexcept;

    static constexpr int window_id_thread {1};
protected:
    HWND m_hwnd {nullptr};
    EventDispatcher* m_dispatcher {nullptr};
    // Вызываем в классах наследниках
    void delayed_construction(Constructor config);
    // Вызываем в крайнем (низшем) классе наследнике
    void construct(HINSTANCE app_hinstance, EventDispatcher& event_dispatcher); 
    WinWindow();
private:
    void destroy();
    // Для отличия окон
    static int s_last_used_id;
    int m_id {0};
    // Храним указатели, а не сами объекты - после перемещения адреса должны сохранятся
    std::atomic_bool* mpf_quit {nullptr};
    EventHandlerOwner m_destroy_handler;
    // Используется только при конструировании в классах наследниках
    std::unique_ptr<Constructor> mp_config {nullptr};
 };

} // namespas windows_wrappers