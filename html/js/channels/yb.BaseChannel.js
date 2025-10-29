(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  // Base class for all channels
  function BaseChannel(type) {
    this.channelType = type;
    this.cfg = {};
    this.data = {};
  }

  //
  // Schema for validation of the config object.
  //
  BaseChannel.prototype.getConfigSchema = function () {
    return {
      id: {
        presence: { allowEmpty: false },
        numericality: {
          onlyInteger: true,
          greaterThan: 0
        }
      },
      name: {
        presence: { allowEmpty: false },
        type: "string",
        length: { minimum: 1, maximum: 63 }
      },
      key: {
        presence: { allowEmpty: false },
        type: "string",
        length: { minimum: 1, maximum: 63 }
      },
      enabled: {
        presence: true,
        inclusion: {
          within: [true, false],
          message: "^enabled must be boolean"
        }
      }
    };
  };

  //
  // Configuration settings for the channel itself
  //
  BaseChannel.prototype.getConfig = function () {
    return this.cfg;
  };

  BaseChannel.prototype.validateConfig = function (cfg) {
    return validate(cfg, this.getConfigSchema());
  };

  BaseChannel.prototype.loadConfig = function (cfg) {
    let errors = this.validateConfig(cfg);
    if (!errors) {
      this.cfg = cfg;
    }
  };

  BaseChannel.prototype.setConfigEntry = function (key, value) {
    let schema = this.getConfigSchema();
    let errors = validate(this.cfg, schema[key]);
    if (!errors)
      this.cfg[key] = value;
    return errors;
  };

  BaseChannel.prototype.saveConfig = function () {
    let errors = this.validateConfig(this.getConfig());
    if (!errors) {
      let command = {
        cmd: "config_channel",
        type: this.channelType,
        config: this.getConfig(),
      };
      YB.client.send(command, true);
    }
  };

  //
  // Data holds the non-config things about the channel eg. state
  //
  BaseChannel.prototype.loadData = function (data) {
    this.data = data;
  };

  BaseChannel.prototype.getData() = function () {
    return this.data;
  };

  //
  // Functions for generating the html for various channel specific UI elements
  //
  BaseChannel.prototype.generateControlUI() = function () {
    throw new Error("generateControlUI() not implemented");
  };

  BaseChannel.prototype.generateEditUI() = function () {
    throw new Error("generateEditUI() not implemented");
  };

  BaseChannel.prototype.generateStatsUI() = function () {
    throw new Error("generateStatsUI() not implemented");
  };

  BaseChannel.prototype.generateGraphsUI() = function () {
    throw new Error("generateStatsUI() not implemented");
  };

  //
  // Functions for updating the UI as new data/configs come in.
  //
  BaseChannel.prototype.updateControlUI() = function () {
    throw new Error("updateControlUI() not implemented");
  };

  BaseChannel.prototype.updateEditUI() = function () {
    throw new Error("updateEditUI() not implemented");
  };

  BaseChannel.prototype.updateStatsUI() = function () {
    throw new Error("updateStatsUI() not implemented");
  };

  BaseChannel.prototype.updateGraphsUI() = function () {
    throw new Error("updateGraphsUI() not implemented");
  };

  //
  // Functions for setting up the UI hooks and callbacks
  //
  BaseChannel.prototype.setupControlUI() = function () {
    throw new Error("setupControlUI() not implemented");
  };

  BaseChannel.prototype.setupEditUI() = function () {
    throw new Error("setupEditUI() not implemented");
  };

  BaseChannel.prototype.setupStatsUI() = function () {
    throw new Error("setupStatsUI() not implemented");
  };

  BaseChannel.prototype.setupGraphsUI() = function () {
    throw new Error("setupGraphsUI() not implemented");
  };


  YB.BaseChannel = BaseChannel;

})(this); //private scope