#define V(x) MakeValue(x, #x)

std::shared_ptr<Value<std::chrono::system_clock::time_point>> eta_expr = [&]() {
  if (eta.shipping_type == ShippingType::kDelivery) {
    return MakeValue(eta.delivery_at->value, "delivery_at");
  } else if (IsRetail(eta.order_type.value())) {
    return MakeValue(eta.pickedup_at->value, "picked_up_at");
  } else {
    return MakeValue(eta.cooking_finishes_at->value, "cooking_finishes_at");
  }
}();
eta.complete_at = esb.Build(Max(eta_expr + V(fallbacks.customer_waiting_duration),
                                V(now) + V(fallbacks.minimal_remaining_duration)));

template <typename T>
class Expression {
public:
  virtual ~Expression() = default;

  virtual T Evaluate() const = 0;
  std::unordered_map<std::string, std::string> GetVariables() const {
    std::unordered_map<std::string, std::string> variables;
    FillVariablesTo(variables);
    return variables;
  }
  virtual std::string GetFormula() const {
    std::ostringstream output;
    FillFormulaTo(output);
    return output.str();
  }

  virtual void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const = 0;
  virtual void FillFormulaTo(std::ostream& output) const = 0;
};

template <typename T>
struct Value : Expression<T> {
  const T& value;
  std::string name;

  Value(const T& value, std::string_view name) : value(value), name(name) {}

  T Evaluate() const override { return value; }

  void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const override {
    variables[name] = ToString(value);
  }

  void FillFormulaTo(std::ostream& output) const override { output << name; }
};

template <typename T>
auto MakeValue(const T& x, std::string_view name) {
  return std::make_shared<Value<T>>(x, name);
}

template <typename T, typename U>
using SumType_t = decltype(std::declval<T>() + std::declval<U>());

template <typename T, typename U>
class Sum : public Expression<SumType_t<T, U>> {
private:
  std::shared_ptr<Expression<T>> lhs_;
  std::shared_ptr<Expression<U>> rhs_;

public:
  Sum(std::shared_ptr<Expression<T>> lhs, std::shared_ptr<Expression<U>> rhs)
      : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  SumType_t<T, U> Evaluate() const override {
    return lhs_->Evaluate() + rhs_->Evaluate();
  }

  void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const override {
    lhs_->FillVariablesTo(variables);
    rhs_->FillVariablesTo(variables);
  }

  void FillFormulaTo(std::ostream& output) const override {
    lhs_->FillFormulaTo(output);
    output << " + ";
    rhs_->FillFormulaTo(output);
  }
};

template <typename U, template <class> typename T, typename V,
          template <class> typename W>
std::enable_if_t<std::is_base_of_v<Expression<U>, T<U>> &&
                     std::is_base_of_v<Expression<V>, W<V>>,
                 std::shared_ptr<Sum<U, V>>>
operator+(std::shared_ptr<T<U>> lhs, std::shared_ptr<W<V>> rhs) {
  return std::make_shared<Sum<U, V>>(std::move(lhs), std::move(rhs));
}
