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

  auto Evaluate() const { return std::max(lhs.Evaluate(), rhs.Evaluate()); }

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

#define V(x) \
  Value { (x), #x }

Estimated CalcCompleteAt(const EstimationData& eta) {
  return V(eta.delivery_started_at) + V(eta.delivery_duration);
}

Estimated CalcCompleteAtWithMax(const EstimationData& eta) {
  return V(eta.delivery_started_at) + Max(V(eta.delivery_duration), V(0s));
}

bool SomeCondition();

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

bool SomeCondition() { return true; }

#undef V

}  // namespace second_attempt

// namespace third_attempt {

// // clang-format off
// template <typename T> class Expression {
// // clang-format on
// public:
//   virtual ~Expression() = default;

//   virtual T Evaluate() const = 0;

// private:
//   virtual void FillVariablesTo(
//       std::unordered_map<std::string, std::string>& variables) const = 0;
//   virtual void FillFormulaTo(std::ostream& output) const = 0;

// public:
//   std::unordered_map<std::string, std::string> GetVariables() const {
//     std::unordered_map<std::string, std::string> variables;
//     FillVariablesTo(variables);
//     return variables;
//   }

//   virtual std::string GetFormula() const {
//     std::ostringstream output;
//     FillFormulaTo(output);
//     return output.str();
//   }
// };

// // clang-format off
// template <typename T> struct Value : Expression<T> {
// // clang-format on
//   const T& value;
//   std::string name;

//   Value(const T& value, std::string_view name) : value(value), name(name) {}

//   T Evaluate() const override { return value; }

//   void FillVariablesTo(
//       std::unordered_map<std::string, std::string>& variables) const override {
//     variables[name] = ToString(value);
//   }

//   void FillFormulaTo(std::ostream& output) const override { output << name; }
// };

// // clang-format off
// template <typename T> auto MakeValue(const T& x, std::string_view name) {
// // clang-format on
//   return std::make_shared<Value<T>>(x, name);
// }

// template <typename T, typename U>
// using SumType_t = decltype(std::declval<T>() + std::declval<U>());

// template <typename T, typename U>
// class Sum : public Expression<SumType_t<T, U>> {
// private:
//   shared_ptr<Expression<T>> lhs_;
//   shared_ptr<Expression<U>> rhs_;

// public:
//   Sum(shared_ptr<Expression<T>> lhs, shared_ptr<Expression<U>> rhs)
//       : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

//   SumType_t<T, U> Evaluate() const override {
//     return lhs_->Evaluate() + rhs_->Evaluate();
//   }

//   void FillVariablesTo(
//       std::unordered_map<std::string, std::string>& variables) const override {
//     lhs_->FillVariablesTo(variables);
//     rhs_->FillVariablesTo(variables);
//   }

//   void FillFormulaTo(std::ostream& output) const override {
//     lhs_->FillFormulaTo(output);
//     output << " + ";
//     rhs_->FillFormulaTo(output);
//   }
// };

// template <typename U, template <class> typename T, typename V,
//           template <class> typename W>
// std::enable_if_t<std::is_base_of_v<Expression<U>, T<U>> &&
//                      std::is_base_of_v<Expression<V>, W<V>>,
//                  shared_ptr<Sum<U, V>>>
// operator+(shared_ptr<T<U>> lhs, shared_ptr<W<V>> rhs) {
//   return std::make_shared<Sum<U, V>>(std::move(lhs), std::move(rhs));
// }

// // clang-format off
// template <typename T, typename U> class MaxExpr : public Expression<Sum<T, U>> {
//   // clang-format on
// private:
//   const T& lhs;
//   const U& rhs;

// public:
//   MaxExpr(const Expression<T>& lhs, const Expression<U>& rhs)
//       : lhs(lhs.AsChild()), rhs(rhs.AsChild()) {}

//   auto Evaluate() const { return std::max(lhs.Evaluate(), rhs.Evaluate()); }

//   void FillVariablesTo(
//       std::unordered_map<std::string, std::string>& variables) const {
//     lhs.FillVariablesTo(variables);
//     rhs.FillVariablesTo(variables);
//   }

//   void FillFormulaTo(std::ostream& output) const {
//     output << "max(";
//     lhs.FillFormulaTo(output);
//     output << ", ";
//     rhs.FillFormulaTo(output);
//     output << ')';
//   }
// };

// template <typename T, typename U>
// auto Max(const Expression<T>& lhs, const Expression<U>& rhs) {
//   return MaxExpr(lhs, rhs);
// }

// #define V(x) \
//   Value { (x), #x }

// Estimated CalcCompleteAt(const EstimationData& eta) {
//   return V(eta.delivery_started_at) + V(eta.delivery_duration);
// }

// bool SomeCondition();

// // Estimated DoesntCompile(const EstimationData& eta) {
// //   unknown delivery_duration_expr;
// //   if (SomeCondition()) {
// //     delivery_duration_expr = V(eta.fallback_delivery_duration);
// //   } else {
// //     delivery_duration_expr =
// //         V(eta.arrival_to_customer_at) - V(eta.delivery_started_at);
// //   }

// //   return Max(delivery_duration_expr, V(0s));
// // }

// bool SomeCondition() { return true; }

// #undef V

// }  // namespace third_attempt

int main(int arc, char* argv[]) {
  const EstimationData eta{
      .delivery_started_at = chrono::system_clock::now() - chrono::hours(2),
      .delivery_duration = chrono::minutes(40),
      .fallback_delivery_duration = chrono::minutes(20),
  };
  {
    auto estimated = first_attempt::CalcCompleteAt(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    auto estimated = second_attempt::CalcCompleteAt(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
  {
    auto estimated = second_attempt::CalcCompleteAtWithMax(eta);
    cout << estimated.formula << " = " << ToString(estimated.value) << endl;
  }
}

string ToString(Timepoint tp) { return std::format("{:%Y-%m-%d %H:%M}", tp); }

template <typename Rep, typename Period>
std::string ToString(const std::chrono::duration<Rep, Period>& value) {
  return std::format(
      "{}s", std::chrono::duration_cast<std::chrono::seconds>(value).count());
}
