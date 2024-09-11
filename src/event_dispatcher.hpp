#pragma once
#include "hkey.hpp"

#include <windows.h>

#include <functional>
#include <unordered_map>
#include <vector>

namespace winenv
{
// Использует класс EventDispatcher 
struct AddressedMessage
{
    UINT m_code {0};
    int m_wid {0};
};
constexpr bool operator==(AddressedMessage left, AddressedMessage right) noexcept
{
    return left.m_wid == right.m_wid &&
            left.m_code == right.m_code;
}

constexpr bool operator!=(AddressedMessage left, AddressedMessage right) noexcept
{
    return !(left == right);
}
}


namespace std
{
// Для работы "unordered" конейнеров
template<>
struct hash<winenv::AddressedMessage>
{
    size_t operator()(const  winenv::AddressedMessage& msg) const noexcept
    {
        std::size_t seed = 0;
        winenv::boost_hash_combine(seed, msg.m_wid);
        winenv::boost_hash_combine(seed, msg.m_code);
        return seed;
    }
};
} // std


namespace winenv
{
class WinWindow;
using HandlerFunctor = std::function<LRESULT(const MSG&)>;

class EventHandler
{
    public:
    bool is_alive() const noexcept;
    LRESULT operator()(const MSG&) const;

    private:
    friend class EventHandlerOwner;
    EventHandler() = default;
    std::shared_ptr<bool> mpf_alive {nullptr};
    std::shared_ptr<const HandlerFunctor> mp_handler {nullptr};
};

class EventHandlerOwner
{
    public:
    EventHandlerOwner();
    explicit EventHandlerOwner(HandlerFunctor functor);
    template <class T>
    EventHandlerOwner(T* instance, LRESULT (T::*method)(const MSG&))
    : mp_handler{new HandlerFunctor(bind_instance(instance, method))},
     mpf_alive{new bool(true)}
    {}
    ~EventHandlerOwner();

    EventHandlerOwner(const EventHandlerOwner& other) = delete;
    EventHandlerOwner& operator=(const EventHandlerOwner& other) = delete;
    EventHandlerOwner(EventHandlerOwner&& other);
    EventHandlerOwner& operator=(EventHandlerOwner&& other);

    EventHandler get() const;
    void set(HandlerFunctor functor);
    template <class T>
    void set(T* instance, LRESULT (T::*method)(const MSG&))
    {
        if (mpf_alive == nullptr) {
            mp_handler.reset(new HandlerFunctor(bind_instance(instance, method)));
            mpf_alive.reset(new bool(true));
        } else {
            *mp_handler = std::move(bind_instance(instance, method));
            *mpf_alive = true;
        }
    }
    void set_alive(bool f_alive);

    private:
    std::shared_ptr<HandlerFunctor> mp_handler  {nullptr};
    std::shared_ptr<bool> mpf_alive {nullptr};
};

using HotkeyId = int;

// Вызывает определенные методами add_..._handling(...) обработчики событий (EventHandler)
// при получении требуемых сообщений MSG из очереди сообщения Windows.
// Хранит указатели на обработчики. Хранение самих обработчиков обеспечивает пользователь.
// Предназначен для работы в одном потоке
class EventDispatcher
{
public:
    // Извлекает сообщения, адресованные данному потоку, из очереди сообщений,
    // вызывает обработчики для заданных сообщений.
    // Вызывает процедуру window_procedure обработки оконных сообщений в параллельном потоке.
    // Параметр msg_filter задает диапазон кодов сообщений, которые будут обрабатываться.
    // Параметр wnd_filter задает окно, сообщения для которого будут обрабатываться.
    // поумолчанию, обрабатываются все сообщения для всех окон, принадлежащих вызывающему потоку.
    void dispatch(std::pair<UINT, UINT> msg_filter = {0, 0}, HWND wnd_filter = nullptr);
    void add_hotkey_handling(Hotkey hk, EventHandler handler);
    // Добавляет обработку сообщений, адресованных потоку и любому окну
    void add_message_handling(UINT message_code, EventHandler handler);
    // Добавляет обработку сообщений адресованных конкретному окну.
    // Обработка выполняются оконной процедурой заданного окна.
    // Если окно WinWindow привязано к экземпляру EventDispatcher, то
    // роль оконной процедура играет метод window_procedure данного экземпляра
    void add_message_handling(int window_id, UINT message_code, EventHandler handler);

    // Прекращает вызовы обработчика.
    // Необходимо, например, если обработчик имеет меньшее время жизни.
    // Если обработчик не будет найден, выбросит исключение.
    // Линейный поиск
    // void remove_handler(const EventHandler* handler);

    // Для обработки оконных сообщений windows.
    // Предназначен для конструирования объекта WinWindow.
    // Обратный вызов происходит только после того,
    // как сообщение было прочитано в методе dispatch
    LRESULT window_procedure(HWND, UINT, WPARAM, LPARAM, int);
private:
    // Пользователь гарантирует корректное состояние указателей
    std::vector<EventHandler> m_handlers;
    // Хранимое значение size_t - номер в векторе обработчиков
    std::unordered_multimap<HotkeyId, size_t> m_key_bindings;
    // Хранимое значение size_t - номер в векторе обработчиков
    std::unordered_multimap<AddressedMessage, size_t> m_msg_bindings;
};
} // namespace winenv

