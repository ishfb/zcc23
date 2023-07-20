
namespace impl {
template <typename T, typename U>
std::common_type_t<T, U> Max(const T& lhs, const U& rhs) {
  return lhs < rhs ? rhs : lhs;
}
}  // namespace impl

// clang-format off
template <typename T> struct EtaValue {
  // clang-format on
  T value;
  std::string formula;
  std::unordered_map<std::string, std::string> variables;

  EtaValue(T value, std::string_view name)
      : value(value), formula(name), variables({{formula, ToString(value)}}) {}

  EtaValue(T value, std::string formula,
           std::unordered_map<std::string, std::string> variables)
      : value(value),
        formula(std::move(formula)),
        variables(std::move(variables)) {}

  template <typename U>
  EtaValue(EtaValue<U>&& that,
           std::enable_if_t<std::is_convertible_v<U, T>, void*> = nullptr)
      : value(that.value),
        formula(std::move(that.formula)),
        variables(std::move(that.variables)) {}
};

template <typename T, typename U>
auto operator+(EtaValue<T> lhs, EtaValue<U> rhs) {
  lhs.formula += " + ";
  lhs.formula += rhs.formula;
  lhs.variables.merge(rhs.variables);
  return EtaValue{lhs.value + rhs.value, std::move(lhs.formula),
                  std::move(lhs.variables)};
}

template <typename T, typename U>
auto operator-(EtaValue<T> lhs, EtaValue<U> rhs) {
  lhs.formula += " - ";
  lhs.formula += rhs.formula;
  lhs.variables.merge(rhs.variables);
  return EtaValue{lhs.value - rhs.value, std::move(lhs.formula),
                  std::move(lhs.variables)};
}

template <typename T, typename U>
auto Max(EtaValue<T> lhs, EtaValue<U> rhs) {
  std::string formula = std::move(lhs.formula);
  formula += "max(";
  std::rotate(formula.begin(), formula.end() - 4, formula.end());
  formula += ", ";
  formula += rhs.formula;
  formula += ')';

  lhs.variables.merge(rhs.variables);
  return EtaValue{impl::Max(lhs.value, rhs.value), std::move(formula),
                  std::move(lhs.variables)};
}

#define V(x) \
  EtaValue { x, #x }

EtaValue<std::chrono::system_clock::time_point> eta_expr =
    eta.shipping_type == ShippingType::kDelivery
        ? V(eta.delivery_at->value)
        : V(eta.cooking_finishes_at->value);

eta.complete_at = esb.Build(Max(eta_expr + V(fallbacks.customer_waiting_duration),
                                V(now) + V(fallbacks.minimal_remaining_duration)));
