utils::EstimationContext UpdateDeliveryAt(EtaInfo& eta) {
  utils::EstimationContext context;
  context.variables[kDeliveryStartedAt] =
      ToString(eta.delivery_started_at.value());
  context.variables[std::string{kDeliveryDuration}] =
      ToString(eta.delivery_duration->value);
  context.formula = fmt::format("{} + {}", kDeliveryStartedAt, kDeliveryDuration);
  eta.delivery_at->value =
      eta.delivery_started_at.value() + eta.delivery_duration->value;
  context.value = ToString(eta.delivery_at->value);
  return context;
}
