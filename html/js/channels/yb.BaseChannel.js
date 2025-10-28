(function (global) { //private scope

  var YB = typeof YB !== "undefined" ? YB : {};

  // Base class for all channels
  function BaseChannel(type) {
    this.channelType = type;
    this.cfg = {};
    this.data = {};
  }

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
        length: { minimum: 1, maximum: 32 }
      },
      key: {
        presence: { allowEmpty: false },
        type: "string",
        length: { minimum: 1, maximum: 32 }
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

  BaseChannel.prototype.getConfig = function () {
    return this.cfg;
  };

  BaseChannel.prototype.validateConfig = function (cfg) {
    return validate(cfg, this.getConfigSchema());
  };

  BaseChannel.prototype.loadConfig = function (cfg) {
    let errors = this.validateConfig(cfg);
    if (!errors)
      this.cfg = cfg;
  };

  BaseChannel.prototype.saveConfig = function () {
    let errors = this.validateConfig(this.getConfig());
    if (!errors) {
      let command = this.getConfig();
      command.cmd = `config_${this.channelType}_channel`;
      YB.client.send(command, true);
    }
  };

  BaseChannel.prototype.loadData = function (data) {
    this.data = data;
  };

  BaseChannel.prototype.getData() = function () {
    return this.data;
  };

  YB.BaseChannel = BaseChannel;

})(this); //private scope