var YB = typeof YB !== "undefined" ? YB : {};

function ServoChannel() {
  YB.BaseChannel.call(this, "servo");
}
ServoChannel.prototype = Object.create(YB.BaseChannel.prototype);
ServoChannel.prototype.constructor = ServoChannel;

ServoChannel.prototype.parseConfig = function (cfg) {
  YB.BaseChannel.prototype.parseConfig.call(this, cfg);
};

YB.ServoChannel = ServoChannel;