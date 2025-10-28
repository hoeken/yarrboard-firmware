var YB = typeof YB !== "undefined" ? YB : {};

// Simple type registry + factory
var YBChannelRegistry = {
  "adc": YB.ADCChannel,
  "dio": YB.DigitalInputChannel,
  "pwm": YB.PWMChannel,
  "relay": YB.RelayChannel,
  "servo": YB.ServoChannel,
  "stepper": YB.StepperChannel,
};

YB.registerChannelType = function (type, ctor) {
  YBChannelRegistry[type] = ctor;
};

YB.channelFromConfig = function (cfg, type) {
  if (!cfg) {
    throw new Error('Channel config must include a "type".');
  }
  var channel = YBChannelRegistry[type];
  if (!channel) {
    throw new Error("Unknown channel type: " + type);
  }

  let myChannel = new channel();
  channel.parseConfig(cfg);

  return channel;
};