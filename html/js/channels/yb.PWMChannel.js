(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  function PWMChannel() {
    YB.BaseChannel.call(this, "pwm");
  }
  PWMChannel.prototype = Object.create(YB.BaseChannel.prototype);
  PWMChannel.prototype.constructor = PWMChannel;

  PWMChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.PWMChannel = PWMChannel;
  YB.ChannelRegistry.registerChannelType("pwm", YB.PWMChannel)

})(this); //private scope