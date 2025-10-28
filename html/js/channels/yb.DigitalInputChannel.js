(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  function DigitalInputChannel() {
    YB.BaseChannel.call(this, "dio");
  }
  DigitalInputChannel.prototype = Object.create(YB.BaseChannel.prototype);
  DigitalInputChannel.prototype.constructor = DigitalInputChannel;

  DigitalInputChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.DigitalInputChannel = DigitalInputChannel;
  YB.ChannelRegistry.registerChannelType("dio", YB.DigitalInputChannel)

})(this); //private scope