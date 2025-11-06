(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};


  // Base class for all channels
  function Brineomatic() {
    this.lastChartUpdate = Date.now();

    this.resultText = {
      "STARTUP": "Starting up.",
      "SUCCESS": "Success",
      "SUCCESS_TIME": "Success: Runtime reached.",
      "SUCCESS_VOLUME": "Success: Volume reached",
      "SUCCESS_TANK_LEVEL": "Success: Tank Full",
      "USER_STOP": "Stopped by user",
      "ERR_FLUSH_VALVE_TIMEOUT": "Flush Valve Timeout",
      "ERR_FILTER_PRESSURE_TIMEOUT": "Filter Pressure Timeout",
      "ERR_FILTER_PRESSURE_LOW": "Filter Pressure Low",
      "ERR_FILTER_PRESSURE_HIGH": "Filter Pressure High",
      "ERR_MEMBRANE_PRESSURE_TIMEOUT": "Membrane Pressure Timeout",
      "ERR_MEMBRANE_PRESSURE_LOW": "Membrane Pressure Low",
      "ERR_MEMBRANE_PRESSURE_HIGH": "Membrane Pressure High",
      "ERR_FLOWRATE_TIMEOUT": "Product Flowrate Timeout",
      "ERR_FLOWRATE_LOW": "Product Flowrate Low",
      "ERR_SALINITY_TIMEOUT": "Product Salinity Timeout",
      "ERR_SALINITY_HIGH": "Product Salinity High",
      "ERR_PRODUCTION_TIMEOUT": "Production Timeout",
      "ERR_MOTOR_TEMPERATURE_HIGH": "Motor Temperature High",
    };

    this.handleConfigMessage = this.handleConfigMessage.bind(this);
    this.handleUpdateMessage = this.handleUpdateMessage.bind(this);
    this.handleStatsMessage = this.handleStatsMessage.bind(this);
    this.handleGraphDataMessage = this.handleGraphDataMessage.bind(this);

    YB.App.addMessageHandler("config", this.handleConfigMessage);
    YB.App.addMessageHandler("update", this.handleUpdateMessage);
    YB.App.addMessageHandler("stats", this.handleStatsMessage);
    YB.App.addMessageHandler("graph_data", this.handleGraphDataMessage);

    this.idle = this.idle.bind(this);
    this.startAutomatic = this.startAutomatic.bind(this);
    this.startDuration = this.startDuration.bind(this);
    this.startVolume = this.startVolume.bind(this);
    this.flush = this.flush.bind(this);
    this.pickle = this.pickle.bind(this);
    this.depickle = this.depickle.bind(this);
    this.stop = this.stop.bind(this);
    this.manual = this.manual.bind(this);
  }

  Brineomatic.prototype.buildGaugeSetup = function () {

    let bootstrapColors = YB.Util.getBootstrapColors();

    this.gaugeSetup = {
      "motor_temperature": {
        "thresholds": [60, 70, 100],
        "colors": [bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "water_temperature": {
        "thresholds": [10, 30, 40, 50],
        "colors": [bootstrapColors.primary, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "filter_pressure": {
        "thresholds": [0, 5, 10, 40, 45, 50],
        "colors": [bootstrapColors.secondary, bootstrapColors.danger, bootstrapColors.warning, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "membrane_pressure": {
        "thresholds": [0, 600, 700, 900, 1000],
        "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.primary, bootstrapColors.success, bootstrapColors.danger]
      },
      "product_salinity": {
        "thresholds": [300, 400, 1500],
        "colors": [bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "brine_salinity": {
        "thresholds": [300, 400, 1500],
        "colors": [bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "product_flowrate": {
        "thresholds": [20, 100, 180, 200, 250],
        "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
      },
      "brine_flowrate": {
        "thresholds": [20, 100, 180, 200, 250],
        "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
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

  Brineomatic.prototype.handleConfigMessage = function (msg) {
    if (msg.brineomatic) {

      //build our UI
      $("#bomInterface").remove();
      $("#controlPage").prepend(this.generateControlUI());
      $("#bomConfig").remove();
      $("#boardConfigForm").after(this.generateEditUI());
      $("#bomStatsDiv").remove();
      $("#statsContainer").prepend(this.generateStatsUI());
      $("#bomGraphs").remove();
      $("#graphsPage").prepend(this.generateGraphsUI());

      //our UI handlers
      $("#brineomaticIdle").on("click", this.idle);
      $("#brineomaticStartAutomatic").on("click", this.startAutomatic);
      $("#brineomaticStartDuration").on("click", this.startDuration);
      $("#brineomaticStartVolume").on("click", this.startVolume);
      $("#brineomaticFlush").on("click", this.flush);
      $("#brineomaticPickle").on("click", this.pickle);
      $("#brineomaticDepickle").on("click", this.depickle);
      $("#brineomaticStop").on("click", this.stop);
      $("#brineomaticManual").on("click", this.manual);

      //visibility
      $('#relayControlDiv').addClass("bomMANUAL");
      $('#servoControlDiv').addClass("bomMANUAL");
      $('#stepperControlDiv').addClass("bomMANUAL");
      $('#bomInformationDiv').show();
      $('#bomControlDiv').show();
      $('#bomStatsDiv').show();
      $('#brightnessUI').hide();

      this.buildGaugeSetup();

      if (!YB.App.isMFD()) {
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
                return `${value}°C`;
              },
              show: true
            },
            min: 0, // 0 is default, //can handle negative min e.g. vacuum / voltage / current flow / rate of change
            max: 80, // 80 is default
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
                return `${value}°C`;
              },
              show: true
            },
            min: 0, // 0 is default, //can handle negative min e.g. vacuum / voltage / current flow / rate of change
            max: 50, // 80 is default
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
                return `${value} PSI`;
              },
              show: true
            },
            min: 0,
            max: 50,
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
                return `${value} PSI`;
              },
              show: true
            },
            min: 0,
            max: 1000,
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
                return `${value} LPH`;
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
                return `${value} LPH`;
              },
              show: true
            },
            min: 0,
            max: 250,
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

        this.volumeGauge = c3.generate({
          bindto: '#volumeGauge',
          data: {
            columns: [
              ['Volume', 0]
            ],
            type: 'gauge',
          },
          gauge: {
            label: {
              format: function (value, ratio) {
                return `${value}L`;
              },
              show: true
            },
            min: 0,
            max: msg.tank_capacity,
          },
          color: {
            pattern: this.gaugeSetup.volume.colors,
            threshold: {
              unit: 'value',
              values: this.gaugeSetup.volume.thresholds
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
              label: 'Pressure (PSI))',
              min: 0,
              max: 1000
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
    };

    //finally, show our interface.
    $('#bomInterface').css('visibility', 'visible');
  }

  Brineomatic.prototype.handleUpdateMessage = function (msg) {
    if (msg.brineomatic) {

      let motor_temperature = Math.round(msg.motor_temperature);
      let water_temperature = Math.round(msg.water_temperature);
      let product_flowrate = Math.round(msg.product_flowrate);
      let brine_flowrate = Math.round(msg.brine_flowrate);
      let volume = msg.volume.toFixed(1);
      if (volume >= 100)
        volume = Math.round(volume);
      let product_salinity = Math.round(msg.product_salinity);
      let brine_salinity = Math.round(msg.brine_salinity);
      let filter_pressure = Math.round(msg.filter_pressure);
      if (filter_pressure < 0 && filter_pressure > -10)
        filter_pressure = 0;
      let membrane_pressure = Math.round(msg.membrane_pressure);
      if (membrane_pressure < 0 && membrane_pressure > -10)
        membrane_pressure = 0;
      let tank_level = Math.round(msg.tank_level * 100);

      //update our gauges.
      if (!YB.App.isMFD()) {
        if (YB.App.currentPage == "control") {
          this.motorTemperatureGauge.load({ columns: [['Motor Temperature', motor_temperature]] });
          this.waterTemperatureGauge.load({ columns: [['Water Temperature', water_temperature]] });
          this.filterPressureGauge.load({ columns: [['Filter Pressure', filter_pressure]] });
          this.membranePressureGauge.load({ columns: [['Membrane Pressure', membrane_pressure]] });
          this.productSalinityGauge.load({ columns: [['Product Salinity', product_salinity]] });
          this.brineSalinityGauge.load({ columns: [['Brine Salinity', brine_salinity]] });
          this.productFlowrateGauge.load({ columns: [['Product Flowrate', product_flowrate]] });
          this.brineFlowrateGauge.load({ columns: [['Brine Flowrate', brine_flowrate]] });
          this.tankLevelGauge.load({ columns: [['Tank Level', tank_level]] });
          this.volumeGauge.load({ columns: [['Volume', volume]] });
        }

        if (YB.App.currentPage == "graphs") {

          //only occasionally update our graph to keep it responsive
          if (Date.now() - this.lastChartUpdate > 1000) {
            this.lastChartUpdate = Date.now();

            const currentTime = new Date(); // Get current time in ISO format
            const formattedTime = d3.timeFormat('%Y-%m-%d %H:%M:%S.%L')(currentTime);

            this.temperatureChart.flow({
              columns: [
                ['x', formattedTime],
                ['Motor Temperature', motor_temperature],
                ['Water Temperature', water_temperature]
              ]
            });

            this.pressureChart.flow({
              columns: [
                ['x', formattedTime],
                ['Filter Pressure', filter_pressure],
                ['Membrane Pressure', membrane_pressure]
              ]
            });

            this.productSalinityChart.flow({
              columns: [
                ['x', formattedTime],
                ['Product Salinity', product_salinity]
              ]
            });

            this.productFlowrateChart.flow({
              columns: [
                ['x', formattedTime],
                ['Product Flowrate', product_flowrate]
              ]
            });

            this.tankLevelChart.flow({
              columns: [
                ['x', formattedTime],
                ['Tank Level', tank_level]
              ]
            });
          }
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

        $("#motorTemperatureData").html(motor_temperature);
        this.setDataColor("motor_temperature", motor_temperature, $("#motorTemperatureData"));

        $("#waterTemperatureData").html(water_temperature);
        this.setDataColor("water_temperature", water_temperature, $("#waterTemperatureData"));

        $("#tankLevelData").html(tank_level);
        this.setDataColor("tank_level", tank_level, $("#tankLevelData"));

        $("#bomVolumeData").html(volume);
        this.setDataColor("volume", volume, $("#bomVolumeData"));
      }

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

      if (msg.depickle_result)
        this.showResult("#bomDePickleResult", msg.depickle_result);

      if (msg.next_flush_countdown > 0)
        $("#bomNextFlushCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.next_flush_countdown / 1000000)));
      else
        $("#bomNextFlushCountdown").hide();

      if (msg.runtime_elapsed > 0)
        $("#bomRuntimeElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.runtime_elapsed / 1000000)));
      else
        $("#bomRuntimeElapsed").hide();

      if (msg.finish_countdown > 0)
        $("#bomFinishCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.finish_countdown / 1000000)));
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
        $("#bomFlushElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.flush_elapsed / 1000000)));
      else
        $("#bomFlushElapsed").hide();

      if (msg.flush_countdown > 0)
        $("#bomFlushCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.flush_countdown / 1000000)));
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
        $("#bomPickleElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.pickle_elapsed / 1000000)));
      else
        $("#bomPickleElapsed").hide();

      if (msg.pickle_countdown > 0)
        $("#bomPickleCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.pickle_countdown / 1000000)));
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
        $("#bomDepickleElapsedData").html(YB.Util.secondsToDhms(Math.round(msg.depickle_elapsed / 1000000)));
      else
        $("#bomDepickleElapsed").hide();

      if (msg.depickle_countdown > 0)
        $("#bomDepickleCountdownData").html(YB.Util.secondsToDhms(Math.round(msg.depickle_countdown / 1000000)));
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
        if (msg.boost_pump_on) {
          $("#bomBoostPumpStatus span").addClass("text-bg-primary");
          $('#bomBoostPumpStatus span').html("ON");
        }
        else {
          $("#bomBoostPumpStatus span").addClass("text-bg-secondary");
          $('#bomBoostPumpStatus span').html("OFF");
        }
      }
      else
        $('#bomBoostPumpStatus').hide();

      if (YB.App.config.has_high_pressure_pump) {
        $('#bomHighPressurePumpStatus span').removeClass();
        $('#bomHighPressurePumpStatus span').addClass("badge");
        if (msg.high_pressure_pump_on) {
          $("#bomHighPressurePumpStatus span").addClass("text-bg-primary");
          $('#bomHighPressurePumpStatus span').html("ON");
        }
        else {
          $("#bomHighPressurePumpStatus span").addClass("text-bg-secondary");
          $('#bomHighPressurePumpStatus span').html("OFF");
        }
      }
      else
        $('#bomHighPressurePumpStatus').hide();

      if (YB.App.config.brineomatic.has_diverter_valve) {
        $('#bomDiverterValveStatus span').removeClass();
        $('#bomDiverterValveStatus span').addClass("badge");
        if (msg.diverter_valve_open) {
          $("#bomDiverterValveStatus span").addClass("text-bg-secondary");
          $('#bomDiverterValveStatus span').html("OVERBOARD");
        }
        else {
          $("#bomDiverterValveStatus span").addClass("text-bg-primary");
          $('#bomDiverterValveStatus span').html("TO TANK");
        }
      }
      else
        $('#bomDiverterValveStatus').hide();

      if (YB.App.config.brineomatic.has_flush_valve) {
        $('#bomFlushValveStatus span').removeClass();
        $('#bomFlushValveStatus span').addClass("badge");
        if (msg.flush_valve_open) {
          $("#bomFlushValveStatus span").addClass("text-bg-primary");
          $('#bomFlushValveStatus span').html("OPEN");
        }
        else {
          $("#bomFlushValveStatus span").addClass("text-bg-secondary");
          $('#bomFlushValveStatus span').html("CLOSED");
        }
      }
      else
        $('#bomFlushValveStatus').hide();

      if (YB.App.config.brineomatic.has_cooling_fan) {
        $('#bomFanStatus span').removeClass();
        $('#bomFanStatus span').addClass("badge");
        if (msg.cooling_fan_on) {
          $("#bomFanStatus span").addClass("text-bg-primary");
          $('#bomFanStatus span').html("ON");
        }
        else {
          $('#bomFanStatus span').html("OFF");
          $("#bomFanStatus span").addClass("text-bg-secondary");
        }
      }
      else
        $('#bomFanStatus').hide();
    }
  }

  Brineomatic.prototype.handleStatsMessage = function (msg) {
    if (msg.brineomatic) {
      let totalVolume = Math.round(msg.total_volume);
      totalVolume = totalVolume.toLocaleString('en-US');
      let totalRuntime = (msg.total_runtime / (60 * 60 * 1000000)).toFixed(1);
      totalRuntime = totalRuntime.toLocaleString('en-US');
      $("#bomTotalCycles").html(msg.total_cycles.toLocaleString('en-US'));
      $("#bomTotalVolume").html(`${totalVolume}L`);
      $("#bomTotalRuntime").html(`${totalRuntime} hours`);
    }
  }

  Brineomatic.prototype.handleGraphDataMessage = function (msg) {
    if (YB.App.currentPage == "graphs") {
      this.timeData = [this.timeData[0]];
      // Replace the rest of this.timeData with incremented timestamps
      const currentTime = new Date(); // Get current time in ISO format
      const timeIncrement = 5000; // Increment in milliseconds (5 seconds)
      for (let i = msg.motor_temperature.length; i > 0; i -= 1) {
        const newTime = new Date(currentTime.getTime() - i * timeIncrement);
        const formattedNewTime = d3.timeFormat('%Y-%m-%d %H:%M:%S.%L')(newTime);
        this.timeData.push(formattedNewTime);
      }

      if (msg.motor_temperature || msg.water_temperature) {
        if (msg.motor_temperature)
          this.motorTemperatureData = [this.motorTemperatureData[0]].concat(msg.motor_temperature);
        if (msg.water_temperature)
          this.waterTemperatureData = [this.waterTemperatureData[0]].concat(msg.water_temperature);

        this.temperatureChart.load({
          columns: [
            this.timeData,
            this.motorTemperatureData,
            this.waterTemperatureData
          ]
        });
      }

      if (msg.filter_pressure || msg.membrane_pressure) {
        if (msg.filter_pressure)
          this.filterPressureData = [this.filterPressureData[0]].concat(msg.filter_pressure);
        if (msg.membrane_pressure)
          this.membranePressureData = [this.membranePressureData[0]].concat(msg.membrane_pressure);
        this.pressureChart.load({
          columns: [
            this.timeData,
            this.filterPressureData,
            this.membranePressureData
          ]
        });
      }

      if (msg.product_salinity) {
        this.productSalinityData = [this.productSalinityData[0]].concat(msg.product_salinity);

        this.productSalinityChart.load({
          columns: [
            this.timeData,
            this.productSalinityData
          ]
        });

      }

      if (msg.product_flowrate) {
        this.productFlowrateData = [this.productFlowrateData[0]].concat(msg.product_flowrate);

        this.productFlowrateChart.load({
          columns: [
            this.timeData,
            this.productFlowrateData
          ]
        });
      }

      if (msg.tank_level) {
        this.tankLevelData = [this.tankLevelData[0]].concat(msg.tank_level);

        this.tankLevelChart.load({
          columns: [
            this.timeData,
            this.tankLevelData
          ]
        });
      }

      //start getting updates too.
      YB.App.startUpdateData();

      YB.App.pageReady.graphs = true;
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
      let micros = duration * 60 * 60 * 1000000;

      YB.client.send({
        "cmd": "start_watermaker",
        "duration": micros
      }, true);
    }
  }

  Brineomatic.prototype.startVolume = function (e) {
    $(e.currentTarget).blur();
    let volume = $("#bomRunVolumeInput").val();

    if (volume > 0) {
      YB.client.send({
        "cmd": "start_watermaker",
        "volume": volume
      }, true);
    }
  }

  Brineomatic.prototype.flush = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomFlushDurationInput").val();

    if (duration > 0) {
      let micros = duration * 60 * 1000000;

      YB.client.send({
        "cmd": "flush_watermaker",
        "duration": micros
      }, true);
    }
  }

  Brineomatic.prototype.pickle = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomPickleDurationInput").val();

    if (duration > 0) {
      let micros = duration * 60 * 1000000;

      YB.client.send({
        "cmd": "pickle_watermaker",
        "duration": micros
      }, true);
    }
  }

  Brineomatic.prototype.depickle = function (e) {
    $(e.currentTarget).blur();
    let duration = $("#bomDepickleDurationInput").val();

    if (duration > 0) {
      let micros = duration * 60 * 1000000;

      YB.client.send({
        "cmd": "depickle_watermaker",
        "duration": micros
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
    YB.client.send({
      "cmd": "idle_watermaker",
    }, true);
  }

  Brineomatic.prototype.generateControlUI = function () {
    return `
      <div id="bomInterface" class="row mx-0" style="visibility:hidden">
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
                      <!-- <tr id="bomVolume" class="bomIDLE bomMANUAL bomRUNNING bomFLUSHING">
                          <th>Volume Produced</th>
                          <td id="bomVolumeData"></td>
                      </tr> -->
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
                                      <div class="col-md-4">
                                          <h5>Automatic</h5>
                                          <div class="small" style="height: 110px">
                                              Run until full.
                                          </div>
                                          <button id="brineomaticStartAutomatic" type="button"
                                              class="btn btn-success my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Start</button>
                                      </div>
                                      <div class="col-md-4">
                                          <h5>Duration</h5>
                                          <div class="small" style="height: 70px">
                                              Run for the time below.
                                          </div>
                                          <div style="height: 40px">
                                              <div class="input-group">
                                                  <input type="text" class="form-control"
                                                      id="bomRunDurationInput" value="3.5">
                                                  <span class="input-group-text">hours</span>
                                              </div>
                                          </div>
                                          <button id="brineomaticStartDuration" type="button"
                                              class="btn btn-success my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Start</button>
                                      </div>
                                      <div class="col-md-4">
                                          <h5>Volume</h5>
                                          <div class="small" style="height: 70px">
                                              Make the amount of water below.
                                          </div>
                                          <div style="height: 40px">
                                              <div class="input-group">
                                                  <input type="text" class="form-control"
                                                      id="bomRunVolumeInput" value="250">
                                                  <span class="input-group-text">liters</span>
                                              </div>
                                          </div>
                                          <button id="brineomaticStartVolume" type="button"
                                              class="btn btn-success my-3" data-bs-dismiss="modal"
                                              style="width: 100%">Start</button>
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
                              <p>Flush the watermaker for the time below.</p>
                              <div class="row">
                                  <div class="col-5">
                                      <div class="input-group">
                                          <input type="text" class="form-control"
                                              id="bomFlushDurationInput" value="5">
                                          <span class="input-group-text">minutes</span>
                                      </div>
                                  </div>
                              </div>
                          </div>
                          <div class="modal-footer">
                              <button type="button" class="btn btn-secondary"
                                  data-bs-dismiss="modal">Cancel</button>
                              <button id="brineomaticFlush" type="button" class="btn btn-primary"
                                  data-bs-dismiss="modal">Flush</button>
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
                                      <div class="input-group">
                                          <input type="text" class="form-control"
                                              id="bomPickleDurationInput" value="5">
                                          <span class="input-group-text">minutes</span>
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
                                      <div class="input-group">
                                          <input type="text" class="form-control"
                                              id="bomDepickleDurationInput" value="15">
                                          <span class="input-group-text">minutes</span>
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
              <div class=" col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Filter Pressure</h6>
                  <div id="filterPressureGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Membrane Pressure</h6>
                  <div id="membranePressureGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class=" col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Product Salinity</h6>
                  <div id="productSalinityGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class=" col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Brine Salinity</h6>
                  <div id="brineSalinityGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Product Flowrate</h6>
                  <div id="productFlowrateGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Brine Flowrate</h6>
                  <div id="brineFlowrateGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Motor Temperature</h6>
                  <div id="motorTemperatureGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Water Temperature</h6>
                  <div id="waterTemperatureGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0 text-center">Tank Level</h6>
                  <div id="tankLevelGauge" class="d-flex justify-content-center"></div>
              </div>
              <div class="col-md-3 col-sm-4 col-6 text-center">
                  <h6 class="my-0" class="my-0">Volume Produced</h6>
                  <div id="volumeGauge" class="d-flex justify-content-center"></div>
              </div>
          </div>
          <div id="bomGaugesMFD" class="mfdShow row gx-0 gy-3 my-3 text-center">
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Filter Pressure</h6>
                  <h1 id="filterPressureData" class="my-0"></h1>
                  <h5 id="filterPressureUnits" class="text-body-tertiary">PSI</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Membrane Pressure</h6>
                  <h1 id="membranePressureData" class="my-0"></h1>
                  <h5 id="membranePressureUnits" class="text-body-tertiary">PSI</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Product Salinity</h6>
                  <h1 id="productSalinityData" class="my-0"></h1>
                  <h5 id="productSalinityUnits" class="text-body-tertiary">PPM</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Brine Salinity</h6>
                  <h1 id="brineSalinityData" class="my-0"></h1>
                  <h5 id="brineSalinityUnits" class="text-body-tertiary">PPM</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Product Flowrate</h6>
                  <h1 id="productFlowrateData" class="my-0"></h1>
                  <h5 id="productFlowrateUnits" class="text-body-tertiary">LPH</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Brine Flowrate</h6>
                  <h1 id="brineFlowrateData" class="my-0"></h1>
                  <h5 id="brineFlowrateUnits" class="text-body-tertiary">LPH</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0">Motor Temperature</h6>
                  <h1 id="motorTemperatureData" class="my-0"></h1>
                  <h5 id="motorTemperatureUnits" class="text-body-tertiary">°C</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0" class="my-0">Water Temperature</h6>
                  <h1 id="waterTemperatureData"></h1>
                  <h5 id="waterTemperatureUnits" class="text-body-tertiary">°C</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0" class="my-0">Tank Level</h6>
                  <h1 id="tankLevelData"></h1>
                  <h5 id="tankLevelUnits" class="text-body-tertiary">%</h5>
              </div>
              <div class="col-md-3 col-sm-4 col-6">
                  <h6 class="my-0" class="my-0">Volume Produced</h6>
                  <h1 id="bomVolumeData"></h1>
                  <h5 id="tankLevelUnits" class="text-body-tertiary">liters</h5>
              </div>
          </div>
      </div>
    `;
  }

  Brineomatic.prototype.generateEditUI = function () {
    return `
      <div id="bomConfig" style="display:none" class="col-md-12">
        <h4>Brineomatic Configuration</h4>
        <div id="bomConfigForm" class="row g-3"></div>
      </div>
    `;
  }

  Brineomatic.prototype.generateStatsUI = function () {
    return `
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

  Brineomatic.prototype.generateGraphsUI = function () {
    return `
      <div id="bomGraphs" class="col-md-12 mfdHide">
        <h3>Graphs</h3>
        <!-- Nav tabs -->
        <ul class="nav nav-tabs" id="bomGraphsTabs" role="tablist">
            <li class="nav-item" role="presentation">
                <button class="nav-link active" id="bomTemperatureGraphTab" data-bs-toggle="tab"
                    data-bs-target="#bomTemperatureGraphPanel" type="button" role="tab"
                    aria-controls="bomTemperatureGraphPanel" aria-selected="true">Temperature</button>
            </li>
            <li class="nav-item" role="presentation">
                <button class="nav-link" id="bomPressureGraphTab" data-bs-toggle="tab"
                    data-bs-target="#bomPressureGraphPanel" type="button" role="tab"
                    aria-controls="bomPressureGraphPanel" aria-selected="false">Pressure</button>
            </li>
            <li class="nav-item" role="presentation">
                <button class="nav-link" id="bomProductSalinityGraphTab" data-bs-toggle="tab"
                    data-bs-target="#bomProductSalinityGraphPanel" type="button" role="tab"
                    aria-controls="bomProductSalinityGraphPanel" aria-selected="false">Product
                    Salinity</button>
            </li>
            <li class="nav-item" role="presentation">
                <button class="nav-link" id="bomProductFlowrateGraphTab" data-bs-toggle="tab"
                    data-bs-target="#bomProductFlowrateGraphPanel" type="button" role="tab"
                    aria-controls="bomProductFlowrateGraphPanel" aria-selected="false">Product
                    Flowrate</button>
            </li>
            <li class="nav-item" role="presentation">
                <button class="nav-link" id="bomTankLevelGraphTab" data-bs-toggle="tab"
                    data-bs-target="#bomTankLevelGraphPanel" type="button" role="tab"
                    aria-controls="bomTankLevelGraphPanel" aria-selected="false">Tank
                    Level</button>
            </li>
        </ul>

        <!-- Tab panes -->
        <div class="tab-content">
            <div class="tab-pane fade show active" id="bomTemperatureGraphPanel" role="tabpanel"
                aria-labelledby="bomTemperatureGraphPanel">
                <h4>Temperatures</h4>
                <div id="temperatureChart" class="row"></div>
            </div>
            <div class="tab-pane fade" id="bomPressureGraphPanel" role="tabpanel"
                aria-labelledby="bomPressureGraphPanel">
                <h4>Pressures</h4>
                <div id="pressureChart" class="row"></div>
            </div>
            <div class="tab-pane fade" id="bomProductSalinityGraphPanel" role="tabpanel"
                aria-labelledby="bomProductSalinityGraphPanel">
                <h4>Product Salinity</h4>
                <div id="productSalinityChart" class="row"></div>
            </div>
            <div class="tab-pane fade" id="bomProductFlowrateGraphPanel" role="tabpanel"
                aria-labelledby="bomProductFlowrateGraphPanel">
                <h4>Product Flowrate</h4>
                <div id="productFlowrateChart" class="row"></div>
            </div>
            <div class="tab-pane fade" id="bomTankLevelGraphPanel" role="tabpanel"
                aria-labelledby="bomTankLevelGraphPanel">
                <h4>Tank Level</h4>
                <div id="tankLevelChart" class="row"></div>
            </div>
        </div>
      </div>
    `;
  }

  YB.Brineomatic = Brineomatic;
  YB.bom = new Brineomatic();

  // expose to global
  global.YB = YB;
})(this);