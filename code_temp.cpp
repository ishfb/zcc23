Estimated CalcCompleteAt(
    const EstimationData& eta) {
    return *(
      V(eta.delivery_started_at) +
      V(eta.delivery_duration));
}

Estimated DoesntCompile(
    const EstimationData& eta) {
  return *(
      V(eta.delivery_started_at) +
      V(eta.delivery_duration) +
      V(0s));
}
