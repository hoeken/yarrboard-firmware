const YarrboardClient = window.YarrboardClient;

//let socket;
let client;
let current_page = null;
let current_config;
let app_username;
let app_password;
let app_role = "nobody";
let default_app_role = "nobody";
let app_update_interval = 500;
let network_config;
let app_config;

let ota_started = false;

const page_list = ["control", "config", "stats", "graphs", "network", "settings", "system"];
const page_ready = {
  "control": false,
  "config": false,
  "stats": false,
  "graphs": false,
  "network": false,
  "settings": false,
  "system": true,
  "login": true
};

const page_permissions = {
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
  "ERR_FLOWRATE_TIMEOUT": "Flowrate Timeout",
  "ERR_FLOWRATE_LOW": "Flowrate Low",
  "ERR_SALINITY_TIMEOUT": "Salinity Timeout",
  "ERR_SALINITY_HIGH": "Salinity High",
  "ERR_PRODUCTION_TIMEOUT": "Production Timeout",
  "ERR_MOTOR_TEMPERATURE_HIGH": "Motor Temperature High",
}

const brineomatic_gauge_setup = {
  "motor_temperature": {
    "thresholds": [50, 75, 100],
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
  "salinity": {
    "thresholds": [200, 300, 1500],
    "colors": [bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
  },
  "flowrate": {
    "thresholds": [20, 100, 180, 200, 250],
    "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.success, bootstrapColors.warning, bootstrapColors.danger]
  },
  "tank_level": {
    "thresholds": [10, 20, 100],
    "colors": [bootstrapColors.secondary, bootstrapColors.warning, bootstrapColors.success]
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
let salinityGauge;
let flowrateGauge;
let tankLevelGauge;

let temperatureChart;
let pressureChart;
let salinityChart;
let flowrateChart;
let tankLevelChart;

let lastChartUpdate = Date.now();
let timeData;
let motorTemperatureData;
let waterTemperatureData;
let filterPressureData;
let membranePressureData;
let salinityData;
let flowrateData;
let tankLevelData;

const BoardNameEdit = (name) => `
<div class="col-12">
  <h4>Board Name</h4>
  <input type="text" class="form-control" id="fBoardName" value="${name}">
  <div class="valid-feedback">Saved!</div>
  <div class="invalid-feedback">Must be 30 characters or less.</div>
</div>
`;

const PWMControlCard = (ch) => `
<div id="pwm${ch.id}" class="col-xs-12 col-sm-6">
  <table class="w-100 h-100 p-2">
    <tr>
      <td id="pwmDutySliderButton${ch.id}" onclick="toggle_duty_cycle(${ch.id})" class="pwmDutySliderButton text-center" style="display: none">
        <svg class="bi bi-brightness-high" xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
          <path d="M8 11a3 3 0 1 1 0-6 3 3 0 0 1 0 6m0 1a4 4 0 1 0 0-8 4 4 0 0 0 0 8M8 0a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-1 0v-2A.5.5 0 0 1 8 0m0 13a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-1 0v-2A.5.5 0 0 1 8 13m8-5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1 0-1h2a.5.5 0 0 1 .5.5M3 8a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1 0-1h2A.5.5 0 0 1 3 8m10.657-5.657a.5.5 0 0 1 0 .707l-1.414 1.415a.5.5 0 1 1-.707-.708l1.414-1.414a.5.5 0 0 1 .707 0m-9.193 9.193a.5.5 0 0 1 0 .707L3.05 13.657a.5.5 0 0 1-.707-.707l1.414-1.414a.5.5 0 0 1 .707 0zm9.193 2.121a.5.5 0 0 1-.707 0l-1.414-1.414a.5.5 0 0 1 .707-.707l1.414 1.414a.5.5 0 0 1 0 .707M4.464 4.465a.5.5 0 0 1-.707 0L2.343 3.05a.5.5 0 1 1 .707-.707l1.414 1.414a.5.5 0 0 1 0 .708z"></path>
        </svg>
        <div class="mt-1 pwmDutyCycle" id="pwmDutyCycle${ch.id}">???</div>
      </td>
      <td>
        <button id="pwmState${ch.id}" type="button" class="btn pwmButton text-center" onclick="toggle_state(${ch.id})">
          <table style="width: 100%">
            <tbody>
              <tr>
                <td class="pwmIcon text-center align-middle pe-2">
                  ${pwm_type_images[ch.type]}
                </td>
                <td class="text-center" style="width: 99%">
                  <div id="pwmName${ch.id}">${ch.name}</div>
                  <div id="pwmStatus${ch.id}"></div>
                </td>
                <td>
                  <div class="text-end pwmData">
                    <div id="pwmVoltage${ch.id}"></div>
                    <div id="pwmCurrent${ch.id}"></div>
                    <div id="pwmWattage${ch.id}"></div>
                  </div>
                </td>
              </tr>
            </tbody>
          </table>
        </button>
      </td>
    </tr>
    <tr id="pwmDutySliderRow${ch.id}" class="align-top" style="display:none">
      <td colspan="2">
        <input type="range" class="form-range" min="0" max="100" id="pwmDutySlider${ch.id}">
      </td>
    </tr>
  </table>
</div>
`;

const PWMEditCard = (ch) => `
<div id="pwmEditCard${ch.id}" class="col-xs-12 col-sm-6">
  <div class="p-3 border border-secondary rounded">
    <h5>Output Channel #${ch.id}</h5>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fPWMName${ch.id}" value="${ch.name}">
      <label for="fPWMName${ch.id}">Name</label>
    </div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
    <div class="form-check form-switch mb-3">
      <input class="form-check-input" type="checkbox" id="fPWMEnabled${ch.id}">
      <label class="form-check-label" for="fPWMEnabled${ch.id}">
        Enabled
      </label>
    </div>
    <div class="form-check form-switch mb-3">
      <input class="form-check-input" type="checkbox" id="fPWMDimmable${ch.id}">
      <label class="form-check-label" for="fPWMDimmable${ch.id}">
        Dimmable?
      </label>
    </div>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fPWMSoftFuse${ch.id}" value="${ch.softFuse}">
      <label for="fPWMSoftFuse${ch.id}">Soft Fuse (Amps)</label>
    </div>
    <div class="invalid-feedback">Must be a number between 0 and 20</div>
    <div class="form-floating mb-3">
      <select id="fPWMDefaultState${ch.id}" class="form-select" aria-label="Default State (on boot)">
        <option value="ON">ON</option>
        <option value="OFF">OFF</option>
      </select>
      <label for="fPWMDefaultState${ch.id}">Default State (on boot)</label>
    </div>
    <div class="form-floating">
      <select id="fPWMType${ch.id}" class="form-select" aria-label="Output Type">
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
      <label for="fPWMType${ch.id}">Output Type</label>
    </div>
  </div>
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

const SwitchControlRow = (id, name) => `
<tr id="switch${id}" class="switchRow">
  <td class="text-center"><button id="switchState${id}" type="button" class="btn btn-sm" style="width: 80px"></button></td>
  <td class="switchName align-middle">${name}</td>
</tr>
`;

const SwitchEditCard = (ch) => `
<div id="switchEditCard${ch.id}" class="col-xs-12 col-sm-6">
  <div class="p-3 border border-secondary rounded">
    <h5>Switch Input #${ch.id}</h5>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fSwitchName${ch.id}" value="${ch.name}">
      <label for="fSwitchName${ch.id}">Name</label>
      <div class="invalid-feedback">Must be 30 characters or less.</div>
    </div>
    <div class="mb-3">
      <div class="form-check form-switch">
        <input class="form-check-input" type="checkbox" id="fSwitchEnabled${ch.id}">
        <label class="form-check-label" for="fSwitchEnabled${ch.id}">Enabled</label>
      </div>
    </div>
    <div class="mb-3">
      <div class="form-floating">
        <select id="fSwitchMode${ch.id}" class="form-select" aria-label="Switch Mode">
          <option value="direct">Direct</option>
          <option value="inverting">Inverting</option>
          <option value="toggle_rising">Toggle Rising</option>
          <option value="toggle_falling">Toggle Falling</option>
        </select>
        <label for="fSwitchMode${ch.id}">Switch Mode</label>
      </div>
    </div>
    <div class="form-floating">
      <select id="fSwitchDefaultState${ch.id}" class="form-select" aria-label="Default State">
        <option value="ON">ON</option>
        <option value="OFF">OFF</option>
      </select>
      <label for="fSwitchDefaultState${ch.id}">Default State (on boot)</label>
    </div>
  </div>
</div>
`;

const RGBControlRow = (id, name) => `
<tr id="rgb${id}" class="rgbRow">
  <td class="text-center"><input id="rgbPicker${id}" type="text"></td>
  <td class="rgbName align-middle">${name}</td>
</tr>
`;

const RGBEditCard = (ch) => `
<div id="rgbEditCard${ch.id}" class="col-xs-12 col-sm-6">
  <div class="p-3 border border-secondary rounded">
    <h5>RGB #${ch.id}</h5>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fRGBName${ch.id}" value="${ch.name}">
      <label for="fRGBName${ch.id}">Name</label>
      <div class="invalid-feedback">Must be 30 characters or less.</div>
    </div>
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fRGBEnabled${ch.id}">
      <label class="form-check-label" for="fRGBEnabled${ch.id}">Enabled</label>
    </div>
  </div>
</div>
`;

const ADCControlRow = (id, name) => `
<tr id="adc${id}" class="adcRow" onclick="toggle_adc_details(${id})">
  <td class="adcName align-middle">${name}</td>
  <td class="adcBar align-middle">
    <div id="adcBar${id}" class="progress" role="progressbar" aria-label="ADC ${id} Reading" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100">
      <div class="progress-bar" style="width: 0%"></div>
    </div>
  </td>
</tr>
<tr id="adc${id}Details" style="display: none">
  <td colspan="2">
    <table style="width: 100%">
      <thead>
          <tr>
              <th class="text-center" scope="col" style="width: 33%">Raw</th>
              <th class="text-center" scope="col" style="width: 33%">Voltage</th>
              <th class="text-center" scope="col" style="width: 33%">%</th>
          </tr>
      </thead>
      <tbody>
        <tr>
          <td class="text-center" id="adcReading${id}"></td>
          <td class="text-center" id="adcVoltage${id}"></td>
          <td class="text-center" id="adcPercentage${id}"></td>
        </tr>
      </tbody>
    </table>
  </td>
</tr>
`;

const ADCEditCard = (id) => `
<div id="adcEditCard${ch.id}" class="col-xs-12 col-sm-6">
  <div class="p-3 border border-secondary rounded">
    <h5>ADC #${ch.id}</h5>
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fADCName${ch.id}" value="${ch.name}">
      <label for="fRGBName${ch.id}">Name</label>
      <div class="invalid-feedback">Must be 30 characters or less.</div>
    </div>
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fADCEnabled${ch.id}">
      <label class="form-check-label" for="fADCEnabled${ch.id}">Enabled</label>
    </div>
</div>
`;

const AlertBox = (message, type) => `
<div>
  <div class="mt-3 alert alert-${type} alert-dismissible" role="alert">
    <div>${message}</div>
    <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
  </div>
</div>`;

let currentPWMSliderID = -1;
let currentServoSliderID = -1;
let currentRGBPickerID = -1;
let currentlyPickingBrightness = false;

function start_yarrboard() {
  yarrboard_log("User Agent: " + navigator.userAgent);
  yarrboard_log("Window Width: " + window.innerWidth);
  yarrboard_log("Window Height: " + window.innerHeight);
  yarrboard_log("Window Location: " + window.location);
  yarrboard_log("Is canvas supported? " + isCanvasSupported());

  //main data connection
  start_websocket();

  //check our connection status.
  setInterval(check_connection_status, 100);

  //light/dark theme init.
  setTheme(getPreferredTheme());
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
}

function load_configs() {
  if (app_role == "nobody")
    return;

  if (app_role == "admin") {
    client.getNetworkConfig();
    client.getAppConfig();
    check_for_updates();
  }

  client.getConfig();
}

function check_connection_status() {
  if (client) {
    let status = client.status();

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
  client = new YarrboardClient(window.location.host, "", "", false, use_ssl);

  client.onopen = function () {
    //figure out what our login situation is
    client.sayHello();
  };

  client.onmessage = function (msg) {
    if (msg.debug)
      yarrboard_log(`SERVER: ${msg.debug}`);
    else if (msg.msg == 'hello') {
      app_role = msg.role;
      default_app_role = msg.default_role;

      //light/dark mode
      if (msg.theme)
        setTheme(msg.theme);

      //auto login?
      if (Cookies.get("username") && Cookies.get("password")) {
        client.login(Cookies.get("username"), Cookies.get("password"));
      } else {
        update_role_ui();
        load_configs();
        open_default_page();
      }
    }
    else if (msg.msg == 'config') {
      // yarrboard_log("config");
      // yarrboard_log(msg);

      current_config = msg;

      //is it our first boot?
      if (msg.first_boot && current_page != "network")
        show_alert(`Welcome to Yarrboard, head over to <a href="#network" onclick="open_page('network')">Network</a> to setup your WiFi.`, "primary");

      //did we get a crash?
      if (msg.has_coredump)
        show_alert(`
          <p>Oops, looks like Yarrboard crashed.</p>
          <p>Please download the <a href="/coredump.txt" target="_blank">coredump</a> and report it to our <a href="https://github.com/hoeken/yarrboard/issues">Github Issue Tracker</a> along with the following information:</p>
          <ul><li>Firmware: ${msg.firmware_version}</li><li>Hardware: ${msg.hardware_version}</li></ul>
        `, "danger");

      //let the people choose their own names!
      $('#boardName').html(msg.name);
      document.title = msg.name;

      //update our footer automatically.
      $('#projectName').html("Yarrboard v" + msg.firmware_version);

      //all our versions
      $("#firmware_version").html(msg.firmware_version);
      $("#hardware_version").html(msg.hardware_version);
      $("#esp_idf_version").html(msg.esp_idf_version);
      $("#arduino_version").html(msg.arduino_version);
      $("#yarrboard_client_version").html(YarrboardClient.version);

      //show some info about restarts
      if (msg.last_restart_reason)
        $("#last_reboot_reason").html(msg.last_restart_reason);
      else
        $("#last_reboot_reason").parent().hide();

      //populate our pwm control table
      $('#controlDiv').hide();
      $('#pwmStatsDiv').hide();

      //do we have pwm channels?
      if (msg.pwm) {
        //fresh slate
        $('#pwmCards').html("");
        for (ch of msg.pwm) {
          if (ch.enabled) {
            $('#pwmCards').append(PWMControlCard(ch));

            if (ch.isDimmable) {
              $(`#pwmDutySliderButton${ch.id}`).show();

              $('#pwmDutySlider' + ch.id).change(set_duty_cycle);

              //update our duty when we move
              $('#pwmDutySlider' + ch.id).on("input", set_duty_cycle);

              //stop updating the UI when we are choosing a duty
              $('#pwmDutySlider' + ch.id).on('focus', function (e) {
                let ele = e.target;
                let id = ele.id.match(/\d+/)[0];
                currentPWMSliderID = id;
              });

              //stop updating the UI when we are choosing a duty
              $('#pwmDutySlider' + ch.id).on('touchstart', function (e) {
                let ele = e.target;
                let id = ele.id.match(/\d+/)[0];
                currentPWMSliderID = id;
              });

              //restart the UI updates when slider is closed
              $('#pwmDutySlider' + ch.id).on("blur", function (e) {
                currentPWMSliderID = -1;
              });

              //restart the UI updates when slider is closed
              $('#pwmDutySlider' + ch.id).on("touchend", function (e) {
                currentPWMSliderID = -1;
              });
            }
          }
        }

        //$('#pwmCards').append(PWMLegendCard());

        $('#pwmStatsTableBody').html("");
        for (ch of msg.pwm) {
          if (ch.enabled) {
            $('#pwmStatsTableBody').append(`<tr id="pwmStats${ch.id}"></tr>`);
            $('#pwmStats' + ch.id).append(`<td class="pwmName">${ch.name}</td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmAmpHours${ch.id}" class="text-end"></td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmWattHours${ch.id}" class="text-end"></td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmOnCount${ch.id}" class="text-end"></td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmTripCount${ch.id}" class="text-end"></td>`);
          }
        }

        $('#pwmStatsTableBody').append(`<tr id="pwmStatsTotal"></tr>`);
        $('#pwmStatsTotal').append(`<th class="pwmName">Total</th>`);
        $('#pwmStatsTotal').append(`<th id="pwmAmpHoursTotal" class="text-end"></th>`);
        $('#pwmStatsTotal').append(`<th id="pwmWattHoursTotal" class="text-end"></th>`);
        $('#pwmStatsTotal').append(`<th id="pwmOnCountTotal" class="text-end"></th>`);
        $('#pwmStatsTotal').append(`<th id="pwmTripCountTotal" class="text-end"></th>`);

        $('#controlDiv').show();
        $('#pwmStatsDiv').show();
      }

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

      //populate our switch control table
      $('#switchControlDiv').hide();
      $('#switchStatsDiv').hide();
      if (msg.switches) {
        $('#switchTableBody').html("");
        for (ch of msg.switches) {
          if (ch.enabled)
            $('#switchTableBody').append(SwitchControlRow(ch.id, ch.name));
        }

        $('#switchStatsTableBody').html("");
        for (ch of msg.switches) {
          if (ch.enabled) {
            $('#switchStatsTableBody').append(`<tr id="switchStats${ch.id}"></tr>`);
            $('#switchStats' + ch.id).append(`<td class="switchName">${ch.name}</td>`);
            $('#switchStats' + ch.id).append(`<td id="switchOnCount${ch.id}" class="text-end"></td>`);
          }
        }

        $('#switchControlDiv').show();
        $('#switchStatsDiv').show();
      }

      //populate our rgb control table
      $('#rgbControlDiv').hide();
      if (msg.rgb) {
        $('#rgbTableBody').html("");
        for (ch of msg.rgb) {
          if (ch.enabled) {
            $('#rgbTableBody').append(RGBControlRow(ch.id, ch.name));

            //init our color picker
            $('#rgbPicker' + ch.id).spectrum({
              color: "#000",
              showPalette: true,
              palette: [
                ["#000", "#444", "#666", "#999", "#ccc", "#eee", "#f3f3f3", "#fff"],
                ["#f00", "#f90", "#ff0", "#0f0", "#0ff", "#00f", "#90f", "#f0f"],
                ["#f4cccc", "#fce5cd", "#fff2cc", "#d9ead3", "#d0e0e3", "#cfe2f3", "#d9d2e9", "#ead1dc"],
                ["#ea9999", "#f9cb9c", "#ffe599", "#b6d7a8", "#a2c4c9", "#9fc5e8", "#b4a7d6", "#d5a6bd"],
                ["#e06666", "#f6b26b", "#ffd966", "#93c47d", "#76a5af", "#6fa8dc", "#8e7cc3", "#c27ba0"],
                ["#c00", "#e69138", "#f1c232", "#6aa84f", "#45818e", "#3d85c6", "#674ea7", "#a64d79"],
                ["#900", "#b45f06", "#bf9000", "#38761d", "#134f5c", "#0b5394", "#351c75", "#741b47"],
                ["#600", "#783f04", "#7f6000", "#274e13", "#0c343d", "#073763", "#20124d", "#4c1130"]
              ]
            });

            //update our color on change
            $('#rgbPicker' + ch.id).change(set_rgb_color);

            //update our color when we move
            $('#rgbPicker' + ch.id).on("move.spectrum", set_rgb_color);

            //stop updating the UI when we are choosing a color
            $('#rgbPicker' + ch.id).on('show.spectrum', function (e) {
              let ele = e.target;
              let id = ele.id.match(/\d+/)[0];
              currentRGBPickerID = id;
            });

            //restart the UI updates when picker is closed
            $('#rgbPicker' + ch.id).on("hide.spectrum", function (e) {
              currentRGBPickerID = -1;
            });
          }
        }

        $('#rgbControlDiv').show();
      }

      //populate our adc control table
      $('#adcControlDiv').hide();
      if (msg.adc) {
        $('#adcTableBody').html("");
        for (ch of msg.adc) {
          if (ch.enabled)
            $('#adcTableBody').append(ADCControlRow(ch.id, ch.name));
        }

        $('#adcControlDiv').show();
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

        if (!isMFD()) {

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
            size: { height: 130 },
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
            size: { height: 130 },
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
            size: { height: 130 },
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
            size: { height: 130 },
            interaction: { enabled: false },
            transition: { duration: 0 },
            legend: { hide: true }
          });

          salinityGauge = c3.generate({
            bindto: '#salinityGauge',
            data: {
              columns: [
                ['Salinity', 0]
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
              pattern: brineomatic_gauge_setup.salinity.colors,
              threshold: {
                unit: 'value',
                values: brineomatic_gauge_setup.salinity.thresholds
              }
            },
            size: { height: 130 },
            interaction: { enabled: false },
            transition: { duration: 0 },
            legend: { hide: true }
          });

          flowrateGauge = c3.generate({
            bindto: '#flowrateGauge',
            data: {
              columns: [
                ['Flowrate', 0]
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
              pattern: brineomatic_gauge_setup.flowrate.colors,
              threshold: {
                unit: 'value',
                values: brineomatic_gauge_setup.flowrate.thresholds
              }
            },
            size: { height: 130 },
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
            size: { height: 130 },
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
          salinityData = ['Salinity'];
          flowrateData = ['Flowrate'];
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

          salinityChart = c3.generate({
            bindto: '#salinityChart',
            data: {
              x: 'x', // Define the x-axis data identifier
              xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
              columns: [
                timeData,
                salinityData
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
                label: 'Salinity (PPM))',
                min: 0,
                max: 1500
              }
            },
            point: { show: false },
            interaction: { enabled: true },
            transition: { duration: 0 }
          });

          flowrateChart = c3.generate({
            bindto: '#flowrateChart',
            data: {
              x: 'x', // Define the x-axis data identifier
              xFormat: '%Y-%m-%d %H:%M:%S.%L', // Format for parsing x-axis data including milliseconds
              columns: [
                timeData,
                flowrateData
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
                label: 'Flowrate (LPH))',
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

      //only do it as needed
      if (!page_ready.config || current_page != "config") {
        //populate our pwm edit table
        $('#boardConfigForm').html(BoardNameEdit(msg.name));

        //validate + save control
        $("#fBoardName").change(validate_board_name);

        //edit controls for each pwm
        $('#pwmConfig').hide();
        if (msg.pwm) {
          $('#pwmConfigForm').html("");
          for (ch of msg.pwm) {
            $('#pwmConfigForm').append(PWMEditCard(ch));
            $(`#fPWMDimmable${ch.id}`).prop("checked", ch.isDimmable);
            $(`#fPWMEnabled${ch.id}`).prop("checked", ch.enabled);
            $(`#fPWMType${ch.id}`).val(ch.type);
            $(`#fPWMDefaultState${ch.id}`).val(ch.defaultState);

            //enable/disable other stuff.
            $(`#fPWMName${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMDimmable${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMSoftFuse${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMType${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMDefaultState${ch.id}`).prop('disabled', !ch.enabled);

            //validate + save
            $(`#fPWMEnabled${ch.id}`).change(validate_pwm_enabled);
            $(`#fPWMName${ch.id}`).change(validate_pwm_name);
            $(`#fPWMDimmable${ch.id}`).change(validate_pwm_dimmable);
            $(`#fPWMSoftFuse${ch.id}`).change(validate_pwm_soft_fuse);
            $(`#fPWMType${ch.id}`).change(validate_pwm_type);
            $(`#fPWMDefaultState${ch.id}`).change(validate_pwm_default_state);
          }
          $('#pwmConfig').show();
        }

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

        //edit controls for each switch
        $('#switchConfig').hide();
        if (msg.switches) {
          $('#switchConfigForm').html("");
          for (ch of msg.switches) {
            $('#switchConfigForm').append(SwitchEditCard(ch));
            $(`#fSwitchEnabled${ch.id}`).prop("checked", ch.enabled);
            $(`#fSwitchMode${ch.id}`).val(ch.mode.toLowerCase());
            $(`#fSwitchDefaultState${ch.id}`).val(ch.defaultState);

            //enable/disable other stuff.
            $(`#fSwitchName${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fSwitchMode${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fSwitchDefaultState${ch.id}`).prop('disabled', !ch.enabled);

            //validate + save
            $(`#fSwitchEnabled${ch.id}`).change(validate_switch_enabled);
            $(`#fSwitchName${ch.id}`).change(validate_switch_name);
            $(`#fSwitchMode${ch.id}`).change(validate_switch_mode);
            $(`#fSwitchDefaultState${ch.id}`).change(validate_switch_default_state);
          }
          $('#switchConfig').show();
        }

        //edit controls for each rgb
        $('#rgbConfig').hide();
        if (msg.rgb) {
          $('#rgbConfigForm').html("");
          for (ch of msg.rgb) {
            $('#rgbConfigForm').append(RGBEditCard(ch));
            $(`#fRGBEnabled${ch.id}`).prop("checked", ch.enabled);

            //enable/disable other stuff.
            $(`#fRGBName${ch.id}`).prop('disabled', !ch.enabled);

            //validate + save
            $(`#fRGBEnabled${ch.id}`).change(validate_rgb_enabled);
            $(`#fRGBName${ch.id}`).change(validate_rgb_name);
          }
          $('#rgbConfig').show();
        }

        //edit controls for each rgb
        $('#adcConfig').hide();
        if (msg.adc) {
          $('#adcConfigForm').html("");
          for (ch of msg.adc) {
            $('#adcConfigForm').append(ADCEditCard(ch));
            $(`#fADCEnabled${ch.id}`).prop("checked", ch.enabled);

            //enable/disable other stuff.
            $(`#fADCName${ch.id}`).prop('disabled', !ch.enabled);

            //validate + save
            $(`#fADCEnabled${ch.id}`).change(validate_adc_enabled);
            $(`#fADCName${ch.id}`).change(validate_adc_name);
          }

          $('#adcConfig').show();
        }
      }

      //did we get brightness?
      if (msg.brightness && !currentlyPickingBrightness)
        $('#brightnessSlider').val(Math.round(msg.brightness * 100));

      if (isMFD()) {
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
        open_page('control');
    }
    else if (msg.msg == 'update') {
      // yarrboard_log("update");
      // yarrboard_log(msg);

      //we need a config loaded.
      if (!current_config)
        return;

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
      //   $("#uptime").html(secondsToDhms(Math.round(msg.uptime/1000000)));

      //or maybe voltage
      // if (msg.bus_voltage)
      // {
      //   $('#bus_voltage_main').html("Bus Voltage: " + msg.bus_voltage.toFixed(2) + "V");
      //   $('#bus_voltage_main').show();
      // }
      // else
      //   $('#bus_voltage_main').hide();

      //our pwm info
      if (msg.pwm) {
        for (ch of msg.pwm) {
          if (current_config.pwm[ch.id].enabled) {
            if (ch.state == "ON") {
              $('#pwmState' + ch.id).addClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-primary");
              $('#pwmState' + ch.id).removeClass("btn-warning");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
              $(`#pwmStatus${ch.id}`).hide();
            }
            else if (ch.state == "TRIPPED") {
              $('#pwmState' + ch.id).addClass("btn-warning");
              $('#pwmState' + ch.id).removeClass("btn-primary");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
              $(`#pwmStatus${ch.id}`).html("SOFT TRIP");
              $(`#pwmStatus${ch.id}`).show();
            }
            else if (ch.state == "BLOWN") {
              $('#pwmState' + ch.id).addClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-primary");
              $('#pwmState' + ch.id).removeClass("btn-warning");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
              $(`#pwmStatus${ch.id}`).html("FUSE BLOWN");
              $(`#pwmStatus${ch.id}`).show();
            }
            else if (ch.state == "BYPASSED") {
              $('#pwmState' + ch.id).addClass("btn-primary");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-warning");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
              $(`#pwmStatus${ch.id}`).html("BYPASSED");
              $(`#pwmStatus${ch.id}`).show();
            }
            else if (ch.state == "OFF") {
              $('#pwmState' + ch.id).addClass("btn-secondary");
              $('#pwmState' + ch.id).removeClass("btn-primary");
              $('#pwmState' + ch.id).removeClass("btn-warning");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $(`#pwmStatus${ch.id}`).hide();
            }

            //duty is a bit of a special case.
            let duty = Math.round(ch.duty * 100);
            if (current_config.pwm[ch.id].isDimmable) {
              if (currentPWMSliderID != ch.id) {
                $('#pwmDutySlider' + ch.id).val(duty);
                $('#pwmDutyCycle' + ch.id).html(`${duty}%`);
                $('#pwmDutyCycle' + ch.id).show();
              }
            }
            else {
              $('#pwmDutyCycle' + ch.id).hide();
            }

            let voltage = ch.voltage.toFixed(1);
            let voltageHTML = `${voltage}V`;
            $('#pwmVoltage' + ch.id).html(voltageHTML);

            let current = ch.current.toFixed(2);
            let currentHTML = `${current}A`;
            $('#pwmCurrent' + ch.id).html(currentHTML);

            let wattage = 0;
            if (ch.voltage) {
              wattage = ch.voltage * ch.current;
              wattage = wattage.toFixed(0);
              $('#pwmWattage' + ch.id).html(`${wattage}W`);
            } else if (msg.bus_voltage) {
              wattage = ch.current * msg.bus_voltage;
              wattage = wattage.toFixed(0);
              $('#pwmWattage' + ch.id).html(`${wattage}W`);
            }
            else
              $('#pwmWattage' + ch.id).hide();
          }
        }
      }

      //our relay info
      if (msg.relay) {
        for (ch of msg.relay) {
          if (current_config.relay[ch.id].enabled) {
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
          if (current_config.servo[ch.id].enabled) {
            if (currentServoSliderID != ch.id) {
              $('#servoSlider' + ch.id).val(ch.angle);
              $('#servoAngle' + ch.id).html(`${ch.angle}°`);
            }
          }
        }
      }

      //our switch info
      if (msg.switches) {
        for (ch of msg.switches) {
          if (current_config.switches[ch.id].enabled) {
            if (ch.state == "ON") {
              $('#switchState' + ch.id).html("ON");
              $('#switchState' + ch.id).removeClass("btn-secondary");
              $('#switchState' + ch.id).addClass("btn-success");
            }
            else if (ch.state == "OFF") {
              $('#switchState' + ch.id).html("OFF");
              $('#switchState' + ch.id).removeClass("btn-success");
              $('#switchState' + ch.id).addClass("btn-secondary");
            }
          }
        }
      }

      //our rgb info
      if (msg.rgb) {
        for (ch of msg.rgb) {
          if (current_config.rgb[ch.id].enabled && currentRGBPickerID != ch.id) {
            let _red = Math.round(255 * ch.red);
            let _green = Math.round(255 * ch.green);
            let _blue = Math.round(255 * ch.blue);

            $("#rgbPicker" + ch.id).spectrum("set", `rgb(${_red}, ${_green}, ${_blue}`);
          }
        }
      }

      //our adc info
      if (msg.adc) {
        for (ch of msg.adc) {
          if (current_config.adc[ch.id].enabled) {
            let reading = Math.round(ch.reading);
            let voltage = ch.voltage.toFixed(2);
            let percentage = ch.percentage.toFixed(1);

            $("#adcReading" + ch.id).html(reading);
            $("#adcVoltage" + ch.id).html(voltage + "V")
            $("#adcPercentage" + ch.id).html(percentage + "%")

            $(`#adcBar${ch.id} div`).css("width", percentage + "%");
            $(`#adcBar${ch.id}`).attr("aria-valuenow", percentage);
          }
        }
      }

      if (msg.brineomatic) {
        let motor_temperature = Math.round(msg.motor_temperature);
        let water_temperature = Math.round(msg.water_temperature);
        let flowrate = Math.round(msg.flowrate);
        let volume = msg.volume.toFixed(1);
        let salinity = Math.round(msg.salinity);
        let filter_pressure = Math.round(msg.filter_pressure);
        if (filter_pressure < 0 && filter_pressure > -10)
          filter_pressure = 0;
        let membrane_pressure = Math.round(msg.membrane_pressure);
        if (membrane_pressure < 0 && membrane_pressure > -10)
          membrane_pressure = 0;
        let tank_level = Math.round(msg.tank_level * 100);

        //update our gauges.
        if (!isMFD()) {
          if (current_page == "control") {
            motorTemperatureGauge.load({ columns: [['Motor Temperature', motor_temperature]] });
            waterTemperatureGauge.load({ columns: [['Water Temperature', water_temperature]] });
            filterPressureGauge.load({ columns: [['Filter Pressure', filter_pressure]] });
            membranePressureGauge.load({ columns: [['Membrane Pressure', membrane_pressure]] });
            salinityGauge.load({ columns: [['Salinity', salinity]] });
            flowrateGauge.load({ columns: [['Flowrate', flowrate]] });
            tankLevelGauge.load({ columns: [['Tank Level', tank_level]] });
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

              salinityChart.flow({
                columns: [
                  ['x', formattedTime],
                  ['Salinity', salinity]
                ]
              });

              flowrateChart.flow({
                columns: [
                  ['x', formattedTime],
                  ['Flowrate', flowrate]
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

          $("#salinityData").html(salinity);
          bom_set_data_color("salinity", salinity, $("#salinityData"));

          $("#flowrateData").html(flowrate);
          bom_set_data_color("flowrate", flowrate, $("#flowrateData"));

          $("#motorTemperatureData").html(motor_temperature);
          bom_set_data_color("motor_temperature", motor_temperature, $("#motorTemperatureData"));

          $("#waterTemperatureData").html(water_temperature);
          bom_set_data_color("water_temperature", water_temperature, $("#waterTemperatureData"));

          $("#tankLevelData").html(tank_level);
          bom_set_data_color("tank_level", tank_level, $("#tankLevelData"));
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
          $("#bomNextFlushCountdownData").html(secondsToDhms(Math.round(msg.next_flush_countdown / 1000000)));
        else
          $("#bomNextFlushCountdown").hide();

        if (msg.runtime_elapsed > 0)
          $("#bomRuntimeElapsedData").html(secondsToDhms(Math.round(msg.runtime_elapsed / 1000000)));
        else
          $("#bomRuntimeElapsed").hide();

        if (msg.finish_countdown > 0)
          $("#bomFinishCountdownData").html(secondsToDhms(Math.round(msg.finish_countdown / 1000000)));
        else
          $("#bomFinishCountdown").hide();

        if (msg.flush_elapsed > 0)
          $("#bomFlushElapsedData").html(secondsToDhms(Math.round(msg.flush_elapsed / 1000000)));
        else
          $("#bomFlushElapsed").hide();

        if (msg.flush_countdown > 0)
          $("#bomFlushCountdownData").html(secondsToDhms(Math.round(msg.flush_countdown / 1000000)));
        else
          $("#bomFlushCountdown").hide();

        if (msg.pickle_elapsed > 0)
          $("#bomPickleElapsedData").html(secondsToDhms(Math.round(msg.pickle_elapsed / 1000000)));
        else
          $("#bomPickleElapsed").hide();

        if (msg.pickle_countdown > 0)
          $("#bomPickleCountdownData").html(secondsToDhms(Math.round(msg.pickle_countdown / 1000000)));
        else
          $("#bomPickleCountdown").hide();

        if (msg.depickle_elapsed > 0)
          $("#bomDepickleElapsedData").html(secondsToDhms(Math.round(msg.depickle_elapsed / 1000000)));
        else
          $("#bomDepickleElapsed").hide();

        if (msg.depickle_countdown > 0)
          $("#bomDepickleCountdownData").html(secondsToDhms(Math.round(msg.depickle_countdown / 1000000)));
        else
          $("#bomDepickleCountdown").hide();

        if (volume > 0)
          $("#bomVolumeData").html(`${volume}L`);
        else
          $("#bomVolume").hide();

        if (current_config.has_boost_pump) {
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

        if (current_config.has_high_pressure_pump) {
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

        if (current_config.has_diverter_valve) {
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

        if (current_config.has_flush_valve) {
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

        if (current_config.has_cooling_fan) {
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

      if (isMFD()) {
        $(".mfdShow").show()
        $(".mfdHide").hide()
      }
      else {
        $(".mfdShow").hide()
        $(".mfdHide").show()
      }

      page_ready.control = true;
    }
    else if (msg.msg == "stats") {
      //yarrboard_log("stats");

      //we need this
      if (!current_config)
        return;

      $("#uptime").html(secondsToDhms(Math.round(msg.uptime / 1000000)));
      if (msg.fps)
        $("#fps").html(msg.fps.toLocaleString("en-US") + " lps");

      //message info
      $("#messages").html(msg.received_message_mps.toLocaleString("en-US") + " mps");
      $("#received_message_mps").html(msg.received_message_mps.toLocaleString("en-US") + " mps");
      $("#received_message_total").html(msg.received_message_total.toLocaleString("en-US"));
      $("#sent_message_mps").html(msg.sent_message_mps.toLocaleString("en-US") + " mps");
      $("#sent_message_total").html(msg.sent_message_total.toLocaleString("en-US"));

      //client info
      $("#total_client_count").html(msg.http_client_count + msg.websocket_client_count);
      $("#http_client_count").html(msg.http_client_count);
      $("#websocket_client_count").html(msg.websocket_client_count);

      //raw data
      $("#heap_size").html(formatBytes(msg.heap_size, 0));
      $("#free_heap").html(formatBytes(msg.free_heap, 0));
      $("#min_free_heap").html(formatBytes(msg.min_free_heap, 0));
      $("#max_alloc_heap").html(formatBytes(msg.max_alloc_heap, 0));

      //our memory bar
      let memory_used = Math.round((msg.heap_size / (msg.heap_size + msg.free_heap)) * 100);
      $(`#memory_usage div`).css("width", memory_used + "%");
      $(`#memory_usage div`).html(formatBytes(msg.heap_size, 0));
      $(`#memory_usage`).attr("aria-valuenow", memory_used);

      $("#rssi").html(msg.rssi + "dBm");
      $("#uuid").html(msg.uuid);
      $("#ip_address").html(msg.ip_address);

      if (msg.bus_voltage)
        $("#bus_voltage").html(msg.bus_voltage.toFixed(1) + "V");
      else
        $("#bus_voltage_row").remove();

      if (msg.fans)
        $("#fan_rpm").html(msg.fans.map((a) => a.rpm + "RPM").join(", "));
      else
        $("#fan_rpm_row").remove();

      if (msg.pwm) {
        let total_ah = 0.0;
        let total_wh = 0.0;
        let total_on_count = 0;
        let total_trip_count = 0;

        for (ch of msg.pwm) {
          if (current_config.pwm[ch.id].enabled) {
            $('#pwmAmpHours' + ch.id).html(formatAmpHours(ch.aH));
            $('#pwmWattHours' + ch.id).html(formatWattHours(ch.wH));
            $('#pwmOnCount' + ch.id).html(ch.state_change_count.toLocaleString("en-US"));
            $('#pwmTripCount' + ch.id).html(ch.soft_fuse_trip_count.toLocaleString("en-US"));

            total_ah += parseFloat(ch.aH);
            total_wh += parseFloat(ch.wH);
            total_on_count += parseInt(ch.state_change_count);
            total_trip_count += parseInt(ch.soft_fuse_trip_count);
          }
        }

        $('#pwmAmpHoursTotal').html(formatAmpHours(total_ah));
        $('#pwmWattHoursTotal').html(formatWattHours(total_wh));
        $('#pwmOnCountTotal').html(total_on_count.toLocaleString("en-US"));
        $('#pwmTripCountTotal').html(total_trip_count.toLocaleString("en-US"));
      }

      if (msg.switches) {
        for (ch of msg.switches) {
          if (current_config.switches[ch.id].enabled) {
            $('#switchOnCount' + ch.id).html(ch.state_change_count.toLocaleString("en-US"));
          }
        }
      }

      if (msg.brineomatic) {
        let totalVolume = Math.round(msg.total_volume);
        let totalRuntime = (msg.total_runtime / (60 * 60 * 1000000)).toFixed(1);
        $("#bomTotalCycles").html(msg.total_cycles);
        $("#bomTotalVolume").html(`${totalVolume}L`);
        $("#bomTotalRuntime").html(`${totalRuntime} hours`);
      }

      page_ready.stats = true;
    }
    else if (msg.msg == "graph_data") {
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

        if (msg.salinity) {
          salinityData = [salinityData[0]].concat(msg.salinity);

          salinityChart.load({
            columns: [
              timeData,
              salinityData
            ]
          });

        }

        if (msg.flowrate) {
          flowrateData = [flowrateData[0]].concat(msg.flowrate);

          flowrateChart.load({
            columns: [
              timeData,
              flowrateData
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
        get_update_data();

        page_ready.graphs = true;
      }
    }
    //load up our network config.
    else if (msg.msg == "network_config") {
      //yarrboard_log("network config");

      //save our config.
      network_config = msg;

      //yarrboard_log(msg);
      $("#wifi_mode").val(msg.wifi_mode);
      $("#wifi_ssid").val(msg.wifi_ssid);
      $("#wifi_pass").val(msg.wifi_pass);
      $("#local_hostname").val(msg.local_hostname);

      page_ready.network = true;
    }
    //load up our network config.
    else if (msg.msg == "app_config") {
      //yarrboard_log("network config");

      //save our config.
      app_config = msg;

      //update some ui stuff
      update_role_ui();
      toggle_role_passwords(msg.default_role);

      //what is our update interval?
      if (msg.app_update_interval)
        app_update_interval = msg.app_update_interval;

      //yarrboard_log(msg);
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

      page_ready.settings = true;
    }
    //load up our network config.
    else if (msg.msg == "ota_progress") {
      //yarrboard_log("ota progress");

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
      yarrboard_log(msg.message);

      //did we turn login off?
      if (msg.message == "Login not required.") {
        Cookies.remove("username");
        Cookies.remove("password");
      }

      //keep the u gotta login to the login page.
      if (msg.message == "You must be logged in.")
        open_page("login");
      else
        show_alert(msg.message);
    }
    else if (msg.msg == "login") {

      //keep the login success stuff on the login page.
      if (msg.message == "Login successful.") {
        //once we know our role, we can load our other configs.
        app_role = msg.role;

        // yarrboard_log(`app_role: ${app_role}`);
        // yarrboard_log(`current_page: ${current_page}`);

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
      yarrboard_log("[socket] Unknown message: " + JSON.stringify(msg));
    }
  };

  //use custom logger
  client.log = yarrboard_log;

  client.start();
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

function isMFD() {
  if (getQueryVariable("mfd_name") !== null)
    return true;

  return false;
}

function show_alert(message, type = 'danger') {
  //we only need one alert at a time.
  $('#liveAlertPlaceholder').html(AlertBox(message, type))

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

function manual_brineomatic() {
  $('#relayControlDiv').toggle();
  $('#servoControlDiv').toggle();
}

function start_brineomatic_manual() {
  client.send({
    "cmd": "start_watermaker",
  }, true);
}

function start_brineomatic_duration() {

  let duration = $("#bomRunDurationInput").val();

  if (duration > 0) {
    //hours to microseconds
    let micros = duration * 60 * 60 * 1000000;

    client.send({
      "cmd": "start_watermaker",
      "duration": micros
    }, true);
  }
}

function start_brineomatic_volume() {
  let volume = $("#bomRunVolumeInput").val();

  if (volume > 0) {
    client.send({
      "cmd": "start_watermaker",
      "volume": volume
    }, true);
  }
}

function flush_brineomatic() {
  let duration = $("#bomFlushDurationInput").val();

  if (duration > 0) {
    let micros = duration * 60 * 1000000;

    client.send({
      "cmd": "flush_watermaker",
      "duration": micros
    }, true);
  }
}

function pickle_brineomatic() {
  let duration = $("#bomPickleDurationInput").val();

  if (duration > 0) {
    let micros = duration * 60 * 1000000;

    client.send({
      "cmd": "pickle_watermaker",
      "duration": micros
    }, true);
  }
}

function depickle_brineomatic() {
  let duration = $("#bomDepickleDurationInput").val();

  if (duration > 0) {
    let micros = duration * 60 * 1000000;

    client.send({
      "cmd": "depickle_watermaker",
      "duration": micros
    }, true);
  }
}

function stop_brineomatic() {
  client.send({
    "cmd": "stop_watermaker",
  }, true);
}

function toggle_state(id) {
  client.togglePWMChannel(id, current_config.hostname, true);
}

function toggle_relay_state(id) {
  client.send({
    "cmd": "toggle_relay_channel",
    "id": id,
    "source": current_config.hostname
  }, true);
}

function toggle_adc_details(id) {
  $(`#adc${id}Details`).toggle();
}

function toggle_duty_cycle(id) {
  $(`#pwmDutySliderRow${id}`).toggle();
}

function open_page(page) {
  // yarrboard_log(`opening ${page}`);

  if (!page_permissions[app_role].includes(page)) {
    yarrboard_log(`${page} not allowed for ${app_role}`);
    return;
  }

  if (page == current_page) {
    // yarrboard_log(`already on ${page}.`);
    //return;
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
    get_update_data();

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

    client.logout();

    if (app_role == "nobody")
      open_page("login");
    else
      open_page("control");
  }
  else {
    //update our nav
    $('#controlNav .nav-link').removeClass("active");
    $(`#${page}Nav a`).addClass("active");

    //is our new page ready?
    on_page_ready();
  }
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
  if (client.isOpen() && (app_role == 'guest' || app_role == 'admin')) {
    //yarrboard_log("get_stats");

    client.getStats();

    //keep loading it while we are here.
    if (current_page == "stats")
      setTimeout(get_stats_data, app_update_interval);
  }
}

function get_graph_data() {
  if (client.isOpen() && (app_role == 'guest' || app_role == 'admin')) {
    client.send({
      "cmd": "get_graph_data"
    });
  }
}

function get_update_data() {
  if (client.isOpen() && (app_role == 'guest' || app_role == 'admin')) {
    //yarrboard_log("get_update");

    client.getUpdate();

    //keep loading it while we are here.
    if (current_page == "control" || current_page == "graphs")
      setTimeout(get_update_data, app_update_interval);
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
    client.send({
      "cmd": "set_boardname",
      "value": value
    });
  }
}

function set_duty_cycle(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  //must be realistic.
  if (value >= 0 && value <= 100) {
    //update our button
    $(`#pwmDutyCycle${id}`).html(Math.round(value) + '%');

    //we want a duty value from 0 to 1
    value = value / 100;

    //update our duty cycle
    client.setPWMChannelDuty(id, value, false);
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

    client.send({
      "cmd": "set_servo_channel",
      "id": id,
      "angle": value
    });
  }
}

function map_value(value, fromLow, fromHigh, toLow, toHigh) {
  return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

function set_brightness(e) {
  let ele = e.target;
  let value = ele.value;

  //must be realistic.
  if (value >= 0 && value <= 100) {
    //we want a duty value from 0 to 1
    value = value / 100;

    //update our duty cycle
    client.setBrightness(value, false);
  }
}

function update_theme_switch() {
  if (this.checked)
    setTheme("dark");
  else
    setTheme("light");
}

function validate_pwm_name(e) {
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

    //set our new pwm name!
    client.send({
      "cmd": "config_pwm_channel",
      "id": id,
      "name": value
    });
  }
}

function validate_pwm_dimmable(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_pwm_channel",
    "id": id,
    "isDimmable": value
  });
}

function validate_pwm_enabled(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fPWMName${id}`).prop('disabled', !value);
  $(`#fPWMDimmable${id}`).prop('disabled', !value);
  $(`#fPWMSoftFuse${id}`).prop('disabled', !value);
  $(`#fPWMType${id}`).prop('disabled', !value);
  $(`#fPWMDefaultState${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_pwm_channel",
    "id": id,
    "enabled": value
  });
}

function validate_pwm_soft_fuse(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = parseFloat(ele.value);

  //real numbers only, pal.
  if (isNaN(value) || value <= 0 || value > 20) {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else {
    $(ele).removeClass("is-invalid");
    $(ele).addClass("is-valid");

    //save it
    client.send({
      "cmd": "config_pwm_channel",
      "id": id,
      "softFuse": value
    });
  }
}

function validate_pwm_type(e) {
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

    //set our new pwm name!
    client.send({
      "cmd": "config_pwm_channel",
      "id": id,
      "type": value
    });
  }
}

function validate_pwm_default_state(e) {
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

    //set our new pwm name!
    client.send({
      "cmd": "config_pwm_channel",
      "id": id,
      "defaultState": value
    });
  }
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

    client.send({
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

  $(`#fPWMName${id}`).prop('disabled', !value);
  $(`#fPWMType${id}`).prop('disabled', !value);
  $(`#fPWMDefaultState${id}`).prop('disabled', !value);

  $(ele).addClass("is-valid");

  client.send({
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

    client.send({
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

    client.send({
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

    client.send({
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

  $(`#fPWMName${id}`).prop('disabled', !value);
  $(`#fPWMType${id}`).prop('disabled', !value);
  $(`#fPWMDefaultState${id}`).prop('disabled', !value);

  $(ele).addClass("is-valid");

  client.send({
    "cmd": "config_servo_channel",
    "id": id,
    "enabled": value
  });
}

function validate_switch_name(e) {
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

    //set our new pwm name!
    client.send({
      "cmd": "config_switch",
      "id": id,
      "name": value
    });
  }
}

function validate_switch_mode(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  //let value = ele.value;
  let value = this.value;

  //$(ele).removeClass("is-invalid");
  $(ele).addClass("is-valid");

  //set our new pwm name!
  client.send({
    "cmd": "config_switch",
    "id": id,
    "mode": value.toUpperCase()
  });
}

function validate_switch_default_state(e) {
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

    //set our new pwm name!
    client.send({
      "cmd": "config_switch",
      "id": id,
      "defaultState": value
    });
  }
}

function validate_switch_enabled(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fSwitchName${id}`).prop('disabled', !value);
  $(`#fSwitchMode${id}`).prop('disabled', !value);
  $(`#fSwitchDefaultState${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_switch",
    "id": id,
    "enabled": value
  });
}

function set_rgb_color(e, color) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];

  let rgb = color.toRgb();

  let red = rgb.r / 255;
  let green = rgb.g / 255;
  let blue = rgb.b / 255;

  client.setRGB(id, red.toFixed(4), green.toFixed(4), blue.toFixed(4), false);
}

function validate_rgb_name(e) {
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

    //set our new pwm name!
    client.send({
      "cmd": "config_rgb",
      "id": id,
      "name": value
    });
  }
}

function validate_rgb_enabled(e) {
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fRGBName${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_rgb",
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

    //set our new pwm name!
    client.send({
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

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_adc",
    "id": id,
    "enabled": value
  });
}

function do_login(e) {
  app_username = $('#username').val();
  app_password = $('#password').val();

  client.login(app_username, app_password);
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
  client.send({
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
  client.send({
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
    "server_cert": server_cert,
    "server_key": server_key
  });

  //if they are changing from client to client, we can't show a success.
  show_alert("App settings have been updated.", "success");
}

function restart_board() {
  if (confirm("Are you sure you want to restart your Yarrboard?")) {
    client.restart();

    show_alert("Yarrboard is now restarting, please be patient.", "primary");

    setTimeout(function () {
      location.reload();
    }, 5000);
  }
}

function reset_to_factory() {
  if (confirm("WARNING! Are you sure you want to reset your Yarrboard to factory defaults?  This cannot be undone.")) {
    client.factoryReset();

    show_alert("Yarrboard is now resetting to factory defaults, please be patient.", "primary");
  }
}

function check_for_updates() {
  //did we get a config yet?
  if (current_config) {
    $.ajax({
      url: "https://raw.githubusercontent.com/hoeken/yarrboard-firmware/main/firmware.json",
      cache: false,
      dataType: "json",
      success: function (jdata) {
        //did we get anything?
        let data;
        for (firmware of jdata)
          if (firmware.type == current_config.hardware_version)
            data = firmware;

        if (!data) {
          show_alert(`Could not find a firmware for this hardware.`, "danger");
          return;
        }

        $("#firmware_checking").hide();

        //do we have a new version?
        if (compareVersions(data.version, current_config.firmware_version)) {
          if (data.changelog) {
            $("#firmware_changelog").append(marked.parse(data.changelog));
            $("#firmware_changelog").show();
          }

          $("#new_firmware_version").html(data.version);
          $("#firmware_bin").attr("href", `${data.url}`);
          $("#firmware_update_available").show();

          show_alert(`There is a <a  onclick="open_page('system')" href="/#system">firmware update</a> available (${data.version}).`, "primary");
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
  client.startOTA();

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
      if (page_list.includes(page))
        open_page(page);
    }
    else
      open_page("control");
  }
  else
    open_page('login');
}

function secondsToDhms(seconds) {
  seconds = Number(seconds);
  var d = Math.floor(seconds / (3600 * 24));
  var h = Math.floor(seconds % (3600 * 24) / 3600);
  var m = Math.floor(seconds % 3600 / 60);
  var s = Math.floor(seconds % 60);

  var dDisplay = d > 0 ? d + (d == 1 ? " day, " : " days, ") : "";
  var hDisplay = h > 0 ? h + (h == 1 ? " hour, " : " hours, ") : "";
  var mDisplay = (m > 0 && d == 0) ? m + (m == 1 ? " minute, " : " minutes, ") : "";
  var sDisplay = (s > 0 && d == 0 && h == 0 && m == 0) ? s + (s == 1 ? " second" : " seconds") : "";

  return (dDisplay + hDisplay + mDisplay + sDisplay).replace(/,\s*$/, "");
}

function formatAmpHours(aH) {
  //low amp hours?
  if (aH < 10)
    return aH.toFixed(3) + "&nbsp;Ah";
  else if (aH < 100)
    return aH.toFixed(2) + "&nbsp;Ah";
  else if (aH < 1000)
    return aH.toFixed(1) + "&nbsp;Ah";

  //okay, now its kilo time
  aH = aH / 1000;
  if (aH < 10)
    return aH.toFixed(3) + "&nbsp;kAh";
  else if (aH < 100)
    return aH.toFixed(2) + "&nbsp;kAh";
  else if (aH < 1000)
    return aH.toFixed(1) + "&nbsp;kAh";

  //okay, now its mega time
  aH = aH / 1000;
  if (aH < 10)
    return aH.toFixed(3) + "&nbsp;MAh";
  else if (aH < 100)
    return aH.toFixed(2) + "&nbsp;MAh";
  else if (aH < 1000)
    return aH.toFixed(1) + "&nbsp;MAh";
  else
    return Math.round(aH) + "&nbsp;MAh";
}

function formatWattHours(wH) {
  //low watt hours?
  if (wH < 10)
    return wH.toFixed(3) + "&nbsp;Wh";
  else if (wH < 100)
    return wH.toFixed(2) + "&nbsp;Wh";
  else if (wH < 1000)
    return wH.toFixed(1) + "&nbsp;Wh";

  //okay, now its kilo time
  wH = wH / 1000;
  if (wH < 10)
    return wH.toFixed(3) + "&nbsp;kWh";
  else if (wH < 100)
    return wH.toFixed(2) + "&nbsp;kWh";
  else if (wH < 1000)
    return wH.toFixed(1) + "&nbsp;kWh";

  //okay, now its mega time
  wH = wH / 1000;
  if (wH < 10)
    return wH.toFixed(3) + "&nbsp;MWh";
  else if (wH < 100)
    return wH.toFixed(2) + "&nbsp;MWh";
  else if (wH < 1000)
    return wH.toFixed(1) + "&nbsp;MWh";
  else
    return Math.round(wH) + "&nbsp;MWh";
}

function formatBytes(bytes, decimals = 1) {
  if (!+bytes) return '0 Bytes'

  const k = 1024
  const dm = decimals < 0 ? 0 : decimals
  const sizes = ['Bytes', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB']

  const i = Math.floor(Math.log(bytes) / Math.log(k))

  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`
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

function yarrboard_log(message) {
  // Create a new Date object
  const currentDate = new Date();

  // Get the parts of the date and time
  const year = currentDate.getFullYear();
  const month = String(currentDate.getMonth() + 1).padStart(2, '0'); // Months are zero-based
  const day = String(currentDate.getDate()).padStart(2, '0');
  const hours = String(currentDate.getHours()).padStart(2, '0');
  const minutes = String(currentDate.getMinutes()).padStart(2, '0');
  const seconds = String(currentDate.getSeconds()).padStart(2, '0');
  const milliseconds = String(currentDate.getMilliseconds()).padStart(3, '0');

  // Create the formatted timestamp
  const formattedTimestamp = `[${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${milliseconds}]`;

  //put it in our textarea - useful for debugging on non-computer interfaces
  let textarea = document.getElementById("debug_log_text");
  textarea.append(`${formattedTimestamp} ${message}\n`);
  textarea.scrollTop = textarea.scrollHeight;

  //standard log.
  console.log(`${formattedTimestamp} ${message}`);
}

window.onerror = function (errorMsg, url, lineNumber) {
  yarrboard_log(`Error: ${errorMsg} on ${url} line ${lineNumber}`);
  return false;
}

window.addEventListener("error", function (e) {
  yarrboard_log(`Error Listener: ${e.error.message}`);
  return false;
});

window.addEventListener('unhandledrejection', function (e) {
  yarrboard_log(`Unhandled Rejection: ${e.reason.message}`);
  return false;
});

function getStoredTheme() { localStorage.getItem('theme'); }
function setStoredTheme() { localStorage.setItem('theme', theme); }

function getPreferredTheme() {
  const storedTheme = getStoredTheme();

  if (storedTheme)
    return storedTheme;

  return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
}

function setTheme(theme) {
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

// $(document).ready(function () {
//   let viewport = getViewport()
//   let debounce
//   $(window).resize(() => {
//     debounce = setTimeout(() => {
//       const currentViewport = getViewport()
//       if (currentViewport !== viewport) {
//         viewport = currentViewport
//         $(window).trigger('newViewport', viewport)
//       }
//     }, 500)
//   })
//   $(window).on('newViewport', (viewport) => {
//     // do something with viewport
//   })
//   // run when page loads
//   $(window).trigger('newViewport', viewport)
// }

/* <option value="light">Light</option>
<option value="motor">Motor</option>
<option value="water_pump">Water Pump</option>
<option value="fuel_pump">Fuel Pump</option>
<option value="fan">Fan</option>
<option value="solenoid">Solenoid</option>
<option value="fridge">Refridgerator</option>
<option value="freezer">Freezer</option>
<option value="charger">Charger</option>
<option value="electronics">Electronics</option>
<option value="other">Other</option> */

const pwm_type_images = {
  "light":
    `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-lightbulb" viewBox="0 0 16 16">
      <path d="M2 6a6 6 0 1 1 10.174 4.31c-.203.196-.359.4-.453.619l-.762 1.769A.5.5 0 0 1 10.5 13a.5.5 0 0 1 0 1 .5.5 0 0 1 0 1l-.224.447a1 1 0 0 1-.894.553H6.618a1 1 0 0 1-.894-.553L5.5 15a.5.5 0 0 1 0-1 .5.5 0 0 1 0-1 .5.5 0 0 1-.46-.302l-.761-1.77a1.964 1.964 0 0 0-.453-.618A5.984 5.984 0 0 1 2 6m6-5a5 5 0 0 0-3.479 8.592c.263.254.514.564.676.941L5.83 12h4.342l.632-1.467c.162-.377.413-.687.676-.941A5 5 0 0 0 8 1"/>
    </svg>`,
  "motor":
    `<svg xmlns="http://www.w3.org/2000/svg" height="32" width="32" version="1.1" fill="currentColor"
	 viewBox="0 0 452.263 452.263" xml:space="preserve">
      <path d="M405.02,161.533c-4.971,0-9,4.029-9,9v53.407h-18.298v-45.114c0-4.971-4.029-9-9-9h-41.756l-7.778-25.93
        c-1.142-3.807-4.646-6.414-8.621-6.414h-63.575v-22.77h40.032c4.971,0,9-4.029,9-9V75.666c0-4.971-4.029-9-9-9H158.064
        c-4.971,0-9,4.029-9,9v30.047c0,4.971,4.029,9,9,9h40.031v22.77h-92.714c-3.297,0-6.33,1.802-7.905,4.698l-15.044,27.646H47.227
        c-4.971,0-9,4.029-9,9v45.114H18v-64.342c0-4.971-4.029-9-9-9s-9,4.029-9,9v186.008c0,4.971,4.029,9,9,9s9-4.029,9-9v-64.342h20.227
        v47.64c0,4.971,4.029,9,9,9h74.936l34.26,44.207c1.705,2.2,4.331,3.487,7.114,3.487h205.185c4.971,0,9-4.029,9-9v-95.333h18.298
        v53.407c0,4.971,4.029,9,9,9c26.05,0,47.243-21.193,47.243-47.243v-87.651C452.263,182.727,431.07,161.533,405.02,161.533z
        M18,263.264V241.94h20.227v21.324H18z M167.064,84.666h110.959v12.047H167.064V84.666z M216.096,114.713h12.896v22.77h-12.896
        V114.713z M56.227,187.826h6.395v132.078h-6.395V187.826z M133.688,323.391c-1.705-2.2-4.331-3.487-7.114-3.487H80.622V187.826
        h7.159c3.297,0,6.33-1.802,7.905-4.698l15.044-27.646h193.14l7.778,25.93c1.142,3.807,4.646,6.414,8.621,6.414h16.151v179.771
        H167.948L133.688,323.391z M359.722,367.597h-5.302V187.826h5.302V367.597z M377.722,263.264V241.94h18.298v21.324H377.722z
        M434.263,296.428c0,12.986-8.508,24.022-20.243,27.827V180.95c11.735,3.804,20.243,14.84,20.243,27.827V296.428z"/>
      </svg>`,
  "water_pump":
    `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-droplet" viewBox="0 0 16 16">
      <path fill-rule="evenodd" d="M7.21.8C7.69.295 8 0 8 0c.109.363.234.708.371 1.038.812 1.946 2.073 3.35 3.197 4.6C12.878 7.096 14 8.345 14 10a6 6 0 0 1-12 0C2 6.668 5.58 2.517 7.21.8zm.413 1.021A31.25 31.25 0 0 0 5.794 3.99c-.726.95-1.436 2.008-1.96 3.07C3.304 8.133 3 9.138 3 10a5 5 0 0 0 10 0c0-1.201-.796-2.157-2.181-3.7l-.03-.032C9.75 5.11 8.5 3.72 7.623 1.82z"/>
      <path fill-rule="evenodd" d="M4.553 7.776c.82-1.641 1.717-2.753 2.093-3.13l.708.708c-.29.29-1.128 1.311-1.907 2.87z"/>
    </svg>`,
  "bilge_pump":
    `<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="32" height="32" fill="currentColor" version="1.1" x="0px" y="0px" viewBox="0 0 88.897 67.50625000000001" xml:space="preserve"><path d="M76.259,5.14l0.55-1.123c0.453-0.92,1.572-1.305,2.471-0.779l2.429,1.404  c0.854,0.494,1.154,1.609,0.718,2.486l-3.249,6.596l-5.552-3.207c-5.162-2.629-11.203-4.502-16.978-4.502H27.557  c-6.152,0-12.794,2.166-18.219,5.158l11.368,23.074h41.212l6.2-12.586l5.459,3.436L65.867,40.75  c-0.057,0.109-0.123,0.217-0.199,0.316l-0.608,1.227l-0.064,0.08c-0.271,0.338-0.745,0.566-1.163,0.656  c-0.972,0.215-2.003-0.121-2.861-0.566c-0.555-0.285-1.477-0.324-2.085-0.295c-1.333,0.061-2.698,0.426-3.935,0.906  c-4.462,1.727-8.49,5.334-11.597,8.885l-0.021,0.021l-2.022,2.025l-2.026-2.025l-0.021-0.021c-3.11-3.551-7.142-7.158-11.611-8.885  c-1.238-0.48-2.605-0.846-3.936-0.906c-0.613-0.029-1.535,0.01-2.093,0.295c-0.857,0.445-1.891,0.781-2.86,0.566  c-0.422-0.09-0.895-0.318-1.167-0.656l-0.063-0.078l-0.697-1.406c-0.045-0.064-0.026-0.035-0.074-0.139l-1.034-2.094l-1.562-3.15  l0.01-0.004L0.198,7.128c-0.435-0.881-0.136-1.992,0.72-2.486l2.429-1.404c0.897-0.525,2.014-0.141,2.47,0.779l0.873,1.777  C12.947,2.404,20.476,0,27.557,0h29.091C63.337,0,70.279,2.117,76.259,5.14L76.259,5.14z"/><polygon fill-rule="evenodd" clip-rule="evenodd" points="41.886,19.367 82.375,19.367 82.375,21.25 82.375,22.857   82.375,24.466 88.897,18.031 82.375,11.601 82.375,14.816 82.375,16.699 41.886,16.699 39.257,16.699 39.223,16.699 39.223,29.947   41.886,29.947 41.886,19.367 "/>`,
  "fuel_pump":
    `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-fuel-pump" viewBox="0 0 16 16">
      <path d="M3 2.5a.5.5 0 0 1 .5-.5h5a.5.5 0 0 1 .5.5v5a.5.5 0 0 1-.5.5h-5a.5.5 0 0 1-.5-.5z"/>
      <path d="M1 2a2 2 0 0 1 2-2h6a2 2 0 0 1 2 2v8a2 2 0 0 1 2 2v.5a.5.5 0 0 0 1 0V8h-.5a.5.5 0 0 1-.5-.5V4.375a.5.5 0 0 1 .5-.5h1.495c-.011-.476-.053-.894-.201-1.222a.97.97 0 0 0-.394-.458c-.184-.11-.464-.195-.9-.195a.5.5 0 0 1 0-1c.564 0 1.034.11 1.412.336.383.228.634.551.794.907.295.655.294 1.465.294 2.081v3.175a.5.5 0 0 1-.5.501H15v4.5a1.5 1.5 0 0 1-3 0V12a1 1 0 0 0-1-1v4h.5a.5.5 0 0 1 0 1H.5a.5.5 0 0 1 0-1H1zm9 0a1 1 0 0 0-1-1H3a1 1 0 0 0-1 1v13h8z"/>
    </svg>`,
  "fan":
    `<svg width="32" height="32" fill="currentColor" id="Layer_1" data-name="Layer 1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 122.88 122.07"><defs><style>.cls-1{fill-rule:evenodd;}</style></defs><title>fan-blades</title><path class="cls-1" d="M67.29,82.9c-.11,1.3-.26,2.6-.47,3.9-1.43,9-5.79,14.34-8.08,22.17C56,118.45,65.32,122.53,73.27,122A37.63,37.63,0,0,0,85,119a45,45,0,0,0,9.32-5.36c20.11-14.8,16-34.9-6.11-46.36a15,15,0,0,0-4.14-1.4,22,22,0,0,1-6,11.07l0,0A22.09,22.09,0,0,1,67.29,82.9ZM62.4,44.22a17.1,17.1,0,1,1-17.1,17.1,17.1,17.1,0,0,1,17.1-17.1ZM84.06,56.83c1.26.05,2.53.14,3.79.29,9.06,1,14.58,5.16,22.5,7.1,9.6,2.35,13.27-7.17,12.41-15.09a37.37,37.37,0,0,0-3.55-11.57,45.35,45.35,0,0,0-5.76-9.08C97.77,9,77.88,14,67.4,36.63a14.14,14.14,0,0,0-1,2.94A22,22,0,0,1,78,45.68l0,0a22.07,22.07,0,0,1,6,11.13Zm-26.9-17c0-1.6.13-3.21.31-4.81,1-9.07,5.12-14.6,7-22.52C66.86,2.89,57.32-.75,49.41.13A37.4,37.4,0,0,0,37.84,3.7a44.58,44.58,0,0,0-9.06,5.78C9.37,25.2,14.39,45.08,37,55.51a14.63,14.63,0,0,0,3.76,1.14A22.12,22.12,0,0,1,57.16,39.83ZM40.66,65.42a52.11,52.11,0,0,1-5.72-.24c-9.08-.88-14.67-4.92-22.62-6.73C2.68,56.25-.83,65.84.16,73.74A37.45,37.45,0,0,0,3.9,85.25a45.06,45.06,0,0,0,5.91,9c16,19.17,35.8,13.87,45.91-8.91a15.93,15.93,0,0,0,.88-2.66A22.15,22.15,0,0,1,40.66,65.42Z"/></svg>`,
  "solenoid":
    `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-magnet" viewBox="0 0 16 16">
    <path d="M8 1a7 7 0 0 0-7 7v3h4V8a3 3 0 0 1 6 0v3h4V8a7 7 0 0 0-7-7m7 11h-4v3h4zM5 12H1v3h4zM0 8a8 8 0 1 1 16 0v8h-6V8a2 2 0 1 0-4 0v8H0z"/>
  </svg>`,
  "fridge":
    `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-thermometer-low" viewBox="0 0 16 16">
    <path d="M9.5 12.5a1.5 1.5 0 1 1-2-1.415V9.5a.5.5 0 0 1 1 0v1.585a1.5 1.5 0 0 1 1 1.415"/>
    <path d="M5.5 2.5a2.5 2.5 0 0 1 5 0v7.55a3.5 3.5 0 1 1-5 0zM8 1a1.5 1.5 0 0 0-1.5 1.5v7.987l-.167.15a2.5 2.5 0 1 0 3.333 0l-.166-.15V2.5A1.5 1.5 0 0 0 8 1"/>
  </svg>`,
  "freezer":
    `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-snow" viewBox="0 0 16 16">
      <path d="M8 16a.5.5 0 0 1-.5-.5v-1.293l-.646.647a.5.5 0 0 1-.707-.708L7.5 12.793V8.866l-3.4 1.963-.496 1.85a.5.5 0 1 1-.966-.26l.237-.882-1.12.646a.5.5 0 0 1-.5-.866l1.12-.646-.884-.237a.5.5 0 1 1 .26-.966l1.848.495L7 8 3.6 6.037l-1.85.495a.5.5 0 0 1-.258-.966l.883-.237-1.12-.646a.5.5 0 1 1 .5-.866l1.12.646-.237-.883a.5.5 0 1 1 .966-.258l.495 1.849L7.5 7.134V3.207L6.147 1.854a.5.5 0 1 1 .707-.708l.646.647V.5a.5.5 0 1 1 1 0v1.293l.647-.647a.5.5 0 1 1 .707.708L8.5 3.207v3.927l3.4-1.963.496-1.85a.5.5 0 1 1 .966.26l-.236.882 1.12-.646a.5.5 0 0 1 .5.866l-1.12.646.883.237a.5.5 0 1 1-.26.966l-1.848-.495L9 8l3.4 1.963 1.849-.495a.5.5 0 0 1 .259.966l-.883.237 1.12.646a.5.5 0 0 1-.5.866l-1.12-.646.236.883a.5.5 0 1 1-.966.258l-.495-1.849-3.4-1.963v3.927l1.353 1.353a.5.5 0 0 1-.707.708l-.647-.647V15.5a.5.5 0 0 1-.5.5z"/>
    </svg>`,
  "charger":
    `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-lightning-charge" viewBox="0 0 16 16">
    <path d="M11.251.068a.5.5 0 0 1 .227.58L9.677 6.5H13a.5.5 0 0 1 .364.843l-8 8.5a.5.5 0 0 1-.842-.49L6.323 9.5H3a.5.5 0 0 1-.364-.843l8-8.5a.5.5 0 0 1 .615-.09zM4.157 8.5H7a.5.5 0 0 1 .478.647L6.11 13.59l5.732-6.09H9a.5.5 0 0 1-.478-.647L9.89 2.41z"/>
  </svg>`,
  "electronics":
    `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-motherboard" viewBox="0 0 16 16">
    <path d="M11.5 2a.5.5 0 0 1 .5.5v7a.5.5 0 0 1-1 0v-7a.5.5 0 0 1 .5-.5m2 0a.5.5 0 0 1 .5.5v7a.5.5 0 0 1-1 0v-7a.5.5 0 0 1 .5-.5m-10 8a.5.5 0 0 0 0 1h6a.5.5 0 0 0 0-1zm0 2a.5.5 0 0 0 0 1h6a.5.5 0 0 0 0-1zM5 3a1 1 0 0 0-1 1h-.5a.5.5 0 0 0 0 1H4v1h-.5a.5.5 0 0 0 0 1H4a1 1 0 0 0 1 1v.5a.5.5 0 0 0 1 0V8h1v.5a.5.5 0 0 0 1 0V8a1 1 0 0 0 1-1h.5a.5.5 0 0 0 0-1H9V5h.5a.5.5 0 0 0 0-1H9a1 1 0 0 0-1-1v-.5a.5.5 0 0 0-1 0V3H6v-.5a.5.5 0 0 0-1 0zm0 1h3v3H5zm6.5 7a.5.5 0 0 0-.5.5v1a.5.5 0 0 0 .5.5h2a.5.5 0 0 0 .5-.5v-1a.5.5 0 0 0-.5-.5z"/>
    <path d="M1 2a2 2 0 0 1 2-2h11a2 2 0 0 1 2 2v11a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2v-2H.5a.5.5 0 0 1-.5-.5v-1A.5.5 0 0 1 .5 9H1V8H.5a.5.5 0 0 1-.5-.5v-1A.5.5 0 0 1 .5 6H1V5H.5a.5.5 0 0 1-.5-.5v-2A.5.5 0 0 1 .5 2zm1 11a1 1 0 0 0 1 1h11a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H3a1 1 0 0 0-1 1z"/>
  </svg>`,
  "other":
    `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-gear" viewBox="0 0 16 16">
      <path d="M8 4.754a3.246 3.246 0 1 0 0 6.492 3.246 3.246 0 0 0 0-6.492zM5.754 8a2.246 2.246 0 1 1 4.492 0 2.246 2.246 0 0 1-4.492 0z"/>
      <path d="M9.796 1.343c-.527-1.79-3.065-1.79-3.592 0l-.094.319a.873.873 0 0 1-1.255.52l-.292-.16c-1.64-.892-3.433.902-2.54 2.541l.159.292a.873.873 0 0 1-.52 1.255l-.319.094c-1.79.527-1.79 3.065 0 3.592l.319.094a.873.873 0 0 1 .52 1.255l-.16.292c-.892 1.64.901 3.434 2.541 2.54l.292-.159a.873.873 0 0 1 1.255.52l.094.319c.527 1.79 3.065 1.79 3.592 0l.094-.319a.873.873 0 0 1 1.255-.52l.292.16c1.64.893 3.434-.902 2.54-2.541l-.159-.292a.873.873 0 0 1 .52-1.255l.319-.094c1.79-.527 1.79-3.065 0-3.592l-.319-.094a.873.873 0 0 1-.52-1.255l.16-.292c.893-1.64-.902-3.433-2.541-2.54l-.292.159a.873.873 0 0 1-1.255-.52l-.094-.319zm-2.633.283c.246-.835 1.428-.835 1.674 0l.094.319a1.873 1.873 0 0 0 2.693 1.115l.291-.16c.764-.415 1.6.42 1.184 1.185l-.159.292a1.873 1.873 0 0 0 1.116 2.692l.318.094c.835.246.835 1.428 0 1.674l-.319.094a1.873 1.873 0 0 0-1.115 2.693l.16.291c.415.764-.42 1.6-1.185 1.184l-.291-.159a1.873 1.873 0 0 0-2.693 1.116l-.094.318c-.246.835-1.428.835-1.674 0l-.094-.319a1.873 1.873 0 0 0-2.692-1.115l-.292.16c-.764.415-1.6-.42-1.184-1.185l.159-.291A1.873 1.873 0 0 0 1.945 8.93l-.319-.094c-.835-.246-.835-1.428 0-1.674l.319-.094A1.873 1.873 0 0 0 3.06 4.377l-.16-.292c-.415-.764.42-1.6 1.185-1.184l.292.159a1.873 1.873 0 0 0 2.692-1.115l.094-.319z"/>
    </svg>`
};

function pwm_get_type_image(ch) {
  if (ch.type == "")
    return;
}

function isCanvasSupported() {
  var elem = document.createElement('canvas');
  return !!(elem.getContext && elem.getContext('2d'));
}