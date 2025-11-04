(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  const YarrboardClient = window.YarrboardClient;

  let current_page = null;
  let last_update;
  let app_username;
  let app_password;
  let app_role = "nobody";
  let default_app_role = "nobody";
  let app_update_interval = 500;
  let network_config;
  let app_config;
  let app_update_interval_id = null;

  let ota_started = false;

  let page_list = ["control", "config", "stats", "graphs", "network", "settings", "system"];
  let page_ready = {
    "control": false,
    "config": false,
    "stats": false,
    "graphs": false,
    "network": false,
    "settings": false,
    "system": true,
    "login": true
  };

  let page_permissions = {
    "nobody": [
      "login"
    ],
    "admin": [
      "control",
      "config",
      "stats",
      "graphs",
      "network",
      "settings",
      "system",
      "login",
      "logout"
    ],
    "guest": [
      "control",
      "stats",
      "graphs",
      "login",
      "logout"
    ]
  };

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

  const adc_types = {
    "raw": "Raw Output",
    "digital_switch": "Digital Switching",
    "thermistor": "Thermistor",
    "4-20ma": "4-20mA Sensor",
    "high_volt_divider": "0-32v Input",
    "low_volt_divider": "0-5v Input",
    "ten_k_pullup": "10k Pullup"
  };

  let adc_running_averages = {};

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

  const BoardNameEdit = (name) => `
<div class="col-12">
  <h4>Board Name</h4>
  <input type="text" class="form-control" id="fBoardName" value="${name}">
  <div class="valid-feedback">Saved!</div>
  <div class="invalid-feedback">Must be 30 characters or less.</div>
</div>
`;

  const RelayControlCard = (ch) => `
<div id="relay${ch.id}" class="col-xs-12 col-sm-6">
  <table class="w-100 h-100 p-2">
    <tr>
      <td>
        <button id="relayState${ch.id}" type="button" class="btn relayButton text-center" onclick="toggle_relay_state(${ch.id})">
          <table style="width: 100%">
            <tbody>
              <tr>
                <td class="relayIcon text-center align-middle pe-2">
                  ${pwm_type_images[ch.type]}
                </td>
                <td class="text-center" style="width: 99%">
                  <div id="relayName${ch.id}">${ch.name}</div>
                  <div id="relayStatus${ch.id}"></div>
                </td>
              </tr>
            </tbody>
          </table>
        </button>
      </td>
    </tr>
  </table>
</div>
`;

  const RelayEditCard = (ch) => `
<div id="relayEditCard${ch.id}" class="col-xs-12 col-sm-6">
  <div class="p-3 border border-secondary rounded">
    <h5>Relay Channel #${ch.id}</h5>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fRelayName${ch.id}" value="${ch.name}">
      <label for="fRelayName${ch.id}">Name</label>
    </div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
    <div class="form-check form-switch mb-3">
      <input class="form-check-input" type="checkbox" id="fRelayEnabled${ch.id}">
      <label class="form-check-label" for="fRelayEnabled${ch.id}">
        Enabled
      </label>
    </div>
    <div class="form-floating mb-3">
      <select id="fRelayDefaultState${ch.id}" class="form-select" aria-label="Default State (on boot)">
        <option value="ON">ON</option>
        <option value="OFF">OFF</option>
      </select>
      <label for="fRelayDefaultState${ch.id}">Default State (on boot)</label>
    </div>
    <div class="form-floating">
      <select id="fRelayType${ch.id}" class="form-select" aria-label="Output Type">
        <option value="light">Light</option>
        <option value="motor">Motor</option>
        <option value="water_pump">Water Pump</option>
        <option value="bilge_pump">Bilge Pump</option>
        <option value="fuel_pump">Fuel Pump</option>
        <option value="fan">Fan</option>
        <option value="solenoid">Solenoid</option>
        <option value="fridge">Refrigerator</option>
        <option value="freezer">Freezer</option>
        <option value="charger">Charger</option>
        <option value="electronics">Electronics</option>
        <option value="other">Other</option>
      </select>
      <label for="fRelayType${ch.id}">Output Type</label>
    </div>
  </div>
</div>
`;

  const ServoControlCard = (ch) => `
<div id="servo${ch.id}" class="col-xs-12 col-sm-6">
  <table class="w-100 h-100 p-2">
    <tr>
      <td width="75%" id="servoName${ch.id}">${ch.name}</td>
      <td id="servoAngle${ch.id}"></td>
    </tr>
    <tr>
      <td colspan="2">
        <input type="range" class="form-range" min="0" max="180" step="1" id="servoSlider${ch.id}" list="servoMarker${ch.id}">
        <datalist id="servoMarker${ch.id}">
          <option value="0"></option>
          <option value="30"></option>
          <option value="60"></option>
          <option value="90"></option>
          <option value="120"></option>
          <option value="150"></option>
          <option value="180"></option>
        </datalist>
      </td>
    </tr>
  </table>
</div>
`;

  const ServoEditCard = (ch) => `
<div id="servoEditCard${ch.id}" class="col-xs-12 col-sm-6">
  <div class="p-3 border border-secondary rounded">
    <h5>Servo Channel #${ch.id}</h5>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fServoName${ch.id}" value="${ch.name}">
      <label for="fServoName${ch.id}">Name</label>
    </div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
    <div class="form-check form-switch mb-3">
      <input class="form-check-input" type="checkbox" id="fServoEnabled${ch.id}">
      <label class="form-check-label" for="fServoEnabled${ch.id}">
        Enabled
      </label>
    </div>
  </div>
</div>
`;

  const ADCControlRow = (id, name, type) => `
<tr id="adc${id}" class="adcRow">
  <td class="adcId align-middle">${id}</td>
  <td class="adcName align-middle">${name}</td>
  <td class="adcType align-middle">${adc_types[type]}</td>
  <td class="adcValue" id="adcValue${id}"></td>
  <!-- <td class="adcBar align-middle">
     <div id="adcBar${id}" class="progress" role="progressbar" aria-label="ADC ${id} Reading" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100">
      <div class="progress-bar" style="width: 0%"></div>
    </div>
  </td> -->
</tr>
`;

  const ADCCalibrationTableRow = (ch, output, calibrated, index) => {
    return `
      <tr id="fADCCalibrationTableRow${ch.id}_${index}">
        <td>
          <div class="input-group input-group-sm">
            <input type="text" class="form-control" id="fADCCalibrationTableOutput${ch.id}_${index}" value="${output}" disabled>
            <span class="input-group-text ADCUnits${ch.id}">${ch.units}</span>
          </div>
        </td>
        <td>
          <div class="input-group input-group-sm">
            <input type="text" class="form-control" id="fADCCalibrationTableCalibrated${ch.id}_${index}" value="${calibrated}" disabled>
            <span class="input-group-text ADCCalibratedUnits${ch.id}">${ch.calibratedUnits}</span>
          </div>
        </td>
        <td>
          <button type="button" class="btn btn-sm btn-outline-danger" id="fADCCalibrationTableRemove${ch.id}_${index}">
            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-trash" viewBox="0 0 16 16">
  <path d="M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0z"></path>
  <path d="M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1zM4.118 4 4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4zM2.5 3h11V2h-11z"></path>
  </svg>
          </button>
        </td>
      </tr>
    `;
  };

  const ADCEditCard = (ch) => {
    // Build options from adc_types
    const options = Object.entries(adc_types)
      .map(([key, label]) => `<option value="${key}">${label}</option>`)
      .join("\n");

    let calibrationTableHtml = "";
    if (YB.App.config.adc[ch.id - 1].hasOwnProperty("calibrationTable")) {
      calibrationTableHtml = YB.App.config.adc[ch.id - 1].calibrationTable
        .map(([output, calibrated], index) => ADCCalibrationTableRow(ch, output, calibrated, index))
        .join("\n");
    }

    return `
    <div id="adcEditCard${ch.id}" class="col-xs-12 col-sm-6">
      <div class="p-3 border border-secondary rounded">
        <h5>ADC #${ch.id}</h5>
        <div class="form-floating mb-3">
          <input type="text" class="form-control" id="fADCName${ch.id}" value="${ch.name}">
          <label for="fADCName${ch.id}">Name</label>
          <div class="invalid-feedback">Must be 30 characters or less.</div>
        </div>
        <div class="form-check form-switch mb-3">
          <input class="form-check-input" type="checkbox" id="fADCEnabled${ch.id}">
          <label class="form-check-label" for="fADCEnabled${ch.id}">Enabled</label>
        </div>
        <div class="form-floating mb-3">
          <select id="fADCType${ch.id}" class="form-select" aria-label="Input Type">
            ${options}
          </select>
          <label for="fADCType${ch.id}">Input Type</label>
        </div>
        <div class="form-floating mb-3">
          <select id="fADCDisplayDecimals${ch.id}" class="form-select" aria-label="Display Format">
            <option value="0">123,456</option>
            <option value="1">123,456.1</option>
            <option value="2">123,456.12</option>
            <option value="3">123,456.123</option>
            <option value="4">123,456.1234</option>
          </select>
          <label for="fADCDisplayDecimals${ch.id}">Display Format</label>
        </div>
        <div class="form-check form-switch">
          <input class="form-check-input" type="checkbox" id="fADCUseCalibrationTable${ch.id}">
          <label class="form-check-label" for="fADCUseCalibrationTable${ch.id}">Use Calibration Table</label>
        </div>
        <div id="fADCCalibrationTableUI${ch.id}" class="form-floating mt-3" style="display: none">
          <h5>Calibration Setup</h5>
          <div class="form-floating mb-3">
            <input type="text" class="form-control" id="fADCCalibratedUnits${ch.id}" value="${ch.calibratedUnits}">
            <label for="fADCCalibratedUnits${ch.id}">Calibrated Units</label>
            <div class="invalid-feedback">Must be 10 characters or less.</div>
          </div>
          <div class="form-floating mb-3">
            <h6>Live Averaged Output <span id="fADCAverageOutputCount${ch.id}" class="small"></span></h6>
            <div class="input-group">
              <input id="fADCAverageOutput${ch.id}" type="text" class="form-control" value="0">
              <span class="input-group-text ADCUnits${ch.id}">${ch.units}</span>
              <button id="fADCAverageOutputCopy${ch.id}" type="button" class="btn btn-sm btn-primary">
                Copy
              </button>
              <button id="fADCAverageOutputReset${ch.id}" type="button" class="btn btn-sm btn-secondary">
                Reset
              </button>
            </div>
          </div>
          <table class="table table-sm">
            <thead>
              <tr>
                <th>Output</th>
                <th>Calibrated</th>
                <th class="text-center"><svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-gear" viewBox="0 0 16 16">
  <path d="M8 4.754a3.246 3.246 0 1 0 0 6.492 3.246 3.246 0 0 0 0-6.492M5.754 8a2.246 2.246 0 1 1 4.492 0 2.246 2.246 0 0 1-4.492 0"/>
  <path d="M9.796 1.343c-.527-1.79-3.065-1.79-3.592 0l-.094.319a.873.873 0 0 1-1.255.52l-.292-.16c-1.64-.892-3.433.902-2.54 2.541l.159.292a.873.873 0 0 1-.52 1.255l-.319.094c-1.79.527-1.79 3.065 0 3.592l.319.094a.873.873 0 0 1 .52 1.255l-.16.292c-.892 1.64.901 3.434 2.541 2.54l.292-.159a.873.873 0 0 1 1.255.52l.094.319c.527 1.79 3.065 1.79 3.592 0l.094-.319a.873.873 0 0 1 1.255-.52l.292.16c1.64.893 3.434-.902 2.54-2.541l-.159-.292a.873.873 0 0 1 .52-1.255l.319-.094c1.79-.527 1.79-3.065 0-3.592l-.319-.094a.873.873 0 0 1-.52-1.255l.16-.292c.893-1.64-.902-3.433-2.541-2.54l-.292.159a.873.873 0 0 1-1.255-.52zm-2.633.283c.246-.835 1.428-.835 1.674 0l.094.319a1.873 1.873 0 0 0 2.693 1.115l.291-.16c.764-.415 1.6.42 1.184 1.185l-.159.292a1.873 1.873 0 0 0 1.116 2.692l.318.094c.835.246.835 1.428 0 1.674l-.319.094a1.873 1.873 0 0 0-1.115 2.693l.16.291c.415.764-.42 1.6-1.185 1.184l-.291-.159a1.873 1.873 0 0 0-2.693 1.116l-.094.318c-.246.835-1.428.835-1.674 0l-.094-.319a1.873 1.873 0 0 0-2.692-1.115l-.292.16c-.764.415-1.6-.42-1.184-1.185l.159-.291A1.873 1.873 0 0 0 1.945 8.93l-.319-.094c-.835-.246-.835-1.428 0-1.674l.319-.094A1.873 1.873 0 0 0 3.06 4.377l-.16-.292c-.415-.764.42-1.6 1.185-1.184l.292.159a1.873 1.873 0 0 0 2.692-1.115z"/>
</svg></th>
              </tr>
            </thead>
            <tbody id="ADCCalibrationTableBody${ch.id}">
              <tr>
                <td>
                  <div class="input-group input-group-sm">
                    <input type="text" class="form-control" id="fADCCalibrationTableOutput${ch.id}" value="">
                    <span class="input-group-text ADCUnits${ch.id}">${ch.units}</span>
                  </div>
                </td>
                <td>
                  <div class="input-group input-group-sm">
                    <input type="text" class="form-control" id="fADCCalibrationTableCalibrated${ch.id}" value="">
                    <span class="input-group-text ADCCalibratedUnits${ch.id}">${ch.calibratedUnits}</span>
                  </div>
                </td>
                <td>
                  <button id="fADCCalibrationTableAdd${ch.id}" type="button" class="btn btn-sm btn-outline-primary">
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-plus" viewBox="0 0 16 16">
  <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4"/>
</svg>
                  </button>
                </td>
              </tr>
              ${calibrationTableHtml}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  `;
  };

  const AlertBox = (message, type) => `
<div>
  <div class="mt-3 alert alert-${type} alert-dismissible" role="alert">
    <div>${message}</div>
    <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
  </div>
</div>`;

  let currentServoSliderID = -1;
  let currentlyPickingBrightness = false;

  function console_send_json(e) {
    e.preventDefault(); // stop real submission

    let text = $("#json_console_payload").val().trim();

    try {
      // Try to parse the JSON
      let obj = JSON.parse(text);

      // Clear the input if successful
      $("#json_console_payload").val("");

      YB.log("Sent: " + JSON.stringify(obj));

      // Send the parsed object (convert back to string if YB.client.send needs text)
      YB.client.send(obj);
    } catch (err) {
      YB.log("Invalid JSON: " + err);
    }
  }

  function setup_debug_terminal() {
    $("#json_console_form").on("submit", console_send_json);
    $("#json_console_send").on("click", console_send_json);

    $("#json_payload").on("keydown", function (e) {
      var key = e.key || e.keyCode || e.which;
      if (key === "Enter" || key === 13) {
        e.preventDefault();
        console_send_json();
      }
    });
  }

  function load_configs() {
    if (app_role == "nobody")
      return;

    if (app_role == "admin") {
      YB.client.getNetworkConfig();
      YB.client.getAppConfig();

      YB.client.send({
        "cmd": "get_full_config"
      });

      check_for_updates();
    }

    YB.client.getConfig();
  }

  function check_connection_status() {
    if (YB.client) {
      let status = YB.client.status();

      //our connection status
      $(".connection_status").hide();

      if (status == "CONNECTING")
        $("#connection_startup").show();
      else if (status == "CONNECTED")
        $("#connection_good").show();
      else if (status == "RETRYING")
        $("#connection_retrying").show();
      else if (status == "FAILED")
        $("#connection_failed").show();
    }
  }

  function start_websocket() {
    //close any old connections
    // if (socket)
    //   socket.close();

    //do we want ssl?
    let use_ssl = false;
    if (document.location.protocol == 'https:')
      use_ssl = true;

    //open it.
    YB.client = new YarrboardClient(window.location.host, "", "", false, use_ssl);

    YB.client.onopen = function () {
      //figure out what our login situation is
      YB.client.sayHello();
    };

    YB.client.onmessage = function (msg) {
      if (msg.debug)
        YB.log(`SERVER: ${msg.debug}`);
      else if (msg.msg == 'hello') {
        app_role = msg.role;
        default_app_role = msg.default_role;

        //light/dark mode
        //let the mfd override with ?mode=night, etc.
        if (!getQueryVariable("mode")) {
          if (msg.theme)
            setTheme(msg.theme);
        }

        //auto login?
        if (Cookies.get("username") && Cookies.get("password")) {
          YB.client.login(Cookies.get("username"), Cookies.get("password"));
        } else {
          update_role_ui();
          load_configs();
          open_default_page();
        }
      }
      else if (msg.msg == 'config') {
        // YB.log("config");
        // YB.log(msg);

        YB.App.config = msg;

        //is it our first boot?
        if (msg.first_boot && current_page != "network")
          show_alert(`Welcome to Yarrboard, head over to <a href="#network" onclick="YB.App.openPage('network')">Network</a> to setup your WiFi.`, "primary");

        //did we get a crash?
        if (msg.has_coredump)
          show_admin_alert(`
          <p>Oops, looks like Yarrboard crashed.</p>
          <p>Please download the <a href="/coredump.txt" target="_blank">coredump</a> and report it to our <a href="https://github.com/hoeken/yarrboard/issues">Github Issue Tracker</a> along with the following information:</p>
          <ul><li>Firmware: ${msg.firmware_version}</li><li>Hardware: ${msg.hardware_version}</li></ul>
        `, "danger");

        //send our boot log
        if (msg.boot_log) {
          var lines = msg.boot_log.split("\n");
          YB.log("Boot Log:");
          for (var line of lines)
            YB.log(line);
        }

        //let the people choose their own names!
        $('#boardName').html(msg.name);
        document.title = msg.name;

        //update our footer automatically.
        $('#projectName').html("Yarrboard v" + msg.firmware_version);

        //all our versions
        $("#firmware_version").html(`v${msg.firmware_version}`);

        //deal with our hash
        let clean_hash = msg.git_hash;
        let is_dirty = false;
        if (clean_hash.endsWith("-dirty")) {
          is_dirty = true;
          clean_hash = clean_hash.slice(0, -6); // remove the "-dirty" suffix
        }
        let short_hash = clean_hash.substring(0, 7); // for display
        if (is_dirty)
          short_hash += "-dirty";
        $("#git_hash").html(`<a href="https://github.com/hoeken/yarrboard-firmware/commit/${clean_hash}">${short_hash}</a>`);

        //various other component versions
        $("#build_time").html(msg.build_time);
        $("#hardware_version").html(msg.hardware_version);
        $("#esp_idf_version").html(`${msg.esp_idf_version}`);
        $("#arduino_version").html(`v${msg.arduino_version}`);
        $("#psychic_http_version").html(`v${msg.psychic_http_version}`);
        $("#yarrboard_client_version").html(`v${YarrboardClient.version}`);

        //show some info about restarts
        if (msg.last_restart_reason)
          $("#last_reboot_reason").html(msg.last_restart_reason);
        else
          $("#last_reboot_reason").parent().hide();

        YB.ChannelRegistry.loadAllChannels(msg);

        //do we have relay channels?
        $('#relayControlDiv').hide();
        $('#relayStatsDiv').hide();
        if (msg.relay) {
          //fresh slate
          $('#relayCards').html("");
          for (ch of msg.relay) {
            if (ch.enabled) {
              $('#relayCards').append(RelayControlCard(ch));
            }
          }

          $('#relayControlDiv').show();
          $('#relayStatsDiv').show();
        }

        //do we have servo channels?
        $('#servoControlDiv').hide();
        $('#servoStatsDiv').hide();
        if (msg.servo) {
          //fresh slate
          $('#servoCards').html("");
          for (ch of msg.servo) {
            if (ch.enabled) {
              $('#servoCards').append(ServoControlCard(ch));

              $('#servoSlider' + ch.id).change(set_servo_angle);

              //update our duty when we move
              $('#servoSlider' + ch.id).on("input", set_servo_angle);

              //stop updating the UI when we are choosing a duty
              $('#servoSlider' + ch.id).on('focus', function (e) {
                let ele = e.target;
                let id = ele.id.match(/\d+/)[0];
                currentServoSliderID = id;
              });

              //stop updating the UI when we are choosing a duty
              $('#servoSlider' + ch.id).on('touchstart', function (e) {
                let ele = e.target;
                let id = ele.id.match(/\d+/)[0];
                currentServoSliderID = id;
              });

              //restart the UI updates when slider is closed
              $('#servoSlider' + ch.id).on("blur", function (e) {
                currentServoSliderID = -1;
              });

              //restart the UI updates when slider is closed
              $('#servoSlider' + ch.id).on("touchend", function (e) {
                currentServoSliderID = -1;
              });
            }
          }

          $('#servoControlDiv').show();
          $('#servoStatsDiv').show();
        }

        //populate our adc control table
        $('#adcControlDiv').hide();
        if (msg.adc) {
          $('#adcTableBody').html("");
          for (ch of msg.adc) {
            if (ch.enabled)
              $('#adcTableBody').append(ADCControlRow(ch.id, ch.name, ch.type));
          }

          $('#adcControlDiv').show();

          //start our update poller.
          if (current_page == 'config')
            start_update_data();
        }

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

          //also hide our other stuff here...
          $('#relayControlDiv').hide();
          $('#servoControlDiv').hide();
        }
        else //non-brineomatic... no graphs
        {
          $(`#graphsNav`).remove(); //remove our graph element
          page_list = page_list.filter(p => p !== "graphs");
          delete page_ready.graphs;
          for (const role in page_permissions) {
            page_permissions[role] = page_permissions[role].filter(p => p !== "graphs");
          }
        }

        //only do it as needed
        if (!page_ready.config || current_page != "config") {

          //board name controls
          $('#boardConfigForm').html(BoardNameEdit(msg.name));
          $("#fBoardName").change(validate_board_name);

          //edit controls for each relay
          $('#relayConfig').hide();
          if (msg.relay) {
            $('#relayConfigForm').html("");
            for (ch of msg.relay) {
              $('#relayConfigForm').append(RelayEditCard(ch));
              $(`#fRelayEnabled${ch.id}`).prop("checked", ch.enabled);
              $(`#fRelayType${ch.id}`).val(ch.type);
              $(`#fRelayDefaultState${ch.id}`).val(ch.defaultState);

              //enable/disable other stuff.
              $(`#fRelayName${ch.id}`).prop('disabled', !ch.enabled);
              $(`#fRelayType${ch.id}`).prop('disabled', !ch.enabled);
              $(`#fRelayDefaultState${ch.id}`).prop('disabled', !ch.enabled);

              //validate + save
              $(`#fRelayEnabled${ch.id}`).change(validate_relay_enabled);
              $(`#fRelayName${ch.id}`).change(validate_relay_name);
              $(`#fRelayType${ch.id}`).change(validate_relay_type);
              $(`#fRelayDefaultState${ch.id}`).change(validate_relay_default_state);
            }
            $('#relayConfig').show();
          }

          //edit controls for each servo
          $('#servoConfig').hide();
          if (msg.servo) {
            $('#servoConfigForm').html("");
            for (ch of msg.servo) {
              $('#servoConfigForm').append(ServoEditCard(ch));
              $(`#fServoEnabled${ch.id}`).prop("checked", ch.enabled);

              //enable/disable other stuff.
              $(`#fServoName${ch.id}`).prop('disabled', !ch.enabled);

              //validate + save
              $(`#fServoEnabled${ch.id}`).change(validate_servo_enabled);
              $(`#fServoName${ch.id}`).change(validate_servo_name);
            }
            $('#servoConfig').show();
          }

          //edit controls for each adc
          $('#adcConfig').hide();
          if (msg.adc) {
            $('#adcConfigForm').html("");
            for (ch of msg.adc) {
              $('#adcConfigForm').append(ADCEditCard(ch));
              $(`#fADCEnabled${ch.id}`).prop("checked", ch.enabled);
              $(`#fADCType${ch.id}`).val(ch.type);
              $(`#fADCDisplayDecimals${ch.id}`).val(ch.displayDecimals);
              $(`#fADCUseCalibrationTable${ch.id}`).prop("checked", ch.useCalibrationTable);
              if (ch.useCalibrationTable)
                $(`#fADCCalibrationTableUI${ch.id}`).show();
              else
                $(`#fADCCalibrationTableUI${ch.id}`).hide();

              //enable/disable other stuff.
              $(`#fADCName${ch.id}`).prop('disabled', !ch.enabled);
              $(`#fADCType${ch.id}`).prop('disabled', !ch.enabled);
              $(`#fADCDisplayDecimals${ch.id}`).prop('disabled', !ch.enabled);
              $(`#fADCUseCalibrationTable${ch.id}`).prop('disabled', !ch.enabled);

              //validate + save
              $(`#fADCEnabled${ch.id}`).change(validate_adc_enabled);
              $(`#fADCName${ch.id}`).change(validate_adc_name);
              $(`#fADCType${ch.id}`).change(validate_adc_type);
              $(`#fADCDisplayDecimals${ch.id}`).change(validate_adc_display_decimals);
              $(`#fADCUseCalibrationTable${ch.id}`).change(validate_adc_use_calibration_table);
              $(`#fADCCalibratedUnits${ch.id}`).change(validate_adc_calibrated_units);

              //calibration table stuff.
              $(`#fADCAverageOutputCopy${ch.id}`).click(adc_calibration_average_copy);
              $(`#fADCAverageOutputReset${ch.id}`).click(adc_calibration_average_reset);
              $(`#fADCCalibrationTableAdd${ch.id}`).click(validate_adc_add_calibration);
              if (YB.App.config.adc[ch.id - 1].hasOwnProperty("calibrationTable")) {
                YB.App.config.adc[ch.id - 1].calibrationTable.map(([output, calibrated], index) => {
                  $(`#fADCCalibrationTableRemove${ch.id}_${index}`).click(validate_adc_remove_calibration);
                });
              }

              //for our live averages on config page.
              adc_running_averages[ch.id] = [];
            }

            $('#adcConfig').show();
          }
        }

        //did we get brightness?
        if (msg.brightness && !currentlyPickingBrightness)
          $('#brightnessSlider').val(Math.round(msg.brightness * 100));

        if (YB.App.isMFD()) {
          $(".mfdShow").show()
          $(".mfdHide").hide()
        }
        else {
          $(".mfdShow").hide()
          $(".mfdHide").show()
        }

        //ready!
        page_ready.config = true;

        if (!current_page)
          YB.App.openPage('control');
      }
      else if (msg.msg == 'update') {
        // YB.log("update");
        // YB.log(msg);

        //we need a config loaded.
        if (!YB.App.config)
          return;

        YB.ChannelRegistry.updateAllChannels(msg);

        //update our clock.
        // let mytime = Date.parse(msg.time);
        // if (mytime)
        // {
        //   let mydate = new Date(mytime);
        //   $('#time').html(mydate.toLocaleString());
        //   $('#time').show();
        // }
        // else
        //   $('#time').hide();

        // if (msg.uptime)
        //   $("#uptime").html(YB.Util.secondsToDhms(Math.round(msg.uptime/1000000)));

        //or maybe voltage
        // if (msg.bus_voltage)
        // {
        //   $('#bus_voltage_main').html("Bus Voltage: " + msg.bus_voltage.toFixed(2) + "V");
        //   $('#bus_voltage_main').show();
        // }
        // else
        //   $('#bus_voltage_main').hide();

        //our relay info
        if (msg.relay) {
          for (ch of msg.relay) {
            if (YB.App.config.relay[ch.id - 1].enabled) {
              if (ch.state == "ON") {
                $('#relayState' + ch.id).addClass("btn-success");
                $('#relayState' + ch.id).removeClass("btn-secondary");
                $(`#relayStatus${ch.id}`).hide();
              }
              else {
                $('#relayState' + ch.id).addClass("btn-secondary");
                $('#relayState' + ch.id).removeClass("btn-success");
                $(`#relayStatus${ch.id}`).hide();
              }
            }
          }
        }

        //our servo info
        if (msg.servo) {
          for (ch of msg.servo) {
            if (YB.App.config.servo[ch.id - 1].enabled) {
              if (currentServoSliderID != ch.id) {
                $('#servoSlider' + ch.id).val(ch.angle);
                $('#servoAngle' + ch.id).html(`${ch.angle}°`);
              }
            }
          }
        }

        //our adc info
        if (msg.adc) {
          for (ch of msg.adc) {
            if (YB.App.config.adc[ch.id - 1].enabled) {

              //load our defaults
              let units = YB.App.config.adc[ch.id - 1].units;
              let value = parseFloat(ch.value);
              let calibrated_value = value;

              // let adc_raw = Math.round(ch.adc_raw);
              // let adc_voltage = ch.adc_voltage.toFixed(2);
              // $("#adcRaw" + ch.id).html(adc_raw);
              // $("#adcVoltage" + ch.id).html(adc_voltage + "V")
              //let percentage = ch.percentage.toFixed(1);
              // $(`#adcBar${ch.id} div`).css("width", percentage + "%");
              // $(`#adcBar${ch.id}`).attr("aria-valuenow", percentage);

              //are we using a calibration table?
              if (YB.App.config.adc[ch.id - 1].useCalibrationTable) {
                units = YB.App.config.adc[ch.id - 1].calibratedUnits;
                calibrated_value = parseFloat(ch.calibrated_value);
              }

              //save it to our running average.
              if (current_page == "config") {
                //manage our sliding window
                adc_running_averages[ch.id].push(value);
                if (adc_running_averages[ch.id].length > 1000)
                  adc_running_averages[ch.id].shift();

                //update our average
                const average = adc_running_averages[ch.id].reduce((accumulator, currentValue) => accumulator + currentValue, 0) / adc_running_averages[ch.id].length;
                $(`#fADCAverageOutput${ch.id}`).val(average.toFixed(4));
                $(`#fADCAverageOutputCount${ch.id}`).html(`(${adc_running_averages[ch.id].length} points)`);
              } else
                adc_running_averages[ch.id] = [];

              //how should we format our value?
              if (YB.App.config.adc[ch.id - 1].displayDecimals >= 0) {
                calibrated_value = YB.Util.formatNumber(calibrated_value, YB.App.config.adc[ch.id - 1].displayDecimals);
              } else {
                switch (YB.App.config.adc[ch.id - 1].type) {
                  case "thermistor":
                    calibrated_value = YB.Util.formatNumber(calibrated_value, 1);
                    break;
                  case "raw":
                  case "4-20ma":
                  case "high_volt_divider":
                  case "low_volt_divider":
                    calibrated_value = YB.Util.formatNumber(calibrated_value, 2);
                    break;
                  case "ten_k_pullup":
                    calibrated_value = YB.Util.formatNumber(calibrated_value, 0);
                    break;
                  default:
                    calibrated_value = YB.Util.formatNumber(calibrated_value, 3);
                }
              }

              //how should we display our value?
              switch (YB.App.config.adc[ch.id - 1].type) {
                case "digital_switch":
                  if (value)
                    $("#adcValue" + ch.id).html(`HIGH`);
                  else
                    $("#adcValue" + ch.id).html(`LOW`);
                  break;
                case "4-20ma":
                  if (value == 0)
                    $("#adcValue" + ch.id).html(`<span class="text-danger">ERROR: No Signal</span>`);
                  else if (value < 4.0)
                    $("#adcValue" + ch.id).html(`<span class="text-danger">ERROR: ???</span>`);
                  else
                    $("#adcValue" + ch.id).html(`${calibrated_value}${units}`);
                  break;
                case "ten_k_pullup":
                  if (value == -2)
                    $("#adcValue" + ch.id).html(`<span class="text-danger">Error: No Connection</span>`);
                  else if (value == -1)
                    $("#adcValue" + ch.id).html(`<span class="text-danger">Error: Negative Voltage</span>`);
                  else
                    $("#adcValue" + ch.id).html(`${calibrated_value}${units}`);
                  break;

                default:
                  $("#adcValue" + ch.id).html(`${calibrated_value}${units}`);
              }
            }
          }
        }

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
            if (current_page == "control") {
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

            if (current_page == "graphs") {

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
            updateProgressBar("bomRunProgressBar", runtimeProgress);
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
            updateProgressBar("bomFlushProgressBar", flushProgress);
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
            updateProgressBar("bomPickleProgressBar", pickleProgress);
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
            updateProgressBar("#bomDepickleProgressBar", depickleProgress);
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
        else
          $('#bomInterface').hide();

        if (YB.App.isMFD()) {
          $(".mfdShow").show()
          $(".mfdHide").hide()
        }
        else {
          $(".mfdShow").hide()
          $(".mfdHide").show()
        }

        //save it for later use.
        last_update = msg;

        page_ready.control = true;
      }
      else if (msg.msg == "stats") {
        //YB.log("stats");

        //we need this
        if (!YB.App.config)
          return;

        YB.ChannelRegistry.updateAllStats(msg);

        $("#uptime").html(YB.Util.secondsToDhms(Math.round(msg.uptime / 1000000)));
        if (msg.fps)
          $("#fps").html(msg.fps.toLocaleString("en-US") + " hz");

        //message info
        $("#received_message_mps").html(msg.received_message_mps.toLocaleString("en-US") + " mps");
        $("#received_message_total").html(msg.received_message_total.toLocaleString("en-US"));
        $("#sent_message_mps").html(msg.sent_message_mps.toLocaleString("en-US") + " mps");
        $("#sent_message_total").html(msg.sent_message_total.toLocaleString("en-US"));

        //client info
        $("#total_client_count").html(msg.http_client_count + msg.websocket_client_count);
        $("#http_client_count").html(msg.http_client_count);
        $("#websocket_client_count").html(msg.websocket_client_count);

        //raw data
        $("#heap_size").html(YB.Util.formatBytes(msg.heap_size, 0));
        $("#free_heap").html(YB.Util.formatBytes(msg.free_heap, 0));
        $("#min_free_heap").html(YB.Util.formatBytes(msg.min_free_heap, 0));
        $("#max_alloc_heap").html(YB.Util.formatBytes(msg.max_alloc_heap, 0));

        //our memory bar
        let memory_used = ((msg.heap_size / (msg.heap_size + msg.free_heap)) * 100).toFixed(2); //esp32-s3 512kb ram
        $(`#memory_usage div`).css("width", memory_used + "%");
        $(`#memory_usage div`).html(YB.Util.formatBytes(msg.heap_size, 0));
        $(`#memory_usage`).attr("aria-valuenow", memory_used);

        //wifi rssi.
        if (msg.rssi) {
          $("#rssi").html(msg.rssi + "dBm");
          $(".rssi_icon").hide();
          let rssi_icon = getWifiIconForRSSI(msg.rssi);
          $(`#${rssi_icon}`).show();
        }
        $("#uuid").html(msg.uuid);
        $("#ip_address").html(msg.ip_address);

        //our mqtt?
        if (YB.App.config.enable_mqtt) {
          if (msg.mqtt_connected)
            $("#mqtt_status").html(`<span class="text-success">Connected</span>`);
          else
            $("#mqtt_status").html(`<span class="text-danger">Disconnected</span>`);
        } else
          $("#mqtt_status").html(`<span>Disabled</span>`);

        if (msg.bus_voltage)
          $("#bus_voltage").html(msg.bus_voltage.toFixed(1) + "V");
        else
          $("#bus_voltage_row").remove();

        if (msg.fans)
          $("#fan_rpm").html(msg.fans.map((a) => a.rpm + "RPM").join(", "));
        else
          $("#fan_rpm_row").remove();

        if (msg.brineomatic) {
          let totalVolume = Math.round(msg.total_volume);
          totalVolume = totalVolume.toLocaleString('en-US');
          let totalRuntime = (msg.total_runtime / (60 * 60 * 1000000)).toFixed(1);
          totalRuntime = totalRuntime.toLocaleString('en-US');
          $("#bomTotalCycles").html(msg.total_cycles.toLocaleString('en-US'));
          $("#bomTotalVolume").html(`${totalVolume}L`);
          $("#bomTotalRuntime").html(`${totalRuntime} hours`);
        }

        page_ready.stats = true;
      } else if (msg.msg == "graph_data") {
        if (current_page == "graphs") {
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
          start_update_data();

          page_ready.graphs = true;
        }
      }
      //load up our network config.
      else if (msg.msg == "full_config") {
        //YB.log("network config");

        //load our config
        const textarea = document.getElementById('configurationTextarea');
        const prettyJSON = JSON.stringify(msg.config, null, 2); // 2-space indentation
        textarea.value = prettyJSON;

        //save handler
        document.getElementById('configurationSave').addEventListener('click', () => {
          const configText = document.getElementById('configurationTextarea').value;

          try {
            // Try to parse the JSON
            const parsed = JSON.parse(configText);

            // Re-serialize it compactly (no whitespace)
            const compact = JSON.stringify(parsed);

            //send it off for saving.
            YB.client.send({
              "cmd": "save_config",
              "config": compact
            }, true);

            //let them know.
            $("#saveIndicator").html("Saved!");

          } catch (err) {
            //show an error
            $("#invalidConfigurationJSON").show();

            //highlight the textarea for feedback
            textarea.style.border = "2px solid red";
            setTimeout(() => textarea.style.border = "", 2000);
          }
        });

        //copy handler
        document.getElementById('configurationCopy').addEventListener('click', () => {
          const text = document.getElementById('configurationTextarea').value;

          if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text)
              .then(() => $("#copyIndicator").html("Copied!"))
              .catch(err => console.error("Clipboard write failed:", err));
          } else {
            // Fallback for insecure contexts aka locally hosted non-secure esp32 pages... or old browsers
            textarea.select();
            try {
              document.execCommand("copy");
              $("#copyIndicator").html("Copied!");
            } catch (err) {
              console.error("Fallback copy failed:", err);
            }
          }
        });

        //download handler
        document.getElementById('configurationDownload').addEventListener('click', () => {
          const text = document.getElementById('configurationTextarea').value;
          const blob = new Blob([text], { type: 'application/json' });
          const url = URL.createObjectURL(blob);
          const a = document.createElement('a');
          a.href = url;
          const today = new Date();
          const dateStr = today.toISOString().split('T')[0];
          a.download = `${YB.App.config.hostname}_${YB.App.config.uuid}_${dateStr}_config.json`;
          document.body.appendChild(a);
          a.click();
          document.body.removeChild(a);
          URL.revokeObjectURL(url); // cleanup
        });

        addConfigurationDragDropHandler();
      }
      //load up our network config.
      else if (msg.msg == "network_config") {
        //YB.log("network config");

        //save our config.
        network_config = msg;

        //YB.log(msg);
        $("#wifi_mode").val(msg.wifi_mode);
        $("#wifi_ssid").val(msg.wifi_ssid);
        $("#wifi_pass").val(msg.wifi_pass);
        $("#local_hostname").val(msg.local_hostname);

        page_ready.network = true;
      }
      //load up our network config.
      else if (msg.msg == "app_config") {
        //YB.log("network config");

        //save our config.
        app_config = msg;

        //update some ui stuff
        update_role_ui();
        toggle_role_passwords(msg.default_role);

        //what is our update interval?
        if (msg.app_update_interval)
          app_update_interval = msg.app_update_interval;

        //YB.log(msg);
        $("#admin_user").val(msg.admin_user);
        $("#admin_pass").val(msg.admin_pass);
        $("#guest_user").val(msg.guest_user);
        $("#guest_pass").val(msg.guest_pass);
        $("#app_update_interval").val(msg.app_update_interval);
        $("#default_role").val(msg.default_role);
        $("#default_role").change(function () { toggle_role_passwords($(this).val()) });
        $("#app_enable_mfd").prop("checked", msg.app_enable_mfd);
        $("#app_enable_api").prop("checked", msg.app_enable_api);
        $("#app_enable_serial").prop("checked", msg.app_enable_serial);
        $("#app_enable_ota").prop("checked", msg.app_enable_ota);

        //for our ssl stuff
        $("#app_enable_ssl").prop("checked", msg.app_enable_ssl);
        $("#server_cert").val(msg.server_cert);
        $("#server_key").val(msg.server_key);

        //hide/show these guys
        if (msg.app_enable_ssl) {
          $("#server_cert_container").show();
          $("#server_key_container").show();
        }

        //make it dynamic too
        $("#app_enable_ssl").change(function () {
          if ($("#app_enable_ssl").prop("checked")) {
            $("#server_cert_container").show();
            $("#server_key_container").show();
          }
          else {
            $("#server_cert_container").hide();
            $("#server_key_container").hide();
          }
        });

        //for our mqtt stuff
        $("#app_enable_mqtt").prop("checked", msg.app_enable_mqtt);
        $("#app_enable_ha_integration").prop("checked", msg.app_enable_ha_integration);
        $("#app_use_hostname_as_mqtt_uuid").prop("checked", msg.app_use_hostname_as_mqtt_uuid);
        $("#mqtt_server").val(msg.mqtt_server);
        $("#mqtt_user").val(msg.mqtt_user);
        $("#mqtt_pass").val(msg.mqtt_pass);
        $("#mqtt_cert").val(msg.mqtt_cert);

        //hide/show these guys
        if (msg.app_enable_mqtt) {
          $(".mqtt_field").show();
        }

        //make it dynamic too
        $("#app_enable_mqtt").change(function () {
          if ($("#app_enable_mqtt").prop("checked")) {
            $(".mqtt_field").show();
          }
          else {
            $("#app_enable_ha_integration").prop("checked", false);
            $(".mqtt_field").hide();
          }
        });

        page_ready.settings = true;
      }
      //load up our network config.
      else if (msg.msg == "ota_progress") {
        //YB.log("ota progress");

        //OTA is blocking... so update our heartbeat
        //TODO: do we need to update this?
        //last_heartbeat = Date.now();

        let progress = Math.round(msg.progress);

        let prog_id = `#firmware_progress`;
        $(prog_id).css("width", progress + "%").text(progress + "%");
        if (progress == 100) {
          $(prog_id).removeClass("progress-bar-animated");
          $(prog_id).removeClass("progress-bar-striped");
        }

        //was that the last?
        if (progress == 100) {
          show_alert("Firmware update successful.", "success");

          //reload our page
          setTimeout(function () {
            location.reload(true);
          }, 2500);
        }
      }
      else if (msg.status == "error") {
        YB.log(msg.message);

        //did we turn login off?
        if (msg.message == "Login not required.") {
          Cookies.remove("username");
          Cookies.remove("password");
        }

        //keep the u gotta login to the login page.
        if (msg.message == "You must be logged in.")
          YB.App.openPage("login");
        else
          show_alert(msg.message);
      }
      else if (msg.msg == "login") {

        //keep the login success stuff on the login page.
        if (msg.message == "Login successful.") {
          //once we know our role, we can load our other configs.
          app_role = msg.role;

          // YB.log(`app_role: ${app_role}`);
          // YB.log(`current_page: ${current_page}`);

          //only needed for login page, otherwise its autologin
          if (current_page == "login") {
            //save user/pass to cookies.
            if (app_username && app_password) {
              Cookies.set('username', app_username, { expires: 365 });
              Cookies.set('password', app_password, { expires: 365 });
            }
          }

          //prep the site
          update_role_ui();
          load_configs();
          open_default_page();
        }
        else
          show_alert(msg.message, "success");
      }
      else if (msg.msg == "set_theme") {
        //light/dark mode
        setTheme(msg.theme);
      }
      else if (msg.msg == "set_brightness") {
        //did we get brightness?
        if (msg.brightness && !currentlyPickingBrightness)
          $('#brightnessSlider').val(Math.round(msg.brightness * 100));
      }
      else if (msg.msg) {
        YB.log("[socket] Unknown message: " + JSON.stringify(msg));
      }
    };

    //use custom logger
    YB.client.log = YB.log;

    YB.client.start();
  }

  function getQueryVariable(name) {
    const query = window.location.search.substring(1);
    const vars = query.split("&");
    for (let i = 0; i < vars.length; i++) {
      const pair = vars[i].split("=");
      if (decodeURIComponent(pair[0]) === name) {
        return decodeURIComponent(pair[1] || "");
      }
    }
    return null; // Return null if the variable is not found
  }

  function show_alert(message, type = 'danger') {
    //we only need one alert at a time.
    $('#liveAlertPlaceholder').html(AlertBox(message, type))

    console.log(`show_alert: ${message}`);

    //make sure we can see it.
    $('html').animate({
      scrollTop: 0
    },
      750 //speed
    );
  }

  function show_admin_alert(message, type = 'danger') {
    //we only need one alert at a time.
    $('#adminAlertPlaceholder').html(AlertBox(message, type))

    console.log(`show_admin_alert: ${message}`);

    //make sure we can see it.
    $('html').animate({
      scrollTop: 0
    },
      750 //speed
    );
  }


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

  function addConfigurationDragDropHandler() {
    const ta = document.getElementById('configurationTextarea');
    const invalid = document.getElementById('invalidConfigurationJSON');
    const openBtn = document.getElementById('configurationOpen');
    const fileInput = document.getElementById('configurationFileInput');
    const MAX_BYTES = 1024 * 1024 * 2; // 2 MB cap

    if (!ta) return;

    function showInvalid(msg) {
      if (invalid) {
        invalid.style.display = '';
        invalid.textContent = msg || 'Invalid JSON found.';
      }
    }
    function hideInvalid() {
      if (invalid) invalid.style.display = 'none';
    }

    async function handleFile(file) {
      if (!file) return;

      if (file.size > MAX_BYTES) {
        showInvalid(`File too large (${(file.size / 1024 / 1024).toFixed(1)} MB). Max ${(MAX_BYTES / 1024 / 1024)} MB.`);
        return;
      }

      const isProbablyText = file.type.startsWith('text/') || /\.json$/i.test(file.name);
      if (!isProbablyText) {
        showInvalid('Please choose a .json or text file.');
        return;
      }

      try {
        const raw = await file.text();
        try {
          const obj = JSON.parse(raw);
          ta.value = JSON.stringify(obj, null, 2);
          hideInvalid();
        } catch {
          ta.value = raw;
          showInvalid('Loaded file, but JSON is invalid.');
        }
        ta.dispatchEvent(new Event('input', { bubbles: true }));
      } catch (err) {
        showInvalid('Failed to read file.');
        console.error(err);
      }
    }

    // Drag & drop visuals
    ['dragenter', 'dragover'].forEach(evt =>
      ta.addEventListener(evt, (e) => {
        e.preventDefault();
        e.stopPropagation();
        ta.classList.add('is-drop-target');
      })
    );
    ['dragleave', 'dragend'].forEach(evt =>
      ta.addEventListener(evt, (e) => {
        e.preventDefault();
        e.stopPropagation();
        ta.classList.remove('is-drop-target');
      })
    );

    // Drop handler
    ta.addEventListener('drop', async (e) => {
      e.preventDefault();
      e.stopPropagation();
      ta.classList.remove('is-drop-target');

      const files = e.dataTransfer && e.dataTransfer.files;
      if (files && files.length > 0) {
        handleFile(files[0]);
      }
    });

    // Open button triggers hidden input
    if (openBtn && fileInput) {
      openBtn.addEventListener('click', () => fileInput.click());
      fileInput.addEventListener('change', (e) => {
        const file = e.target.files && e.target.files[0];
        handleFile(file);
        // allow re-selecting the same file later
        e.target.value = '';
      });
    }
  }

  function toggle_relay_state(id) {
    YB.client.send({
      "cmd": "toggle_relay_channel",
      "id": id,
      "source": YB.App.config.hostname
    }, true);
  }

  function on_page_ready() {
    //is our page ready yet?
    if (page_ready[current_page]) {
      $("#loading").hide();
      $(`#${current_page}Page`).show();
    }
    else {
      $("#loading").show();
      setTimeout(on_page_ready, 100);
    }
  }

  function get_stats_data() {
    if (YB.client.isOpen() && (app_role == 'guest' || app_role == 'admin')) {
      //YB.log("get_stats");

      YB.client.getStats();

      //keep loading it while we are here.
      if (current_page == "stats")
        setTimeout(get_stats_data, app_update_interval);
    }
  }

  function get_graph_data() {
    if (YB.client.isOpen() && (app_role == 'guest' || app_role == 'admin')) {
      YB.client.send({
        "cmd": "get_graph_data"
      });
    }
  }

  function start_update_data() {
    if (!app_update_interval_id) {
      //YB.log("starting updates");
      app_update_interval_id = setInterval(get_update_data, app_update_interval);
    } else {
      //YB.log("updates already running");
    }
  }

  function get_update_data() {
    if (YB.client.isOpen() && (app_role == 'guest' || app_role == 'admin')) {
      //YB.log("get_update");

      YB.client.getUpdate();

      //keep loading it while we are here.
      if (current_page == "control" || current_page == "graphs" || (current_page == "config" && YB.App.config && YB.App.config.hasOwnProperty("adc")))
        return;
      else {
        //YB.log("stopping updates");
        clearInterval(app_update_interval_id);
        app_update_interval_id = 0;
      }
    }
  }

  function validate_board_name(e) {
    let ele = e.target;
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      //set our new board name!
      YB.client.send({
        "cmd": "set_boardname",
        "value": value
      });
    }
  }

  function set_servo_angle(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    //must be realistic.
    if (value >= 0 && value <= 180) {
      //update our button
      $(`#servoAngle${id}`).html(`${value}°`);

      YB.client.send({
        "cmd": "set_servo_channel",
        "id": id,
        "angle": value
      });
    }
  }

  function set_brightness(e) {
    let ele = e.target;
    let value = ele.value;

    //must be realistic.
    if (value >= 0 && value <= 100) {
      //we want a duty value from 0 to 1
      value = value / 100;

      //update our duty cycle
      YB.client.setBrightness(value, false);
    }
  }

  function update_theme_switch() {
    if (this.checked)
      setTheme("dark");
    else
      setTheme("light");
  }

  function validate_relay_name(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_relay_channel",
        "id": id,
        "name": value
      });
    }
  }

  function validate_relay_enabled(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.checked;

    $(ele).addClass("is-valid");

    YB.client.send({
      "cmd": "config_relay_channel",
      "id": id,
      "enabled": value
    });
  }

  function validate_relay_type(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_relay_channel",
        "id": id,
        "type": value
      });
    }
  }

  function validate_relay_default_state(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 10) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_relay_channel",
        "id": id,
        "defaultState": value
      });
    }
  }

  function validate_servo_name(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_servo_channel",
        "id": id,
        "name": value
      });
    }
  }

  function validate_servo_enabled(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.checked;

    $(ele).addClass("is-valid");

    YB.client.send({
      "cmd": "config_servo_channel",
      "id": id,
      "enabled": value
    });
  }

  function validate_adc_name(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_adc",
        "id": id,
        "name": value
      });
    }
  }

  function validate_adc_enabled(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.checked;

    //enable/disable other stuff.
    $(`#fADCName${id}`).prop('disabled', !value);
    $(`#fADCType${id}`).prop('disabled', !value);
    $(`#fADCUseCalibrationTable${id}`).prop('disabled', !value);
    if (value)
      $(`#fADCCalibrationTableUI${id}`).show();
    else
      $(`#fADCCalibrationTableUI${id}`).hide();


    //nothing really to validate here.
    $(ele).addClass("is-valid");

    //save it
    YB.client.send({
      "cmd": "config_adc",
      "id": id,
      "enabled": value
    });
  }

  function validate_adc_use_calibration_table(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.checked;

    //nothing really to validate here.
    $(ele).addClass("is-valid");

    if (value)
      $(`#fADCCalibrationTableUI${id}`).show();
    else
      $(`#fADCCalibrationTableUI${id}`).hide();

    //save it
    YB.client.send({
      "cmd": "config_adc",
      "id": id,
      "useCalibrationTable": value
    });
  }

  function validate_adc_calibrated_units(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_adc",
        "id": id,
        "calibratedUnits": value
      });

      //update places that use this.
      $(`.ADCCalibratedUnits${id}`).html(value);
    }
  }

  function adc_calibration_average_copy(e) {
    let ele = e.currentTarget;
    let id = ele.id.match(/\d+/)[0];

    let output = parseFloat($(`#fADCAverageOutput${id}`).val());
    $(`#fADCCalibrationTableOutput${id}`).val(output);
    $(`#fADCCalibrationTableCalibrated${id}`).focus();
  }

  function adc_calibration_average_reset(e) {
    let ele = e.currentTarget;
    let id = ele.id.match(/\d+/)[0];

    $(`#fADCAverageOutput${id}`).val(0.0);
    adc_running_averages[id] = [];
  }

  function validate_adc_add_calibration(e) {
    let ele = e.currentTarget;
    let id = ele.id.match(/\d+/)[0];

    let output = parseFloat($(`#fADCCalibrationTableOutput${id}`).val());
    let calibrated = parseFloat($(`#fADCCalibrationTableCalibrated${id}`).val());

    if (false) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      // $(ele).removeClass("is-invalid");
      // $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_adc",
        "id": id,
        "add_calibration": [output, calibrated]
      });

      //re-initialize
      $(`#fADCCalibrationTableOutput${id}`).val("");
      $(`#fADCCalibrationTableCalibrated${id}`).val("");

      //new row for the ui
      let index = 0;
      if (Array.isArray(YB.App.config.adc[id - 1].calibrationTable)) {
        index = YB.App.config.adc[id - 1].calibrationTable.length;
      }
      let newRow = ADCCalibrationTableRow(YB.App.config.adc[id], output, calibrated, index);
      $(`#ADCCalibrationTableBody${id}`).append(newRow);
      $(`#fADCCalibrationTableRemove${id}_${index}`).click(validate_adc_remove_calibration);
      YB.App.config.adc[id - 1].calibrationTable.push([output, calibrated]); //temporarily save it.
    }
  }

  function validate_adc_remove_calibration(e) {
    let ele = e.currentTarget;

    const match = ele.id.match(/(\d+)_(\d+)/);
    const id = parseInt(match[1], 10);
    const index = parseInt(match[2], 10);

    const output = parseFloat($(`#fADCCalibrationTableOutput${id}_${index}`).val());
    const calibrated = parseFloat($(`#fADCCalibrationTableCalibrated${id}_${index}`).val());

    if (false) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      // $(ele).removeClass("is-invalid");
      // $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_adc",
        "id": id,
        "remove_calibration": [output, calibrated]
      });

      //remove our row.
      $(`#fADCCalibrationTableRow${id}_${index}`).remove();
    }
  }

  function validate_adc_type(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = ele.value;

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_adc",
        "id": id,
        "type": value
      });
    }
  }

  function validate_adc_display_decimals(e) {
    let ele = e.target;
    let id = ele.id.match(/\d+/)[0];
    let value = parseInt(ele.value);

    if (value.length <= 0 || value.length > 30) {
      $(ele).removeClass("is-valid");
      $(ele).addClass("is-invalid");
    }
    else {
      $(ele).removeClass("is-invalid");
      $(ele).addClass("is-valid");

      YB.client.send({
        "cmd": "config_adc",
        "id": id,
        "display_decimals": value
      });

      //update our current config.
      YB.App.config.adc[id - 1].displayDecimals = value;
    }
  }

  function do_login(e) {
    app_username = $('#username').val();
    app_password = $('#password').val();

    YB.client.login(app_username, app_password);
  }

  function save_network_settings() {
    //get our data
    let wifi_mode = $("#wifi_mode").val();
    let wifi_ssid = $("#wifi_ssid").val();
    let wifi_pass = $("#wifi_pass").val();
    let local_hostname = $("#local_hostname").val();

    //we should probably do a bit of verification here

    //if they are changing from client to client, we can't show a success.
    show_alert("Yarrboard may be unresponsive while changing WiFi settings. Make sure you connect to the right network after updating.", "primary");

    //okay, send it off.
    YB.client.send({
      "cmd": "set_network_config",
      "wifi_mode": wifi_mode,
      "wifi_ssid": wifi_ssid,
      "wifi_pass": wifi_pass,
      "local_hostname": local_hostname
    });

    //reload our page
    setTimeout(function () {
      location.reload();
    }, 2500);
  }

  function save_app_settings() {
    //get our data
    let admin_user = $("#admin_user").val();
    let admin_pass = $("#admin_pass").val();
    let guest_user = $("#guest_user").val();
    let guest_pass = $("#guest_pass").val();
    let default_role = $("#default_role option:selected").val();
    let app_enable_mfd = $("#app_enable_mfd").prop("checked");
    let app_enable_api = $("#app_enable_api").prop("checked");
    let app_enable_serial = $("#app_enable_serial").prop("checked");
    let app_enable_ota = $("#app_enable_ota").prop("checked");
    let app_enable_ssl = $("#app_enable_ssl").prop("checked");
    let app_enable_mqtt = $("#app_enable_mqtt").prop("checked");
    let app_enable_ha_integration = $("#app_enable_ha_integration").prop("checked");
    let app_use_hostname_as_mqtt_uuid = $("#app_use_hostname_as_mqtt_uuid").prop("checked");
    let mqtt_server = $("#mqtt_server").val();
    let mqtt_user = $("#mqtt_user").val();
    let mqtt_pass = $("#mqtt_pass").val();
    let mqtt_cert = $("#mqtt_cert").val();
    let server_cert = $("#server_cert").val();
    let server_key = $("#server_key").val();
    let update_interval = $("#app_update_interval").val();

    //we should probably do a bit of verification here
    update_interval = Math.max(100, update_interval);
    update_interval = Math.min(5000, update_interval);
    app_update_interval = update_interval;

    //remember it and update our UI
    default_app_role = default_role;
    update_role_ui();

    //helper function to keep admin logged in.
    if (default_app_role != "admin") {
      Cookies.set('username', admin_user, { expires: 365 });
      Cookies.set('password', admin_pass, { expires: 365 });
    }
    else {
      Cookies.remove("username");
      Cookies.remove("password");
    }

    //okay, send it off.
    YB.client.send({
      "cmd": "set_app_config",
      "admin_user": admin_user,
      "admin_pass": admin_pass,
      "guest_user": guest_user,
      "guest_pass": guest_pass,
      "app_update_interval": app_update_interval,
      "default_role": default_role,
      "app_enable_mfd": app_enable_mfd,
      "app_enable_api": app_enable_api,
      "app_enable_serial": app_enable_serial,
      "app_enable_ota": app_enable_ota,
      "app_enable_ssl": app_enable_ssl,
      "app_enable_mqtt": app_enable_mqtt,
      "app_enable_ha_integration": app_enable_ha_integration,
      "app_use_hostname_as_mqtt_uuid": app_use_hostname_as_mqtt_uuid,
      "mqtt_server": mqtt_server,
      "mqtt_user": mqtt_user,
      "mqtt_pass": mqtt_pass,
      "mqtt_cert": mqtt_cert,
      "server_cert": server_cert,
      "server_key": server_key
    });

    show_alert("App settings have been updated.", "success");
  }

  function restart_board() {
    if (confirm("Are you sure you want to restart your Yarrboard?")) {
      YB.client.restart();

      show_alert("Yarrboard is now restarting, please be patient.", "primary");

      setTimeout(function () {
        location.reload();
      }, 5000);
    }
  }

  function reset_to_factory() {
    if (confirm("WARNING! Are you sure you want to reset your Yarrboard to factory defaults?  This cannot be undone.")) {
      YB.client.factoryReset();

      show_alert("Yarrboard is now resetting to factory defaults, please be patient.", "primary");
    }
  }

  function check_for_updates() {
    //did we get a config yet?
    if (YB.App.config) {
      //dont look up firmware if we're in development mode.
      if (YB.App.config.is_development) {
        $("#firmware_checking").html("Development Mode, automatic firmware checking disabled.")
        return;
      }

      $.ajax({
        url: "https://raw.githubusercontent.com/hoeken/yarrboard-firmware/main/firmware.json",
        cache: false,
        dataType: "json",
        success: function (jdata) {
          //did we get anything?
          let data;
          for (firmware of jdata)
            if (firmware.type == YB.App.config.hardware_version)
              data = firmware;

          if (!data) {
            //show_alert(`Could not find a firmware for this hardware.`, "danger");
            return;
          }

          $("#firmware_checking").hide();

          //do we have a new version?
          if (compareVersions(data.version, YB.App.config.firmware_version)) {
            if (data.changelog) {
              $("#firmware_changelog").append(marked.parse(data.changelog));
              $("#firmware_changelog").show();
            }

            $("#new_firmware_version").html(data.version);
            $("#firmware_bin").attr("href", `${data.url}`);
            $("#firmware_update_available").show();

            show_alert(`There is a <a onclick="YB.App.openPage('system')" href="/#system">firmware update</a> available (${data.version}).`, "primary");
          }
          else
            $("#firmware_up_to_date").show();
        }
      });
    }
    //wait for it.
    else
      setTimeout(check_for_updates, 1000);
  }

  function update_firmware() {
    $("#btn_update_firmware").prop("disabled", true);
    $("#progress_wrapper").show();

    //okay, send it off.
    YB.client.startOTA();

    ota_started = true;
  }

  function toggle_role_passwords(role) {
    if (role == "admin") {
      $(".admin_credentials").hide();
      $(".guest_credentials").hide();
    }
    else if (role == "guest") {
      $(".admin_credentials").show();
      $(".guest_credentials").hide();
    }
    else {
      $(".admin_credentials").show();
      $(".guest_credentials").show();
    }
  }

  function update_role_ui() {
    //what nav tabs should we be able to see?
    if (app_role == "admin") {
      $("#navbar").show();
      $(".nav-permission").show();
    }
    else if (app_role == "guest") {
      $("#navbar").show();
      $(".nav-permission").hide();
      page_permissions[app_role].forEach((page) => {
        $(`#${page}Nav`).show();
      });
    }
    else {
      $("#navbar").hide();
      $(".nav-permission").hide();
    }

    //show login or not?
    $('#loginNav').hide();
    if (default_app_role == 'nobody' && app_role == 'nobody')
      $('#loginNav').show();
    if (default_app_role == 'guest' && app_role == 'guest')
      $('#loginNav').show();

    //show logout or not?
    $('#logoutNav').hide();
    if (default_app_role == 'nobody' && app_role != 'nobody')
      $('#logoutNav').show();
    if (default_app_role == 'guest' && app_role == 'admin')
      $('#logoutNav').show();
  }

  function open_default_page() {
    if (app_role != 'nobody') {
      //check to see if we want a certain page
      if (window.location.hash) {
        let page = window.location.hash.substring(1);
        if (page != "login" && page_list.includes(page))
          YB.App.openPage(page);
        else
          YB.App.openPage("control");
      }
      else
        YB.App.openPage("control");
    }
    else
      YB.App.openPage('login');
  }

  // return true if 'first' is greater than or equal to 'second'
  function compareVersions(first, second) {

    var a = first.split('.');
    var b = second.split('.');

    for (var i = 0; i < a.length; ++i) {
      a[i] = Number(a[i]);
    }
    for (var i = 0; i < b.length; ++i) {
      b[i] = Number(b[i]);
    }
    if (a.length == 2) {
      a[2] = 0;
    }

    if (a[0] > b[0]) return true;
    if (a[0] < b[0]) return false;

    if (a[1] > b[1]) return true;
    if (a[1] < b[1]) return false;

    if (a[2] > b[2]) return true;
    if (a[2] < b[2]) return false;

    return true;
  }



  function getStoredTheme() { localStorage.getItem('theme'); }
  function setStoredTheme() { localStorage.setItem('theme', theme); }

  function getPreferredTheme() {
    //did we get one passed in? b&g, etc pass in like this.
    let mode = getQueryVariable("mode");
    // YB.log(`mode: ${mode}`);
    if (mode !== null) {
      if (mode == "night")
        return "dark";
      else
        return "light";
    }

    //do we have stored one?
    const storedTheme = getStoredTheme();
    if (storedTheme)
      return storedTheme;

    //prefs?
    return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
  }

  function setTheme(theme) {
    // YB.log(`set theme ${theme}`);
    if (theme === 'auto' && window.matchMedia('(prefers-color-scheme: dark)').matches)
      document.documentElement.setAttribute('data-bs-theme', 'dark');
    else
      document.documentElement.setAttribute('data-bs-theme', theme);

    let currentTheme = document.documentElement.getAttribute('data-bs-theme');
    if (currentTheme == "light")
      $("#darkSwitch").prop('checked', false);
    else
      $("#darkSwitch").prop('checked', true);
  }

  window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
    const storedTheme = getStoredTheme();
    if (storedTheme !== 'light' && storedTheme !== 'dark')
      setTheme(getPreferredTheme());
  });

  function getViewport() {
    // https://stackoverflow.com/a/8876069
    const width = Math.max(
      document.documentElement.clientWidth,
      window.innerWidth || 0
    )
    if (width <= 576) return 'xs'
    if (width <= 768) return 'sm'
    if (width <= 992) return 'md'
    if (width <= 1200) return 'lg'
    if (width <= 1400) return 'xl'
    return 'xxl'
  }

  function isCanvasSupported() {
    var elem = document.createElement('canvas');
    return !!(elem.getContext && elem.getContext('2d'));
  }

  // Function to update the progress bar
  function updateProgressBar(ele, progress) {
    // Ensure the progress value is within bounds
    const clampedProgress = Math.round(Math.min(Math.max(progress, 0), 100));

    // Get the progress container and inner progress bar
    const progressContainer = document.getElementById(ele);
    const progressBar = progressContainer.querySelector(".progress-bar");

    if (progressContainer && progressBar) {
      // Update the width style property
      progressBar.style.width = clampedProgress + "%";

      // Update the aria-valuenow attribute for accessibility
      progressContainer.setAttribute("aria-valuenow", clampedProgress);

      // Optional: Update the text inside the progress bar
      progressBar.textContent = clampedProgress + "%";
    } else {
      console.error("Progress bar element not found!");
    }
  }

  function getWifiIconForRSSI(rssi) {
    let level;

    if (rssi === null || rssi === undefined || rssi <= -100) {
      level = 0; // No signal / disconnected
    } else if (rssi >= -55) {
      level = 4; // Excellent
    } else if (rssi >= -65) {
      level = 3; // Good
    } else if (rssi >= -75) {
      level = 2; // Fair
    } else if (rssi >= -85) {
      level = 1; // Weak
    } else {
      level = 0; // No signal
    }

    return `rssi_icon_${level}`;
  }

  YB.App = {
    config: {},
    start: function () {
      setup_debug_terminal();

      YB.log("User Agent: " + navigator.userAgent);
      YB.log("Window Width: " + window.innerWidth);
      YB.log("Window Height: " + window.innerHeight);
      YB.log("Window Location: " + window.location);
      YB.log("Device Pixel Ratio: " + window.devicePixelRatio);
      YB.log("Is canvas supported? " + isCanvasSupported());

      //main data connection
      start_websocket();

      //check our connection status.
      setInterval(check_connection_status, 100);

      //light/dark theme init.
      let theme = getPreferredTheme();
      // YB.log(`preferred theme: ${theme}`);
      setTheme(theme);
      $("#darkSwitch").change(update_theme_switch);

      //brightness slider callbacks
      $("#brightnessSlider").change(set_brightness);
      $("#brightnessSlider").on("input", set_brightness);

      //stop updating the UI when we are choosing
      $("#brightnessSlider").on('focus', function (e) {
        currentlyPickingBrightness = true;
      });

      //stop updating the UI when we are choosing
      $("#brightnessSlider").on('touchstart', function (e) {
        currentlyPickingBrightness = true;
      });

      //restart the UI updates when slider is closed
      $("#brightnessSlider").on("blur", function (e) {
        currentlyPickingBrightness = false;
      });

      //restart the UI updates when slider is closed
      $("#brightnessSlider").on("touchend", function (e) {
        currentlyPickingBrightness = false;
      });
    },

    openPage: function (page) {
      //YB.log(`opening ${page}`);

      if (!page_permissions[app_role].includes(page)) {
        YB.log(`${page} not allowed for ${app_role}`);
        return;
      }

      current_page = page;

      //request our stats.
      if (page == "stats")
        get_stats_data();

      //request our historical graph data (if any)
      if (page == "graphs") {
        get_graph_data();
      }

      //request our control updates.
      if (page == "control")
        start_update_data();

      //we need updates for adc config page.
      if (page == "config" && YB.App.config && YB.App.config.hasOwnProperty("adc"))
        start_update_data();

      //hide all pages.
      $("div.pageContainer").hide();

      //special stuff
      if (page == "login") {
        //hide our nav bar
        $("#navbar").hide();

        //enter triggers login
        $(document).on('keypress', function (e) {
          if (e.which == 13)
            do_login();
        });
      }

      //sad to see you go.
      if (page == "logout") {
        Cookies.remove("username");
        Cookies.remove("password");

        app_role = default_app_role;
        update_role_ui();

        YB.client.logout();

        if (app_role == "nobody")
          YB.App.openPage("login");
        else
          YB.App.openPage("control");
      }
      else {
        //update our nav
        $('#controlNav .nav-link').removeClass("active");
        $(`#${page}Nav a`).addClass("active");

        //is our new page ready?
        on_page_ready();
      }
    },


    isMFD: function () {
      if (getQueryVariable("mfd_name") !== null)
        return true;

      return false;
    },

  };

  // expose to global
  global.YB = YB;
})(this);