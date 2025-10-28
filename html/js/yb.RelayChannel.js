var YB = typeof YB !== "undefined" ? YB : {};

function RelayChannel() {
  YB.BaseChannel.call(this, "relay");
}
RelayChannel.prototype = Object.create(YB.BaseChannel.prototype);
RelayChannel.prototype.constructor = RelayChannel;

RelayChannel.prototype.parseConfig = function (cfg) {
  if (!cfg) return this;

  return YB.BaseChannel.prototype.parseConfig.call(this, cfg);
};

YB.RelayChannel = RelayChannel;