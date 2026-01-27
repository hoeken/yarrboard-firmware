(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};


  // Base class for all channels
  function Brineomatic() {
    this.lastChartUpdate = Date.now();

    this.resultText = {
      "STARTUP": "Starting up.",
      "SUCCESS": "Success",
      "SUCCESS_TIME": "Success: Time OK",
      "SUCCESS_VOLUME": "Success: Volume OK",
      "SUCCESS_TANK_LEVEL": "Success: Tank Full",
      "SUCCESS_SALINITY": "Success: Salinity OK",
      "USER_STOP": "Stopped by user",

      "ERR_FILTER_PRESSURE_TIMEOUT": "Filter pressure timeout",
      "ERR_FILTER_PRESSURE_LOW": "Filter pressure low",
      "ERR_FILTER_PRESSURE_HIGH": "Filter pressure high",

      "ERR_MEMBRANE_PRESSURE_TIMEOUT": "Membrane pressure timeout",
      "ERR_MEMBRANE_PRESSURE_LOW": "Membrane pressure low",
      "ERR_MEMBRANE_PRESSURE_HIGH": "Membrane pressure high",

      "ERR_PRODUCT_FLOWRATE_TIMEOUT": "Product flowrate timeout",
      "ERR_PRODUCT_FLOWRATE_LOW": "Product flowrate low",
      "ERR_PRODUCT_FLOWRATE_HIGH": "Product flowrate high",

      "ERR_FLUSH_FLOWRATE_LOW": "Flush flowrate low",
      "ERR_FLUSH_FILTER_PRESSURE_LOW": "Flush filter pressure low",
      "ERR_FLUSH_VALVE_ON": "Flush valve not closed",
      "ERR_FLUSH_TIMEOUT": "Flush timed out",

      "ERR_BRINE_FLOWRATE_LOW": "Brine flowrate low",
      "ERR_TOTAL_FLOWRATE_LOW": "Total flowrate low",

      "ERR_DIVERTER_VALVE_OPEN": "Diverter valve not closing",

      "ERR_PRODUCT_SALINITY_TIMEOUT": "Product salinity timeout",
      "ERR_PRODUCT_SALINITY_HIGH": "Product salinity high",

      "ERR_PRODUCTION_TIMEOUT": "Production timeout",
      "ERR_MOTOR_TEMPERATURE_HIGH": "Motor temperature high",
    };

    this.handleConfigMessage = this.handleConfigMessage.bind(this);
    this.handleUpdateMessage = this.handleUpdateMessage.bind(this);
    this.handleStatsMessage = this.handleStatsMessage.bind(this);

    this.handleBrineomaticConfigSave = this.handleBrineomaticConfigSave.bind(this);
    this.handleHardwareConfigSave = this.handleHardwareConfigSave.bind(this);
    this.handleSafeguardsConfigSave = this.handleSafeguardsConfigSave.bind(this);

    YB.App.onMessage("config", this.handleConfigMessage);
    YB.App.onMessage("update", this.handleUpdateMessage);
    YB.App.onMessage("stats", this.handleStatsMessage);

    this.idle = this.idle.bind(this);
    this.startAutomatic = this.startAutomatic.bind(this);
    this.startDuration = this.startDuration.bind(this);
    this.startVolume = this.startVolume.bind(this);
    this.flushAutomatic = this.flushAutomatic.bind(this);
    this.flushDuration = this.flushDuration.bind(this);
    this.flushVolume = this.flushVolume.bind(this);
    this.pickle = this.pickle.bind(this);
    this.depickle = this.depickle.bind(this);
    this.stop = this.stop.bind(this);
    this.manual = this.manual.bind(this);

    this.toggleBoostPump = this.toggleBoostPump.bind(this);
    this.toggleHighPressurePump = this.toggleHighPressurePump.bind(this);
    this.toggleDiverterValve = this.toggleDiverterValve.bind(this);
    this.toggleFlushValve = this.toggleFlushValve.bind(this);
    this.toggleCoolingFan = this.toggleCoolingFan.bind(this);
  }

  Brineomatic.prototype.buildGaugeSetup = function () {

    let bootstrapColors = YB.Util.getBootstrapColors();

    this.gaugeSetup = {
      "motor_temperature": {
        get thresholds() {
          const celsiusThresholds = [60, 70, 100];
          return celsiusThresholds.map(temp => YB.bom.convertTemperature(temp, "C", YB.App.config.brineomatic.temperature_units));
        },
        "colors": [bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "water_temperature": {
        get thresholds() {
          const celsiusThresholds = [10, 30, 40, 50];
          return celsiusThresholds.map(temp => YB.bom.convertTemperature(temp, "C", YB.App.config.brineomatic.temperature_units));
        },
        "colors": [bootstrapColors.primary, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "filter_pressure": {
        get thresholds() {
          const barThresholds = [0, 0.34, 0.69, 2.76, 3.10, 3.45];
          return barThresholds.map(pressure => YB.bom.convertPressure(pressure, "Bar", YB.App.config.brineomatic.pressure_units));
        },
        "colors": [bootstrapColors.secondary, bootstrapColors.danger, bootstrapColors.warning, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "membrane_pressure": {
        get thresholds() {
          const barThresholds = [0, 41.4, 48.3, 62.1, 69.0];
          return barThresholds.map(pressure => YB.bom.convertPressure(pressure, "Bar", YB.App.config.brineomatic.pressure_units));
        },
        "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.primary, bootstrapColors.success, bootstrapColors.danger]
      },
      "product_salinity": {
        "thresholds": [1, 300, 400, 1500],
        "colors": [bootstrapColors.secondary, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "brine_salinity": {
        "thresholds": [1, 750, 1500],
        "colors": [bootstrapColors.secondary, bootstrapColors.primary, bootstrapColors.success]
      },
      "product_flowrate": {
        get thresholds() {
          const lphThresholds = [20, 100, 180, 200, 250];
          return lphThresholds.map(flowrate => YB.bom.convertFlowrate(flowrate, "lph", YB.App.config.brineomatic.flowrate_units));
        },
        "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "brine_flowrate": {
        get thresholds() {
          const lphThresholds = [100, 300];
          return lphThresholds.map(flowrate => YB.bom.convertFlowrate(flowrate, "lph", YB.App.config.brineomatic.flowrate_units));
        },
        "colors": [bootstrapColors.secondary, bootstrapColors.success]
      },
      "total_flowrate": {
        get thresholds() {
          const lphThresholds = [100, 600];
          return lphThresholds.map(flowrate => YB.bom.convertFlowrate(flowrate, "lph", YB.App.config.brineomatic.flowrate_units));
        },
        "colors": [bootstrapColors.secondary, bootstrapColors.success]
      },
      "tank_level": {
        "thresholds": [10, 20, 100],
        "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.success]
      },
      "volume": {
        "thresholds": [0, 1],
        "colors": [bootstrapColors.secondary, bootstrapColors.success]
      }
    }
  }

  Brineomatic.prototype.createGauges = function () {
    this.motorTemperatureGauge = c3.generate({
      bindto: '#motorTemperatureGauge',
      data: {
        columns: [
          ['Motor Temperature', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortTemperatureUnits(YB.App.config.brineomatic.temperature_units);
            return `${value}°${short}`;
          },
          show: true
        },
        min: Math.round(YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units)),
        max: Math.round(YB.bom.convertTemperature(80, "C", YB.App.config.brineomatic.temperature_units))
      },
      color: {
        pattern: this.gaugeSetup.motor_temperature.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.motor_temperature.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.waterTemperatureGauge = c3.generate({
      bindto: '#waterTemperatureGauge',
      data: {
        columns: [
          ['Water Temperature', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortTemperatureUnits(YB.App.config.brineomatic.temperature_units);
            return `${value}°${short}`;
          },
          show: true
        },
        min: Math.round(YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units)),
        max: Math.round(YB.bom.convertTemperature(50, "C", YB.App.config.brineomatic.temperature_units))
      },
      color: {
        pattern: this.gaugeSetup.water_temperature.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.water_temperature.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.filterPressureGauge = c3.generate({
      bindto: '#filterPressureGauge',
      data: {
        columns: [
          ['Filter Pressure', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortPressureUnits(YB.App.config.brineomatic.pressure_units);
            return `${value} ${short}`;
          },
          show: true
        },
        min: Math.round(YB.bom.convertPressure(0, "Bar", YB.App.config.brineomatic.pressure_units)),
        max: Math.round(YB.bom.convertPressure(3.45, "Bar", YB.App.config.brineomatic.pressure_units)),
      },
      color: {
        pattern: this.gaugeSetup.filter_pressure.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.filter_pressure.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.membranePressureGauge = c3.generate({
      bindto: '#membranePressureGauge',
      data: {
        columns: [
          ['Membrane Pressure', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortPressureUnits(YB.App.config.brineomatic.pressure_units);
            return `${value} ${short}`;
          },
          show: true
        },
        min: Math.round(YB.bom.convertPressure(0, "Bar", YB.App.config.brineomatic.pressure_units)),
        max: Math.round(YB.bom.convertPressure(69.0, "Bar", YB.App.config.brineomatic.pressure_units)),
      },
      color: {
        pattern: this.gaugeSetup.membrane_pressure.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.membrane_pressure.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.productSalinityGauge = c3.generate({
      bindto: '#productSalinityGauge',
      data: {
        columns: [
          ['Product Salinity', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            return `${value} PPM`;
          },
          show: true
        },
        min: 0,
        max: 1500,
      },
      color: {
        pattern: this.gaugeSetup.product_salinity.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.product_salinity.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.brineSalinityGauge = c3.generate({
      bindto: '#brineSalinityGauge',
      data: {
        columns: [
          ['Brine Salinity', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            return `${value} PPM`;
          },
          show: true
        },
        min: 0,
        max: 1500,
      },
      color: {
        pattern: this.gaugeSetup.brine_salinity.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.brine_salinity.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.productFlowrateGauge = c3.generate({
      bindto: '#productFlowrateGauge',
      data: {
        columns: [
          ['Product Flowrate', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortFlowrateUnits(YB.App.config.brineomatic.flowrate_units);
            return `${value} ${short}`;
          },
          show: true
        },
        min: 0,
        max: 250,
      },
      color: {
        pattern: this.gaugeSetup.product_flowrate.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.product_flowrate.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.brineFlowrateGauge = c3.generate({
      bindto: '#brineFlowrateGauge',
      data: {
        columns: [
          ['Brine Flowrate', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortFlowrateUnits(YB.App.config.brineomatic.flowrate_units);
            return `${value} ${short}`;
          },
          show: true
        },
        min: 0,
        max: 600,
      },
      color: {
        pattern: this.gaugeSetup.brine_flowrate.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.brine_flowrate.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.totalFlowrateGauge = c3.generate({
      bindto: '#totalFlowrateGauge',
      data: {
        columns: [
          ['Total Flowrate', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            let short = YB.bom.getShortFlowrateUnits(YB.App.config.brineomatic.flowrate_units);
            return `${value} ${short}`;
          },
          show: true
        },
        min: 0,
        max: 600,
      },
      color: {
        pattern: this.gaugeSetup.total_flowrate.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.total_flowrate.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    this.tankLevelGauge = c3.generate({
      bindto: '#tankLevelGauge',
      data: {
        columns: [
          ['Tank Level', 0]
        ],
        type: 'gauge',
      },
      gauge: {
        label: {
          format: function (value, ratio) {
            return `${value}%`;
          },
          show: true
        },
        min: 0,
        max: 100,
      },
      color: {
        pattern: this.gaugeSetup.tank_level.colors,
        threshold: {
          unit: 'value',
          values: this.gaugeSetup.tank_level.thresholds
        }
      },
      size: { height: 130, width: 200 },
      interaction: { enabled: false },
      transition: { duration: 0 },
      legend: { hide: true }
    });

    // Define the data
    this.timeData = ['x'];
    this.motorTemperatureData = ['Motor Temperature'];
    this.waterTemperatureData = ['Water Temperature'];
    this.filterPressureData = ['Filter Pressure'];
    this.membranePressureData = ['Membrane Pressure'];
    this.productSalinityData = ['Product Salinity'];
    this.productFlowrateData = ['Product Flowrate'];
    this.tankLevelData = ['Tank Level'];

    this.temperatureChart = c3.generate({
      bindto: '#temperatureChart',
      data: {
        x: 'x', // Define the x-axis data identifier
        xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
        columns: [
          this.timeData,
          this.motorTemperatureData,
          this.waterTemperatureData
        ],
        type: 'line' // Line chart
      },
      axis: {
        x: {
          type: 'timeseries',
          label: 'Time',
          tick: {
            format: '%H:%M:%S',
            culling: true,
            fit: true
          }
        },
        y: {
          label: 'Temperature (°C)',
          min: 0,
          max: 80 // Adjust as needed
        }
      },
      point: { show: false },
      interaction: { enabled: true },
      transition: { duration: 0 }
    });

    this.pressureChart = c3.generate({
      bindto: '#pressureChart',
      data: {
        x: 'x', // Define the x-axis data identifier
        xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
        columns: [
          this.timeData,
          this.filterPressureData,
          this.membranePressureData
        ],
        type: 'line' // Line chart
      },
      axis: {
        x: {
          type: 'timeseries',
          label: 'Time',
          tick: {
            format: '%H:%M:%S'
          }
        },
        y: {
          label: 'Pressure (Bar))',
          min: 0,
          max: 69.0
        }
      },
      point: { show: false },
      interaction: { enabled: true },
      transition: { duration: 0 }
    });

    this.productSalinityChart = c3.generate({
      bindto: '#productSalinityChart',
      data: {
        x: 'x', // Define the x-axis data identifier
        xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
        columns: [
          this.timeData,
          this.productSalinityData
        ],
        type: 'line' // Line chart
      },
      axis: {
        x: {
          type: 'timeseries',
          label: 'Time',
          tick: {
            format: '%H:%M:%S'
          }
        },
        y: {
          label: 'Product Salinity (PPM))',
          min: 0,
          max: 1500
        }
      },
      point: { show: false },
      interaction: { enabled: true },
      transition: { duration: 0 }
    });

    this.productFlowrateChart = c3.generate({
      bindto: '#productFlowrateChart',
      data: {
        x: 'x', // Define the x-axis data identifier
        xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
        columns: [
          this.timeData,
          this.productFlowrateData
        ],
        type: 'line' // Line chart
      },
      axis: {
        x: {
          type: 'timeseries',
          label: 'Time',
          tick: {
            format: '%H:%M:%S'
          }
        },
        y: {
          label: 'Product Flowrate (LPH))',
          min: 0,
          max: 200
        }
      },
      point: { show: false },
      interaction: { enabled: true },
      transition: { duration: 0 }
    });

    this.tankLevelChart = c3.generate({
      bindto: '#tankLevelChart',
      data: {
        x: 'x', // Define the x-axis data identifier
        xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
        columns: [
          this.timeData,
          this.tankLevelData
        ],
        type: 'line' // Line chart
      },
      axis: {
        x: {
          type: 'timeseries',
          label: 'Time',
          tick: {
            format: '%H:%M:%S'
          }
        },
        y: {
          label: 'Tank Level (%))',
          min: 0,
          max: 100
        }
      },
      point: { show: false },
      interaction: { enabled: true },
      transition: { duration: 0 }
    });
  }

  Brineomatic.prototype.handleConfigMessage = function (msg) {
    if (msg.brineomatic) {

      //build our UI
      YB.App.getPage("home").setContent(this.generateControlUI());

      //we're only using part of the channels
      YB.ChannelRegistry.loadAllChannels(msg);
      YB.App.removeSettingsPanel("relay");
      YB.App.removeSettingsPanel("servo");
      YB.App.removeSettingsPanel("stepper");

      let brineomaticPanel = new YB.SettingsPanel({
        name: 'brineomatic',
        displayName: 'Brineomatic',
        position: "general",
        content: this.generateSettingsUI()
      });
      YB.App.addSettingsPanel(brineomaticPanel);
      brineomaticPanel.setup();

      let hardwarePanel = new YB.SettingsPanel({
        name: 'hardware',
        displayName: 'Hardware',
        position: "brineomatic",
        content: this.generateHardwareSettingsUI()
      });
      YB.App.addSettingsPanel(hardwarePanel);
      hardwarePanel.setup();

      let safeguardsPanel = new YB.SettingsPanel({
        name: 'safeguards',
        displayName: 'Safeguards',
        position: "hardware",
        content: this.generateSafeguardsSettingsUI()
      });
      YB.App.addSettingsPanel(safeguardsPanel);
      safeguardsPanel.setup();

      $("#statsContainer").html(this.generateStatsUI());

      //hide our channel specific divs
      $("#relayConfig").hide();
      $("#servoConfig").hide();
      $("#stepperConfig").hide();
      $('#relayControlDiv').hide();
      $('#servoControlDiv').hide();
      $('#stepperControlDiv').hide();

      this.addEditUIHandlers();
      this.updateEditUIData(msg.brineomatic);
      this.updateHardwareUIConfig(msg.brineomatic);

      //edit UI handlers
      $("#bomConfig").show();

      //enable the form - it was disabled when the old one was submitted
      //this prevents multiple submission race conditons that look like settings not getting saved
      $("#hardwareSettingsPanel")
        .find("input, select, textarea, button")
        .prop("disabled", false);

      //our UI handlers
      $("#brineomaticIdle").on("click", this.idle);
      $("#brineomaticStartAutomatic").on("click", this.startAutomatic);
      $("#brineomaticStartDuration").on("click", this.startDuration);
      $("#brineomaticStartVolume").on("click", this.startVolume);
      $("#brineomaticFlushAutomatic").on("click", this.flushAutomatic);
      $("#brineomaticFlushDuration").on("click", this.flushDuration);
      $("#brineomaticFlushVolume").on("click", this.flushVolume);
      $("#brineomaticPickle").on("click", this.pickle);
      $("#brineomaticDepickle").on("click", this.depickle);
      $("#brineomaticStop").on("click", this.stop);
      $("#brineomaticManual").on("click", this.manual);

      $("#boostPumpControlButton").on("click", this.toggleBoostPump);
      $("#highPressurePumpControlButton").on("click", this.toggleHighPressurePump);
      $("#diverterValveControlButton").on("click", this.toggleDiverterValve);
      $("#flushValveControlButton").on("click", this.toggleFlushValve);
      $("#coolingFanControlButton").on("click", this.toggleCoolingFan);
      $("#advancedModeButton").on("click", this.advanced);

      //visibility
      $('#bomInformationDiv').show();
      $('#bomControlDiv').show();
      $('#bomStatsDiv').show();
      $('#brightnessUI').hide();

      this.buildGaugeSetup();
      if (!YB.App.isMFD())
        this.createGauges();

      //finally, show our interface.
      $('#bomInterface').css('visibility', 'visible');
    };
  }

  Brineomatic.prototype.handleUpdateMessage = function (msg) {
    if (!YB.App.config.brineomatic)
      return;

    let motor_temperature = YB.bom.convertTemperature(msg.motor_temperature, "C", YB.App.config.brineomatic.temperature_units);
    motor_temperature = this.formatReadable(motor_temperature);

    let water_temperature = YB.bom.convertTemperature(msg.water_temperature, "C", YB.App.config.brineomatic.temperature_units);
    water_temperature = this.formatReadable(water_temperature);

    let product_flowrate = YB.bom.convertFlowrate(msg.product_flowrate, "lph", YB.App.config.brineomatic.flowrate_units);
    product_flowrate = this.formatReadable(product_flowrate);

    let brine_flowrate = YB.bom.convertFlowrate(msg.brine_flowrate, "lph", YB.App.config.brineomatic.flowrate_units);
    brine_flowrate = this.formatReadable(brine_flowrate);

    let total_flowrate = YB.bom.convertFlowrate(msg.total_flowrate, "lph", YB.App.config.brineomatic.flowrate_units);
    total_flowrate = this.formatReadable(total_flowrate);

    let volume = YB.bom.convertVolume(msg.volume, "liters", YB.App.config.brineomatic.volume_units);
    volume = this.formatReadable(volume);

    let flush_volume = YB.bom.convertVolume(msg.flush_volume, "liters", YB.App.config.brineomatic.volume_units);
    flush_volume = this.formatReadable(volume);

    let product_salinity = this.formatReadable(msg.product_salinity);
    let brine_salinity = this.formatReadable(msg.brine_salinity);

    let filter_pressure = parseFloat(msg.filter_pressure);
    if (filter_pressure < 0 && filter_pressure > -10)
      filter_pressure = 0;
    filter_pressure = YB.bom.convertPressure(msg.filter_pressure, "Bar", YB.App.config.brineomatic.pressure_units);
    filter_pressure = this.formatReadable(filter_pressure);

    let membrane_pressure = parseFloat(msg.membrane_pressure);
    if (membrane_pressure < 0 && membrane_pressure > -10)
      membrane_pressure = 0;
    membrane_pressure = YB.bom.convertPressure(msg.membrane_pressure, "Bar", YB.App.config.brineomatic.pressure_units);
    membrane_pressure = this.formatReadable(membrane_pressure);

    let tank_level = this.formatReadable(msg.tank_level * 100);

    //errors or no?
    let err = filter_pressure < 0;
    $(".filterPressureContent").toggle(!err);
    $(".filterPressureError").toggle(err);

    err = membrane_pressure < 0;
    $(".membranePressureContent").toggle(!err);
    $(".membranePressureError").toggle(err);

    err = motor_temperature < 0;
    $(".motorTemperatureContent").toggle(!err);
    $(".motorTemperatureError").toggle(err);

    err = tank_level < 0;
    $(".tankLevelContent").toggle(!err);
    $(".tankLevelError").toggle(err);

    //update our gauges.
    if (!YB.App.isMFD()) {
      if (YB.App.currentPage == "home") {
        this.motorTemperatureGauge.load({ columns: [['Motor Temperature', motor_temperature]] });
        this.waterTemperatureGauge.load({ columns: [['Water Temperature', water_temperature]] });
        this.filterPressureGauge.load({ columns: [['Filter Pressure', filter_pressure]] });
        this.membranePressureGauge.load({ columns: [['Membrane Pressure', membrane_pressure]] });
        this.productSalinityGauge.load({ columns: [['Product Salinity', product_salinity]] });
        this.brineSalinityGauge.load({ columns: [['Brine Salinity', brine_salinity]] });
        this.productFlowrateGauge.load({ columns: [['Product Flowrate', product_flowrate]] });
        this.brineFlowrateGauge.load({ columns: [['Brine Flowrate', brine_flowrate]] });
        this.totalFlowrateGauge.load({ columns: [['Total Flowrate', total_flowrate]] });
        this.tankLevelGauge.load({ columns: [['Tank Level', tank_level]] });
      }
    } else {
      $("#filterPressureData").html(filter_pressure);
      this.setDataColor("filter_pressure", filter_pressure, $("#filterPressureData"));

      $("#membranePressureData").html(membrane_pressure);
      this.setDataColor("membrane_pressure", membrane_pressure, $("#membranePressureData"));

      $("#productSalinityData").html(product_salinity);
      this.setDataColor("product_salinity", product_salinity, $("#productSalinityData"));

      $("#brineSalinityData").html(brine_salinity);
      this.setDataColor("brine_salinity", brine_salinity, $("#brineSalinityData"));

      $("#productFlowrateData").html(product_flowrate);
      this.setDataColor("product_flowrate", product_flowrate, $("#productFlowrateData"));

      $("#brineFlowrateData").html(brine_flowrate);
      this.setDataColor("brine_flowrate", brine_flowrate, $("#brineFlowrateData"));

      $("#totalFlowrateData").html(total_flowrate);
      this.setDataColor("total_flowrate", total_flowrate, $("#totalFlowrateData"));

      $("#motorTemperatureData").html(motor_temperature);
      this.setDataColor("motor_temperature", motor_temperature, $("#motorTemperatureData"));

      $("#waterTemperatureData").html(water_temperature);
      this.setDataColor("water_temperature", water_temperature, $("#waterTemperatureData"));

      $("#tankLevelData").html(tank_level);
      this.setDataColor("tank_level", tank_level, $("#tankLevelData"));

    }

    $(".bomVolumeData").html(volume);
    this.setDataColor("volume", volume, $(".bomVolumeData"));
    $(".bomFlushVolumeData").html(flush_volume);
    this.setDataColor("volume", flush_volume, $(".bomFlushVolumeData"));

    $("#bomStatus").html(msg.status);
    $("#bomStatus").removeClass();
    $("#bomStatus").addClass("badge");

    if (msg.status == "STARTUP")
      $("#bomStatus").addClass("text-bg-info");
    else if (msg.status == "IDLE")
      $("#bomStatus").addClass("text-bg-secondary");
    else if (msg.status == "MANUAL")
      $("#bomStatus").addClass("text-bg-secondary");
    else if (msg.status == "RUNNING")
      $("#bomStatus").addClass("text-bg-success");
    else if (msg.status == "FLUSHING")
      $("#bomStatus").addClass("text-bg-primary");
    else if (msg.status == "PICKLING")
      $("#bomStatus").addClass("text-bg-warning");
    else if (msg.status == "DEPICKLING")
      $("#bomStatus").addClass("text-bg-warning");
    else if (msg.status == "PICKLED")
      $("#bomStatus").addClass("text-bg-warning");
    else if (msg.status == "STOPPING")
      $("#bomStatus").addClass("text-bg-info");
    else
      $("#bomStatus").addClass("text-bg-danger");

    // hide all BOM states except the one we want
    $(`.bomSTARTUP, .bomIDLE, .bomMANUAL, .bomRUNNING, .bomFLUSHING, .bomPICKLING, .bomPICKLED, .bomDEPICKLING, .bomSTOPPING`)
      .not(`.bom${msg.status}`)
      .hide();

    $(`.bom${msg.status}`).show();

    if (msg.run_result)
      this.showResult("#bomRunResult", msg.run_result);

    if (msg.flush_result)
      this.showResult("#bomFlushResult", msg.flush_result);

    if (msg.pickle_result)
      this.showResult("#bomPickleResult", msg.pickle_result);

    if (msg.pickled_on > 0) {
      let current_time = Math.floor(Date.now() / 1000);
      let duration = current_time - msg.pickled_on;
      let time_ago = YB.Util.secondsToDhms(duration, 1);
      let date_obj = new Date(msg.pickled_on * 1000);
      let pickle_date = date_obj.toLocaleDateString("en-US", {
        year: "numeric",
        month: "short",
        day: "numeric"
      });

      $('#bomPickledSince').html(`${pickle_date}<br/>(${time_ago} ago)`);
      $('#bomPickledSinceRow').show();
    } else
      $('#bomPickledSinceRow').hide();

    if (msg.depickle_result)
      this.showResult("#bomDePickleResult", msg.depickle_result);

    if (msg.next_flush_countdown > 0)
      $("#bomNextFlushCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.next_flush_countdown / 1000)));
    else
      $("#bomNextFlushCountdown").hide();

    if (msg.runtime_elapsed > 0)
      $("#bomRuntimeElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.runtime_elapsed / 1000)));
    else
      $("#bomRuntimeElapsed").hide();

    if (msg.finish_countdown > 0)
      $("#bomFinishCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.finish_countdown / 1000)));
    else
      $("#bomFinishCountdown").hide();

    if (msg.runtime_elapsed > 0 && msg.finish_countdown > 0) {
      const runtimeProgress = (msg.runtime_elapsed / (msg.runtime_elapsed + msg.finish_countdown)) * 100;
      YB.Util.updateProgressBar("bomRunProgressBar", runtimeProgress);
      $('#bomRunProgressRow').show();
    } else {
      $('#bomRunProgressRow').hide();
    }

    if (msg.flush_elapsed > 0)
      $("#bomFlushElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.flush_elapsed / 1000)));
    else
      $("#bomFlushElapsed").hide();

    if (msg.flush_countdown > 0)
      $("#bomFlushCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.flush_countdown / 1000)));
    else
      $("#bomFlushCountdown").hide();

    if (msg.flush_elapsed > 0 && msg.flush_countdown > 0) {
      const flushProgress = (msg.flush_elapsed / (msg.flush_elapsed + msg.flush_countdown)) * 100;
      YB.Util.updateProgressBar("bomFlushProgressBar", flushProgress);
      $('#bomFlushProgressRow').show();
    } else {
      $('#bomFlushProgressRow').hide();
    }

    if (msg.pickle_elapsed > 0)
      $("#bomPickleElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.pickle_elapsed / 1000)));
    else
      $("#bomPickleElapsed").hide();

    if (msg.pickle_countdown > 0)
      $("#bomPickleCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.pickle_countdown / 1000)));
    else
      $("#bomPickleCountdown").hide();

    if (msg.pickle_elapsed > 0 && msg.pickle_countdown > 0) {
      const pickleProgress = (msg.pickle_elapsed / (msg.pickle_elapsed + msg.pickle_countdown)) * 100;
      YB.Util.updateProgressBar("bomPickleProgressBar", pickleProgress);
      $('#bomPickleProgressRow').show();
    } else {
      $('#bomPickleProgressRow').hide();
    }

    if (msg.depickle_elapsed > 0)
      $("#bomDepickleElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.depickle_elapsed / 1000)));
    else
      $("#bomDepickleElapsed").hide();

    if (msg.depickle_countdown > 0)
      $("#bomDepickleCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.depickle_countdown / 1000)));
    else
      $("#bomDepickleCountdown").hide();

    if (msg.depickle_elapsed > 0 && msg.depickle_countdown > 0) {
      const depickleProgress = (msg.depickle_elapsed / (msg.depickle_elapsed + msg.depickle_countdown)) * 100;
      YB.Util.updateProgressBar("#bomDepickleProgressBar", depickleProgress);
      $('#bomDepickleProgressRow').show();
    } else {
      $('#bomDepickleProgressRow').hide();
    }

    if (YB.App.config.brineomatic.has_boost_pump) {
      $('#bomBoostPumpStatus span').removeClass();
      $('#bomBoostPumpStatus span').addClass("badge");
      $('#boostPumpControlUI').show();
      if (msg.boost_pump_on) {
        $("#bomBoostPumpStatus span").addClass("text-bg-primary");
        $('#bomBoostPumpStatus span').html("ON");
        $('#manualBoostPumpStatus').html("ON");
        $('#boostPumpControlButton').removeClass("btn-secondary").addClass("btn-success");
      }
      else {
        $("#bomBoostPumpStatus span").addClass("text-bg-secondary");
        $('#bomBoostPumpStatus span').html("OFF");
        $('#manualBoostPumpStatus').html("OFF");
        $('#boostPumpControlButton').removeClass("btn-success").addClass("btn-secondary");
      }
    }
    else {
      $('#bomBoostPumpStatus').hide();
      $('#boostPumpControlUI').hide();
    }

    if (YB.App.config.brineomatic.has_high_pressure_pump) {
      $('#bomHighPressurePumpStatus span').removeClass();
      $('#bomHighPressurePumpStatus span').addClass("badge");
      $('#highPressurePumpControlUI').show();
      if (msg.high_pressure_pump_on) {
        $("#bomHighPressurePumpStatus span").addClass("text-bg-primary");
        $('#bomHighPressurePumpStatus span').html("ON");
        $('#manualHighPressurePumpStatus').html("ON");
        $('#highPressurePumpControlButton').removeClass("btn-secondary").addClass("btn-success");
      }
      else {
        $("#bomHighPressurePumpStatus span").addClass("text-bg-secondary");
        $('#bomHighPressurePumpStatus span').html("OFF");
        $('#manualHighPressurePumpStatus').html("OFF");
        $('#highPressurePumpControlButton').removeClass("btn-success").addClass("btn-secondary");
      }
    }
    else {
      $('#highPressurePumpControlUI').hide();
      $('#bomHighPressurePumpStatus').hide();
    }

    if (YB.App.config.brineomatic.has_diverter_valve) {
      $('#bomDiverterValveStatus span').removeClass();
      $('#bomDiverterValveStatus span').addClass("badge");
      $('#diverterValveControlUI').show();

      if (msg.diverter_valve_open) {
        $("#bomDiverterValveStatus span").addClass("text-bg-secondary");
        $('#bomDiverterValveStatus span').html("OVERBOARD");
        $('#manualDiverterValveStatus').html("OVERBOARD");
        $('#diverterValveControlButton').removeClass("btn-success").addClass("btn-secondary");
      }
      else {
        $("#bomDiverterValveStatus span").addClass("text-bg-primary");
        $('#bomDiverterValveStatus span').html("TO TANK");
        $('#manualDiverterValveStatus').html("TO TANK");
        $('#diverterValveControlButton').removeClass("btn-secondary").addClass("btn-success");
      }
    }
    else {
      $('#diverterValveControlUI').hide();
      $('#bomDiverterValveStatus').hide();

    }

    if (YB.App.config.brineomatic.has_flush_valve) {
      $('#bomFlushValveStatus span').removeClass();
      $('#bomFlushValveStatus span').addClass("badge");
      $('#flushValveControlUI').show();

      if (msg.flush_valve_open) {
        $("#bomFlushValveStatus span").addClass("text-bg-primary");
        $('#bomFlushValveStatus span').html("OPEN");
        $('#manualFlushValveStatus').html("OPEN");
        $('#flushValveControlButton').removeClass("btn-secondary").addClass("btn-success");
      }
      else {
        $("#bomFlushValveStatus span").addClass("text-bg-secondary");
        $('#bomFlushValveStatus span').html("CLOSED");
        $('#manualFlushValveStatus').html("CLOSED");
        $('#flushValveControlButton').removeClass("btn-success").addClass("btn-secondary");
      }
    }
    else {
      $('#flushValveControlUI').hide();
      $('#bomFlushValveStatus').hide();
    }

    if (YB.App.config.brineomatic.has_cooling_fan) {
      $('#bomFanStatus span').removeClass();
      $('#bomFanStatus span').addClass("badge");
      $('#coolingFanControlUI').show();

      if (msg.cooling_fan_on) {
        $("#bomFanStatus span").addClass("text-bg-primary");
        $('#bomFanStatus span').html("ON");
        $('#manualCoolingFanStatus').html("ON");
        $('#coolingFanControlButton').removeClass("btn-secondary").addClass("btn-success");
      }
      else {
        $('#bomFanStatus span').html("OFF");
        $("#bomFanStatus span").addClass("text-bg-secondary");
        $('#manualCoolingFanStatus').html("OFF");
        $('#coolingFanControlButton').removeClass("btn-success").addClass("btn-secondary");
      }
    }
    else {
      $('#coolingFanControlUI').show();
      $('#bomFanStatus').hide();
    }

    //disable our hardware form when not idle.
    if (msg.status == "IDLE" || msg.status == "MANUAL") {
      $("#hardwareSettingsPanel")
        .find("input, select, textarea, button")
        .prop("disabled", false);
      $("#hardwareSettingsDisabled").hide();
    } else {
      $("#hardwareSettingsPanel")
        .find("input, select, textarea, button")
        .prop("disabled", true);
      $("#hardwareSettingsDisabled").show();
    }
  }

  Brineomatic.prototype.handleStatsMessage = function (msg) {
    if (msg.brineomatic) {
      let totalVolume = YB.bom.convertVolume(msg.total_volume, "liters", YB.App.config.brineomatic.volume_units);
      totalVolume = Math.round(totalVolume);
      totalVolume = totalVolume.toLocaleString('en-US');
      let volumeUnits = YB.App.config.brineomatic.volume_units;

      let totalRuntime = (msg.total_runtime / (60 * 60)).toFixed(1);
      totalRuntime = totalRuntime.toLocaleString('en-US');

      $("#bomTotalCycles").html(msg.total_cycles.toLocaleString('en-US'));
      $("#bomTotalVolume").html(`${totalVolume}${volumeUnits}`);
      $("#bomTotalRuntime").html(`${totalRuntime} hours`);
    }
  }

  Brineomatic.prototype.setDataColor = function (name, value, ele) {
    // Check if the name exists in this.gaugeSetup
    const setup = this.gaugeSetup[name];
    if (!setup) {
      console.warn(`No setup found for name: ${name}`);
      return;
    }

    const { thresholds, colors } = setup;

    // Ensure thresholds and colors arrays are of equal length
    if (thresholds.length !== colors.length) {
      console.error(`Thresholds and colors arrays length mismatch for name: ${name}`);
      return;
    }

    // Iterate over thresholds to find the appropriate color
    for (let i = 0; i < thresholds.length; i++) {
      if (value <= thresholds[i]) {
        ele.css("color", colors[i]);
        return;
      }
    }

    // If value exceeds all thresholds, set color to the last color
    ele.css("color", colors[colors.length - 1]);
  }

  Brineomatic.prototype.showResult = function (result_div, result) {
    if (result != "STARTUP") {
      if (this.resultText[result])
        $(result_div).html(this.resultText[result]);
      else
        $(result_div).html(result);

      $(result_div).removeClass();
      $(result_div).addClass("badge");
      if (result.startsWith("SUCCESS"))
        $(result_div).addClass("text-bg-success");
      else if (result == "USER_STOP")
        $(result_div).addClass("text-bg-primary");
      else if (result.startsWith("ERR"))
        $(result_div).addClass("text-bg-danger");
      else
        $(result_div).addClass("text-bg-warning");
    }
    else
      $(`${result_div}Row`).hide();
  }

  Brineomatic.prototype.startAutomatic = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "start_watermaker",
    }, true);
  }

  Brineomatic.prototype.startDuration = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomRunDurationInput").val();

    if (duration > 0) {
      //hours to microseconds
      let millis = duration * 60 * 60 * 1000;

      YB.client.send({
        "cmd": "start_watermaker",
        "duration": millis
      }, true);
    }
  }

  Brineomatic.prototype.startVolume = function (e) {
    $(e.currentTarget).blur();
    let volume = $("#bomRunVolumeInput").val();

    // Convert from user's units to liters (firmware base unit)
    volume = YB.bom.convertVolume(volume, YB.App.config.brineomatic.volume_units, "liters");

    if (volume > 0) {
      YB.client.send({
        "cmd": "start_watermaker",
        "volume": volume
      }, true);
    }
  }

  Brineomatic.prototype.flushAutomatic = function (e) {
    $(e.currentTarget).blur();

    YB.client.send({
      "cmd": "flush_watermaker"
    }, true);
  }

  Brineomatic.prototype.flushDuration = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomFlushDurationInput").val();

    if (duration > 0) {
      let millis = duration * 60 * 1000;

      YB.client.send({
        "cmd": "flush_watermaker",
        "duration": millis
      }, true);
    }
  }

  Brineomatic.prototype.flushVolume = function (e) {
    $(e.currentTarget).blur();
    let volume = $("#bomFlushVolumeInput").val();

    // Convert from user's units to liters (firmware base unit)
    volume = YB.bom.convertVolume(volume, YB.App.config.brineomatic.volume_units, "liters");

    if (volume > 0) {
      YB.client.send({
        "cmd": "flush_watermaker",
        "volume": volume
      }, true);
    }
  }

  Brineomatic.prototype.pickle = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomPickleDurationInput").val();

    if (duration > 0) {
      let millis = duration * 60 * 1000;

      YB.client.send({
        "cmd": "pickle_watermaker",
        "duration": millis
      }, true);
    }
  }

  Brineomatic.prototype.depickle = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomDepickleDurationInput").val();

    if (duration > 0) {
      let millis = duration * 60 * 1000;

      YB.client.send({
        "cmd": "depickle_watermaker",
        "duration": millis
      }, true);
    }
  }

  Brineomatic.prototype.stop = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "stop_watermaker",
    }, true);
  }

  Brineomatic.prototype.manual = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "manual_watermaker",
    }, true);
  }

  Brineomatic.prototype.idle = function (e) {
    $(e.currentTarget).blur();

    $("#servoControlDiv").hide();
    $("#stepperControlDiv").hide();
    $('#servoControlDiv').removeClass("bomMANUAL");
    $('#stepperControlDiv').removeClass("bomMANUAL");


    YB.client.send({
      "cmd": "idle_watermaker",
    }, true);
  }

  Brineomatic.prototype.advanced = function (e) {
    $(e.currentTarget).blur();
    $("#servoControlDiv").hide();
    $("#stepperControlDiv").hide();
    $('#servoControlDiv').toggleClass("bomMANUAL");
    $('#stepperControlDiv').toggleClass("bomMANUAL");
  }

  Brineomatic.prototype.toggleBoostPump = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "set_watermaker",
      "boost_pump": "TOGGLE"
    });
  }

  Brineomatic.prototype.toggleHighPressurePump = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "set_watermaker",
      "high_pressure_pump": "TOGGLE"
    });
  }

  Brineomatic.prototype.toggleDiverterValve = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "set_watermaker",
      "diverter_valve": "TOGGLE"
    });
  }

  Brineomatic.prototype.toggleFlushValve = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "set_watermaker",
      "flush_valve": "TOGGLE"
    });
  }

  Brineomatic.prototype.toggleCoolingFan = function (e) {
    $(e.currentTarget).blur();
    YB.client.send({
      "cmd": "set_watermaker",
      "cooling_fan": "TOGGLE"
    });
  }

  Brineomatic.prototype.generateControlUI = function () {
    return /* html */ `
      <div id="bomInterface" class="row mb-3" style="visibility:hidden">
          <div id="bomInformationDiv" style="display:none" class="col-md-6">
              <h1 class="text-center">Mode: <span class="badge" id="bomStatus"></span></h1>
              <table id="bomTable" class="table table-hover">
                  <tbody id="bomTableBody">
                      <tr id="bomRunResultRow" class="bomIDLE bomFLUSHING" style="display: none">
                          <th>Last Run Result</th>
                          <td><span id="bomRunResult"></span></td>
                      </tr>
                      <tr id="bomFlushResultRow" class="bomIDLE" style="display: none">
                          <th>Flush Result</th>
                          <td><span id="bomFlushResult"></span></td>
                      </tr>
                      <tr id="bomPickleResultRow" class="bomPICKLED" style="display: none">
                          <th>Pickle Result</th>
                          <td><span id="bomPickleResult"></span></td>
                      </tr>
                      <tr id="bomPickledSinceRow" class="bomPICKLED" style="display: none">
                          <th>Pickled Since</th>
                          <td><span id="bomPickledSince"></span></td>
                      </tr>
                      <tr id="bomDePickleResultRow" class="bomIDLE" style="display: none">
                          <th>Depickle Result</th>
                          <td><span id="bomDePickleResult"></span></td>
                      </tr>
                      <tr id="bomNextFlushCountdown" class="bomIDLE" style="display: none">
                          <th>Next Autoflush</th>
                          <td id="bomNextFlushCountdownData"></td>
                      </tr>
                      <tr id="bomRuntimeElapsed" class="bomIDLE bomRUNNING bomFLUSHING"
                          style="display: none">
                          <th>Runtime Elapsed</th>
                          <td id="bomRuntimeElapsedData"></td>
                      </tr>
                      <tr id="bomFinishCountdown" class="bomRUNNING" style="display: none">
                          <th>Runtime Remaining</th>
                          <td id="bomFinishCountdownData"></td>
                      </tr>
                      <tr id="bomRunProgressRow" class="bomRUNNING" style="display: none">
                          <td colspan="2">
                              <div id="bomRunProgressBar" class="progress" role="progressbar"
                                  aria-label="Run Progress" aria-valuenow="0" aria-valuemin="0"
                                  aria-valuemax="100">
                                  <div class="progress-bar"></div>
                              </div>
                          </td>
                      </tr>
                      <tr id="bomFlushElapsed" class="bomIDLE bomFLUSHING" style="display: none">
                          <th>Flush Elapsed</th>
                          <td id="bomFlushElapsedData"></td>
                      </tr>
                      <tr id="bomFlushCountdown" class="bomIDLE bomFLUSHING" style="display: none">
                          <th>Flush Remaining</th>
                          <td id="bomFlushCountdownData"></td>
                      </tr>
                      <tr id="bomFlushProgressRow" class="bomFLUSHING" style="display: none">
                          <td colspan="2">
                              <div id="bomFlushProgressBar" class="progress" role="progressbar"
                                  aria-label="Basic example" aria-valuenow="0" aria-valuemin="0"
                                  aria-valuemax="100">
                                  <div class="progress-bar"></div>
                              </div>
                          </td>
                      </tr>
                      <tr id="bomPickleElapsed" class="bomPICKLING" style="display: none">
                          <th>Pickling Elapsed</th>
                          <td id="bomPickleElapsedData"></td>
                      </tr>
                      <tr id="bomPickleCountdown" class="bomPICKLING" style="display: none">
                          <th>Pickling Remaining</th>
                          <td id="bomPickleCountdownData"></td>
                      </tr>
                      <tr id="bomPickleProgressRow" class="bomPICKLING" style="display: none">
                          <td colspan="2">
                              <div id="bomPickleProgressBar" class="progress" role="progressbar"
                                  aria-label="Basic example" aria-valuenow="0" aria-valuemin="0"
                                  aria-valuemax="100">
                                  <div class="progress-bar"></div>
                              </div>
                          </td>
                      </tr>
                      <tr id="bomDepickleElapsed" class="bomDEPICKLING" style="display: none">
                          <th>De-Pickling Elapsed</th>
                          <td id="bomDepickleElapsedData"></td>
                      </tr>
                      <tr id="bomDepickleCountdown" class="bomDEPICKLING" style="display: none">
                          <th>De-Pickling Remaining</th>
                          <td id="bomDepickleCountdownData"></td>
                      </tr>
                      <tr id="bomDepickleProgressRow" class="bomDEPICKLING" style="display: none">
                          <td colspan="2">
                              <div id="bomDepickleProgressBar" class="progress" role="progressbar"
                                  aria-label="Basic example" aria-valuenow="0" aria-valuemin="0"
                                  aria-valuemax="100">
                                  <div class="progress-bar"></div>
                              </div>
                          </td>
                      </tr>
                  </tbody>
              </table>
          </div>

          <div id="bomControlDiv" style="display:none" class="col-md-6">
              <div id="bomControlButtons" class="row g-2 justify-content-center">
                  <div id="runBrineomatic" class="col-6 bomIDLE">
                      <button class="btn btn-success brineomaticControlButton" type="button"
                          data-bs-toggle="modal" data-bs-target="#startBrineomaticModal">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-play-circle" viewBox="0 0 16 16">
                              <path
                                  d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16" />
                              <path
                                  d="M6.271 5.055a.5.5 0 0 1 .52.038l3.5 2.5a.5.5 0 0 1 0 .814l-3.5 2.5A.5.5 0 0 1 6 10.5v-5a.5.5 0 0 1 .271-.445" />
                          </svg>
                          <span class="align-middle mx-2">START</span>
                      </button>
                  </div>
                  <div id="flushBrineomatic" class="col-6 bomIDLE bomPICKLED">
                      <button class="btn btn-primary brineomaticControlButton" type="button"
                          data-bs-toggle="modal" data-bs-target="#flushBrineomaticModal">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-droplet" viewBox="0 0 16 16">
                              <path fill-rule="evenodd"
                                  d="M7.21.8C7.69.295 8 0 8 0q.164.544.371 1.038c.812 1.946 2.073 3.35 3.197 4.6C12.878 7.096 14 8.345 14 10a6 6 0 0 1-12 0C2 6.668 5.58 2.517 7.21.8m.413 1.021A31 31 0 0 0 5.794 3.99c-.726.95-1.436 2.008-1.96 3.07C3.304 8.133 3 9.138 3 10a5 5 0 0 0 10 0c0-1.201-.796-2.157-2.181-3.7l-.03-.032C9.75 5.11 8.5 3.72 7.623 1.82z" />
                              <path fill-rule="evenodd"
                                  d="M4.553 7.776c.82-1.641 1.717-2.753 2.093-3.13l.708.708c-.29.29-1.128 1.311-1.907 2.87z" />
                          </svg>
                          <span class="align-middle mx-2">FLUSH</span>
                      </button>
                  </div>
                  <div id="pickleBrineomatic" class="col-6 bomIDLE">
                      <button class="btn btn-warning brineomaticControlButton" type="button"
                          data-bs-toggle="modal" data-bs-target="#pickleBrineomaticModal">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-shield-plus" viewBox="0 0 16 16">
                              <path
                                  d="M5.338 1.59a61 61 0 0 0-2.837.856.48.48 0 0 0-.328.39c-.554 4.157.726 7.19 2.253 9.188a10.7 10.7 0 0 0 2.287 2.233c.346.244.652.42.893.533q.18.085.293.118a1 1 0 0 0 .101.025 1 1 0 0 0 .1-.025q.114-.034.294-.118c.24-.113.547-.29.893-.533a10.7 10.7 0 0 0 2.287-2.233c1.527-1.997 2.807-5.031 2.253-9.188a.48.48 0 0 0-.328-.39c-.651-.213-1.75-.56-2.837-.855C9.552 1.29 8.531 1.067 8 1.067c-.53 0-1.552.223-2.662.524zM5.072.56C6.157.265 7.31 0 8 0s1.843.265 2.928.56c1.11.3 2.229.655 2.887.87a1.54 1.54 0 0 1 1.044 1.262c.596 4.477-.787 7.795-2.465 9.99a11.8 11.8 0 0 1-2.517 2.453 7 7 0 0 1-1.048.625c-.28.132-.581.24-.829.24s-.548-.108-.829-.24a7 7 0 0 1-1.048-.625 11.8 11.8 0 0 1-2.517-2.453C1.928 10.487.545 7.169 1.141 2.692A1.54 1.54 0 0 1 2.185 1.43 63 63 0 0 1 5.072.56" />
                              <path
                                  d="M8 4.5a.5.5 0 0 1 .5.5v1.5H10a.5.5 0 0 1 0 1H8.5V9a.5.5 0 0 1-1 0V7.5H6a.5.5 0 0 1 0-1h1.5V5a.5.5 0 0 1 .5-.5" />
                          </svg>
                          <span class="align-middle mx-2">PICKLE</span>
                      </button>
                  </div>
                  <div id="depickleBrineomatic" class="col-6 bomPICKLED">
                      <button class="btn btn-warning brineomaticControlButton" type="button"
                          data-bs-toggle="modal" data-bs-target="#depickleBrineomaticModal">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-shield-slash" viewBox="0 0 16 16">
                              <path fill-rule="evenodd"
                                  d="M1.093 3.093c-.465 4.275.885 7.46 2.513 9.589a11.8 11.8 0 0 0 2.517 2.453c.386.273.744.482 1.048.625.28.132.581.24.829.24s.548-.108.829-.24a7 7 0 0 0 1.048-.625 11.3 11.3 0 0 0 1.733-1.525l-.745-.745a10.3 10.3 0 0 1-1.578 1.392c-.346.244-.652.42-.893.533q-.18.085-.293.118a1 1 0 0 1-.101.025 1 1 0 0 1-.1-.025 2 2 0 0 1-.294-.118 6 6 0 0 1-.893-.533 10.7 10.7 0 0 1-2.287-2.233C3.053 10.228 1.879 7.594 2.06 4.06zM3.98 1.98l-.852-.852A59 59 0 0 1 5.072.559C6.157.266 7.31 0 8 0s1.843.265 2.928.56c1.11.3 2.229.655 2.887.87a1.54 1.54 0 0 1 1.044 1.262c.483 3.626-.332 6.491-1.551 8.616l-.77-.77c1.042-1.915 1.72-4.469 1.29-7.702a.48.48 0 0 0-.33-.39c-.65-.213-1.75-.56-2.836-.855C9.552 1.29 8.531 1.067 8 1.067c-.53 0-1.552.223-2.662.524a50 50 0 0 0-1.357.39zm9.666 12.374-13-13 .708-.708 13 13z" />
                          </svg>
                          <span class="align-middle mx-2">DEPICKLE</span>
                      </button>
                  </div>
                  <div id="stopBrineomatic"
                      class="col-6 bomRUNNING bomFLUSHING bomPICKLING bomDEPICKLING">
                      <button class="btn btn-danger brineomaticControlButton" type="button"
                          data-bs-toggle="modal" data-bs-target="#stopBrineomaticModal">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-stop-circle" viewBox="0 0 16 16">
                              <path
                                  d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16" />
                              <path
                                  d="M5 6.5A1.5 1.5 0 0 1 6.5 5h3A1.5 1.5 0 0 1 11 6.5v3A1.5 1.5 0 0 1 9.5 11h-3A1.5 1.5 0 0 1 5 9.5z" />
                          </svg>
                          <span class="align-middle mx-2">STOP</span>
                      </button>
                  </div>
                  <div id="manualBrineomatic" class="col-6 bomIDLE">
                      <button class="btn btn-secondary brineomaticControlButton" type="button"
                          data-bs-toggle="modal" data-bs-target="#manualBrineomaticModal">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-wrench-adjustable-circle"
                              viewBox="0 0 16 16">
                              <path
                                  d="M12.496 8a4.5 4.5 0 0 1-1.703 3.526L9.497 8.5l2.959-1.11q.04.3.04.61" />
                              <path
                                  d="M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0m-1 0a7 7 0 1 0-13.202 3.249l1.988-1.657a4.5 4.5 0 0 1 7.537-4.623L7.497 6.5l1 2.5 1.333 3.11c-.56.251-1.18.39-1.833.39a4.5 4.5 0 0 1-1.592-.29L4.747 14.2A7 7 0 0 0 15 8m-8.295.139a.25.25 0 0 0-.288-.376l-1.5.5.159.474.808-.27-.595.894a.25.25 0 0 0 .287.376l.808-.27-.595.894a.25.25 0 0 0 .287.376l1.5-.5-.159-.474-.808.27.596-.894a.25.25 0 0 0-.288-.376l-.808.27z" />
                          </svg>
                          <span class="align-middle mx-2">MANUAL</span>
                      </button>
                  </div>
                  <div id="idleBrineomatic" class="col-6 bomMANUAL">
                      <button id="brineomaticIdle" class="btn btn-primary brineomaticControlButton"
                          type="button">
                          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"
                              fill="currentColor" class="bi bi-arrow-counterclockwise"
                              viewBox="0 0 16 16">
                              <path fill-rule="evenodd"
                                  d="M8 3a5 5 0 1 1-4.546 2.914.5.5 0 0 0-.908-.417A6 6 0 1 0 8 2z" />
                              <path
                                  d="M8 4.466V.534a.25.25 0 0 0-.41-.192L5.23 2.308a.25.25 0 0 0 0 .384l2.36 1.966A.25.25 0 0 0 8 4.466" />
                          </svg>
                          <span class="align-middle mx-2">BACK</span>
                      </button>
                  </div>
                  <div id="statusBrineomatic"
                      class="col-12 bomRUNNING bomFLUSHING bomPICKLING bomDEPICKLING bomSTOPPING">
                      <table id="bomStatusTable" class="table table-hover">
                          <tbody id="bomStatusTableBody">
                              <tr id="bomBoostPumpStatus">
                                  <th>Boost Pump:</th>
                                  <td><span></span></td>
                              </tr>
                              <tr id="bomHighPressurePumpStatus">
                                  <th>High Pressure Pump:</th>
                                  <td><span></span></td>
                              </tr>
                              <tr id="bomDiverterValveStatus">
                                  <th>Diverter Valve:</th>
                                  <td><span></span></td>
                              </tr>
                              <tr id="bomFanStatus">
                                  <th>Cooling Fan:</th>
                                  <td><span></span></td>
                              </tr>
                              <tr id="bomFlushValveStatus">
                                  <th>Flush Valve:</th>
                                  <td><span></span></td>
                              </tr>
                          </tbody>
                      </table>
                  </div>
              </div>

              <div class="modal fade" id="startBrineomaticModal" tabindex="-1" role="dialog"
                  aria-labelledby="startBrineomaticModalTitle" aria-hidden="true">
                  <div class="modal-dialog modal-dialog-centered" role="document">
                      <div class="modal-content">
                          <div class="modal-header">
                              <h1 class="modal-title fs-5" id="startBrineomaticModalTitle">Start
                                  Watermaker -
                                  Choose Mode
                              </h1>
                              <button type="button" class="btn-close" data-bs-dismiss="modal"
                                  aria-label="Close"></button>
                          </div>
                          <div class="modal-body">
                              <div class="container-fluid">
                                  <div class="row">
                                      <div id="startRunAutomaticDialog" class="col">
                                          <h5>Automatic</h5>
                                          <div class="small" style="height: 110px">
                                              Run until full.
                                          </div>
                                          <button id="brineomaticStartAutomatic" type="button"
                                              class="btn btn-success my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Start</button>
                                      </div>
                                      <div id="startRunDurationDialog" class="col">
                                          <h5>Duration</h5>
                                          <div class="small" style="height: 70px">
                                              Run for the time below.
                                          </div>
                                          <div style="height: 40px">
                                              <div class="input-group has-validation">
                                                  <input type="text" class="form-control text-center"
                                                      id="bomRunDurationInput" value="3.5">
                                                  <span class="input-group-text">hours</span>
                                                  <div class="invalid-feedback"></div>
                                              </div>
                                          </div>
                                          <button id="brineomaticStartDuration" type="button"
                                              class="btn btn-success my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Start</button>
                                      </div>
                                      <div id="startRunVolumeDialog" class="col">
                                          <h5>Volume</h5>
                                          <div class="small" style="height: 70px">
                                              Make the amount of water below.
                                          </div>
                                          <div style="height: 40px">
                                              <div class="input-group has-validation">
                                                  <input type="text" class="form-control text-center"
                                                      id="bomRunVolumeInput" value="250">
                                                  <span class="input-group-text volumeUnitsLong">liters</span>
                                                  <div class="invalid-feedback"></div>
                                              </div>
                                          </div>
                                          <button id="brineomaticStartVolume" type="button"
                                              class="btn btn-success my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Start</button>
                                      </div>
                                  </div>
                              </div>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                          </div>
                      </div>
                  </div>
              </div>

              <div class="modal fade" id="flushBrineomaticModal" tabindex="-1" role="dialog"
                  aria-labelledby="flushBrineomaticModalTitle" aria-hidden="true">
                  <div class="modal-dialog modal-dialog-centered" role="document">
                      <div class="modal-content">
                          <div class="modal-header">
                              <h1 class="modal-title fs-5" id="flushBrineomaticModalTitle">Flush
                                  Watermaker
                              </h1>
                              <button type="button" class="btn-close" data-bs-dismiss="modal"
                                  aria-label="Close"></button>
                          </div>
                          <div class="modal-body">
                              <div class="container-fluid">
                                  <div class="row">
                                      <div id="startFlushAutomaticDialog" class="col">
                                          <h5>Automatic</h5>
                                          <div class="small" style="height: 110px">
                                              Flush until clean
                                          </div>
                                          <button id="brineomaticFlushAutomatic" type="button"
                                              class="btn btn-primary my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Flush</button>
                                      </div>
                                      <div id="startFlushDurationDialog" class="col">
                                          <h5>Duration</h5>
                                          <div class="small" style="height: 70px">
                                              Flush for the time below.
                                          </div>
                                          <div style="height: 40px">
                                              <div class="input-group has-validation">
                                                  <input type="text" class="form-control text-center"
                                                      id="bomFlushDurationInput" value="5">
                                                  <span class="input-group-text">minutes</span>
                                                  <div class="invalid-feedback"></div>
                                              </div>
                                          </div>
                                          <button id="brineomaticFlushDuration" type="button"
                                              class="btn btn-primary my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Flush</button>
                                      </div>
                                      <div id="startFlushVolumeDialog" class="col">
                                          <h5>Volume</h5>
                                          <div class="small" style="height: 70px">
                                              Flush the volume of water below.
                                          </div>
                                          <div style="height: 40px">
                                              <div class="input-group has-validation">
                                                  <input type="text" class="form-control text-center"
                                                      id="bomFlushVolumeInput" value="15">
                                                  <span class="input-group-text volumeUnitsLong">liters</span>
                                                  <div class="invalid-feedback"></div>
                                              </div>
                                          </div>
                                          <button id="brineomaticFlushVolume" type="button"
                                              class="btn btn-primary my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Flush</button>
                                      </div>
                                  </div>
                              </div>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                          </div>
                      </div>
                  </div>
              </div>

              <div class="modal fade" id="pickleBrineomaticModal" tabindex="-1" role="dialog"
                  aria-labelledby="pickleBrineomaticModalTitle" aria-hidden="true">
                  <div class="modal-dialog modal-dialog-centered" role="document">
                      <div class="modal-content">
                          <div class="modal-header">
                              <h1 class="modal-title fs-5" id="pickleBrineomaticModalTitle">Pickle
                                  Watermaker
                              </h1>
                              <button type="button" class="btn-close" data-bs-dismiss="modal"
                                  aria-label="Close"></button>
                          </div>
                          <div class="modal-body">
                              <div class="alert alert-warning" role="alert">
                                  Make sure you have your watermaker plumbing configured for pickling. The
                                  input and
                                  output of the watermaker should lead to a bucket that contains your
                                  pickling
                                  solution.
                              </div>
                              <p>Pickle the watermaker for the time below.</p>
                              <div class="row">
                                  <div class="col-5">
                                      <div class="input-group has-validation">
                                          <input type="text" class="form-control text-center"
                                              id="bomPickleDurationInput" value="5">
                                          <span class="input-group-text">minutes</span>
                                          <div class="invalid-feedback"></div>
                                      </div>
                                  </div>
                              </div>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                              <button id="brineomaticPickle" type="button" class="btn btn-warning"
                                  data-bs-dismiss="modal">Pickle</button>
                          </div>
                      </div>
                  </div>
              </div>

              <div class="modal fade" id="depickleBrineomaticModal" tabindex="-1" role="dialog"
                  aria-labelledby="depickleBrineomaticModalTitle" aria-hidden="true">
                  <div class="modal-dialog modal-dialog-centered" role="document">
                      <div class="modal-content">
                          <div class="modal-header">
                              <h1 class="modal-title fs-5" id="depickleBrineomaticModalTitle">De-pickle
                                  Watermaker
                              </h1>
                              <button type="button" class="btn-close" data-bs-dismiss="modal"
                                  aria-label="Close"></button>
                          </div>
                          <div class="modal-body">
                              <div class="alert alert-warning" role="alert">
                                  De-pickling the watermaker will flush the membrane with salt water for
                                  the
                                  time below in order to clean the membrane
                                  and plumbing of the pickling solution.
                              </div>
                              <p>De-Pickle the watermaker for the time below.</p>
                              <div class="row">
                                  <div class="col-5">
                                      <div class="input-group has-validation">
                                          <input type="text" class="form-control text-center"
                                              id="bomDepickleDurationInput" value="15">
                                          <span class="input-group-text">minutes</span>
                                          <div class="invalid-feedback"></div>
                                      </div>
                                  </div>
                              </div>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                              <button id="brineomaticDepickle" type="button" class="btn btn-warning"
                                  data-bs-dismiss="modal">De-Pickle</button>
                          </div>
                      </div>
                  </div>
              </div>

              <div class="modal fade" id="stopBrineomaticModal" tabindex="-1" role="dialog"
                  aria-labelledby="stopBrineomaticModalTitle" aria-hidden="true">
                  <div class="modal-dialog modal-dialog-centered" role="document">
                      <div class="modal-content">
                          <div class="modal-header">
                              <h1 class="modal-title fs-5" id="stopBrineomaticTitle">Stop Watermaker
                              </h1>
                              <button type="button" class="btn-close" data-bs-dismiss="modal"
                                  aria-label="Close"></button>
                          </div>
                          <div class="modal-body">
                              <p>If currently RUNNING, it will start a FLUSH cycle.</p>
                              <p>If currently FLUSHING or PICKLING, it will just stop.</p>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                              <button id="brineomaticStop" type="button" class="btn btn-danger"
                                  data-bs-dismiss="modal">Stop</button>
                          </div>
                      </div>
                  </div>
              </div>

              <div class="modal fade" id="manualBrineomaticModal" tabindex="-1" role="dialog"
                  aria-labelledby="manualBrineomaticModalTitle" aria-hidden="true">
                  <div class="modal-dialog modal-dialog-centered" role="document">
                      <div class="modal-content">
                          <div class="modal-header">
                              <h1 class="modal-title fs-5" id="stopBrineomaticTitle">Manual Mode
                              </h1>
                              <button type="button" class="btn-close" data-bs-dismiss="modal"
                                  aria-label="Close"></button>
                          </div>
                          <div class="modal-body">
                              <p>Manual mode gives you low level access to control individual components
                                  of your watermaker. Autoflush, fan/temperature control, etc will be
                                  disabled.</p>
                              <div class="alert alert-warning" role="alert">
                                  Warning: there are no safety checks in manual mode, please be careful as
                                  you could possibly damage your watermaker.
                              </div>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                              <button id="brineomaticManual" type="button" class="btn btn-success"
                                  data-bs-dismiss="modal">Continue</button>
                          </div>
                      </div>
                  </div>
              </div>
          </div>
          <div id="bomGauges" class="row gx-0 my-3 mfdHide">
              <div class="filterPressureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Filter Pressure</h6>
                  <div class="filterPressureError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="filterPressureContent">
                    <div id="filterPressureGauge" class="d-flex justify-content-center"></div>
                  </div>
              </div>
              <div class="membranePressureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Membrane Pressure</h6>
                  <div class="membranePressureError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="membranePressureContent">
                    <div id="membranePressureGauge" class="d-flex justify-content-center"></div>
                  </div>
              </div>
              <div class="productSalinityUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Product Salinity</h6>
                  <div id="productSalinityGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="brineSalinityUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Brine Salinity</h6>
                  <div id="brineSalinityGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="productFlowrateUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Product Flowrate</h6>
                  <div id="productFlowrateGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="brineFlowrateUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Brine Flowrate</h6>
                  <div id="brineFlowrateGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="totalFlowrateUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Total Flowrate</h6>
                  <div id="totalFlowrateGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="motorTemperatureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Motor Temperature</h6>
                  <div class="motorTemperatureError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="motorTemperatureContent">
                    <div id="motorTemperatureGauge" class="d-flex justify-content-center"></div>
                  </div>
              </div>
              <div class="waterTemperatureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Water Temperature</h6>
                  <div id="waterTemperatureGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="tankLevelUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Tank Level</h6>
                  <div class="tankLevelError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="tankLevelContent">
                    <div id="tankLevelGauge" class="d-flex justify-content-center"></div>
                  </div>
              </div>
              <div class="productVolumeUI col-md-3 col-sm-4 col-6 text-center">
                  <h6 class="my-0">Product Volume</h6>
                  <h1 class="bomVolumeData my-0 mt-3"></h1>
                  <h5 id="volumeUnits" class="text-body-tertiary volumeUnitsLong">liters</h5>
              </div>
              <div class="flushVolumeUI col-md-3 col-sm-4 col-6 text-center">
                  <h6 class="my-0">Flush Volume</h6>
                  <h1 class="bomFlushVolumeData my-0 mt-3"></h1>
                  <h5 id="volumeUnits" class="text-body-tertiary volumeUnitsLong">liters</h5>
              </div>
          </div>
          <div id="bomGaugesMFD" class="mfdShow row gx-0 gy-3 my-3 text-center">
              <div class="filterPressureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Filter Pressure</h6>
                  <div class="filterPressureError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="filterPressureContent">
                    <h1 id="filterPressureData" class="my-0"></h1>
                    <h5 id="filterPressureUnits" class="text-body-tertiary pressureUnits">Bar</h5>
                  </div>
              </div>
              <div class="membranePressureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Membrane Pressure</h6>
                  <div class="membranePressureError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="membranePressureContent">
                    <h1 id="membranePressureData" class="my-0"></h1>
                    <h5 id="membranePressureUnits" class="text-body-tertiary pressureUnits">Bar</h5>
                  </div>
              </div>
              <div class="productSalinityUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Product Salinity</h6>
                  <h1 id="productSalinityData" class="my-0"></h1>
                  <h5 id="productSalinityUnits" class="text-body-tertiary">PPM</h5>
              </div>
              <div class="brineSalinityUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Brine Salinity</h6>
                  <h1 id="brineSalinityData" class="my-0"></h1>
                  <h5 id="brineSalinityUnits" class="text-body-tertiary">PPM</h5>
              </div>
              <div class="productFlowrateUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Product Flowrate</h6>
                  <h1 id="productFlowrateData" class="my-0"></h1>
                  <h5 id="productFlowrateUnits" class="text-body-tertiary">LPH</h5>
              </div>
              <div class="brineFlowrateUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Brine Flowrate</h6>
                  <h1 id="brineFlowrateData" class="my-0"></h1>
                  <h5 id="brineFlowrateUnits" class="text-body-tertiary">LPH</h5>
              </div>
              <div class="totalFlowrateUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Total Flowrate</h6>
                  <h1 id="totalFlowrateData" class="my-0"></h1>
                  <h5 id="totalFlowrateUnits" class="text-body-tertiary">LPH</h5>
              </div>
              <div class="motorTemperatureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Motor Temperature</h6>
                  <div class="motorTemperatureError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="motorTemperatureContent">
                    <h1 id="motorTemperatureData" class="my-0"></h1>
                    <h5 id="motorTemperatureUnits" class="text-body-tertiary">°<span class="temperatureUnits">C</span></h5>
                  </div>
              </div>
              <div class="waterTemperatureUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Water Temperature</h6>
                  <h1 id="waterTemperatureData" class="my-0"></h1>
                  <h5 id="waterTemperatureUnits" class="text-body-tertiary">°<span class="temperatureUnits">C</span></h5>
              </div>
              <div class="tankLevelUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Tank Level</h6>
                  <div class="tankLevelError">
                    <div class="d-flex align-items-center justify-content-center">
                      <h4 class="text-danger text-center m-0">No Data</h4>
                    </div>
                  </div>
                  <div class="tankLevelContent">
                    <h1 id="tankLevelData" class="my-0"></h1>
                    <h5 id="tankLevelUnits" class="text-body-tertiary">%</h5>
                  </div>
              </div>
              <div class="productVolumeUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Product Volume</h6>
                  <h1 class="bomVolumeData" class="my-0"></h1>
                  <h5 id="volumeUnits" class="text-body-tertiary volumeUnitsLong">liters</h5>
              </div>
              <div class="flushVolumeUI col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Flush Volume</h6>
                  <h1 class="bomFlushVolumeData" class="my-0"></h1>
                  <h5 id="volumeUnits" class="text-body-tertiary volumeUnitsLong">liters</h5>
              </div>
          </div>
          <div id="bomManualControls" class="col bomMANUAL">
            <h4>Manual Watermaker Controls</h4>
            <div class="row gx-3 gy-3 my-3">
              <div id="boostPumpControlUI" class="col-xs-12 sm-6 col-md-4 text-center">
                <button id="boostPumpControlButton" type="button" class="btn btn-secondary relayButton">
                  <h4>Boost Pump</h4>
                  <div id="manualBoostPumpStatus">OFF</div>
                </button>
              </div>
              <div id="highPressurePumpControlUI" class="col-xs-12 sm-6 col-md-4 text-center">
                <button id="highPressurePumpControlButton" type="button" class="btn btn-secondary relayButton">
                  <h4>High Pressure Pump</h4>
                  <div id="manualHighPressurePumpStatus">OFF</div>
                </button>
              </div>
              <div id="diverterValveControlUI" class="col-xs-12 sm-6 col-md-4 text-center">
                <button id="diverterValveControlButton" type="button" class="btn btn-secondary relayButton">
                  <h4>Diverter Valve</h4>
                  <div id="manualDiverterValveStatus">CLOSED</div>
                </button>
              </div>
              <div id="flushValveControlUI" class="col-xs-12 sm-6 col-md-4 text-center">
                <button id="flushValveControlButton" type="button" class="btn btn-secondary relayButton">
                  <h4>Flush Valve</h4>
                  <div id="manualFlushValveStatus">CLOSED</div>
                </button>
              </div>
              <div id="coolingFanControlUI" class="col-xs-12 sm-6 col-md-4 text-center">
                <button id="coolingFanControlButton" type="button" class="btn btn-secondary relayButton">
                  <h4>Cooling Fan</h4>
                  <div id="manualCoolingFanStatus">CLOSED</div>
                </button>
              </div>
              <div class="col-xs-12 sm-6 col-md-4 text-center">
                <button id="advancedModeButton" type="button" class="btn btn-secondary relayButton">
                  <h4 class="my-0">Advanced Mode</h4>
                </button>
              </div>
          </div>
      </div>
    `;
  }

  Brineomatic.prototype.generateSettingsUI = function () {
    return /*html*/ `
      <div class="form-floating mb-3">
          <select id="autoflush_mode" class="form-select" aria-label="Autoflush Mode">
            <option value="NONE">None (Autoflush Disabled)</option>
            <option value="SALINITY">By Salinity</option>
            <option value="TIME">By Time</option>
            <option value="VOLUME">By Volume</option>
          </select>
          <label for="autoflush_mode">Autoflush Mode</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Autoflush Salinity</span>
          <input id="autoflush_salinity" type="text" class="form-control text-end">
          <span class="input-group-text">PPM</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Autoflush Duration</span>
          <input id="autoflush_duration" type="text" class="form-control text-end">
          <span class="input-group-text">minutes</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Autoflush Volume</span>
          <input id="autoflush_volume" type="text" class="form-control text-end">
          <span class="input-group-text volumeUnitsLong">liters</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Autoflush Interval</span>
          <input id="autoflush_interval" type="text" class="form-control text-end">
          <span class="input-group-text">hours</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
        <input class="form-check-input" type="checkbox" id="autoflush_use_high_pressure_motor">
        <label class="form-check-label" for="autoflush_use_high_pressure_motor">
          Use high pressure motor during autoflush
        </label>
        <div class="invalid-feedback"></div>
      </div>

      <hr class="bold">

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Tank Capacity</span>
          <input id="tank_capacity" type="text" class="form-control text-end">
          <span class="input-group-text volumeUnitsLong">liters</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="temperature_units" class="form-select" aria-label="Temperature Units">
            <option value="celsius">Celsius</option>
            <option value="fahrenheit">Fahrenheit</option>
          </select>
          <label for="temperature_units">Temperature Units</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="pressure_units" class="form-select" aria-label="Pressure Units">
            <option value="psi">PSI</option>
            <option value="bar">Bar</option>
            <option value="kilopascal">Kpa</option>
          </select>
          <label for="pressure_units">Pressure Units</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="volume_units" class="form-select" aria-label="Volume Units">
            <option value="liters">Liters</option>
            <option value="gallons">Gallons</option>
          </select>
          <label for="volume_units">Volume Units</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="flowrate_units" class="form-select" aria-label="Flowrate Units">
            <option value="lph">LPH (liters per hour)</option>
            <option value="gph">GPH (gallons per hour)</option>
          </select>
          <label for="flowrate_units">Flowrate Units</label>
          <div class="invalid-feedback"></div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="success_melody" class="form-select" aria-label="Success Melody">
          </select>
          <label for="success_melody">Success Melody</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="error_melody" class="form-select" aria-label="Error Melody">
          </select>
          <label for="error_melody">Error Melody</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="text-center">
          <button id="saveBrineomaticSettings" type="button" class="btn btn-primary">
              Save General Settings
          </button>
      </div>
    `;
  }

  Brineomatic.prototype.generateHardwareSettingsUI = function () {
    let relayOptions = "";
    let relays = YB.ChannelRegistry.getChannelsByType("relay");
    for (ch of relays) {
      relayOptions += `<option value="${ch.id}">Relay ${ch.id}</option>`;
    };

    let servoOptions = "";
    let servos = YB.ChannelRegistry.getChannelsByType("servo");
    for (ch of servos) {
      servoOptions += `<option value="${ch.id}">Servo ${ch.id}</option>`;
    };

    let stepperOptions = "";
    let steppers = YB.ChannelRegistry.getChannelsByType("stepper");
    for (ch of steppers) {
      stepperOptions += `<option value="${ch.id}">Stepper ${ch.id}</option>`;
    };

    let membranePressure = "";
    if (YB.App.config.capabilities.brineomatic.hp_sensor)
      membranePressure = /*html*/ `
      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="has_membrane_pressure_sensor">
          <label class="form-check-label" for="has_membrane_pressure_sensor">
              Has Membrane Pressure Sensor
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="has_membrane_pressure_sensor_form" class="row g-3 mb-3">
        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Min</span>
            <input type="text" class="form-control text-end" id="membrane_pressure_sensor_min">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>

        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Max</span>
            <input type="text" class="form-control text-end" id="membrane_pressure_sensor_max">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>
    `;

    let filterPressure = "";
    if (YB.App.config.capabilities.brineomatic.lp_sensor)
      filterPressure = /*html*/ `
      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="has_filter_pressure_sensor">
          <label class="form-check-label" for="has_filter_pressure_sensor">
              Has Filter Pressure Sensor
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="has_filter_pressure_sensor_form" class="row g-3 mb-3">
        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Min</span>
            <input type="text" class="form-control text-end" id="filter_pressure_sensor_min">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>

        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Max</span>
            <input type="text" class="form-control text-end" id="filter_pressure_sensor_max">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>
    `;

    let productFlow = "";
    if (YB.App.config.capabilities.brineomatic.product_flowmeter)
      productFlow = /*html*/ `
      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="has_product_flow_sensor">
          <label class="form-check-label" for="has_product_flow_sensor">
              Has Product Flow Sensor
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="has_product_flow_sensor_form" class="mb-3">
        <div class="input-group has-validation">
          <input type="text" class="form-control text-end" id="product_flowmeter_ppl">
          <span class="input-group-text">PPL (Pulses Per Liter)</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>
    `;

    let brineFlow = "";
    if (YB.App.config.capabilities.brineomatic.brine_flowmeter)
      brineFlow = /*html*/ `
      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="has_brine_flow_sensor">
          <label class="form-check-label" for="has_brine_flow_sensor">
              Has Brine Flow Sensor
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="has_brine_flow_sensor_form" class="mb-3">
        <div class="input-group has-validation">
          <input type="text" class="form-control text-end" id="brine_flowmeter_ppl">
          <span class="input-group-text">PPL (Pulses Per Liter)</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>
    `;

    let productTDS = "";
    if (YB.App.config.capabilities.brineomatic.product_tds)
      productTDS = /*html*/ `
      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="has_product_tds_sensor">
          <label class="form-check-label" for="has_product_tds_sensor">
              Has Product TDS Sensor
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="has_product_tds_sensor_form" class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Calibration Offset</span>
          <input type="text" class="form-control text-end" id="product_tds_sensor_offset">
          <span class="input-group-text">PPM</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>
    `;

    let brineTDS = "";
    if (YB.App.config.capabilities.brineomatic.brine_tds)
      brineTDS = /*html*/ `
      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="has_brine_tds_sensor">
          <label class="form-check-label" for="has_brine_tds_sensor">
              Has Brine TDS Sensor
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="has_brine_tds_sensor_form" class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Calibration Offset</span>
          <input type="text" class="form-control text-end" id="brine_tds_sensor_offset">
          <span class="input-group-text">PPM</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>
    `;

    let motorTemperature = "";
    if (YB.App.config.capabilities.brineomatic.motor_temperature)
      motorTemperature = /*html*/ `
      <div class="form-floating mb-3">
          <select id="motor_temperature_sensor_type" class="form-select" aria-label="Motor Temperature Sensor">
            <option value="NONE">None</option>
            <option value="EXTERNAL">External (via NodeRED or API)</option>
            <option value="DS18B20">DS18B20 (directly connected)</option>
          </select>
          <label for="motor_temperature_sensor_type">Motor Temperature Sensor</label>
          <div class="invalid-feedback"></div>
      </div>
    `;

    let waterTemperature = "";
    if (YB.App.config.capabilities.brineomatic.water_temperature)
      waterTemperature = /*html*/ `
      <div class="form-floating mb-3">
          <select id="water_temperature_sensor_type" class="form-select" aria-label="Water Temperature Sensor">
            <option value="NONE">None</option>
            <option value="EXTERNAL">External (via NodeRED or API)</option>
            <option value="DS18B20">DS18B20 (directly connected)</option>
          </select>
          <label for="water_temperature_sensor_type">Water Temperature Sensor</label>
          <div class="invalid-feedback"></div>
      </div>
    `;

    return /*html*/ `
      <div id="hardwareSettingsDisabled" class="alert alert-warning" role="alert" style="display: none">
        Hardware configuration disabled. <b>IDLE</b> or <b>MANUAL</b> mode only.
      </div>
      <div class="form-floating mb-3">
          <select id="boost_pump_control" class="form-select" aria-label="Boost Pump">
              <option value="NONE">None</option>
              <option value="MANUAL">Manual</option>
              <option value="RELAY">Relay</option>
          </select>
          <label for="boost_pump_control">Boost Pump Control</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="boost_pump_relay_id" class="form-select" aria-label="Boost Pump Relay Channel">
            ${relayOptions}
          </select>
          <label for="boost_pump_relay_id">Boost Pump Relay Channel</label>
            <div class="invalid-feedback"></div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="boost_pump_relay_inverted">
          <label class="form-check-label" for="boost_pump_relay_inverted">
              Is Boost Pump Relay Inverted?
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="high_pressure_pump_control" class="form-select" aria-label="High Pressure Pump Control">
              <option value="NONE">None</option>
              <option value="MANUAL">Manual</option>
              <option value="RELAY">Relay</option>
              <option value="MODBUS">Modbus / RS-485</option>
          </select>
          <label for="high_pressure_pump_control">High Pressure Pump Control</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="high_pressure_relay_id" class="form-select" aria-label="High Pressure Pump Relay Channel">
            ${relayOptions}
          </select>
          <label for="high_pressure_relay_id">High Pressure Pump Relay Channel</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="high_pressure_relay_inverted">
          <label class="form-check-label" for="high_pressure_relay_inverted">
              Is High Pressure Pump Relay Inverted?
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="high_pressure_modbus_options form-floating mb-3">
          <select id="high_pressure_modbus_device" class="form-select" aria-label="High Pressure Pump Modbus Device">
            <option value="GD20">INVT GD20</option>
          </select>
          <label for="high_pressure_modbus_device">High Pressure Pump Modbus Device</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="high_pressure_modbus_options row g-3 mb-3">
        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Modbus Slave ID</span>
            <input type="text" class="form-control text-end" id="high_pressure_modbus_slave_id">
            <div class="invalid-feedback"></div>
          </div>
        </div>

        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Frequency</span>
            <input type="text" class="form-control text-end" id="high_pressure_modbus_frequency">
            <span class="input-group-text">hz</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="high_pressure_valve_control" class="form-select" aria-label="High Pressure Valve Control">
              <option value="NONE">None</option>
              <option value="MANUAL">Manual</option>
              <option value="STEPPER">Stepper</option>
          </select>
          <label for="high_pressure_valve_control">High Pressure Valve Control</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="mb-3" style="display: none">
        <div class="input-group has-validation">
          <span class="input-group-text">Pressure Target</span>
          <input id="membrane_pressure_target" type="text" class="form-control text-end">
          <span class="input-group-text pressureUnits">Bar</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div id="high_pressure_valve_stepper_options">
        <div class="form-floating mb-3">
          <select id="high_pressure_valve_stepper_id" class="form-select" aria-label="High Pressure Valve Stepper Channel">
            ${stepperOptions}
          </select>
          <label for="high_pressure_valve_stepper_id">High Pressure Valve Stepper Channel</label>
          <div class="invalid-feedback"></div>
        </div>

        <div class="row g-3 mb-3">
          <h6>Stepper Motor Configuration</h6>
          
          <div class="col-12 col-md-6 mt-1">
            <div class="input-group has-validation">
              <span class="input-group-text">Step Angle</span>
              <input type="text" class="form-control text-end" id="high_pressure_stepper_step_angle">
              <span class="input-group-text">°</span>
              <div class="invalid-feedback"></div>
          </div>
          </div>

          <div class="col-12 col-md-6 mt-1">
            <div class="input-group has-validation">
              <span class="input-group-text">Gear Ratio</span>
              <input type="text" class="form-control text-end" id="high_pressure_stepper_gear_ratio">
              <span class="input-group-text">to 1</span>
            <div class="invalid-feedback"></div>
            </div>
          </div>
        </div>

        <div class="row g-3 mb-3">
          <h6>High Pressure Valve Close (Pressure On)</h6>
          
          <div class="col-12 col-md-6 mt-1">
            <div class="input-group has-validation">
              <span class="input-group-text">Angle</span>
              <input type="text" class="form-control text-end" id="high_pressure_stepper_close_angle">
              <span class="input-group-text">°</span>
              <div class="invalid-feedback"></div>
            </div>
          </div>

          <div class="col-12 col-md-6 mt-1">
            <div class="input-group has-validation">
              <span class="input-group-text">Speed</span>
              <input type="text" class="form-control text-end" id="high_pressure_stepper_close_speed">
              <span class="input-group-text">RPM</span>
              <div class="invalid-feedback"></div>
            </div>
          </div>
        </div>

        <div class="row g-3 mb-3">
          <h6>High Pressure Valve Open (Pressure Off)</h6>
          <div class="col-12 col-md-6 mt-1">
            <div class="input-group has-validation">
              <span class="input-group-text">Angle</span>
              <input type="text" class="form-control text-end" id="high_pressure_stepper_open_angle">
              <span class="input-group-text">°</span>
              <div class="invalid-feedback"></div>
            </div>
          </div>

          <div class="col-12 col-md-6 mt-1">
            <div class="input-group has-validation">
              <span class="input-group-text">Speed</span>
              <input type="text" class="form-control text-end" id="high_pressure_stepper_open_speed">
              <span class="input-group-text">RPM</span>
              <div class="invalid-feedback"></div>
            </div>
          </div>
        </div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="diverter_valve_control" class="form-select" aria-label="Diverter Valve Control">
              <option value="NONE">None</option>
              <option value="MANUAL">Manual</option>
              <option value="RELAY">Relay</option>
              <option value="SERVO">Servo</option>
          </select>
          <label for="diverter_valve_control">Diverter Valve Control</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="diverter_valve_relay_id" class="form-select" aria-label="Diverter Valve Relay Channel">
            ${relayOptions}
          </select>
          <label for="diverter_valve_relay_id">Diverter Valve Relay Channel</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="diverter_valve_relay_inverted">
          <label class="form-check-label" for="diverter_valve_relay_inverted">
              Is Diverter Valve Relay Inverted?
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="diverter_valve_servo_id" class="form-select" aria-label="Diverter Valve Servo Channel">
            ${servoOptions}
          </select>
          <label for="diverter_valve_servo_id">Diverter Valve Servo Channel</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="row g-3 mb-3">
        <h6>Diverter Valve Settings (Open = Overboard)</h6>

        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Open</span>
            <input type="text" class="form-control text-end" id="diverter_valve_open_angle">
            <span class="input-group-text">°</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>

        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Close</span>
            <input type="text" class="form-control text-end" id="diverter_valve_close_angle">
            <span class="input-group-text">°</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="flush_valve_control" class="form-select" aria-label="Flush Valve Control">
              <option value="NONE">None</option>
              <option value="MANUAL">Manual</option>
              <option value="RELAY">Relay</option>
          </select>
          <label for="flush_valve_control">Flush Valve Control</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="flush_valve_relay_id" class="form-select" aria-label="Flush Valve Relay Channel">
            ${relayOptions}
          </select>
          <label for="flush_valve_relay_id">Flush Valve Relay Channel</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="flush_valve_relay_inverted">
          <label class="form-check-label" for="flush_valve_relay_inverted">
              Is Flush Valve Relay Inverted?
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <hr class="bold">

      <div class="form-floating mb-3">
          <select id="cooling_fan_control" class="form-select" aria-label="Cooling Fan Control">
              <option value="NONE">None</option>
              <option value="MANUAL">Manual</option>
              <option value="RELAY">Relay</option>
          </select>
          <label for="cooling_fan_control">Cooling Fan Control</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-floating mb-3">
          <select id="cooling_fan_relay_id" class="form-select" aria-label="Cooling Fan Relay Channel">
            ${relayOptions}
          </select>
          <label for="cooling_fan_relay_id">Cooling Fan Relay Channel</label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="cooling_fan_relay_inverted">
          <label class="form-check-label" for="cooling_fan_relay_inverted">
              Is Cooling Fan Relay Inverted?
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div class="row g-3 mb-3">
        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">On Temp</span>
            <input type="text" class="form-control text-end" id="cooling_fan_on_temperature">
            <span class="input-group-text temperatureUnits">C</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>

        <div class="col-12 col-md-6 mt-1">
          <div class="input-group has-validation">
            <span class="input-group-text">Off Temp</span>
            <input type="text" class="form-control text-end" id="cooling_fan_off_temperature">
            <span class="input-group-text temperatureUnits">C</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <hr class="bold">

      ${membranePressure}
      ${filterPressure}
      ${productFlow}
      ${brineFlow}
      ${productTDS}
      ${brineTDS}
      ${motorTemperature}
      ${waterTemperature}

      <div class="text-center mb-3">
          <button id="saveHardwareSettings" type="button" class="btn btn-primary">
              Save Hardware Settings
          </button>
      </div>

      <div class="alert alert-info mb-0" role="alert">
        Brineomatic will restart after updating hardware configuration.
      </div>
    `;
  }

  Brineomatic.prototype.generateSafeguardsSettingsUI = function () {
    return /*html*/ `
      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Flush Timeout</span>
          <input id="flush_timeout" type="text" class="form-control text-end">
          <span class="input-group-text">seconds</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">High Pressure Timeout</span>
          <input id="membrane_pressure_timeout" type="text" class="form-control text-end">
          <span class="input-group-text">seconds</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Product Flowrate Timeout</span>
          <input id="product_flowrate_timeout" type="text" class="form-control text-end">
          <span class="input-group-text">seconds</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Product Salinity Timeout</span>
          <input id="product_salinity_timeout" type="text" class="form-control text-end">
          <span class="input-group-text">seconds</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <div class="mb-3">
        <div class="input-group has-validation">
          <span class="input-group-text">Production Runtime Timeout</span>
          <input id="production_runtime_timeout" type="text" class="form-control text-end">
          <span class="input-group-text">hours</span>
          <div class="invalid-feedback"></div>
        </div>
      </div>

      <hr class="bold">

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_membrane_pressure_high_check">
          <label class="form-check-label" for="enable_membrane_pressure_high_check">
              Membrane Pressure High
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_membrane_pressure_high_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="membrane_pressure_high_threshold">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="membrane_pressure_high_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_membrane_pressure_low_check">
          <label class="form-check-label" for="enable_membrane_pressure_low_check">
              Membrane Pressure Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_membrane_pressure_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="membrane_pressure_low_threshold">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation has-validation">
            <input type="text" class="form-control text-end" id="membrane_pressure_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_filter_pressure_high_check">
          <label class="form-check-label" for="enable_filter_pressure_high_check">
              Filter Pressure High
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_filter_pressure_high_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="filter_pressure_high_threshold">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="filter_pressure_high_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_filter_pressure_low_check">
          <label class="form-check-label" for="enable_filter_pressure_low_check">
              Filter Pressure Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_filter_pressure_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="filter_pressure_low_threshold">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="filter_pressure_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_product_flowrate_high_check">
          <label class="form-check-label" for="enable_product_flowrate_high_check">
              Product Flowrate High
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_product_flowrate_high_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="product_flowrate_high_threshold">
            <span class="input-group-text flowrateUnits">LPH</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="product_flowrate_high_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_product_flowrate_low_check">
          <label class="form-check-label" for="enable_product_flowrate_low_check">
              Product Flowrate Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_product_flowrate_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="product_flowrate_low_threshold">
            <span class="input-group-text flowrateUnits">LPH</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="product_flowrate_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_run_total_flowrate_low_check">
          <label class="form-check-label" for="enable_run_total_flowrate_low_check">
              Run Total Flowrate Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_run_total_flowrate_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="run_total_flowrate_low_threshold">
            <span class="input-group-text flowrateUnits">LPH</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="run_total_flowrate_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_pickle_total_flowrate_low_check">
          <label class="form-check-label" for="enable_pickle_total_flowrate_low_check">
              De/Pickle Total Flowrate Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_pickle_total_flowrate_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="pickle_total_flowrate_low_threshold">
            <span class="input-group-text flowrateUnits">LPH</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="pickle_total_flowrate_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_diverter_valve_closed_check">
          <label class="form-check-label" for="enable_diverter_valve_closed_check">
              Diverter Valve Opening / Closing
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_diverter_valve_closed_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="diverter_valve_closed_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_product_salinity_high_check">
          <label class="form-check-label" for="enable_product_salinity_high_check">
              Product Salinity High
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_product_salinity_high_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="product_salinity_high_threshold">
            <span class="input-group-text">PPM</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="product_salinity_high_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_motor_temperature_check">
          <label class="form-check-label" for="enable_motor_temperature_check">
              Motor Temperature
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_motor_temperature_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="motor_temperature_high_threshold">
            <span class="input-group-text temperatureUnits">C</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="motor_temperature_high_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_flush_flowrate_low_check">
          <label class="form-check-label" for="enable_flush_flowrate_low_check">
              Flush Flowrate Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_flush_flowrate_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="flush_flowrate_low_threshold">
            <span class="input-group-text flowrateUnits">LPH</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="flush_flowrate_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_flush_filter_pressure_low_check">
          <label class="form-check-label" for="enable_flush_filter_pressure_low_check">
              Flush Filter Pressure Low
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_flush_filter_pressure_low_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="flush_filter_pressure_low_threshold">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="flush_filter_pressure_low_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="enable_flush_valve_off_check">
          <label class="form-check-label" for="enable_flush_valve_off_check">
              Flush Valve Off
          </label>
          <div class="invalid-feedback"></div>
      </div>

      <div id="enable_flush_valve_off_check_form" class="row">
        <div class="col-12 col-md-6">
          <div class="input-group has-validation mb-3">
            <input type="text" class="form-control text-end" id="flush_valve_off_threshold">
            <span class="input-group-text pressureUnits">Bar</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
        <div class="col-12 col-md-6">
          <div class="input-group has-validation">
            <input type="text" class="form-control text-end" id="flush_valve_off_delay">
            <span class="input-group-text">Delay (ms)</span>
            <div class="invalid-feedback"></div>
          </div>
        </div>
      </div>

      <div class="text-center">
          <button id="saveSafeguardsSettings" type="button" class="btn btn-primary">
              Save Safeguard Settings
          </button>
      </div>
    `;
  }

  Brineomatic.prototype.updateEditUIData = function (data) {
    $("#autoflush_mode").val(data.autoflush_mode);
    $("#autoflush_salinity").val(data.autoflush_salinity);
    $("#autoflush_duration").val(data.autoflush_duration / (60 * 1000));
    $("#autoflush_volume").val(YB.bom.convertVolume(data.autoflush_volume, "liters", YB.App.config.brineomatic.volume_units));
    $("#autoflush_interval").val(data.autoflush_interval / (60 * 60 * 1000));
    $("#autoflush_use_high_pressure_motor").prop('checked', data.autoflush_use_high_pressure_motor);

    $("#tank_capacity").val(this.formatReadable(YB.bom.convertVolume(data.tank_capacity, "liters", YB.App.config.brineomatic.volume_units)));
    $("#temperature_units").val(data.temperature_units);
    $("#pressure_units").val(data.pressure_units);
    $("#volume_units").val(data.volume_units);
    $("#flowrate_units").val(data.flowrate_units);

    this.updateTemperatureUnits(data.temperature_units);
    this.updatePressureUnits(data.pressure_units);
    this.updateVolumeUnits(data.volume_units);
    this.updateFlowrateUnits(data.flowrate_units);

    YB.Util.populateMelodySelector($("#success_melody"));
    $("#success_melody").val(data.success_melody);
    YB.Util.populateMelodySelector($("#error_melody"));
    $("#error_melody").val(data.error_melody);

    $("#boost_pump_control").val(data.boost_pump_control);
    $("#boost_pump_relay_id").val(data.boost_pump_relay_id);
    $("#boost_pump_relay_inverted").prop('checked', data.boost_pump_relay_inverted);

    $("#high_pressure_pump_control").val(data.high_pressure_pump_control);
    $("#high_pressure_relay_id").val(data.high_pressure_relay_id);
    $("#high_pressure_relay_inverted").prop('checked', data.high_pressure_relay_inverted);
    $("#high_pressure_modbus_device").val(data.high_pressure_modbus_device);
    $("#high_pressure_modbus_slave_id").val(data.high_pressure_modbus_slave_id);
    $("#high_pressure_modbus_frequency").val(data.high_pressure_modbus_frequency);

    $("#high_pressure_valve_control").val(data.high_pressure_valve_control);
    $("#high_pressure_valve_stepper_id").val(data.high_pressure_valve_stepper_id);
    $("#high_pressure_stepper_step_angle").val(data.high_pressure_stepper_step_angle);
    $("#high_pressure_stepper_gear_ratio").val(data.high_pressure_stepper_gear_ratio);
    $("#high_pressure_stepper_close_angle").val(data.high_pressure_stepper_close_angle);
    $("#high_pressure_stepper_close_speed").val(data.high_pressure_stepper_close_speed);
    $("#high_pressure_stepper_open_angle").val(data.high_pressure_stepper_open_angle);
    $("#high_pressure_stepper_open_speed").val(data.high_pressure_stepper_open_speed);

    let membrane_pressure_target = YB.bom.convertPressure(data.membrane_pressure_target, "Bar", YB.App.config.brineomatic.pressure_units);
    membrane_pressure_target = this.formatReadable(membrane_pressure_target);
    $("#membrane_pressure_target").val(membrane_pressure_target);

    $("#diverter_valve_control").val(data.diverter_valve_control);
    $("#diverter_valve_relay_id").val(data.diverter_valve_relay_id);
    $("#diverter_valve_relay_inverted").prop('checked', data.diverter_valve_relay_inverted);

    $("#diverter_valve_servo_id").val(data.diverter_valve_servo_id);
    $("#diverter_valve_open_angle").val(data.diverter_valve_open_angle);
    $("#diverter_valve_close_angle").val(data.diverter_valve_close_angle);

    $("#flush_valve_control").val(data.flush_valve_control);
    $("#flush_valve_relay_id").val(data.flush_valve_relay_id);
    $("#flush_valve_relay_inverted").prop('checked', data.flush_valve_relay_inverted);

    $("#cooling_fan_control").val(data.cooling_fan_control);
    $("#cooling_fan_relay_id").val(data.cooling_fan_relay_id);
    $("#cooling_fan_relay_inverted").prop('checked', data.cooling_fan_relay_inverted);

    let cooling_fan_on_temp = YB.bom.convertTemperature(data.cooling_fan_on_temperature, "C", YB.App.config.brineomatic.temperature_units);
    cooling_fan_on_temp = Math.round(cooling_fan_on_temp);
    $("#cooling_fan_on_temperature").val(cooling_fan_on_temp);

    let cooling_fan_off_temp = YB.bom.convertTemperature(data.cooling_fan_off_temperature, "C", YB.App.config.brineomatic.temperature_units);
    cooling_fan_off_temp = Math.round(cooling_fan_off_temp);
    $("#cooling_fan_off_temperature").val(cooling_fan_off_temp);

    $("#has_membrane_pressure_sensor").prop('checked', data.has_membrane_pressure_sensor);

    let membrane_pressure_sensor_min = YB.bom.convertPressure(data.membrane_pressure_sensor_min, "Bar", YB.App.config.brineomatic.pressure_units);
    membrane_pressure_sensor_min = this.formatReadable(membrane_pressure_sensor_min);
    $("#membrane_pressure_sensor_min").val(membrane_pressure_sensor_min);

    let membrane_pressure_sensor_max = YB.bom.convertPressure(data.membrane_pressure_sensor_max, "Bar", YB.App.config.brineomatic.pressure_units);
    membrane_pressure_sensor_max = this.formatReadable(membrane_pressure_sensor_max);
    $("#membrane_pressure_sensor_max").val(membrane_pressure_sensor_max);

    $("#has_filter_pressure_sensor").prop('checked', data.has_filter_pressure_sensor);

    let filter_pressure_sensor_min = YB.bom.convertPressure(data.filter_pressure_sensor_min, "Bar", YB.App.config.brineomatic.pressure_units);
    filter_pressure_sensor_min = this.formatReadable(filter_pressure_sensor_min);
    $("#filter_pressure_sensor_min").val(filter_pressure_sensor_min);

    let filter_pressure_sensor_max = YB.bom.convertPressure(data.filter_pressure_sensor_max, "Bar", YB.App.config.brineomatic.pressure_units);
    filter_pressure_sensor_max = this.formatReadable(filter_pressure_sensor_max);
    $("#filter_pressure_sensor_max").val(filter_pressure_sensor_max);

    $("#has_product_tds_sensor").prop('checked', data.has_product_tds_sensor);
    $("#product_tds_sensor_offset").val(data.product_tds_sensor_offset);

    $("#has_brine_tds_sensor").prop('checked', data.has_brine_tds_sensor);
    $("#brine_tds_sensor_offset").val(data.brine_tds_sensor_offset);

    $("#has_product_flow_sensor").prop('checked', data.has_product_flow_sensor);
    $("#product_flowmeter_ppl").val(data.product_flowmeter_ppl);

    $("#has_brine_flow_sensor").prop('checked', data.has_brine_flow_sensor);
    $("#brine_flowmeter_ppl").val(data.brine_flowmeter_ppl);

    $("#motor_temperature_sensor_type").val(data.motor_temperature_sensor_type);
    $("#water_temperature_sensor_type").val(data.water_temperature_sensor_type);

    $("#flush_timeout").val(data.flush_timeout / (1000));
    $("#membrane_pressure_timeout").val(data.membrane_pressure_timeout / (1000));
    $("#product_flowrate_timeout").val(data.product_flowrate_timeout / (1000));
    $("#product_salinity_timeout").val(data.product_salinity_timeout / (1000));
    $("#production_runtime_timeout").val(data.production_runtime_timeout / (60 * 60 * 1000));

    $("#enable_membrane_pressure_high_check").prop('checked', data.enable_membrane_pressure_high_check);
    $("#membrane_pressure_high_delay").val(data.membrane_pressure_high_delay);
    let membrane_pressure_high_threshold = YB.bom.convertPressure(data.membrane_pressure_high_threshold, "Bar", YB.App.config.brineomatic.pressure_units);
    membrane_pressure_high_threshold = this.formatReadable(membrane_pressure_high_threshold);
    $("#membrane_pressure_high_threshold").val(membrane_pressure_high_threshold);

    $("#enable_membrane_pressure_low_check").prop('checked', data.enable_membrane_pressure_low_check);
    $("#membrane_pressure_low_delay").val(data.membrane_pressure_low_delay);
    let membrane_pressure_low_threshold = YB.bom.convertPressure(data.membrane_pressure_low_threshold, "Bar", YB.App.config.brineomatic.pressure_units);
    membrane_pressure_low_threshold = this.formatReadable(membrane_pressure_low_threshold);
    $("#membrane_pressure_low_threshold").val(membrane_pressure_low_threshold);

    $("#enable_filter_pressure_high_check").prop('checked', data.enable_filter_pressure_high_check);
    $("#filter_pressure_high_delay").val(data.filter_pressure_high_delay);
    let filter_pressure_high_threshold = YB.bom.convertPressure(data.filter_pressure_high_threshold, "Bar", YB.App.config.brineomatic.pressure_units);
    filter_pressure_high_threshold = this.formatReadable(filter_pressure_high_threshold);
    $("#filter_pressure_high_threshold").val(filter_pressure_high_threshold);

    $("#enable_filter_pressure_low_check").prop('checked', data.enable_filter_pressure_low_check);
    $("#filter_pressure_low_delay").val(data.filter_pressure_low_delay);
    let filter_pressure_low_threshold = YB.bom.convertPressure(data.filter_pressure_low_threshold, "Bar", YB.App.config.brineomatic.pressure_units);
    filter_pressure_low_threshold = this.formatReadable(filter_pressure_low_threshold);
    $("#filter_pressure_low_threshold").val(filter_pressure_low_threshold);

    $("#enable_product_flowrate_high_check").prop('checked', data.enable_product_flowrate_high_check);
    $("#product_flowrate_high_threshold").val(this.formatReadable(YB.bom.convertFlowrate(data.product_flowrate_high_threshold, "lph", YB.App.config.brineomatic.flowrate_units)));
    $("#product_flowrate_high_delay").val(data.product_flowrate_high_delay);

    $("#enable_product_flowrate_low_check").prop('checked', data.enable_product_flowrate_low_check);
    $("#product_flowrate_low_threshold").val(this.formatReadable(YB.bom.convertFlowrate(data.product_flowrate_low_threshold, "lph", YB.App.config.brineomatic.flowrate_units)));
    $("#product_flowrate_low_delay").val(data.product_flowrate_low_delay);

    $("#enable_run_total_flowrate_low_check").prop('checked', data.enable_run_total_flowrate_low_check);
    $("#run_total_flowrate_low_threshold").val(this.formatReadable(YB.bom.convertFlowrate(data.run_total_flowrate_low_threshold, "lph", YB.App.config.brineomatic.flowrate_units)));
    $("#run_total_flowrate_low_delay").val(data.run_total_flowrate_low_delay);

    $("#enable_pickle_total_flowrate_low_check").prop('checked', data.enable_pickle_total_flowrate_low_check);
    $("#pickle_total_flowrate_low_threshold").val(this.formatReadable(YB.bom.convertFlowrate(data.pickle_total_flowrate_low_threshold, "lph", YB.App.config.brineomatic.flowrate_units)));
    $("#pickle_total_flowrate_low_delay").val(data.pickle_total_flowrate_low_delay);

    $("#enable_diverter_valve_closed_check").prop('checked', data.enable_diverter_valve_closed_check);
    $("#diverter_valve_closed_delay").val(data.diverter_valve_closed_delay);

    $("#enable_product_salinity_high_check").prop('checked', data.enable_product_salinity_high_check);
    $("#product_salinity_high_threshold").val(data.product_salinity_high_threshold);
    $("#product_salinity_high_delay").val(data.product_salinity_high_delay);

    $("#enable_motor_temperature_check").prop('checked', data.enable_motor_temperature_check);
    $("#motor_temperature_high_delay").val(data.motor_temperature_high_delay);

    let motor_temp_threshold = YB.bom.convertTemperature(data.motor_temperature_high_threshold, "C", YB.App.config.brineomatic.temperature_units);
    motor_temp_threshold = Math.round(motor_temp_threshold);
    $("#motor_temperature_high_threshold").val(motor_temp_threshold);

    $("#enable_flush_flowrate_low_check").prop('checked', data.enable_flush_flowrate_low_check);
    $("#flush_flowrate_low_threshold").val(this.formatReadable(YB.bom.convertFlowrate(data.flush_flowrate_low_threshold, "lph", YB.App.config.brineomatic.flowrate_units)));
    $("#flush_flowrate_low_delay").val(data.flush_flowrate_low_delay);

    $("#enable_flush_filter_pressure_low_check").prop('checked', data.enable_flush_filter_pressure_low_check);
    $("#flush_filter_pressure_low_delay").val(data.flush_filter_pressure_low_delay);
    let flush_filter_pressure_low_threshold = YB.bom.convertPressure(data.flush_filter_pressure_low_threshold, "Bar", YB.App.config.brineomatic.pressure_units);
    flush_filter_pressure_low_threshold = this.formatReadable(flush_filter_pressure_low_threshold);
    $("#flush_filter_pressure_low_threshold").val(flush_filter_pressure_low_threshold);

    $("#enable_flush_valve_off_check").prop('checked', data.enable_flush_valve_off_check);
    $("#flush_valve_off_threshold").val(data.flush_valve_off_threshold);
    $("#flush_valve_off_delay").val(data.flush_valve_off_delay);
  }

  Brineomatic.prototype.updateHardwareUIConfig = function (data) {
    // control hardware
    this.updateAutoflushVisibility(data.autoflush_mode);
    this.updateBoostPumpVisibility(data.boost_pump_control);
    this.updateHighPressurePumpVisibility(data.high_pressure_pump_control);
    this.updateHighPressureValveVisibility(data.high_pressure_valve_control);
    this.updateDiverterValveVisibility(data.diverter_valve_control);
    this.updateFlushValveVisibility(data.flush_valve_control);
    this.updateCoolingFanVisibility(data.cooling_fan_control);

    this.updateMembranePressureVisibility(data.has_membrane_pressure_sensor);
    this.updateFilterPressureVisibility(data.has_filter_pressure_sensor);
    this.updateProductFlowrateVisibility(data.has_product_flow_sensor);
    this.updateBrineFlowrateVisibility(data.has_brine_flow_sensor);
    this.updateProductTDSVisibility(data.has_product_tds_sensor);
    this.updateBrineTDSVisibility(data.has_brine_tds_sensor);
    this.updateMotorTemperatureVisibility(data.motor_temperature_sensor_type);
    this.updateWaterTemperatureVisibility(data.water_temperature_sensor_type);

    this.updateDiverterValveClosedCheckVisibility(data.has_product_flow_sensor, data.has_brine_flow_sensor);
    this.updateFlushValveClosedCheckVisibility(data.has_filter_pressure_sensor, data.has_brine_flow_sensor);

    this.updateSafeguardChecks();

    //control UI gauges
    $(".filterPressureUI").toggle(!!data.has_filter_pressure_sensor);
    $(".membranePressureUI").toggle(!!data.has_membrane_pressure_sensor);
    $(".productSalinityUI").toggle(!!data.has_product_tds_sensor);
    $(".brineSalinityUI").toggle(!!data.has_brine_tds_sensor);
    $(".productFlowrateUI").toggle(!!data.has_product_flow_sensor);
    $(".brineFlowrateUI").toggle(!!data.has_brine_flow_sensor);
    $(".totalFlowrateUI").toggle(!!data.has_brine_flow_sensor && !!data.has_product_flow_sensor);
    $(".motorTemperatureUI").toggle(data.motor_temperature_sensor_type != "NONE");
    $(".waterTemperatureUI").toggle(data.water_temperature_sensor_type != "NONE");
    $(".productVolumeUI").toggle(!!data.has_product_flow_sensor);
    $(".flushVolumeUI").toggle(!!data.has_brine_flow_sensor);
  }

  Brineomatic.prototype.updateSafeguardChecks = function () {
    //sub forms for each checkbox
    $('#bomConfigForm input.form-check-input').each(function () { });
    $('#bomConfigForm input.form-check-input').each(function () {
      if (!this.checked || this.disabled)
        $('#bomConfigForm #' + this.id + '_form').hide();
      else
        $('#bomConfigForm #' + this.id + '_form').show();
    });
  }

  Brineomatic.prototype.addEditUIHandlers = function (data) {
    $("#temperature_units").on("change", (e) => {
      // helper function
      const convertTemperatureField = (fieldId) => {
        let value = parseFloat($(fieldId).val());
        value = YB.bom.convertTemperature(value, YB.App.config.brineomatic.temperature_units, e.target.value);
        value = Math.round(value);
        $(fieldId).val(value);
      };

      convertTemperatureField("#motor_temperature_high_threshold");
      convertTemperatureField("#cooling_fan_on_temperature");
      convertTemperatureField("#cooling_fan_off_temperature");

      //now do everything else.
      YB.bom.updateTemperatureUnits(e.target.value);
    });

    $("#pressure_units").on("change", (e) => {
      // helper function
      const convertPressureField = (fieldId) => {
        let value = parseFloat($(fieldId).val());
        value = YB.bom.convertPressure(value, YB.App.config.brineomatic.pressure_units, e.target.value);
        value = this.formatReadable(value);
        $(fieldId).val(value);
      };

      // Hardware config pressure fields
      convertPressureField("#membrane_pressure_target");
      convertPressureField("#membrane_pressure_sensor_min");
      convertPressureField("#membrane_pressure_sensor_max");
      convertPressureField("#filter_pressure_sensor_min");
      convertPressureField("#filter_pressure_sensor_max");

      // Safeguards config pressure fields
      convertPressureField("#membrane_pressure_high_threshold");
      convertPressureField("#membrane_pressure_low_threshold");
      convertPressureField("#filter_pressure_high_threshold");
      convertPressureField("#filter_pressure_low_threshold");
      convertPressureField("#flush_filter_pressure_low_threshold");

      //now do everything else.
      YB.bom.updatePressureUnits(e.target.value);
    });

    $("#volume_units").on("change", (e) => {
      // helper function
      const convertVolumeField = (fieldId) => {
        let value = parseFloat($(fieldId).val());
        value = YB.bom.convertVolume(value, YB.App.config.brineomatic.volume_units, e.target.value);
        value = this.formatReadable(value);
        $(fieldId).val(value);
      };

      // Brineomatic config volume fields
      convertVolumeField("#autoflush_volume");
      convertVolumeField("#tank_capacity");

      //now do everything else.
      YB.bom.updateVolumeUnits(e.target.value);
    });

    $("#flowrate_units").on("change", (e) => {
      // helper function
      const convertFlowrateField = (fieldId) => {
        let value = parseFloat($(fieldId).val());
        value = YB.bom.convertFlowrate(value, YB.App.config.brineomatic.flowrate_units, e.target.value);
        value = this.formatReadable(value);
        $(fieldId).val(value);
      };

      // Safeguards config flowrate fields
      convertFlowrateField("#product_flowrate_high_threshold");
      convertFlowrateField("#product_flowrate_low_threshold");
      convertFlowrateField("#run_total_flowrate_low_threshold");
      convertFlowrateField("#pickle_total_flowrate_low_threshold");
      convertFlowrateField("#flush_flowrate_low_threshold");

      //now do everything else.
      YB.bom.updateFlowrateUnits(e.target.value);
    });

    $("#autoflush_mode").on("change", (e) => {
      YB.bom.updateAutoflushVisibility(e.target.value);
    });

    $("#boost_pump_control").on("change", (e) => {
      YB.bom.updateBoostPumpVisibility(e.target.value);
    });

    $("#high_pressure_pump_control").on("change", (e) => {
      YB.bom.updateHighPressurePumpVisibility(e.target.value);
    });

    $("#high_pressure_valve_control").on("change", (e) => {
      YB.bom.updateHighPressureValveVisibility(e.target.value);
    });

    $("#diverter_valve_control").on("change", (e) => {
      YB.bom.updateDiverterValveVisibility(e.target.value);
    });

    $("#flush_valve_control").on("change", (e) => {
      YB.bom.updateFlushValveVisibility(e.target.value);
    });

    $("#cooling_fan_control").on("change", (e) => {
      YB.bom.updateCoolingFanVisibility(e.target.value);
    });

    $("#has_membrane_pressure_sensor").on("change", (e) => {
      YB.bom.updateMembranePressureVisibility(e.target.checked);
      YB.bom.updateSafeguardChecks();
    });

    $("#has_filter_pressure_sensor").on("change", (e) => {
      YB.bom.updateFilterPressureVisibility(e.target.checked);

      let has_brine_flow_sensor = $('#has_brine_flow_sensor').prop('checked');
      this.updateFlushValveClosedCheckVisibility(e.target.checked, has_brine_flow_sensor);

      YB.bom.updateSafeguardChecks();
    });

    $("#has_product_flow_sensor").on("change", (e) => {
      YB.bom.updateProductFlowrateVisibility(e.target.checked);

      let has_brine_flow_sensor = $('#has_brine_flow_sensor').prop('checked');
      YB.bom.updateDiverterValveClosedCheckVisibility(e.target.checked, has_brine_flow_sensor);

      YB.bom.updateSafeguardChecks();
    });

    $("#has_brine_flow_sensor").on("change", (e) => {
      YB.bom.updateBrineFlowrateVisibility(e.target.checked);

      let has_product_flow_sensor = $('#has_product_flow_sensor').prop('checked');
      YB.bom.updateDiverterValveClosedCheckVisibility(has_product_flow_sensor, e.target.checked);

      let has_filter_pressure_sensor = $('#has_filter_pressure_sensor').prop('checked');
      this.updateFlushValveClosedCheckVisibility(has_filter_pressure_sensor, e.target.checked);

      YB.bom.updateSafeguardChecks();
    });

    $("#has_product_tds_sensor").on("change", (e) => {
      YB.bom.updateProductTDSVisibility(e.target.checked);
      YB.bom.updateSafeguardChecks();
    });

    $("#has_brine_tds_sensor").on("change", (e) => {
      YB.bom.updateBrineTDSVisibility(e.target.checked);
      YB.bom.updateSafeguardChecks();
    });

    $("#motor_temperature_sensor_type").on("change", (e) => {
      YB.bom.updateMotorTemperatureVisibility(e.target.value);
      YB.bom.updateSafeguardChecks();
    });

    $("#water_temperature_sensor_type").on("change", (e) => {
      YB.bom.updateWaterTemperatureVisibility(e.target.value);
      YB.bom.updateSafeguardChecks();
    });

    $('#bomConfigForm input.form-check-input').on('change', function () { $('#bomConfigForm #' + this.id + '_form').toggle(this.checked); });

    $("#saveBrineomaticSettings").on('click', this.handleBrineomaticConfigSave);
    $("#saveHardwareSettings").on('click', this.handleHardwareConfigSave);
    $("#saveSafeguardsSettings").on('click', this.handleSafeguardsConfigSave);
  }

  Brineomatic.prototype.updateTemperatureUnits = function (units) {
    //we need this as its pulled dynamically in other places
    YB.App.config.brineomatic.temperature_units = units;

    //update static units
    let short = YB.bom.getShortTemperatureUnits(units);
    $(".temperatureUnits").html(short);

    //update temperature gauge thresholds
    if (this.motorTemperatureGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units));
      let tempMax = Math.round(YB.bom.convertTemperature(80, "C", YB.App.config.brineomatic.temperature_units));
      this.motorTemperatureGauge.internal.config.gauge_min = tempMin;
      this.motorTemperatureGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.motorTemperatureGauge.internal.config.color_threshold.values = this.gaugeSetup.motor_temperature.thresholds;
      this.motorTemperatureGauge.internal.levelColor = this.motorTemperatureGauge.internal.generateLevelColor.call(this.motorTemperatureGauge.internal);
    }

    if (this.waterTemperatureGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units));
      let tempMax = Math.round(YB.bom.convertTemperature(50, "C", YB.App.config.brineomatic.temperature_units));
      this.waterTemperatureGauge.internal.config.gauge_min = tempMin;
      this.waterTemperatureGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.waterTemperatureGauge.internal.config.color_threshold.values = this.gaugeSetup.water_temperature.thresholds;
      this.waterTemperatureGauge.internal.levelColor = this.waterTemperatureGauge.internal.generateLevelColor.call(this.waterTemperatureGauge.internal);
    }
  }

  Brineomatic.prototype.updatePressureUnits = function (units) {
    //we need this as its pulled dynamically in other places
    YB.App.config.brineomatic.pressure_units = units;

    //update static units
    let short = YB.bom.getShortPressureUnits(units);
    $(".pressureUnits").html(short);

    //update gauge thresholds
    if (this.membranePressureGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertPressure(0, "Bar", YB.App.config.brineomatic.pressure_units));
      let tempMax = Math.round(YB.bom.convertPressure(69.0, "Bar", YB.App.config.brineomatic.pressure_units));
      this.membranePressureGauge.internal.config.gauge_min = tempMin;
      this.membranePressureGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.membranePressureGauge.internal.config.color_threshold.values = this.gaugeSetup.membrane_pressure.thresholds;
      this.membranePressureGauge.internal.levelColor = this.membranePressureGauge.internal.generateLevelColor.call(this.membranePressureGauge.internal);
    }

    if (this.filterPressureGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertPressure(0, "Bar", YB.App.config.brineomatic.pressure_units));
      let tempMax = Math.round(YB.bom.convertPressure(3.45, "Bar", YB.App.config.brineomatic.pressure_units));
      this.filterPressureGauge.internal.config.gauge_min = tempMin;
      this.filterPressureGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.filterPressureGauge.internal.config.color_threshold.values = this.gaugeSetup.filter_pressure.thresholds;
      this.filterPressureGauge.internal.levelColor = this.filterPressureGauge.internal.generateLevelColor.call(this.filterPressureGauge.internal);
    }
  }

  Brineomatic.prototype.updateVolumeUnits = function (units) {
    //we need this as its pulled dynamically in other places
    YB.App.config.brineomatic.volume_units = units;

    //update static units
    let short = YB.bom.getShortVolumeUnits(units);
    $(".volumeUnits").html(short);
    $(".volumeUnitsLong").html(units);
  }

  Brineomatic.prototype.updateFlowrateUnits = function (units) {
    //we need this as its pulled dynamically in other places
    YB.App.config.brineomatic.flowrate_units = units;

    //update static units
    let short = YB.bom.getShortFlowrateUnits(units);
    $(".flowrateUnits").html(short);

    //update gauge thresholds
    if (this.productFlowrateGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertFlowrate(0, "lph", YB.App.config.brineomatic.flowrate_units));
      let tempMax = Math.round(YB.bom.convertFlowrate(250, "lph", YB.App.config.brineomatic.flowrate_units));
      this.productFlowrateGauge.internal.config.gauge_min = tempMin;
      this.productFlowrateGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.productFlowrateGauge.internal.config.color_threshold.values = this.gaugeSetup.product_flowrate.thresholds;
      this.productFlowrateGauge.internal.levelColor = this.productFlowrateGauge.internal.generateLevelColor.call(this.productFlowrateGauge.internal);
    }

    if (this.brineFlowrateGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertFlowrate(0, "lph", YB.App.config.brineomatic.flowrate_units));
      let tempMax = Math.round(YB.bom.convertFlowrate(600, "lph", YB.App.config.brineomatic.flowrate_units));
      this.brineFlowrateGauge.internal.config.gauge_min = tempMin;
      this.brineFlowrateGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.brineFlowrateGauge.internal.config.color_threshold.values = this.gaugeSetup.brine_flowrate.thresholds;
      this.brineFlowrateGauge.internal.levelColor = this.brineFlowrateGauge.internal.generateLevelColor.call(this.brineFlowrateGauge.internal);
    }

    if (this.totalFlowrateGauge) {
      //update our min/max
      let tempMin = Math.round(YB.bom.convertFlowrate(0, "lph", YB.App.config.brineomatic.flowrate_units));
      let tempMax = Math.round(YB.bom.convertFlowrate(600, "lph", YB.App.config.brineomatic.flowrate_units));
      this.totalFlowrateGauge.internal.config.gauge_min = tempMin;
      this.totalFlowrateGauge.internal.config.gauge_max = tempMax;

      //thresholds
      this.totalFlowrateGauge.internal.config.color_threshold.values = this.gaugeSetup.total_flowrate.thresholds;
      this.totalFlowrateGauge.internal.levelColor = this.totalFlowrateGauge.internal.generateLevelColor.call(this.totalFlowrateGauge.internal);
    }
  }

  Brineomatic.prototype.updateAutoflushVisibility = function (mode) {
    // Hide all autoflush fields first
    $("#autoflush_salinity").closest(".mb-3").hide();
    $("#autoflush_duration").closest(".mb-3").hide();
    $("#autoflush_volume").closest(".mb-3").hide();
    $("#autoflush_interval").closest(".mb-3").hide();
    $("#autoflush_use_high_pressure_motor").closest(".mb-3").hide();

    // Show based on mode
    switch (mode) {
      case "SALINITY":
        $("#autoflush_salinity").closest(".mb-3").show();
        $("#autoflush_interval").closest(".mb-3").show();
        $("#autoflush_use_high_pressure_motor").closest(".mb-3").show();
        break;

      case "TIME":
        $("#autoflush_duration").closest(".mb-3").show();
        $("#autoflush_interval").closest(".mb-3").show();
        $("#autoflush_use_high_pressure_motor").closest(".mb-3").show();
        break;

      case "VOLUME":
        $("#autoflush_volume").closest(".mb-3").show();
        $("#autoflush_interval").closest(".mb-3").show();
        $("#autoflush_use_high_pressure_motor").closest(".mb-3").show();
        break;

      case "NONE":
      default:
        // None → show nothing
        break;
    }
  }

  Brineomatic.prototype.updateBoostPumpVisibility = function (mode) {
    const relayDiv = $("#boost_pump_relay_id").closest(".form-floating");
    const invertedDiv = $("#boost_pump_relay_inverted").closest(".form-check");

    relayDiv.hide();
    invertedDiv.hide();

    if (mode === "RELAY") {
      relayDiv.show();
      invertedDiv.show();
    }

    $("#boostPumpControlUI").toggle(mode !== "NONE");
  }

  Brineomatic.prototype.updateHighPressurePumpVisibility = function (mode) {
    const relayDiv = $("#high_pressure_relay_id").closest(".form-floating");
    const invertedDiv = $("#high_pressure_relay_inverted").closest(".form-check");
    const modbusOptions = $(".high_pressure_modbus_options");

    relayDiv.hide();
    invertedDiv.hide();
    modbusOptions.hide();

    if (mode === "RELAY") {
      relayDiv.show();
      invertedDiv.show();
    } else if (mode === "MODBUS") {
      modbusOptions.show();
    }

    $("#runBrineomatic").toggleClass("bomIDLE", mode !== "NONE");
    $("#runBrineomatic").toggle(mode !== "NONE");
    $("#pickleBrineomatic").toggleClass("bomIDLE", mode !== "NONE");
    $("#pickleBrineomatic").toggle(mode !== "NONE");
    $("#depickleBrineomatic").toggleClass("bomPICKLED", mode !== "NONE");
    $("#depickleBrineomatic").toggle(mode !== "NONE");

    $("#highPressurePumpControlUI").toggle(mode !== "NONE");
  };

  Brineomatic.prototype.updateHighPressureValveVisibility = function (mode) {
    const pressureTargetDiv = $("#membrane_pressure_target").closest(".mb-3");
    const stepperDiv = $("#high_pressure_valve_stepper_options");

    // Hide everything first
    pressureTargetDiv.hide();
    stepperDiv.hide();

    switch (mode) {
      case "MANUAL":
        break;

      case "STEPPER":
        stepperDiv.show();
        break;

      case "NONE":
      default:
        // nothing shown
        break;
    }
  };

  Brineomatic.prototype.updateDiverterValveVisibility = function (mode) {
    const relayDiv = $("#diverter_valve_relay_id").closest(".form-floating");
    const invertedDiv = $("#diverter_valve_relay_inverted").closest(".form-check");
    const servoDiv = $("#diverter_valve_servo_id").closest(".form-floating");
    const angleDiv = $("#diverter_valve_open_angle").closest(".row");

    // Hide everything first
    relayDiv.hide();
    invertedDiv.hide();
    servoDiv.hide();
    angleDiv.hide();

    switch (mode) {
      case "RELAY":
        relayDiv.show();
        invertedDiv.show();
        break;

      case "SERVO":
        servoDiv.show();
        angleDiv.show();
        break;

      case "MANUAL":
      case "NONE":
      default:
        // nothing shown
        break;
    }

    $("#diverterValveControlUI").toggle(mode !== "NONE");
  };

  Brineomatic.prototype.generateStatsUI = function () {
    return /* html */ `
      <div id="bomStatsDiv" style="display: none">
        <h5>Brineomatic Statistics</h5>
        <table id="bomStatsTable" class="table table-hover">
            <thead>
                <tr>
                    <th scope="col">Name</th>
                    <th class="text-end" scope="col">Info</th>
                </tr>
            </thead>
            <tbody id="bomStatsTableBody" class="table-group-divider">
                <tr>
                    <th scope="row">Total Cycles</th>
                    <td class="text-end" id="bomTotalCycles"></td>
                </tr>
                <tr>
                    <th scope="row">Total Volume</th>
                    <td class="text-end" id="bomTotalVolume"></td>
                </tr>
                <tr>
                    <th scope="row">Total Runtime</th>
                    <td class="text-end" id="bomTotalRuntime"></td>
                </tr>
            </tbody>
        </table>
      </div>
    `;
  }

  Brineomatic.prototype.updateFlushValveVisibility = function (mode) {
    const relayDiv = $("#flush_valve_relay_id").closest(".form-floating");
    const invertedDiv = $("#flush_valve_relay_inverted").closest(".form-check");

    relayDiv.hide();
    invertedDiv.hide();

    if (mode === "RELAY") {
      relayDiv.show();
      invertedDiv.show();
    }

    $("#flushBrineomatic").toggleClass("bomIDLE bomPICKLED", mode !== "NONE");
    $("#flushBrineomatic").toggle(mode !== "NONE")
    $("#flushValveControlUI").toggle(mode !== "NONE");
  };

  Brineomatic.prototype.updateCoolingFanVisibility = function (mode) {
    const relayDiv = $("#cooling_fan_relay_id").closest(".form-floating");
    const tempDiv = $("#cooling_fan_on_temperature").closest(".row");
    const invertedDiv = $("#cooling_fan_relay_inverted").closest(".form-check");

    // Hide everything first
    relayDiv.hide();
    tempDiv.hide();
    invertedDiv.hide();

    switch (mode) {
      case "RELAY":
        relayDiv.show();
        tempDiv.show();
        invertedDiv.show();
        break;

      case "MANUAL":
      case "NONE":
      default:
        break;
    }

    $("#coolingFanControlUI").toggle(mode !== "NONE");
  };

  Brineomatic.prototype.updateMembranePressureVisibility = function (hasSensor) {
    $("#enable_membrane_pressure_high_check").prop("disabled", !hasSensor);
    $("#enable_membrane_pressure_low_check").prop("disabled", !hasSensor);
    $("#has_membrane_pressure_sensor_form").toggle(hasSensor);
  }

  Brineomatic.prototype.updateFilterPressureVisibility = function (hasSensor) {
    $("#enable_filter_pressure_high_check").prop("disabled", !hasSensor);
    $("#enable_filter_pressure_low_check").prop("disabled", !hasSensor);
    $("#enable_flush_filter_pressure_low_check").prop("disabled", !hasSensor);
    $("#has_filter_pressure_sensor_form").toggle(hasSensor);
  }

  Brineomatic.prototype.updateProductFlowrateVisibility = function (hasSensor) {
    $("#enable_product_flowrate_high_check").prop("disabled", !hasSensor);
    $("#enable_product_flowrate_low_check").prop("disabled", !hasSensor);
    $("#startRunVolumeDialog").toggle(hasSensor);
    $("#has_product_flow_sensor_form").toggle(hasSensor);
  }

  Brineomatic.prototype.updateBrineFlowrateVisibility = function (hasSensor) {
    $("#enable_run_total_flowrate_low_check").prop("disabled", !hasSensor);
    $("#enable_pickle_total_flowrate_low_check").prop("disabled", !hasSensor);
    $("#enable_flush_flowrate_low_check").prop("disabled", !hasSensor);
    $("#startFlushVolumeDialog").toggle(hasSensor);
    $("#has_brine_flow_sensor_form").toggle(hasSensor);
  }

  Brineomatic.prototype.updateProductTDSVisibility = function (hasSensor) {
    $("#enable_product_salinity_high_check").prop("disabled", !hasSensor);
    $("#has_product_tds_sensor_form").toggle(hasSensor);
  }

  Brineomatic.prototype.updateBrineTDSVisibility = function (hasSensor) {
    $("#startFlushAutomaticDialog").toggle(hasSensor);
    $("#has_brine_tds_sensor_form").toggle(hasSensor);
  }

  Brineomatic.prototype.updateMotorTemperatureVisibility = function (type) {
    $("#enable_motor_temperature_check").prop("disabled", type == "NONE");
  }

  Brineomatic.prototype.updateWaterTemperatureVisibility = function (type) {
  }

  Brineomatic.prototype.updateDiverterValveClosedCheckVisibility = function (has_product_flow_sensor, has_brine_flow_sensor) {
    $("#enable_diverter_valve_closed_check").prop("disabled", !(has_product_flow_sensor && has_brine_flow_sensor));
  }

  Brineomatic.prototype.updateFlushValveClosedCheckVisibility = function (has_filter_pressure_sensor, has_brine_flow_sensor) {
    $("#enable_flush_valve_off_check").prop("disabled", !(has_filter_pressure_sensor || has_brine_flow_sensor));
  }

  Brineomatic.prototype.getBrineomaticConfigFormData = function () {
    const data = {};

    data.autoflush_mode = $("#autoflush_mode").val();
    data.autoflush_salinity = parseFloat($("#autoflush_salinity").val());
    data.autoflush_duration = Math.round(parseFloat($("#autoflush_duration").val()) * 60 * 1000);
    data.autoflush_volume = YB.bom.convertVolume(parseFloat($("#autoflush_volume").val()), YB.App.config.brineomatic.volume_units, "liters");
    data.autoflush_interval = Math.round(parseFloat($("#autoflush_interval").val()) * 60 * 60 * 1000);
    data.autoflush_use_high_pressure_motor = $("#autoflush_use_high_pressure_motor").prop("checked");

    data.tank_capacity = YB.bom.convertVolume(parseFloat($("#tank_capacity").val()), YB.App.config.brineomatic.volume_units, "liters");
    data.temperature_units = $("#temperature_units").val();
    data.pressure_units = $("#pressure_units").val();
    data.volume_units = $("#volume_units").val();
    data.flowrate_units = $("#flowrate_units").val();

    data.success_melody = $("#success_melody").val();
    data.error_melody = $("#error_melody").val();

    return data;
  };

  Brineomatic.prototype.getHardwareConfigFormData = function () {
    const data = {};

    data.boost_pump_control = $("#boost_pump_control").val();
    data.boost_pump_relay_id = parseInt($("#boost_pump_relay_id").val());
    data.boost_pump_relay_inverted = $("#boost_pump_relay_inverted").prop("checked");

    data.high_pressure_pump_control = $("#high_pressure_pump_control").val();
    data.high_pressure_relay_id = parseInt($("#high_pressure_relay_id").val());
    data.high_pressure_relay_inverted = $("#high_pressure_relay_inverted").prop("checked");
    data.high_pressure_modbus_device = $("#high_pressure_modbus_device").val();
    data.high_pressure_modbus_slave_id = parseInt($("#high_pressure_modbus_slave_id").val());
    data.high_pressure_modbus_frequency = parseFloat($("#high_pressure_modbus_frequency").val());

    data.high_pressure_valve_control = $("#high_pressure_valve_control").val();
    data.high_pressure_valve_stepper_id = parseInt($("#high_pressure_valve_stepper_id").val());
    data.high_pressure_stepper_step_angle = parseFloat($("#high_pressure_stepper_step_angle").val());
    data.high_pressure_stepper_gear_ratio = parseFloat($("#high_pressure_stepper_gear_ratio").val());
    data.high_pressure_stepper_close_angle = parseFloat($("#high_pressure_stepper_close_angle").val());
    data.high_pressure_stepper_close_speed = parseFloat($("#high_pressure_stepper_close_speed").val());
    data.high_pressure_stepper_open_angle = parseFloat($("#high_pressure_stepper_open_angle").val());
    data.high_pressure_stepper_open_speed = parseFloat($("#high_pressure_stepper_open_speed").val());

    let membrane_pressure_target = parseFloat($("#membrane_pressure_target").val());
    data.membrane_pressure_target = YB.bom.convertPressure(membrane_pressure_target, YB.App.config.brineomatic.pressure_units, "Bar");

    data.diverter_valve_control = $("#diverter_valve_control").val();
    data.diverter_valve_relay_id = parseInt($("#diverter_valve_relay_id").val());
    data.diverter_valve_relay_inverted = $("#diverter_valve_relay_inverted").prop("checked");
    data.diverter_valve_servo_id = parseInt($("#diverter_valve_servo_id").val());
    data.diverter_valve_open_angle = parseFloat($("#diverter_valve_open_angle").val());
    data.diverter_valve_close_angle = parseFloat($("#diverter_valve_close_angle").val());

    data.flush_valve_control = $("#flush_valve_control").val();
    data.flush_valve_relay_id = parseInt($("#flush_valve_relay_id").val());
    data.flush_valve_relay_inverted = $("#flush_valve_relay_inverted").prop("checked");

    data.cooling_fan_control = $("#cooling_fan_control").val();
    data.cooling_fan_relay_id = parseInt($("#cooling_fan_relay_id").val());
    data.cooling_fan_relay_inverted = $("#cooling_fan_relay_inverted").prop("checked");

    let cooling_fan_on_temp = parseFloat($("#cooling_fan_on_temperature").val());
    data.cooling_fan_on_temperature = YB.bom.convertTemperature(cooling_fan_on_temp, YB.App.config.brineomatic.temperature_units, "C");

    let cooling_fan_off_temp = parseFloat($("#cooling_fan_off_temperature").val());
    data.cooling_fan_off_temperature = YB.bom.convertTemperature(cooling_fan_off_temp, YB.App.config.brineomatic.temperature_units, "C");

    data.has_membrane_pressure_sensor = $("#has_membrane_pressure_sensor").prop("checked");
    let membrane_pressure_sensor_min = parseFloat($("#membrane_pressure_sensor_min").val());
    data.membrane_pressure_sensor_min = YB.bom.convertPressure(membrane_pressure_sensor_min, YB.App.config.brineomatic.pressure_units, "Bar");
    let membrane_pressure_sensor_max = parseFloat($("#membrane_pressure_sensor_max").val());
    data.membrane_pressure_sensor_max = YB.bom.convertPressure(membrane_pressure_sensor_max, YB.App.config.brineomatic.pressure_units, "Bar");

    data.has_filter_pressure_sensor = $("#has_filter_pressure_sensor").prop("checked");
    let filter_pressure_sensor_min = parseFloat($("#filter_pressure_sensor_min").val());
    data.filter_pressure_sensor_min = YB.bom.convertPressure(filter_pressure_sensor_min, YB.App.config.brineomatic.pressure_units, "Bar");
    let filter_pressure_sensor_max = parseFloat($("#filter_pressure_sensor_max").val());
    data.filter_pressure_sensor_max = YB.bom.convertPressure(filter_pressure_sensor_max, YB.App.config.brineomatic.pressure_units, "Bar");

    data.has_product_tds_sensor = $("#has_product_tds_sensor").prop("checked");
    data.product_tds_sensor_offset = parseFloat($("#product_tds_sensor_offset").val());

    data.has_brine_tds_sensor = $("#has_brine_tds_sensor").prop("checked");
    data.brine_tds_sensor_offset = parseFloat($("#brine_tds_sensor_offset").val());

    data.has_product_flow_sensor = $("#has_product_flow_sensor").prop("checked");
    data.product_flowmeter_ppl = parseInt($("#product_flowmeter_ppl").val());

    data.has_brine_flow_sensor = $("#has_brine_flow_sensor").prop("checked");
    data.brine_flowmeter_ppl = parseInt($("#brine_flowmeter_ppl").val());

    data.motor_temperature_sensor_type = $("#motor_temperature_sensor_type").val();
    data.water_temperature_sensor_type = $("#water_temperature_sensor_type").val();

    return data;
  };

  Brineomatic.prototype.getSafeguardsConfigFormData = function () {
    const data = {};

    data.flush_timeout = $("#flush_timeout").val() * 1000;
    data.membrane_pressure_timeout = Math.round(parseFloat($("#membrane_pressure_timeout").val()) * 1000);
    data.product_flowrate_timeout = Math.round(parseFloat($("#product_flowrate_timeout").val() * 1000));
    data.product_salinity_timeout = Math.round(parseFloat($("#product_salinity_timeout").val() * 1000));
    data.production_runtime_timeout = Math.round(parseFloat($("#production_runtime_timeout").val() * 60 * 60 * 1000));

    data.enable_membrane_pressure_high_check = $("#enable_membrane_pressure_high_check").prop("checked");
    let membrane_pressure_high_threshold = parseFloat($("#membrane_pressure_high_threshold").val());
    data.membrane_pressure_high_threshold = YB.bom.convertPressure(membrane_pressure_high_threshold, YB.App.config.brineomatic.pressure_units, "Bar");
    data.membrane_pressure_high_delay = parseInt($("#membrane_pressure_high_delay").val());

    data.enable_membrane_pressure_low_check = $("#enable_membrane_pressure_low_check").prop("checked");
    let membrane_pressure_low_threshold = parseFloat($("#membrane_pressure_low_threshold").val());
    data.membrane_pressure_low_threshold = YB.bom.convertPressure(membrane_pressure_low_threshold, YB.App.config.brineomatic.pressure_units, "Bar");
    data.membrane_pressure_low_delay = parseInt($("#membrane_pressure_low_delay").val());

    data.enable_filter_pressure_high_check = $("#enable_filter_pressure_high_check").prop("checked");
    let filter_pressure_high_threshold = parseFloat($("#filter_pressure_high_threshold").val());
    data.filter_pressure_high_threshold = YB.bom.convertPressure(filter_pressure_high_threshold, YB.App.config.brineomatic.pressure_units, "Bar");
    data.filter_pressure_high_delay = parseInt($("#filter_pressure_high_delay").val());

    data.enable_filter_pressure_low_check = $("#enable_filter_pressure_low_check").prop("checked");
    let filter_pressure_low_threshold = parseFloat($("#filter_pressure_low_threshold").val());
    data.filter_pressure_low_threshold = YB.bom.convertPressure(filter_pressure_low_threshold, YB.App.config.brineomatic.pressure_units, "Bar");
    data.filter_pressure_low_delay = parseInt($("#filter_pressure_low_delay").val());

    data.enable_product_flowrate_high_check = $("#enable_product_flowrate_high_check").prop("checked");
    data.product_flowrate_high_threshold = YB.bom.convertFlowrate(parseFloat($("#product_flowrate_high_threshold").val()), YB.App.config.brineomatic.flowrate_units, "lph");
    data.product_flowrate_high_delay = parseInt($("#product_flowrate_high_delay").val());

    data.enable_product_flowrate_low_check = $("#enable_product_flowrate_low_check").prop("checked");
    data.product_flowrate_low_threshold = YB.bom.convertFlowrate(parseFloat($("#product_flowrate_low_threshold").val()), YB.App.config.brineomatic.flowrate_units, "lph");
    data.product_flowrate_low_delay = parseInt($("#product_flowrate_low_delay").val());

    data.enable_run_total_flowrate_low_check = $("#enable_run_total_flowrate_low_check").prop("checked");
    data.run_total_flowrate_low_threshold = YB.bom.convertFlowrate(parseFloat($("#run_total_flowrate_low_threshold").val()), YB.App.config.brineomatic.flowrate_units, "lph");
    data.run_total_flowrate_low_delay = parseInt($("#run_total_flowrate_low_delay").val());

    data.enable_pickle_total_flowrate_low_check = $("#enable_pickle_total_flowrate_low_check").prop("checked");
    data.pickle_total_flowrate_low_threshold = YB.bom.convertFlowrate(parseFloat($("#pickle_total_flowrate_low_threshold").val()), YB.App.config.brineomatic.flowrate_units, "lph");
    data.pickle_total_flowrate_low_delay = parseInt($("#pickle_total_flowrate_low_delay").val());

    data.enable_diverter_valve_closed_check = $("#enable_diverter_valve_closed_check").prop("checked");
    data.diverter_valve_closed_delay = parseInt($("#diverter_valve_closed_delay").val());

    data.enable_product_salinity_high_check = $("#enable_product_salinity_high_check").prop("checked");
    data.product_salinity_high_threshold = parseFloat($("#product_salinity_high_threshold").val());
    data.product_salinity_high_delay = parseInt($("#product_salinity_high_delay").val());

    data.enable_motor_temperature_check = $("#enable_motor_temperature_check").prop("checked");
    data.motor_temperature_high_delay = parseInt($("#motor_temperature_high_delay").val());

    let motor_temp_threshold = parseFloat($("#motor_temperature_high_threshold").val());
    data.motor_temperature_high_threshold = YB.bom.convertTemperature(motor_temp_threshold, YB.App.config.brineomatic.temperature_units, "C");

    data.enable_flush_flowrate_low_check = $("#enable_flush_flowrate_low_check").prop("checked");
    data.flush_flowrate_low_threshold = YB.bom.convertFlowrate(parseFloat($("#flush_flowrate_low_threshold").val()), YB.App.config.brineomatic.flowrate_units, "lph");
    data.flush_flowrate_low_delay = parseInt($("#flush_flowrate_low_delay").val());

    data.enable_flush_filter_pressure_low_check = $("#enable_flush_filter_pressure_low_check").prop("checked");
    let flush_filter_pressure_low_threshold = parseFloat($("#flush_filter_pressure_low_threshold").val());
    data.flush_filter_pressure_low_threshold = YB.bom.convertPressure(flush_filter_pressure_low_threshold, YB.App.config.brineomatic.pressure_units, "Bar");
    data.flush_filter_pressure_low_delay = parseInt($("#flush_filter_pressure_low_delay").val());

    data.enable_flush_valve_off_check = $("#enable_flush_valve_off_check").prop("checked");
    data.flush_valve_off_threshold = parseFloat($("#flush_valve_off_threshold").val());
    data.flush_valve_off_delay = parseInt($("#flush_valve_off_delay").val());

    return data;
  };

  Brineomatic.prototype.getBrineomaticConfigSchema = function () {
    return {
      autoflush_mode: {
        presence: true,
        inclusion: ["NONE", "TIME", "SALINITY", "VOLUME"]
      },

      autoflush_salinity: {
        numericality: {
          onlyInteger: true,
          greaterThan: 0
        }
      },

      autoflush_duration: {
        numericality: {
          greaterThan: 0
        }
      },

      autoflush_volume: {
        numericality: {
          greaterThan: 0
        }
      },

      autoflush_interval: {
        numericality: {
          greaterThan: 0
        }
      },

      autoflush_use_high_pressure_motor: {
        inclusion: [true, false]
      },

      tank_capacity: {
        presence: true,
        numericality: {
          greaterThan: 0
        }
      },

      temperature_units: {
        presence: true,
        inclusion: ["celsius", "fahrenheit"]
      },

      pressure_units: {
        presence: true,
        inclusion: ["kilopascal", "psi", "bar"]
      },

      volume_units: {
        presence: true,
        inclusion: ["liters", "gallons"]
      },

      flowrate_units: {
        presence: true,
        inclusion: ["lph", "gph"]
      },

      success_melody: {
        presence: true
      },

      error_melody: {
        presence: true
      }
    };
  }

  Brineomatic.prototype.getHardwareConfigSchema = function () {
    return {
      boost_pump_control: {
        presence: true,
        inclusion: ["NONE", "MANUAL", "RELAY"]
      },

      boost_pump_relay_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        },
        relayUnique: {}
      },

      boost_pump_relay_inverted: {
        inclusion: [true, false]
      },

      high_pressure_pump_control: {
        presence: true,
        inclusion: ["NONE", "MANUAL", "RELAY", "MODBUS"]
      },

      high_pressure_relay_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        },
        relayUnique: {}
      },

      high_pressure_relay_inverted: {
        inclusion: [true, false]
      },

      high_pressure_modbus_device: {
        inclusion: ["GD20"]
      },

      high_pressure_modbus_slave_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0,
          lessThanOrEqualTo: 255
        }
      },

      high_pressure_modbus_frequency: {
        numericality: {
          greaterThanOrEqualTo: 0.0,
          lessThanOrEqualTo: 400.0
        }
      },

      high_pressure_valve_control: {
        presence: true,
        inclusion: ["NONE", "MANUAL", "STEPPER"]
      },

      membrane_pressure_target: {
        numericality: {
          greaterThan: 0
        }
      },

      high_pressure_valve_stepper_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        }
      },

      high_pressure_stepper_step_angle: {
        numericality: {
          greaterThan: 0,
          lessThanOrEqualTo: 90
        }
      },

      high_pressure_stepper_gear_ratio: {
        numericality: {
          greaterThan: 0
        }
      },

      high_pressure_stepper_close_angle: {
        numericality: {
          greaterThanOrEqualTo: 0,
          lessThanOrEqualTo: 5000
        }
      },

      high_pressure_stepper_close_speed: {
        numericality: {
          greaterThan: 0,
          lessThanOrEqualTo: 200
        }
      },

      high_pressure_stepper_open_angle: {
        numericality: {
          greaterThanOrEqualTo: 0,
          lessThanOrEqualTo: 5000
        }
      },

      high_pressure_stepper_open_speed: {
        numericality: {
          greaterThan: 0,
          lessThanOrEqualTo: 200
        }
      },

      diverter_valve_control: {
        presence: true,
        inclusion: ["NONE", "MANUAL", "RELAY", "SERVO"]
      },

      diverter_valve_relay_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        }
      },

      diverter_valve_relay_inverted: {
        inclusion: [true, false]
      },

      diverter_valve_servo_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        }
      },

      diverter_valve_open_angle: {
        numericality: {
          greaterThanOrEqualTo: 0,
          lessThanOrEqualTo: 180
        }
      },

      diverter_valve_close_angle: {
        numericality: {
          greaterThanOrEqualTo: 0,
          lessThanOrEqualTo: 180
        }
      },

      flush_valve_control: {
        presence: true,
        inclusion: ["NONE", "MANUAL", "RELAY"]
      },

      flush_valve_relay_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        },
        relayUnique: {}
      },

      flush_valve_relay_inverted: {
        inclusion: [true, false]
      },

      cooling_fan_control: {
        presence: true,
        inclusion: ["NONE", "MANUAL", "RELAY"]
      },

      cooling_fan_relay_id: {
        numericality: {
          onlyInteger: true,
          greaterThanOrEqualTo: 0
        },
        relayUnique: {}
      },

      cooling_fan_relay_inverted: {
        inclusion: [true, false]
      },

      cooling_fan_on_temperature: {
        numericality: {
          get greaterThanOrEqualTo() {
            let temp = YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units);
            temp = Math.round(temp);
            return temp;
          },
          get lessThanOrEqualTo() {
            let temp = YB.bom.convertTemperature(100, "C", YB.App.config.brineomatic.temperature_units);
            temp = Math.round(temp);
            return temp;
          }
        }
      },

      cooling_fan_off_temperature: {
        numericality: {
          get greaterThanOrEqualTo() {
            let temp = YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units);
            temp = Math.round(temp);
            return temp;
          },
          get lessThanOrEqualTo() {
            let temp = YB.bom.convertTemperature(100, "C", YB.App.config.brineomatic.temperature_units);
            temp = Math.round(temp);
            return temp;
          }
        }
      },

      has_membrane_pressure_sensor: {
        inclusion: [true, false]
      },

      membrane_pressure_sensor_min: {
        numericality: {
          greaterThanOrEqualTo: 0
        }
      },

      membrane_pressure_sensor_max: {
        numericality: {
          greaterThan: 0
        }
      },

      has_filter_pressure_sensor: {
        inclusion: [true, false]
      },

      filter_pressure_sensor_min: {
        numericality: {
          greaterThanOrEqualTo: 0
        }
      },

      filter_pressure_sensor_max: {
        numericality: {
          greaterThan: 0
        }
      },

      has_product_tds_sensor: { inclusion: [true, false] },

      product_tds_sensor_offset: {
        numericality: {
          greaterThanOrEqualTo: -1000,
          lessThanOrEqualTo: 1000
        }
      },

      has_brine_tds_sensor: { inclusion: [true, false] },

      brine_tds_sensor_offset: {
        numericality: {
          greaterThanOrEqualTo: -1000,
          lessThanOrEqualTo: 1000
        }
      },

      has_product_flow_sensor: { inclusion: [true, false] },

      product_flowmeter_ppl: {
        numericality: {
          greaterThan: 0
        }
      },

      has_brine_flow_sensor: { inclusion: [true, false] },

      brine_flowmeter_ppl: {
        numericality: {
          greaterThan: 0
        }
      },

      motor_temperature_sensor_type: {
        presence: true,
        inclusion: ["NONE", "EXTERNAL", "DS18B20"]
      },

      water_temperature_sensor_type: {
        presence: true,
        inclusion: ["NONE", "EXTERNAL", "DS18B20"]
      }
    };
  }

  Brineomatic.prototype.getSafeguardsConfigSchema = function () {
    return {
      flush_timeout: {
        numericality: {
          greaterThan: 0
        }
      },

      membrane_pressure_timeout: {
        numericality: {
          greaterThan: 0
        }
      },

      product_flowrate_timeout: {
        numericality: {
          greaterThan: 0
        }
      },

      product_salinity_timeout: {
        numericality: {
          greaterThan: 0
        }
      },

      production_runtime_timeout: {
        numericality: {
          greaterThan: 0
        }
      },

      enable_membrane_pressure_high_check: { inclusion: [true, false] },
      membrane_pressure_high_threshold: { numericality: { greaterThan: 0 } },
      membrane_pressure_high_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_membrane_pressure_low_check: { inclusion: [true, false] },
      membrane_pressure_low_threshold: { numericality: { greaterThan: 0 } },
      membrane_pressure_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_filter_pressure_high_check: { inclusion: [true, false] },
      filter_pressure_high_threshold: { numericality: { greaterThan: 0 } },
      filter_pressure_high_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_filter_pressure_low_check: { inclusion: [true, false] },
      filter_pressure_low_threshold: { numericality: { greaterThan: 0 } },
      filter_pressure_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_product_flowrate_high_check: { inclusion: [true, false] },
      product_flowrate_high_threshold: { numericality: { greaterThan: 0 } },
      product_flowrate_high_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_product_flowrate_low_check: { inclusion: [true, false] },
      product_flowrate_low_threshold: { numericality: { greaterThan: 0 } },
      product_flowrate_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_run_total_flowrate_low_check: { inclusion: [true, false] },
      run_total_flowrate_low_threshold: { numericality: { greaterThan: 0 } },
      run_total_flowrate_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_pickle_total_flowrate_low_check: { inclusion: [true, false] },
      pickle_total_flowrate_low_threshold: { numericality: { greaterThan: 0 } },
      pickle_total_flowrate_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_diverter_valve_closed_check: { inclusion: [true, false] },
      diverter_valve_closed_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_product_salinity_high_check: { inclusion: [true, false] },
      product_salinity_high_threshold: { numericality: { greaterThan: 0 } },
      product_salinity_high_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_motor_temperature_check: { inclusion: [true, false] },
      motor_temperature_high_delay: { numericality: { greaterThanOrEqualTo: 0 } },
      motor_temperature_high_threshold: {
        numericality: {
          get greaterThan() {
            let temp = YB.bom.convertTemperature(0, "C", YB.App.config.brineomatic.temperature_units);
            temp = Math.round(temp);
            return temp;
          }
        }
      },

      enable_flush_flowrate_low_check: { inclusion: [true, false] },
      flush_flowrate_low_threshold: { numericality: { greaterThan: 0 } },
      flush_flowrate_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_flush_filter_pressure_low_check: { inclusion: [true, false] },
      flush_filter_pressure_low_threshold: { numericality: { greaterThan: 0 } },
      flush_filter_pressure_low_delay: { numericality: { greaterThanOrEqualTo: 0 } },

      enable_flush_valve_off_check: { inclusion: [true, false] },
      flush_valve_off_threshold: { numericality: { greaterThan: 0 } },
      flush_valve_off_delay: { numericality: { greaterThanOrEqualTo: 0 } }
    };
  }

  Brineomatic.prototype.handleBrineomaticConfigSave = function (event) {
    let data = this.getBrineomaticConfigFormData();
    let schema = this.getBrineomaticConfigSchema();
    let errors = validate(data, schema);

    YB.Util.showFormValidationResults(data, errors);

    //bail on fail.
    if (errors) {
      YB.Util.flashClass($("#brineomaticSettingsPanel"), "border-danger");
      YB.Util.flashClass($("#saveBrineomaticSettings"), "btn-danger");
      return;
    }

    //flash whole form green.
    YB.Util.flashClass($("#brineomaticSettingsPanel"), "border-success");
    YB.Util.flashClass($("#saveBrineomaticSettings"), "btn-success");

    //okay, send it off.
    data["cmd"] = "brineomatic_save_general_config";
    YB.client.send(data, true);
  };

  Brineomatic.prototype.handleHardwareConfigSave = function (e) {
    let data = this.getHardwareConfigFormData();
    let schema = this.getHardwareConfigSchema();
    let errors = validate(data, schema);

    YB.Util.showFormValidationResults(data, errors);

    //bail on fail.
    if (errors) {
      YB.Util.flashClass($("#hardwareSettingsPanel"), "border-danger");
      YB.Util.flashClass($("#saveHardwareSettings"), "btn-danger");
      return;
    }

    //disable the form - it will be re-enabled when our new config comes in.
    //this prevents multiple submission race conditons that look like settings not getting saved
    $("#hardwareSettingsPanel")
      .find("input, select, textarea, button")
      .prop("disabled", true);

    //flash whole form green.
    YB.Util.flashClass($("#hardwareSettingsPanel"), "border-success");
    YB.Util.flashClass($("#saveHardwareSettings"), "btn-success");

    //update our UI too.
    this.updateHardwareUIConfig(data);

    //okay, send it off.
    data["cmd"] = "brineomatic_save_hardware_config";
    YB.client.send(data, true);

    // reload because of the restart
    // temporary hack - real fix is to handle reconnections properly
    // eg treat it like a fresh page reload: new config, etc.
    setTimeout(YB.App.loadConfigs, 5000);
  };

  Brineomatic.prototype.handleSafeguardsConfigSave = function (e) {
    let data = this.getSafeguardsConfigFormData();
    let schema = this.getSafeguardsConfigSchema();
    let errors = validate(data, schema);

    YB.Util.showFormValidationResults(data, errors);

    //bail on fail.
    if (errors) {
      YB.Util.flashClass($("#safeguardsSettingsPanel"), "border-danger");
      YB.Util.flashClass($("#saveSafeguardsSettings"), "btn-danger");
      return;
    }

    //flash whole form green.
    YB.Util.flashClass($("#safeguardsSettingsPanel"), "border-success");
    YB.Util.flashClass($("#saveSafeguardsSettings"), "btn-success");

    //okay, send it off.
    data["cmd"] = "brineomatic_save_safeguards_config";
    YB.client.send(data, true);
  };

  Brineomatic.prototype.getShortTemperatureUnits = function (unit) {
    const lower = unit.toLowerCase();
    if (lower === 'celsius') return 'C';
    if (lower === 'fahrenheit') return 'F';
    return unit;
  }

  Brineomatic.prototype.getShortPressureUnits = function (unit) {
    const lower = unit.toLowerCase();
    if (lower === 'kilopascal') return 'Kpa';
    if (lower === 'psi') return 'PSI';
    if (lower === 'bar') return 'Bar';
    return unit;
  }

  Brineomatic.prototype.getShortVolumeUnits = function (unit) {
    const lower = unit.toLowerCase();
    if (lower === 'liters') return 'L';
    if (lower === 'gallons') return 'G';
    return unit;
  }

  Brineomatic.prototype.getShortFlowrateUnits = function (unit) {
    const lower = unit.toLowerCase();
    if (lower === 'lph' || lower === 'liters per hour') return 'LPH';
    if (lower === 'gph' || lower === 'gallons per hour') return 'GPH';
    return unit;
  }

  // units = C or F (or celsius, fahrenheit)
  Brineomatic.prototype.convertTemperature = function (value, start_units, end_units) {
    // Normalize long-form units to short-form
    const normalizeTemp = (unit) => {
      const lower = unit.toLowerCase();
      if (lower === 'celsius') return 'C';
      if (lower === 'fahrenheit') return 'F';
      return unit;
    };

    start_units = normalizeTemp(start_units);
    end_units = normalizeTemp(end_units);

    // If units are the same, no conversion needed
    if (start_units === end_units) {
      return value;
    }

    // Convert from Celsius to Fahrenheit
    if (start_units === 'C' && end_units === 'F') {
      return (value * 9 / 5) + 32;
    }

    // Convert from Fahrenheit to Celsius
    if (start_units === 'F' && end_units === 'C') {
      return (value - 32) * 5 / 9;
    }

    // Invalid units provided
    return value;
  };

  // units = PSI / Bar / Kpa (or psi, bar, kilopascal)
  Brineomatic.prototype.convertPressure = function (value, start_units, end_units) {
    // Normalize long-form units to short-form
    const normalizePressure = (unit) => {
      const lower = unit.toLowerCase();
      if (lower === 'kilopascal') return 'Kpa';
      if (lower === 'psi') return 'PSI';
      if (lower === 'bar') return 'Bar';
      return unit;
    };

    start_units = normalizePressure(start_units);
    end_units = normalizePressure(end_units);

    // If units are the same, no conversion needed
    if (start_units === end_units) {
      return value;
    }

    // Convert from PSI
    if (start_units === 'PSI') {
      if (end_units === 'Bar') {
        return value * 0.0689476;
      }
      if (end_units === 'Kpa') {
        return value * 6.89476;
      }
    }

    // Convert from Bar
    if (start_units === 'Bar') {
      if (end_units === 'PSI') {
        return value * 14.5038;
      }
      if (end_units === 'Kpa') {
        return value * 100;
      }
    }

    // Convert from Kpa
    if (start_units === 'Kpa') {
      if (end_units === 'PSI') {
        return value * 0.145038;
      }
      if (end_units === 'Bar') {
        return value * 0.01;
      }
    }

    // Invalid units provided
    return value;
  };

  //units: G (gallons) / L (liters) (or gallons, liters)
  Brineomatic.prototype.convertVolume = function (value, start_units, end_units) {
    // Normalize long-form units to short-form
    const normalizeVolume = (unit) => {
      const lower = unit.toLowerCase();
      if (lower === 'liters') return 'L';
      if (lower === 'gallons') return 'G';
      return unit;
    };

    start_units = normalizeVolume(start_units);
    end_units = normalizeVolume(end_units);

    // If units are the same, no conversion needed
    if (start_units === end_units) {
      return value;
    }

    // Convert from Gallons to Liters
    if (start_units === 'G' && end_units === 'L') {
      return value * 3.78541;
    }

    // Convert from Liters to Gallons
    if (start_units === 'L' && end_units === 'G') {
      return value * 0.264172;
    }

    // Invalid units provided
    return value;
  }

  //units: gph (gallons per hour) / lph (liters per hour)
  Brineomatic.prototype.convertFlowrate = function (value, start_units, end_units) {
    // Normalize to lowercase for consistent comparison
    start_units = start_units.toLowerCase();
    end_units = end_units.toLowerCase();

    // If units are the same, no conversion needed
    if (start_units === end_units) {
      return value;
    }

    // Convert from Gallons per hour to Liters per hour
    if (start_units === 'gph' && end_units === 'lph') {
      return value * 3.78541;
    }

    // Convert from Liters per hour to Gallons per hour
    if (start_units === 'lph' && end_units === 'gph') {
      return value * 0.264172;
    }

    // Invalid units provided
    return value;
  }

  /**
   * Formats a number for form inputs with dynamic precision.
   * @param {number} value - The number to format.
   * @param {number} totalPrecision - The target number of total digits (default 3).
   */
  Brineomatic.prototype.formatReadable = function (value, totalPrecision = 3) {
    if (value === 0) return "0";

    const absValue = Math.abs(value);

    // Count integer digits (e.g., 123.4 -> 3)
    // For numbers < 1, we treat integer digits as 1 (the leading 0)
    const integerDigits = absValue >= 1 ? Math.floor(Math.log10(absValue)) + 1 : 1;

    // Calculate decimals: total precision minus the space taken by whole numbers
    let decimalPlaces = Math.max(0, totalPrecision - integerDigits);

    // If the number is very small (0.00...), we want to ensure 
    // we still show digits based on the precision.
    if (absValue < 1 && absValue > 0) {
      const firstSignificant = Math.ceil(-Math.log10(absValue));
      decimalPlaces = Math.max(decimalPlaces, firstSignificant + (totalPrecision - 1));
    }

    return new Intl.NumberFormat('en-US', {
      useGrouping: false, // Removes commas
      minimumFractionDigits: 0,
      maximumFractionDigits: decimalPlaces,
    }).format(value);
  }

  YB.Brineomatic = Brineomatic;
  YB.bom = new Brineomatic();

  validate.validators.relayUnique = function (value, options, key, attributes) {
    const map = {
      boost_pump_relay_id: "boost_pump_control",
      flush_valve_relay_id: "flush_valve_control",
      cooling_fan_relay_id: "cooling_fan_control",
      high_pressure_relay_id: "high_pressure_pump_control",
      diverter_valve_relay_id: "diverter_valve_control",
    };

    const controlField = map[key];
    if (!controlField) return; // not a monitored field

    // Only enforce uniqueness if this control is set to RELAY
    if (attributes[controlField] !== "RELAY") {
      return;
    }

    // Let numericality/presence handle empty/invalid
    if (value === null || value === undefined || value === "") {
      return;
    }

    // Check other fields that are also RELAY
    for (const [relayKey, ctrlKey] of Object.entries(map)) {
      if (relayKey === key) continue; // skip self
      if (attributes[ctrlKey] !== "RELAY") continue;

      if (attributes[relayKey] === value) {
        // Duplicate found
        return `must be unique; also used by ${relayKey}`;
      }
    }

    // undefined = no error
  };

  // expose to global
  global.YB = YB;
})(this);