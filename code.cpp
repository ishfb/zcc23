#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

using Timepoint = chrono::system_clock::time_point;

string ToString(Timepoint);
string ToString(chrono::seconds);

struct Estimated {
  Timepoint value;
  string formula;
  unordered_map<string, string> variables;
};

struct EstimationData {
  Timepoint delivery_started_at;
  chrono::seconds delivery_duration;
  chrono::seconds fallback_delivery_duration;
};

namespace first_attempt {

Estimated CalcCompleteAt(const EstimationData& eta) {
  Estimated context;
  context.variables["delivery_started_at"] = ToString(eta.delivery_started_at);
  context.variables[std::string{"delivery_duration"}] =
      ToString(eta.delivery_duration);
  context.formula = "delivery_started_at + delivery_duration";
  context.value = eta.delivery_started_at + eta.delivery_duration;

  return context;
}

}  // namespace first_attempt

int main(int arc, char* argv[]) {
  const EstimationData eta{
      .delivery_started_at = chrono::system_clock::now() - chrono::hours(2),
      .delivery_duration = chrono::minutes(40),
      .fallback_delivery_duration = chrono::minutes(20),
  };
  auto estimated = first_attempt::CalcCompleteAt(eta);
  cout << estimated.formula;
}

string ToString(Timepoint) { return ""; }
string ToString(chrono::seconds) { return ""; }
