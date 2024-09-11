#include "event_driven.hpp"

namespace winenv
{
EventDriven::MethodOwnerI::MethodOwnerI(EventDriven *&top_owner)
: mpp_top_owner{&top_owner}
{}

void EventDriven::MethodOwnerI::set_invalid()
{
  for (auto& h: m_handlers) {
    h.set_alive(false);
  }
}

EventDriven::EventDriven(EventDriven && other)
: mp_owner{nullptr},
  m_owner_classes{std::move(other.m_owner_classes)}
  {
    for (auto& oc : m_owner_classes) {
      oc->set_invalid(); 
    }
  }

  EventDriven &EventDriven::operator=(EventDriven &&other)
  {
    mp_owner = nullptr;
    m_owner_classes = std::move(other.m_owner_classes);
    for (auto& oc : m_owner_classes) {
      oc->set_invalid(); 
    }
    return *this;
  }

  void EventDriven::reasign_owner(EventDriven *owner)
  { 
    mp_owner = owner;
    for(auto& oc : m_owner_classes) {
      oc->reasign_owner(mp_owner);
    }
  }
} // namespace winenv
