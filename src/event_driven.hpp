#pragma once
#include "event_dispatcher.hpp"

#include <any>
#include <list>

namespace winenv
{
// Для того чтобы инициализировать EventDispatcher вперед других родительских классов
class HasEventDispatcher
{
  protected:
  EventDispatcher m_dispatcher;
};

// Управляет хранением обработчиков событий для класса наследника
class EventDriven
{  
  private:
  class MethodOwnerI
  {
    public:
    MethodOwnerI(EventDriven*& top_owner);
    virtual ~MethodOwnerI(){};
    virtual void reasign_owner(EventDriven*& top_owner) = 0;
    void set_invalid();
    template<class OwnerType>
    bool same_owner_type() {
      return typeid(OwnerType).hash_code() == m_owner_type;
    }
    protected:
    size_t m_owner_type;
    EventDriven** mpp_top_owner {nullptr};
    std::vector<EventHandlerOwner> m_handlers;
  };

  template<class OwnerType>
  struct MethodOwner : public MethodOwnerI
  {
    using MethodType = LRESULT (OwnerType::*)(const MSG&);
    using MethodContainer = std::list<MethodType>;

    MethodOwner(EventDriven*& top_owner)
    : MethodOwnerI{top_owner}
    {
      m_owner_type = typeid(OwnerType).hash_code();
    }

    EventHandler method_handle(LRESULT (OwnerType::*method)(const MSG&))
    {
      size_t i = 0;
      for (auto& mptr : m_methods) {
        if (method == mptr) {
          break;
        }
        ++i;
      }
      if (i != m_methods.size()) {
        return m_handlers[i].get();
      } else {
        EventHandlerOwner temp_handler;
        if (*mpp_top_owner != nullptr) {
          OwnerType* casted_owner = dynamic_cast<OwnerType*>(*mpp_top_owner);
          if (casted_owner == nullptr) {
            throw std::runtime_error("Bad cast. EventDriven object owner");
          }
          temp_handler = EventHandlerOwner(casted_owner, method);
        }

        auto insert_res = m_methods.insert(m_methods.end(), method);
        m_handlers.push_back(std::move(temp_handler));
        
        return m_handlers[m_handlers.size() - 1].get();
      }
    }

    void reasign_owner(EventDriven*& top_owner) override
    {
      OwnerType* casted_owner = dynamic_cast<OwnerType*>(top_owner);
      // Владелец - дочерний класс либо класс, неотносящийся к данному классу.
      // Не меняем владельца
      if (casted_owner == nullptr) {
        return;
      }
      typename MethodContainer::iterator method_it = m_methods.begin();
      for (auto &ho : m_handlers) {
        ho.set(casted_owner, *method_it);
        ++method_it;
      }
    }
    private:
    MethodContainer m_methods;
  };

  // Только для наследования
  protected:
  EventDriven() = default;
  inline virtual ~EventDriven(){};
  EventDriven(const EventDriven&) = delete;
  EventDriven& operator=(const EventDriven&) = delete;
  EventDriven(EventDriven&&);
  EventDriven& operator=(EventDriven&&);

  // Шаблон необходим для работы в классах наследниках
  template <class OwnerType>
  EventHandler method_handle(LRESULT (OwnerType::*method)(const MSG&))
  {
    using MethodType = LRESULT (OwnerType::*)(const MSG&);
    using MethodContainer = std::list<MethodType>;

    size_t owner_num = 0;
    for (auto &mo : m_owner_classes) {
      if (mo->same_owner_type<OwnerType>()) {
        break;
      }
      ++owner_num;
    }
    if (owner_num == m_owner_classes.size()) {
      std::unique_ptr<MethodOwnerI> new_owner_class_up = std::make_unique<MethodOwner<OwnerType>>(mp_owner);
      auto casted_owner_class = dynamic_cast<MethodOwner<OwnerType>*>(new_owner_class_up.get());
      EventHandler handler = casted_owner_class->method_handle(method);
      m_owner_classes.push_back(std::move(new_owner_class_up));
      return handler;
    } else {
      std::unique_ptr<MethodOwnerI>& owner_class_up = m_owner_classes[owner_num];
      auto casted_owner_class = static_cast<MethodOwner<OwnerType>*>(owner_class_up.get());
      return casted_owner_class->method_handle(method);
    }
  }

  void reasign_owner(EventDriven *owner);

  private:
  EventDriven* mp_owner {nullptr};
  std::vector<std::unique_ptr<MethodOwnerI>> m_owner_classes;
};
} // namespace winenv
