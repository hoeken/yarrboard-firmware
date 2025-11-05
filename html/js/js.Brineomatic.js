(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  // Channel registry + factory under YB
  YB.Brineomatic = {

  };

  ///////////////////////////////////////////////////////////////////////
  //////////////////////////OLD CODE/////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////

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





  // expose to global
  global.YB = YB;
})(this);