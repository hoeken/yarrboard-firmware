// Global namespace (safe re-declare across files)
var YB = typeof YB !== "undefined" ? YB : {};

// Base class for all channels
function BaseChannel(type) {
  this.channelType = type;
}

BaseChannel.prototype.parseConfig = function (cfg) {
  this.id = parseInt(cfg.id);
  this.name = String(cfg.name);
  this.key = String(cfg.key);
  this.enabled = Boolean(cfg.enabled);
};

BaseChannel.prototype.toJSON = function () {
  return {
    id: this.id,
    name: this.name,
    key: this.key,
    enabled: this.enabled,
  };
};

YB.BaseChannel = BaseChannel;