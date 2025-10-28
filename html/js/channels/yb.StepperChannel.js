(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  function StepperChannel() {
    YB.BaseChannel.call(this, "stepper");
  }
  StepperChannel.prototype = Object.create(YB.BaseChannel.prototype);
  StepperChannel.prototype.constructor = StepperChannel;

  StepperChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.StepperChannel = StepperChannel;
  YB.ChannelRegistry.registerChannelType("stepper", YB.StepperChannel)

})(this); //private scope