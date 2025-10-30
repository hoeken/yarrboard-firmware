(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  // Channel registry + factory under YB
  YB.ChannelRegistry = {
    // Map of type keys to channel constructors
    // "pwm": YB.PWMChannel,
    // "relay": YB.RelayChannel,
    //  etc.
    channelConstructors: {},

    //this is where our actual channel objects will live
    // "pwm": [{ch1}, {ch2}, etc],
    // "relay": [{ch1}, {ch2}, etc]
    //  etc.
    channels: {},

    // Register or override a channel type
    registerChannelType: function (type, ctor) {
      this.channelConstructors[type] = ctor;
    },

    // Create a channel instance from a config object
    channelFromConfig: function (cfg, type) {
      if (!cfg) {
        throw new Error('Invalid channel config.');
      }

      var ctor = this.channelConstructors[type];
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

      this.addChannel(instance);

      return instance;
    },

    addChannel(instance) {
      if (typeof instance.channelType !== "string" || !instance.channelType.length) {
        throw new Error("Invalid channel type.");
      }
      if (!instance) {
        throw new Error("Channel instance is required.");
      }

      // Ensure array exists
      if (!this.channels.hasOwnProperty(instance.channelType)) {
        this.channels[instance.channelType] = [];
      }

      // Add it
      this.channels[instance.channelType].push(instance);

      return instance;
    },

    getChannelById(id, type) {
      if (typeof type !== "string" || !type.length) {
        throw new Error("Type must be a non-empty string.");
      }
      if (typeof id !== "number") {
        throw new Error("ID must be a number.");
      }

      var arr = this.channels[type];
      if (!arr || !arr.length) {
        return null;
      }

      for (var i = 0; i < arr.length; i++) {
        if (arr[i].id === id) {
          return arr[i];
        }
      }

      return null;
    },

    getChannelByKey(key, type) {
      if (typeof type !== "string" || !type.length) {
        throw new Error("Type must be a non-empty string.");
      }
      if (typeof key !== "string" || !key.length) {
        throw new Error("Key must be a non-empty string.");
      }

      var arr = this.channels[type];
      if (!arr || !arr.length) {
        return null;
      }

      for (var i = 0; i < arr.length; i++) {
        if (arr[i].key === key) {
          return arr[i];
        }
      }

      return null;
    },

    loadAllChannels: function (cfg) {
      for (var ctype of this.channelConstructors) {
        if (!cfg.hasOwnProperty(ctype))
          continue;

        for (var channel_config of list) {
          let ch = this.channelFromConfig(channel_config, ctype);

          ch.generateControlUI();
          ch.setupControlUI();

          ch.generateEditUI();
          ch.setupEditUI();

          ch.generateStatsUI();
          ch.setupStatsUI();

          ch.generateGraphsUI();
          ch.setupGraphsUI();
        }
      }
    },

    updateAllChannels: function (update) {
      for (var ctype of this.channels) {
        if (!update.hasOwnProperty(ctype))
          continue;

        for (chdata of update[ctype]) {
          let ch = this.getChannelById(chdata.id);
          ch.loadData(chdata);

          if (ch.updateControlUI)
            ch.updateControlUI();
          if (ch.updateEditUI)
            ch.updateEditUI();
          if (ch.updateStatsUI)
            ch.updateStatsUI();
          if (ch.updateGraphsUI)
            ch.updateGraphsUI();
        }
      }
    },
  };
})(this); //private scope