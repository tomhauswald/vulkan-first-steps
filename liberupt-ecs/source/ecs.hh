#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <utility>

template <typename Tuple, typename T, size_t Index = 0>
constexpr size_t tuple_type_index() {
  if constexpr (std::is_same_v<std::tuple_element_t<Index, Tuple>, T>) {
    return Index;
  } else {
    return tuple_type_index<Tuple, T, Index + 1>();
  }
}

template <typename... Cs>
struct ComponentList {
  static constexpr size_t kNumTypes = sizeof...(Cs);
  using Mask = std::bitset<kNumTypes>;
  using Tuple = std::tuple<Cs...>;

  template <size_t Index>
  using NthType = std::tuple_element_t<Index, Tuple>;

  template <size_t MaxEntities>
  using Storage = std::tuple<std::array<Cs, MaxEntities>...>;
};

template <typename... Cs>
struct System {
  static constexpr size_t kNumComponentTypes = sizeof...(Cs);

  template <size_t Index>
  using NthComponentType = std::tuple_element_t<Index, std::tuple<Cs...>>;

  virtual void update(float dt, Cs&...) = 0;
};

template <typename... Ss>
struct SystemList {
  static constexpr size_t kNumSystems = sizeof...(Ss);
  using Tuple = std::tuple<Ss...>;
  using Storage = Tuple;

  template <size_t Index>
  using NthType = std::tuple_element_t<Index, Tuple>;
};

template <size_t MaxEntities, typename ComponentListT, typename SystemListT>
class EntityComponentSystem {
 public:
  static constexpr size_t kMaxEntities = MaxEntities;
  static constexpr size_t kNumComponentTypes = ComponentListT::kNumTypes;
  static constexpr size_t kNumSystemTypes =
      std::tuple_size_v<typename SystemListT::Tuple>;

  using ComponentMask = std::bitset<kNumComponentTypes>;

  template <size_t Index>
  using NthComponentType = typename ComponentListT::template NthType<Index>;
  template <size_t Index>
  using NthSystemType = typename SystemListT::template NthType<Index>;

  typename ComponentListT::template Storage<MaxEntities> components;
  typename SystemListT::Storage systems;

  template <typename C>
  bool hasComponent(size_t id) const {
    return entityComponentMasks_[id].test(componentTypeIndex<C>());
  }

  template <typename C>
  C& addComponent(size_t id) {
    entityComponentMasks_[id].set(componentTypeIndex<C>());
    return std::get<componentTypeIndex<C>()>(components)[id];
  }

  template <typename C>
  void removeComponent(size_t id) {
    entityComponentMasks_[id].reset(componentTypeIndex<C>());
  }

  size_t createEntity() { return numActiveEntities_++; }

  void update(float dt) {
    for (size_t e = 0; e < numActiveEntities_; ++e) updateSystems(e, dt);
  }

 private:
  size_t numActiveEntities_ = 0;
  std::array<ComponentMask, kMaxEntities> entityComponentMasks_ = {};

  template <typename C>
  static constexpr size_t componentTypeIndex() {
    return tuple_type_index<typename ComponentListT::Tuple, C>();
  }

  template <typename S, size_t Index = 0>
  constexpr bool isSystemApplicable(size_t e) {
    if constexpr (Index >= S::kNumComponentTypes) {
      return true;
    } else {
      return hasComponent<typename S::template NthComponentType<Index>>(e) &&
             isSystemApplicable<S, Index + 1>(e);
    }
  }

  template <typename S, size_t Index = 0>
  constexpr auto getSystemComponents(size_t e) {
    if constexpr (Index >= S::kNumComponentTypes) {
      return std::make_tuple();
    } else {
      using C = typename S::template NthComponentType<Index>;
      auto& c = std::get<std::array<C, MaxEntities>>(components)[e];
      return std::tuple_cat(std::forward_as_tuple(c),
                            getSystemComponents<S, Index + 1>(e));
    }
  }

  template <size_t Index = 0>
  void updateSystems(size_t e, float dt) {
    if constexpr (Index < kNumSystemTypes) {
      using SystemType = NthSystemType<Index>;
      if (isSystemApplicable<SystemType>(e)) {
        auto& sys = std::get<Index>(systems);
        std::apply([&](auto&&... components) { sys.update(dt, components...); },
                   getSystemComponents<SystemType>(e));
      }
      if constexpr (Index < kNumSystemTypes - 1) {
        updateSystems<Index + 1>(e, dt);
      }
    };
  }
};
