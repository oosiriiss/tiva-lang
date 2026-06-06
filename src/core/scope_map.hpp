#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

struct TransparentStringHash {
  using is_transparent = void;  // NOLINT

  auto operator()(std::string_view view) const {
    return std::hash<std::string_view>{}(view);
  }
  auto operator()(const char *str) const {
    return std::hash<const char *>{}(str);
  }
  auto operator()(const std::string &str) const {
    return std::hash<std::string>{}(str);
  }
};

template <class Value>
struct ScopeGuard;

template <class Value>
struct ScopeContainer {
 public:
  using Scope = std::unordered_map<std::string, Value, TransparentStringHash,
                                   std::equal_to<>>;

 public:
  [[nodiscard]] constexpr auto scopeGuard() -> ScopeGuard<Value>;

  constexpr void putVariable(const std::string &name, Value &val);
  constexpr void putVariable(std::string_view name, Value &val);

  [[nodiscard]] constexpr auto findVariable(const std::string &name)
      -> std::optional<std::reference_wrapper<Value>>;
  [[nodiscard]] constexpr auto findVariable(std::string_view name)
      -> std::optional<std::reference_wrapper<Value>>;

 private:
  constexpr void beginScope();
  constexpr void endScope();
  constexpr auto currentScope() -> Scope &;

 private:
  // Contains one global scope at start
  std::vector<Scope> scopeValues_{Scope{}};

  friend ScopeGuard<Value>;
};

template <class Value>
struct ScopeGuard {
  constexpr ScopeGuard(ScopeContainer<Value> *scopeContainer);
  constexpr ~ScopeGuard();

  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard(ScopeGuard &&) = delete;
  auto operator=(const ScopeGuard &) -> ScopeGuard & = delete;
  auto operator=(ScopeGuard &&) -> ScopeGuard & = delete;

  ScopeContainer<Value> *scopeContainer;
};

template <class Value>
constexpr auto ScopeContainer<Value>::scopeGuard() -> ScopeGuard<Value> {
  return ScopeGuard<Value>{this};
}

template <class Value>
constexpr void ScopeContainer<Value>::putVariable(const std::string &name,
                                                  Value &val) {
  currentScope().emplace(name, val);
}
template <class Value>
constexpr void ScopeContainer<Value>::putVariable(std::string_view name,
                                                  Value &val) {
  currentScope().emplace(name, val);
}

template <class Value>
constexpr auto ScopeContainer<Value>::findVariable(const std::string &name)
    -> std::optional<std::reference_wrapper<Value>> {
  return findVariable(std::string_view{name});
}
template <class Value>
constexpr auto ScopeContainer<Value>::findVariable(const std::string_view name)
    -> std::optional<std::reference_wrapper<Value>> {
  for (size_t i = scopeValues_.size(); i-- > 0;) {
    auto &scope = scopeValues_[i];

    auto iter = scope.find(name);
    if (iter != scope.end()) {
      return std::optional<std::reference_wrapper<Value>>{
          std::ref(iter->second)};
    }
  }
  return std::nullopt;
}

template <class Value>
constexpr void ScopeContainer<Value>::beginScope() {
  scopeValues_.emplace_back(Scope{});
}

template <class Value>
constexpr void ScopeContainer<Value>::endScope() {
  if (scopeValues_.empty()) {
    throw std::logic_error(
        "Trying to pop an empty scope. There must be a "
        "mismatch between creating and deleting scopes");
  }
  scopeValues_.pop_back();
}

template <class Value>
constexpr auto ScopeContainer<Value>::currentScope() -> Scope & {
  if (scopeValues_.empty()) {
    throw std::logic_error(
        "Scope values is empty. There must have been a "
        "skill issue as global scope disappeared");
  }

  return scopeValues_.back();
}

template <class Value>
constexpr ScopeGuard<Value>::ScopeGuard(ScopeContainer<Value> *scopeContainer)
    : scopeContainer{scopeContainer} {
  scopeContainer->beginScope();
}

template <class Value>
constexpr ScopeGuard<Value>::~ScopeGuard() {
  scopeContainer->endScope();
}
