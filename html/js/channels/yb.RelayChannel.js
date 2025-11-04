(function (global) { //private scope

  // work in the global YB namespace.
  var YB = window.YB || {};

  function RelayChannel() {
    YB.BaseChannel.call(this, "relay");
  }
  RelayChannel.prototype = Object.create(YB.BaseChannel.prototype);
  RelayChannel.prototype.constructor = RelayChannel;

  RelayChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.RelayChannel = RelayChannel;
  YB.ChannelRegistry.registerChannelType("relay", YB.RelayChannel)

  window.YB = YB; // <-- this line makes it global

})(this); //private scope