// Global namespace (safe re-declare across files)
var YB = typeof YB !== "undefined" ? YB : {};

// Base class for all channels
function BaseChannel(type) {
  this.type = type;   // "relay", "pwm", etc.
  this.id = null;
  this.name = null;
  this.enabled = true;
}

BaseChannel.prototype.parseConfig = function (cfg) {
  if (!cfg) return this;

  if (cfg.id != null) this.id = String(cfg.id);
  if (cfg.name != null) this.name = String(cfg.name);
  if (cfg.icon != null) this.icon = String(cfg.icon);
  if (cfg.enabled != null) this.enabled = !!cfg.enabled;

  if (cfg.meta && typeof cfg.meta === "object") {
    for (var k in cfg.meta) {
      if (Object.prototype.hasOwnProperty.call(cfg.meta, k)) {
        this.meta[k] = cfg.meta[k];
      }
    }
  }
  return this;
};

BaseChannel.prototype.toJSON = function () {
  return {
    type: this.type,
    id: this.id,
    name: this.name,
    icon: this.icon,
    enabled: this.enabled,
    meta: this.meta
  };
};

YB.BaseChannel = BaseChannel;