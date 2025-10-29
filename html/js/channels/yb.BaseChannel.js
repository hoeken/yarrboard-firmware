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
  BaseChannel.prototype.generateControlUI() = function () { };
  BaseChannel.prototype.generateEditUI() = function () { };
  BaseChannel.prototype.generateStatsUI() = function () { };
  BaseChannel.prototype.generateGraphsUI() = function () { };

  //
  // Functions for updating the UI as new data/configs come in.
  //
  BaseChannel.prototype.updateControlUI() = function () { };
  BaseChannel.prototype.updateEditUI() = function () { };
  BaseChannel.prototype.updateStatsUI() = function () { };
  BaseChannel.prototype.updateGraphsUI() = function () { };

  //
  // Functions for setting up the UI hooks and callbacks
  //
  BaseChannel.prototype.setupControlUI() = function () { };
  BaseChannel.prototype.setupEditUI() = function () { };
  BaseChannel.prototype.setupStatsUI() = function () { };
  BaseChannel.prototype.setupGraphsUI() = function () { };


  YB.BaseChannel = BaseChannel;

})(this); //private scope