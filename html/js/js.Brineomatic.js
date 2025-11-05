(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  // Channel registry + factory under YB
  YB.Brineomatic = {

  };

  ///////////////////////////////////////////////////////////////////////
  //////////////////////////OLD CODE/////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////


  function handleConfigMessage(msg) {
    //UI for brineomatic
    $('bomInformationDiv').hide();
    $('bomControlDiv').hide();
    $('#bomStatsDiv').hide();

    if (msg.brineomatic) {
      $('#bomInformationDiv').show();
      $('#bomControlDiv').show();
      $('#bomStatsDiv').show();
      $('#brightnessUI').hide();

      if (!YB.App.isMFD()) {
        motorTemperatureGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.motor_temperature.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.motor_temperature.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        waterTemperatureGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.water_temperature.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.water_temperature.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        filterPressureGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.filter_pressure.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.filter_pressure.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        membranePressureGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.membrane_pressure.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.membrane_pressure.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        productSalinityGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.product_salinity.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.product_salinity.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        brineSalinityGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.brine_salinity.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.brine_salinity.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        productFlowrateGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.product_flowrate.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.product_flowrate.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        brineFlowrateGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.brine_flowrate.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.brine_flowrate.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        tankLevelGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.tank_level.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.tank_level.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        volumeGauge = c3.generate({
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
            pattern: brineomatic_gauge_setup.volume.colors,
            threshold: {
              unit: 'value',
              values: brineomatic_gauge_setup.volume.thresholds
            }
          },
          size: { height: 130, width: 200 },
          interaction: { enabled: false },
          transition: { duration: 0 },
          legend: { hide: true }
        });

        // Define the data
        timeData = ['x'];
        motorTemperatureData = ['Motor Temperature'];
        waterTemperatureData = ['Water Temperature'];
        filterPressureData = ['Filter Pressure'];
        membranePressureData = ['Membrane Pressure'];
        productSalinityData = ['Product Salinity'];
        productFlowrateData = ['Product Flowrate'];
        tankLevelData = ['Tank Level'];

        temperatureChart = c3.generate({
          bindto: '#temperatureChart',
          data: {
            x: 'x', // Define the x-axis data identifier
            xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
            columns: [
              timeData,
              motorTemperatureData,
              waterTemperatureData
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

        pressureChart = c3.generate({
          bindto: '#pressureChart',
          data: {
            x: 'x', // Define the x-axis data identifier
            xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
            columns: [
              timeData,
              filterPressureData,
              membranePressureData
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

        productSalinityChart = c3.generate({
          bindto: '#productSalinityChart',
          data: {
            x: 'x', // Define the x-axis data identifier
            xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
            columns: [
              timeData,
              productSalinityData
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

        productFlowrateChart = c3.generate({
          bindto: '#productFlowrateChart',
          data: {
            x: 'x', // Define the x-axis data identifier
            xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
            columns: [
              timeData,
              productFlowrateData
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

        tankLevelChart = c3.generate({
          bindto: '#tankLevelChart',
          data: {
            x: 'x', // Define the x-axis data identifier
            xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
            columns: [
              timeData,
              tankLevelData
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
    }
  }

  function handleUpdateMessage(msg) {
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
          motorTemperatureGauge.load({ columns: [['Motor Temperature', motor_temperature]] });
          waterTemperatureGauge.load({ columns: [['Water Temperature', water_temperature]] });
          filterPressureGauge.load({ columns: [['Filter Pressure', filter_pressure]] });
          membranePressureGauge.load({ columns: [['Membrane Pressure', membrane_pressure]] });
          productSalinityGauge.load({ columns: [['Product Salinity', product_salinity]] });
          brineSalinityGauge.load({ columns: [['Brine Salinity', brine_salinity]] });
          productFlowrateGauge.load({ columns: [['Product Flowrate', product_flowrate]] });
          brineFlowrateGauge.load({ columns: [['Brine Flowrate', brine_flowrate]] });
          tankLevelGauge.load({ columns: [['Tank Level', tank_level]] });
          volumeGauge.load({ columns: [['Volume', volume]] });
        }

        if (YB.App.currentPage == "graphs") {

          //only occasionally update our graph to keep it responsive
          if (Date.now() - lastChartUpdate > 1000) {
            lastChartUpdate = Date.now();

            const currentTime = new Date(); // Get current time in ISO format
            const formattedTime = d3.timeFormat('%Y-%m-%d %H:%M:%S.%L')(currentTime);

            temperatureChart.flow({
              columns: [
                ['x', formattedTime],
                ['Motor Temperature', motor_temperature],
                ['Water Temperature', water_temperature]
              ]
            });

            pressureChart.flow({
              columns: [
                ['x', formattedTime],
                ['Filter Pressure', filter_pressure],
                ['Membrane Pressure', membrane_pressure]
              ]
            });

            productSalinityChart.flow({
              columns: [
                ['x', formattedTime],
                ['Product Salinity', product_salinity]
              ]
            });

            productFlowrateChart.flow({
              columns: [
                ['x', formattedTime],
                ['Product Flowrate', product_flowrate]
              ]
            });

            tankLevelChart.flow({
              columns: [
                ['x', formattedTime],
                ['Tank Level', tank_level]
              ]
            });
          }
        }
      } else {
        $("#filterPressureData").html(filter_pressure);
        bom_set_data_color("filter_pressure", filter_pressure, $("#filterPressureData"));

        $("#membranePressureData").html(membrane_pressure);
        bom_set_data_color("membrane_pressure", membrane_pressure, $("#membranePressureData"));

        $("#productSalinityData").html(product_salinity);
        bom_set_data_color("product_salinity", product_salinity, $("#productSalinityData"));

        $("#brineSalinityData").html(brine_salinity);
        bom_set_data_color("brine_salinity", brine_salinity, $("#brineSalinityData"));

        $("#productFlowrateData").html(product_flowrate);
        bom_set_data_color("product_flowrate", product_flowrate, $("#productFlowrateData"));

        $("#brineFlowrateData").html(brine_flowrate);
        bom_set_data_color("brine_flowrate", brine_flowrate, $("#brineFlowrateData"));

        $("#motorTemperatureData").html(motor_temperature);
        bom_set_data_color("motor_temperature", motor_temperature, $("#motorTemperatureData"));

        $("#waterTemperatureData").html(water_temperature);
        bom_set_data_color("water_temperature", water_temperature, $("#waterTemperatureData"));

        $("#tankLevelData").html(tank_level);
        bom_set_data_color("tank_level", tank_level, $("#tankLevelData"));

        $("#bomVolumeData").html(volume);
        bom_set_data_color("volume", volume, $("#bomVolumeData"));
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

      //default to hide all.
      $(".bomSTARTUP").hide();
      $(".bomIDLE").hide();
      $(".bomMANUAL").hide();
      $(".bomRUNNING").hide();
      $(".bomFLUSHING").hide();
      $(".bomPICKLING").hide();
      $(".bomPICKLED").hide();
      $(".bomDEPICKLING").hide();
      $(".bomSTOPPING").hide();

      //show everything for our state
      $(`.bom${msg.status}`).show();

      if (msg.run_result)
        show_brineomatic_result("#bomRunResult", msg.run_result);

      if (msg.flush_result)
        show_brineomatic_result("#bomFlushResult", msg.flush_result);

      if (msg.pickle_result)
        show_brineomatic_result("#bomPickleResult", msg.pickle_result);

      if (msg.depickle_result)
        show_brineomatic_result("#bomDePickleResult", msg.depickle_result);

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

      $('#bomInterface').css('visibility', 'visible');
    }

  }

  function handleStatsMessage(msg) {
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

  function getBootstrapColors() {
    const colors = {};
    const styles = getComputedStyle(document.documentElement);

    // List of Bootstrap color variable names
    const colorNames = [
      '--bs-primary',
      '--bs-secondary',
      '--bs-success',
      '--bs-danger',
      '--bs-warning',
      '--bs-info',
      '--bs-light',
      '--bs-dark'
    ];

    // Loop through each variable and store its value without the `--bs-` prefix
    colorNames.forEach(color => {
      const name = color.replace('--bs-', ''); // Remove the prefix
      colors[name] = styles.getPropertyValue(color).trim();
    });

    return colors;
  }

  const bootstrapColors = getBootstrapColors();

  const brineomatic_result_text = {
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
  }

  const brineomatic_gauge_setup = {
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
  };

  function bom_set_data_color(name, value, ele) {
    // Check if the name exists in brineomatic_gauge_setup
    const setup = brineomatic_gauge_setup[name];
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

  let motorTemperatureGauge;
  let waterTemperatureGauge;
  let filterPressureGauge;
  let membranePressureGauge;
  let productSalinityGauge;
  let productFlowrateGauge;
  let brineSalinityGauge;
  let brineFlowrateGauge;
  let tankLevelGauge;

  let temperatureChart;
  let pressureChart;
  let productSalinityChart;
  let productFlowrateChart;
  let tankLevelChart;

  let lastChartUpdate = Date.now();
  let timeData;
  let motorTemperatureData;
  let waterTemperatureData;
  let filterPressureData;
  let membranePressureData;
  let productSalinityData;
  let productFlowrateData;
  let tankLevelData;

  function show_brineomatic_result(result_div, result) {
    if (result != "STARTUP") {
      if (brineomatic_result_text[result])
        $(result_div).html(brineomatic_result_text[result]);
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

  function start_brineomatic_manual() {
    YB.client.send({
      "cmd": "start_watermaker",
    }, true);
  }

  function start_brineomatic_duration() {

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

  function start_brineomatic_volume() {
    let volume = $("#bomRunVolumeInput").val();

    if (volume > 0) {
      YB.client.send({
        "cmd": "start_watermaker",
        "volume": volume
      }, true);
    }
  }

  function flush_brineomatic() {
    let duration = $("#bomFlushDurationInput").val();

    if (duration > 0) {
      let micros = duration * 60 * 1000000;

      YB.client.send({
        "cmd": "flush_watermaker",
        "duration": micros
      }, true);
    }
  }

  function pickle_brineomatic() {
    let duration = $("#bomPickleDurationInput").val();

    if (duration > 0) {
      let micros = duration * 60 * 1000000;

      YB.client.send({
        "cmd": "pickle_watermaker",
        "duration": micros
      }, true);
    }
  }

  function depickle_brineomatic() {
    let duration = $("#bomDepickleDurationInput").val();

    if (duration > 0) {
      let micros = duration * 60 * 1000000;

      YB.client.send({
        "cmd": "depickle_watermaker",
        "duration": micros
      }, true);
    }
  }

  function stop_brineomatic() {
    YB.client.send({
      "cmd": "stop_watermaker",
    }, true);
  }

  function manual_brineomatic() {
    YB.client.send({
      "cmd": "manual_watermaker",
    }, true);
  }

  function idle_brineomatic() {
    YB.client.send({
      "cmd": "idle_watermaker",
    }, true);
  }


  handleGraphDataMessage: function (msg) {
    if (YB.App.currentPage == "graphs") {
      timeData = [timeData[0]];
      // Replace the rest of timeData with incremented timestamps
      const currentTime = new Date(); // Get current time in ISO format
      const timeIncrement = 5000; // Increment in milliseconds (5 seconds)
      for (let i = msg.motor_temperature.length; i > 0; i -= 1) {
        const newTime = new Date(currentTime.getTime() - i * timeIncrement);
        const formattedNewTime = d3.timeFormat('%Y-%m-%d %H:%M:%S.%L')(newTime);
        timeData.push(formattedNewTime);
      }

      if (msg.motor_temperature || msg.water_temperature) {
        if (msg.motor_temperature)
          motorTemperatureData = [motorTemperatureData[0]].concat(msg.motor_temperature);
        if (msg.water_temperature)
          waterTemperatureData = [waterTemperatureData[0]].concat(msg.water_temperature);

        temperatureChart.load({
          columns: [
            timeData,
            motorTemperatureData,
            waterTemperatureData
          ]
        });
      }

      if (msg.filter_pressure || msg.membrane_pressure) {
        if (msg.filter_pressure)
          filterPressureData = [filterPressureData[0]].concat(msg.filter_pressure);
        if (msg.membrane_pressure)
          membranePressureData = [membranePressureData[0]].concat(msg.membrane_pressure);
        pressureChart.load({
          columns: [
            timeData,
            filterPressureData,
            membranePressureData
          ]
        });
      }

      if (msg.product_salinity) {
        productSalinityData = [productSalinityData[0]].concat(msg.product_salinity);

        productSalinityChart.load({
          columns: [
            timeData,
            productSalinityData
          ]
        });

      }

      if (msg.product_flowrate) {
        productFlowrateData = [productFlowrateData[0]].concat(msg.product_flowrate);

        productFlowrateChart.load({
          columns: [
            timeData,
            productFlowrateData
          ]
        });
      }

      if (msg.tank_level) {
        tankLevelData = [tankLevelData[0]].concat(msg.tank_level);

        tankLevelChart.load({
          columns: [
            timeData,
            tankLevelData
          ]
        });
      }

      //start getting updates too.
      YB.App.startUpdateData();

      YB.App.pageReady.graphs = true;
    }
  },


  YB.App.addMessageHandler("graph_data", YB.Brineomatic.handleGraphDataMessage);



  // expose to global
  global.YB = YB;
})(this);