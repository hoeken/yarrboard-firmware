var YB = typeof YB !== "undefined" ? YB : {};

function PWMChannel() {
  YB.BaseChannel.call(this, "pwm");
  this.pin = null;
  this.frequency = 1000;   // Hz
  this.minDuty = 0.0;      // 0..1
  this.maxDuty = 1.0;      // 0..1
  this.gamma = 1.0;
  this.initialDuty = 0.0;  // 0..1
}
PWMChannel.prototype = Object.create(YB.BaseChannel.prototype);
PWMChannel.prototype.constructor = PWMChannel;

PWMChannel.prototype.parseConfig = function (cfg) {
  if (!cfg) return this;

  if (cfg.pin != null) this.pin = cfg.pin | 0;
  if (cfg.frequency != null) this.frequency = cfg.frequency | 0;
  if (cfg.minDuty != null) this.minDuty = Number(cfg.minDuty);
  if (cfg.maxDuty != null) this.maxDuty = Number(cfg.maxDuty);
  if (cfg.gamma != null) this.gamma = Number(cfg.gamma);
  if (cfg.initialDuty != null) this.initialDuty = Number(cfg.initialDuty);

  // sanity checks
  if (this.minDuty < 0) this.minDuty = 0;
  if (this.maxDuty > 1) this.maxDuty = 1;
  if (this.maxDuty < this.minDuty) this.maxDuty = this.minDuty;
  if (this.initialDuty < this.minDuty) this.initialDuty = this.minDuty;
  if (this.initialDuty > this.maxDuty) this.initialDuty = this.maxDuty;

  return YB.BaseChannel.prototype.parseConfig.call(this, cfg);
};

YB.PWMChannel = PWMChannel;