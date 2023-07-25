#include <charconv>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

using Timepoint = chrono::system_clock::time_point;

string ToString(Timepoint);

template <typename Rep, typename Period>
std::string ToString(const std::chrono::duration<Rep, Period>& value);

bool SomeCondition();

template <typename T, typename U>
auto MaxImpl(const T& lhs, const U& rhs) {
  return lhs > rhs ? lhs : rhs;
}

struct Estimated {
  Timepoint value;
  string formula;
  unordered_map<string, string> variables;
};

struct EstimationData {
  Timepoint delivery_started_at;
  Timepoint arrival_to_customer_at;
  chrono::seconds delivery_duration;
  chrono::seconds fallback_delivery_duration;
};

namespace first_attempt {

Estimated CalcCompleteAt(const EstimationData& eta) {
  Estimated result;
  result.variables["delivery_started_at"] = ToString(eta.delivery_started_at);
  result.variables[std::string{"delivery_duration"}] =
      ToString(eta.delivery_duration);
  result.formula = "delivery_started_at + delivery_duration";
  result.value = eta.delivery_started_at + eta.delivery_duration;

  return result;
}

}  // namespace first_attempt

namespace second_attempt {

// clang-format off
template <typename T> class Expression {
  // clang-format on
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

  operator Estimated() const {
    return Estimated{.value = this->Evaluate(),
                     .formula = this->GetFormula(),
                     .variables = this->GetVariables()};
  }

  const T& AsChild() const { return static_cast<const T&>(*this); }
};

// clang-format off
template <typename T> struct Value : Expression<Value<T>> {
  // clang-format on
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

// clang-format off
template <typename T, typename U> class Sum : public Expression<Sum<T, U>> {
  // clang-format on
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

// clang-format off
template <typename T, typename U> class MaxExpr : public Expression<MaxExpr<T, U>> {
  // clang-format on
private:
  const T& lhs;
  const U& rhs;

public:
  MaxExpr(const Expression<T>& lhs, const Expression<U>& rhs)
      : lhs(lhs.AsChild()), rhs(rhs.AsChild()) {}

  auto Evaluate() const { return MaxImpl(lhs.Evaluate(), rhs.Evaluate()); }

  void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const {
    lhs.FillVariablesTo(variables);
    rhs.FillVariablesTo(variables);
  }

  void FillFormulaTo(std::ostream& output) const {
    output << "max(";
    lhs.FillFormulaTo(output);
    output << ", ";
    rhs.FillFormulaTo(output);
    output << ')';
  }
};

template <typename T, typename U>
auto Max(const Expression<T>& lhs, const Expression<U>& rhs) {
  return MaxExpr(lhs, rhs);
}

// clang-format off
#define V(x) Value {(x), #x}
// clang-format on

Estimated CalcCompleteAt(const EstimationData& eta) {
  return V(eta.delivery_started_at) + V(eta.delivery_duration);
}

Estimated CalcCompleteAtWithMax(const EstimationData& eta) {
  return V(eta.delivery_started_at) + Max(V(eta.delivery_duration), V(0s));
}

// Estimated DoesntCompile(const EstimationData& eta) {
//   unknown delivery_duration_expr;
//   if (SomeCondition()) {
//     delivery_duration_expr = V(eta.fallback_delivery_duration);
//   } else {
//     delivery_duration_expr =
//         V(eta.arrival_to_customer_at) - V(eta.delivery_started_at);
//   }

//   return Max(delivery_duration_expr, V(0s));
// }

#undef V

}  // namespace second_attempt

namespace third_attempt {

// clang-format off
template <typename T> class Expression {
  // clang-format on
public:
  virtual ~Expression() = default;

  virtual T Evaluate() const = 0;

  virtual void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const = 0;
  virtual void FillFormulaTo(std::ostream& output) const = 0;

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

  operator Estimated() const {
    return Estimated{.value = this->Evaluate(),
                     .formula = this->GetFormula(),
                     .variables = this->GetVariables()};
  }
};

// clang-format off
template <typename T> struct Value : Expression<T> {
  // clang-format on
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

// clang-format off
template <typename T> auto MakeValue(const T& x, std::string_view name) {
  // clang-format on
  return std::make_shared<Value<T>>(x, name);
}

template <typename T, typename U, typename OpType>
class BinaryOp : public Expression<OpType> {
protected:
  shared_ptr<Expression<T>> lhs_;
  shared_ptr<Expression<U>> rhs_;

public:
  BinaryOp(shared_ptr<Expression<T>> lhs, shared_ptr<Expression<U>> rhs)
      : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  void FillVariablesTo(
      std::unordered_map<std::string, std::string>& variables) const override {
    lhs_->FillVariablesTo(variables);
    rhs_->FillVariablesTo(variables);
  }
};

template <typename T, typename U>
using SumType_t = decltype(std::declval<T>() + std::declval<U>());

template <typename T, typename U>
class Sum : public BinaryOp<T, U, SumType_t<T, U>> {
public:
  using BinaryOp<T, U, SumType_t<T, U>>::BinaryOp;

  SumType_t<T, U> Evaluate() const override {
    return this->lhs_->Evaluate() + this->rhs_->Evaluate();
  }

  void FillFormulaTo(std::ostream& output) const override {
    this->lhs_->FillFormulaTo(output);
    output << " + ";
    this->rhs_->FillFormulaTo(output);
  }
};

template <typename U, template <class> typename T, typename V,
          template <class> typename W>
std::enable_if_t<std::is_base_of_v<Expression<U>, T<U>> &&
                     std::is_base_of_v<Expression<V>, W<V>>,
                 shared_ptr<Sum<U, V>>>
operator+(shared_ptr<T<U>> lhs, shared_ptr<W<V>> rhs) {
  return std::make_shared<Sum<U, V>>(std::move(lhs), std::move(rhs));
}

template <typename T, typename U>
using DiffType_t = decltype(std::declval<T>() - std::declval<U>());

template <typename T, typename U>
class Diff : public BinaryOp<T, U, DiffType_t<T, U>> {
public:
  using BinaryOp<T, U, DiffType_t<T, U>>::BinaryOp;

  DiffType_t<T, U> Evaluate() const override {
    return this->lhs_->Evaluate() + this->rhs_->Evaluate();
  }

  void FillFormulaTo(std::ostream& output) const override {
    this->lhs_->FillFormulaTo(output);
    output << " - ";
    this->rhs_->FillFormulaTo(output);
  }
};

template <typename U, template <class> typename T, typename V,
          template <class> typename W>
std::enable_if_t<std::is_base_of_v<Expression<U>, T<U>> &&
                     std::is_base_of_v<Expression<V>, W<V>>,
                 shared_ptr<Diff<U, V>>>
operator-(shared_ptr<T<U>> lhs, shared_ptr<W<V>> rhs) {
  return std::make_shared<Diff<U, V>>(std::move(lhs), std::move(rhs));
}

template <typename T, typename U>
using MaxType_t = decltype(MaxImpl(declval<T>(), std::declval<U>()));

template <typename T, typename U>
class MaxOp : public BinaryOp<T, U, DiffType_t<T, U>> {
public:
  using BinaryOp<T, U, DiffType_t<T, U>>::BinaryOp;

  MaxType_t<T, U> Evaluate() const override {
    return MaxImpl(this->lhs_->Evaluate(), this->rhs_->Evaluate());
  }

  void FillFormulaTo(std::ostream& output) const override {
    output << "max(";
    this->lhs_->FillFormulaTo(output);
    output << ", ";
    this->rhs_->FillFormulaTo(output);
    output << ')';
  }
};

template <typename U, template <class> typename T, typename V,
          template <class> typename W>
std::enable_if_t<std::is_base_of_v<Expression<U>, T<U>> &&
                     std::is_base_of_v<Expression<V>, W<V>>,
                 shared_ptr<MaxOp<U, V>>>
Max(shared_ptr<T<U>> lhs, shared_ptr<W<V>> rhs) {
  return std::make_shared<MaxOp<U, V>>(std::move(lhs), std::move(rhs));
}

// clang-format off
#define V(x) MakeValue(x, #x)
// clang-format on

Estimated CalcCompleteAt(const EstimationData& eta) {
  return *(V(eta.delivery_started_at) + V(eta.delivery_duration));
}

Estimated UsesSubexpression(const EstimationData& eta) {
  std::shared_ptr<Expression<Timepoint>> delivery_duration_expr;
  if (SomeCondition()) {
    delivery_duration_expr = V(eta.arrival_to_customer_at);
  } else {
    delivery_duration_expr =
        V(eta.delivery_started_at) + V(eta.fallback_delivery_duration);
  }

  return *(delivery_duration_expr + V(eta.fallback_delivery_duration));
}

// Estimated DoesntCompile(const EstimationData& eta) {
//   return *(V(eta.delivery_started_at) + V(eta.delivery_duration) + V(0s));
// }

#undef V

}  // namespace third_attempt

namespace fourth_attempt {

// clang-format off
template <typename T> struct Value {
  // clang-format on
  T value;
  std::string formula;
  std::unordered_map<std::string, std::string> variables;

  Value(T value, std::string_view name)
      : value(value), formula(name), variables({{formula, ToString(value)}}) {}

  Value(T value, std::string formula,
        std::unordered_map<std::string, std::string> variables)
      : value(value),
        formula(std::move(formula)),
        variables(std::move(variables)) {}

  template <typename U>
  Value(Value<U>&& that,
        std::enable_if_t<std::is_convertible_v<U, T>, void*> = nullptr)
      : value(that.value),
        formula(std::move(that.formula)),
        variables(std::move(that.variables)) {}

  operator Estimated() && {
    return Estimated{.value = this->value,
                     .formula = std::move(this->formula),
                     .variables = std::move(this->variables)};
  }
};

template <typename T, typename U>
auto operator+(Value<T> lhs, Value<U> rhs) {
  lhs.formula += " + ";
  lhs.formula += rhs.formula;
  lhs.variables.merge(rhs.variables);
  return Value{lhs.value + rhs.value, std::move(lhs.formula),
               std::move(lhs.variables)};
}

template <typename T, typename U>
auto operator-(Value<T> lhs, Value<U> rhs) {
  lhs.formula += " - ";
  lhs.formula += rhs.formula;
  lhs.variables.merge(rhs.variables);
  return Value{lhs.value - rhs.value, std::move(lhs.formula),
               std::move(lhs.variables)};
}

template <typename T, typename U>
auto Max(Value<T> lhs, Value<U> rhs) {
  std::string formula = std::move(lhs.formula);
  formula += "max(";
  std::rotate(formula.begin(), formula.end() - 4, formula.end());
  formula += ", ";
  formula += rhs.formula;
  formula += ')';

  lhs.variables.merge(rhs.variables);
  return Value{MaxImpl(lhs.value, rhs.value), std::move(formula),
               std::move(lhs.variables)};
}

// clang-format off
#define V(x) Value {x, #x}
// clang-format on

Estimated CalcCompleteAt(const EstimationData& eta) {
  return V(eta.delivery_started_at) + V(eta.delivery_duration);
}

Estimated UsesSubexpression(const EstimationData& eta) {
  const Value<Timepoint> delivery_duration_expr = [&] {
    if (SomeCondition()) {
      return V(eta.arrival_to_customer_at);
    } else {
      return V(eta.delivery_started_at) + V(eta.fallback_delivery_duration);
    }
  }();

  return delivery_duration_expr + V(eta.fallback_delivery_duration);
}

Estimated ComplexExpressions(const EstimationData& eta) {
  const Value<Timepoint> delivery_duration_expr = [&] {
    if (SomeCondition()) {
      return V(eta.arrival_to_customer_at);
    } else {
      return V(eta.delivery_started_at) + V(eta.fallback_delivery_duration);
    }
  }();

  const auto now = chrono::system_clock::now();
  return Max(delivery_duration_expr + V(eta.fallback_delivery_duration),
             V(now) + V(eta.fallback_delivery_duration) + V(0s));
}

}  // namespace fourth_attempt

namespace expression_templates {
struct Node {
  string_view value;

  constexpr Node(string_view s) : value(s) {}

  constexpr size_t Size() const { return value.size(); }

  void AssignTo(string& dest) const { dest += value; }
};

template <typename T, typename U>
struct StrSum {
  const T& lhs;
  const U& rhs;

  constexpr StrSum(const T& lhs, const U& rhs) : lhs(lhs), rhs(rhs) {}

  constexpr size_t Size() const { return lhs.Size() + rhs.Size(); }

  operator string() const {
    string result;
    result.reserve(this->Size());
    AssignTo(result);
    return result;
  }

  void AssignTo(string& dest) const {
    lhs.AssignTo(dest);
    rhs.AssignTo(dest);
  }
};

template <typename T>
constexpr StrSum<T, Node> operator + (const T& lhs, const Node& rhs) {
  return {lhs, rhs};
}

}  // namespace expression_templates

int main(int arc, char* argv[]) {
  const EstimationData eta{
      .delivery_started_at = chrono::system_clock::now() - chrono::hours(2),
      .arrival_to_customer_at =
          chrono::system_clock::now() - chrono::minutes(120 - 40),
      .delivery_duration = chrono::minutes(40),
      .fallback_delivery_duration = chrono::minutes(20),
  };
  {
    cout << "1st attempt:\n";
    auto estimated = first_attempt::CalcCompleteAt(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    cout << "2nd attempt:\n";
    auto estimated = second_attempt::CalcCompleteAt(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    auto estimated = second_attempt::CalcCompleteAtWithMax(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    cout << "3rd attempt:\n";
    auto estimated = third_attempt::CalcCompleteAt(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    auto estimated = third_attempt::UsesSubexpression(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    cout << "4th attempt:\n";
    auto estimated = fourth_attempt::CalcCompleteAt(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    auto estimated = fourth_attempt::UsesSubexpression(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    auto estimated = fourth_attempt::ComplexExpressions(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    using namespace expression_templates;
    string hello = Node("Hello") + Node(", ") + Node("world") + Node("!\n");
    cout << hello; 
  }
}

string ToString(Timepoint tp) { return std::format("{:%Y-%m-%d %H:%M}", tp); }

template <typename Rep, typename Period>
std::string ToString(const std::chrono::duration<Rep, Period>& value) {
  return std::format(
      "{}s", std::chrono::duration_cast<std::chrono::seconds>(value).count());
}

bool SomeCondition() { return true; }
