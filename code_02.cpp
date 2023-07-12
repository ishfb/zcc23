#define V(x) \
  eats_eta::utils::estimation_calc::Value { x, #x }

utils::EstimationContext UpdateDeliveryAt(EtaInfo& eta) {
  utils::EstimationContext context;
  utils::EstimationBuilder esb{context};
  auto& delivery_duration = eta.delivery_duration->value;
  auto& delivery_started_at = eta.delivery_started_at.value();
  eta.delivery_at = esb.Build(V(delivery_started_at) + V(delivery_duration));
  return context;
}

#undef V

template <typename T>
class Expression {
public:
  auto Evaluate() const { return AsChild().Evaluate(); }
  std::unordered_map<std::string, std::string> GetVariables() const {
    std::unordered_map<std::string, std::string> variables;
    AsChild().FillVariablesTo(variables);
    return variables;
  }

  std::string GetFormula() const {
    std::ostringstream output;
    AsChild().FillFormulaTo(output);
    return output.str();
  }

  const T& AsChild() const { return static_cast<const T&>(*this); }
};

template <typename T>
struct Value : Expression<Value<T>> {
  const T& value;
  std::string name;

  Value(const T& value, std::string_view name) : value(value), name(name) {}

  T Evaluate() const { return value; }

  void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const {
    variables[name] = ToString(value);
  }

  void FillFormulaTo(std::ostream& output) const { output << name; }
};

template <typename T, typename U>
class Sum : public Expression<Sum<T, U>> {
private:
  const T& lhs;
  const U& rhs;

public:
  Sum(const Expression<T>& lhs, const Expression<U>& rhs)
      : lhs(lhs.AsChild()), rhs(rhs.AsChild()) {}

  auto Evaluate() const { return lhs.Evaluate() + rhs.Evaluate(); }

  void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const {
    lhs.FillVariablesTo(variables);
    rhs.FillVariablesTo(variables);
  }

  void FillFormulaTo(std::ostream& output) const {
    lhs.FillFormulaTo(output);
    output << " + ";
    rhs.FillFormulaTo(output);
  }
};

template <typename T, typename U>
auto operator+(const Expression<T>& lhs, const Expression<U>& rhs) {
  return Sum(lhs, rhs);
}

struct EstimationBuilder {
  EstimationContext& context_;

  template <typename T>
  auto Build(const estimation_calc::Expression<T>& expression) const {
    auto value = impl::CastValue(expression.Evaluate());
    context_.value = ToString(value);
    context_.formula = expression.GetFormula();
    context_.variables = expression.GetVariables();

    return value;
  }
};
