#ifndef EVENTBUS_H
#define EVENTBUS_H  

#include <unordered_map>
#include <typeindex>
#include <list>

class Event {
public:
    Event() = default;
    virtual ~Event() = default;
};

class IEventCallBack {
public:
    virtual ~IEventCallBack() = default;
    virtual void Excute(Event& e) = 0;
};

template <typename TOwner, typename TEvent>
class EventCallBack : public IEventCallBack {
private:
    typedef void (TOwner::* Func)(TEvent&);

public:
    EventCallBack(TOwner* owner, Func func) : _owner(owner), _func(func) {}
    virtual ~EventCallBack() override = default;

    virtual void Excute(Event& e) override {
        std::invoke(_func, _owner, static_cast<TEvent&>(e));
    }

private:
    TOwner* _owner;
    Func _func;
};

typedef std::list<std::unique_ptr<IEventCallBack>> HandlerList;

class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;

    template<typename TEvent, typename ...TArgs>
    void EmitEvent(TArgs&& ...args) {
        auto type = std::type_index(typeid(TEvent));
        if (_subscribers.find(type) == _subscribers.end()) return;
        for (auto& callBack : *_subscribers[type]) {
            TEvent event(std::forward<TArgs>(args)...);
            callBack->Excute(event);
        }
    }

    template<typename TOwner, typename TEvent>
    void Subscribe(TOwner* owner, void (TOwner::* callback)(TEvent&)) {
        auto type = std::type_index(typeid(TEvent));
        auto handler = std::make_unique<EventCallBack<TOwner, TEvent>>(owner, callback);

        if (_subscribers.find(type) == _subscribers.end()) {
            _subscribers[type] = std::make_unique<HandlerList>();
        }

        // std::move는 unique_ptr의 주인을 옮길때 사용
        _subscribers[type]->push_back(std::move(handler));
    }

    void Reset() { _subscribers.clear(); }

private:
    std::unordered_map<std::type_index, std::unique_ptr<HandlerList>> _subscribers;
};

#endif