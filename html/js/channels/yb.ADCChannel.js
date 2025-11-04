(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  // ----- ADCChannel -----
  function ADCChannel() {
    YB.BaseChannel.call(this, "adc");
  }
  ADCChannel.prototype = Object.create(YB.BaseChannel.prototype);
  ADCChannel.prototype.constructor = ADCChannel;

  ADCChannel.prototype.getConfigSchema = function () {
    // copy base schema to avoid mutating the base literal
    var base = YB.BaseChannel.prototype.getConfigSchema.call(this);
    var schema = Object.assign({}, base);

    // Channel-specific fields
    schema.type = {
      presence: { allowEmpty: false },
      type: "string",
      length: { minimum: 1, maximum: 32 }
    };

    // Integer between 0–6 (display precision)
    schema.displayDecimals = {
      numericality: {
        onlyInteger: true,
        greaterThanOrEqualTo: 0,
        lessThanOrEqualTo: 4
      }
    };

    // Boolean flag for whether calibration is used
    schema.useCalibrationTable = {
      inclusion: {
        within: [true, false],
        message: "^useCalibrationTable must be boolean"
      }
    };

    // Optional calibrated units string
    schema.calibratedUnits = {
      type: "string",
      length: { minimum: 0, maximum: 7 }
    };

    // Optional calibration table — must be array if present
    schema.calibrationTable = {
      type: "array"
    };

    return schema;
  };

  // Export
  YB.ADCChannel = ADCChannel;

  YB.ChannelRegistry.registerChannelType("adc", YB.ADCChannel)

  // expose to global
  global.YB = YB;
})(this);
