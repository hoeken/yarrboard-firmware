(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

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
        numericality: { onlyInteger: true, greaterThan: 0 }
      },
      key: {
        presence: { allowEmpty: false },
        type: "string",
        length: { minimum: 1, maximum: 63 }
      },
      name: {
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
    var errors = this.validateConfig(cfg);
    if (!errors) {
      // keep existing keys unless overwritten
      this.cfg = Object.assign({}, this.cfg, cfg);

      this.id = cfg.id;
      this.key = cfg.key;
      this.enabled = cfg.enabled;
      this.name = cfg.name;
    }
    return errors;
  };

  // validate and set a single config key
  BaseChannel.prototype.setConfigEntry = function (key, value) {
    var schema = this.getConfigSchema()[key];
    if (!schema) return { [key]: ["unknown config key"] };

    // validate.js has validate.single(value, constraints)
    var errors = validate.single(value, schema);
    if (!errors) this.cfg[key] = value;
    return errors;
  };

  BaseChannel.prototype.saveConfig = function () {
    var errors = this.validateConfig(this.getConfig());
    if (!errors) {
      let command = {
        cmd: `config_${this.channelType}_channel`,
        id: this.id,
        config: this.getConfig(),
      };
      if (YB.client && typeof YB.client.send === "function") {
        YB.client.send(command, true);
      } else {
        console.warn("[YB] client.send not available; skipped send", command);
      }
    }
    return errors;
  };

  //
  // Data holds the non-config things about the channel eg. state
  //
  BaseChannel.prototype.loadData = function (data) {
    this.data = data || {};
  };

  BaseChannel.prototype.getData = function () {
    return this.data;
  };

  //
  // Nitty gritty of form handling.
  //
  BaseChannel.prototype.handleEditForm = function (newcfg, event) {

    //clear our errors
    for (const f of Object.keys(newcfg)) {
      $(`#f-${this.channelType}-${f}-${this.id}`).removeClass("is-valid is-invalid");
    }

    //check for errors, and then save it.
    let errors = this.loadConfig(newcfg);
    if (errors) {
      for (const field of Object.keys(errors)) {

        //add our invalid class
        const $el = $(`#f-${this.channelType}-${field}-${this.id}`);
        $el.removeClass("is-valid").addClass("is-invalid");

        // Find the right feedback element (works for form-floating and form-switch)
        const msg = errors[field][0];
        const $fb = $el.siblings(".invalid-feedback")
          .add($el.closest(".form-check, .form-floating").find(".invalid-feedback"))
          .first();
        $fb.text(msg);
      }
    } else {
      $(event.target).addClass("is-valid");
      this.saveConfig();
    }
  };

  //
  // Functions for generating the html for various channel specific UI elements
  //
  BaseChannel.prototype.generateControlUI = function () { };
  BaseChannel.prototype.generateEditUI = function () { };
  BaseChannel.prototype.generateStatsUI = function () { };
  BaseChannel.prototype.generateGraphsUI = function () { };

  //
  // Functions for updating the UI as new data/configs come in.
  //
  BaseChannel.prototype.updateControlUI = function () { };
  BaseChannel.prototype.updateEditUI = function () { };
  BaseChannel.prototype.updateStatsUI = function () { };
  BaseChannel.prototype.updateGraphsUI = function () { };

  //
  // Functions for setting up the UI hooks and callbacks
  //
  BaseChannel.prototype.setupControlUI = function () { };
  BaseChannel.prototype.setupEditUI = function () { };
  BaseChannel.prototype.setupStatsUI = function () { };
  BaseChannel.prototype.setupGraphsUI = function () { };

  YB.BaseChannel = BaseChannel;
  global.YB = YB; // expose to global

})(this);