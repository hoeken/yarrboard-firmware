var YB = typeof YB !== "undefined" ? YB : {};

// Simple type registry + factory
var YBChannelRegistry = {
  relay: YB.RelayChannel,
  pwm: YB.PWMChannel
};

YB.registerChannelType = function (type, ctor) {
  YBChannelRegistry[type] = ctor;
};

YB.channelFromConfig = function (cfg) {
  if (!cfg || !cfg.type) {
    throw new Error('Channel config must include a "type".');
  }
  var Ctor = YBChannelRegistry[cfg.type];
  if (!Ctor) {
    throw new Error("Unknown channel type: " + cfg.type);
  }
  return new Ctor().parseConfig(cfg);
};

YB.channelsFromArray = function (arr) {
  var out = [];
  for (var i = 0; i < arr.length; i++) {
    out.push(YB.channelFromConfig(arr[i]));
  }
  return out;
};