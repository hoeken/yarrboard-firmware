(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  // Channel registry + factory under YB
  YB.ChannelRegistry = {
    // Map of type keys to constructor functions
    channels: {},
    // "adc": YB.ADCChannel,
    // "dio": YB.DigitalInputChannel,
    // "pwm": YB.PWMChannel,
    // "relay": YB.RelayChannel,
    // "servo": YB.ServoChannel,
    // "stepper": YB.StepperChannel

    // Register or override a channel type
    registerChannelType: function (type, ctor) {
      this.channels[type] = ctor;
    },

    // Create a channel instance from a config object
    channelFromConfig: function (cfg, type) {
      if (!cfg) {
        throw new Error('Invalid channel config.');
      }

      var ctor = this.channels[type];
      if (!ctor) {
        throw new Error("Unknown channel type: " + type);
      }

      var instance = new ctor();

      // Prefer instance method if available
      if (typeof instance.parseConfig === "function") {
        instance.parseConfig(cfg);
      } else if (typeof ctor.parseConfig === "function") {
        // Fallback if you keep a static parseConfig on the constructor
        ctor.parseConfig(cfg, instance);
      }

      return instance;
    }
  };
})(this); //private scope