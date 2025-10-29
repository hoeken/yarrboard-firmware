(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  function ADCChannel() {
    YB.BaseChannel.call(this, "adc");
  }
  ADCChannel.prototype = Object.create(YB.BaseChannel.prototype);
  ADCChannel.prototype.constructor = ADCChannel;

  ADCChannel.prototype.getConfigSchema = function () {
    let schema = YB.BaseChannel.prototype.getConfigSchema.call(this);

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
  }

  YB.ADCChannel = ADCChannel;
  YB.ChannelRegistry.registerChannelType("adc", YB.ADCChannel)

})(this); //private scope