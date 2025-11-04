(function (global) { //private scope

  // work in the global YB namespace.
  var YB = window.YB || {};

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

  window.YB = YB; // <-- this line makes it global

})(this); //private scope