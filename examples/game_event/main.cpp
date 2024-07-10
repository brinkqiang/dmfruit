#include <fruit/fruit.h>
#include <iostream>
#include <typeindex>
#include <map>
#include <unordered_map>
#include <vector>

// Base event class
class GameEvent {
public:
  virtual ~GameEvent() = default;
};

// Specific event types
class PlayerMoveEvent : public GameEvent {
public:
  uint64_t uid;
  float x;
  float y;
  PlayerMoveEvent(uint64_t uid, float x, float y) : uid(uid), x(x), y(y) {}
};

class EnemySpawnEvent : public GameEvent {
public:
  std::string enemyType;
  uint64_t uid;
  float x;
  float y;
  EnemySpawnEvent(const std::string& type, uint64_t uid, float x, float y) : enemyType(type), uid(uid), x(x), y(y) {}
};

// Base listener class
class EventListener {
public:
  virtual ~EventListener() = default;
};

// Template for type-specific listeners
template <typename T>
class TypedEventListener : virtual public EventListener {
public:
  virtual void onEvent(const T& event) = 0;
};

// Concrete listeners
class PlayerMovementListener : virtual public TypedEventListener<PlayerMoveEvent> {
public:
  INJECT(PlayerMovementListener()) = default;
  void onEvent(const PlayerMoveEvent& event) override {
    std::cout << "PlayerMovementListener uid: " << event.uid << " moved to (" << event.x << ", " << event.y << ")" << std::endl;
  }
};

class EnemySpawnListener : virtual public TypedEventListener<EnemySpawnEvent> {
public:
  INJECT(EnemySpawnListener()) = default;
  void onEvent(const EnemySpawnEvent& event) override {
    std::cout << "EnemySpawnListener uid: " << event.uid << " enemyType:" << event.enemyType << " spawned at (" << event.x << ", " << event.y << ")" << std::endl;
  }
};

class PlayerListener : virtual public TypedEventListener<PlayerMoveEvent>,
                       virtual public TypedEventListener<EnemySpawnEvent> {
public:
  INJECT(PlayerListener()) = default;
  void onEvent(const PlayerMoveEvent& event) override {
    std::cout << "PlayerListener uid: " << event.uid << " moved to (" << event.x << ", " << event.y << ")"
              << std::endl;
  }
  void onEvent(const EnemySpawnEvent& event) override {
    std::cout << "PlayerListener uid: " << event.uid << " enemyType:" << event.enemyType << " spawned at ("
              << event.x << ", " << event.y << ")" << std::endl;
  }
};

class EventDispatcher {
private:
  std::unordered_map<std::type_index, std::vector<std::function<void(const GameEvent&)>>> listeners;

public:
  template <typename T>
  void addEventListener(TypedEventListener<T>* listener) {
    listeners[std::type_index(typeid(T))].push_back([listener](const GameEvent& e) {
      if (const T* typedEvent = dynamic_cast<const T*>(&e)) {
        listener->onEvent(*typedEvent);
      }
    });
  }

  void dispatchEvent(const GameEvent& event) {
    auto it = listeners.find(std::type_index(typeid(event)));
    if (it != listeners.end()) {
      for (const auto& listener : it->second) {
        listener(event);
      }
    }
  }
};

// Usage remains the same as before
fruit::Component<EventDispatcher> getEventSystemComponent() {
  return fruit::createComponent()
      .addMultibinding<EventListener, PlayerMovementListener>()
      .addMultibinding<EventListener, EnemySpawnListener>()
      .addMultibinding<EventListener, PlayerListener>()
      .registerProvider([]() { return new EventDispatcher(); });
}

int main() {
  fruit::Injector<EventDispatcher> injector(getEventSystemComponent);

  auto dispatcher = injector.get<EventDispatcher*>();
  auto listeners = injector.getMultibindings<EventListener>();

  // Register listeners
  for (auto listener : listeners) {
    if (auto playerMoveListener = dynamic_cast<PlayerMovementListener*>(listener)) {
      dispatcher->addEventListener(playerMoveListener);
    }
    if (auto enemySpawnListener = dynamic_cast<EnemySpawnListener*>(listener)) {
      dispatcher->addEventListener(enemySpawnListener);
    }
    if (auto playerListener = dynamic_cast<PlayerListener*>(listener)) {
        dispatcher->addEventListener(dynamic_cast<TypedEventListener<PlayerMoveEvent>*>(playerListener));
        dispatcher->addEventListener(dynamic_cast<TypedEventListener<EnemySpawnEvent>*>(playerListener));
    }
  }

  // Dispatch events
  dispatcher->dispatchEvent(PlayerMoveEvent(10001, 10.0f, 20.0f));
  dispatcher->dispatchEvent(EnemySpawnEvent("Goblin", 10001, 15.0f, 25.0f));

  return 0;
}