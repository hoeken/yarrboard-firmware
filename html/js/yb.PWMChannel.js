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
  YB.BaseChannel.prototype.parseConfig.call(this, cfg);
};

YB.PWMChannel = PWMChannel;