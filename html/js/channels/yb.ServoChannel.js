(function (global) { //private scope

  // work in the global YB namespace.
  var YB = window.YB || {};

  function ServoChannel() {
    YB.BaseChannel.call(this, "servo");
  }
  ServoChannel.prototype = Object.create(YB.BaseChannel.prototype);
  ServoChannel.prototype.constructor = ServoChannel;

  ServoChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.ServoChannel = ServoChannel;
  YB.ChannelRegistry.registerChannelType("servo", YB.ServoChannel)

  window.YB = YB; // <-- this line makes it global

})(this); //private scope