var YB = typeof YB !== "undefined" ? YB : {};

function ADCChannel() {
  YB.BaseChannel.call(this, "adc");
}
ADCChannel.prototype = Object.create(YB.BaseChannel.prototype);
ADCChannel.prototype.constructor = ADCChannel;

ADCChannel.prototype.parseConfig = function (cfg) {
  YB.BaseChannel.prototype.parseConfig.call(this, cfg);

  this.type = String(cfg.type);
  this.displayDecimals = parseFloat(cfg.displayDecimals);
  this.units = String(cfg.units);
  this.useCalibrationTable = Boolean(cfg.useCalibrationTable);
  this.calibratedUnits = String(cfg.calibratedUnits);
  this.calibrationTable = cfg.calibrationTable;
};

YB.ADCChannel = ADCChannel;