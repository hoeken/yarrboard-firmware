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

// let socket_retries = 0;
// let retry_time = 0;
// let last_heartbeat = 0;
// let heartbeat_rate = 5000;

let ota_started = false;

const page_list = ["control", "config", "stats", "network", "settings", "system"];
const page_ready = {
  "control": false,
  "config":  false,
  "stats":   false,
  "network": false,
  "settings": false,
  "system":  true,
  "login":  true
};

const page_permissions = {
  "nobody": [
    "login"
  ],
  "admin": [
    "control",
    "config",
    "stats",
    "network",
    "settings",
    "system",
    "login",
    "logout"
  ],
  "guest": [
    "control",
    "stats",
    "login",
    "logout"
  ]
};

const BoardNameEdit = (name) => `
<div class="col-12">
  <label for="fBoardName" class="form-label">Board Name</label>
  <input type="text" class="form-control" id="fBoardName" value="${name}">
  <div class="valid-feedback">Saved!</div>
  <div class="invalid-feedback">Must be 30 characters or less.</div>
</div>
`;

const PWMControlRow = (id, name) => `
<tr id="pwm${id}" class="pwmRow">
  <td class="text-center"><button id="pwmState${id}" type="button" class="btn btn-sm" onclick="toggle_state(${id})" style="width: 60px"></button></td>
  <td class="pwmName">${name}</td>
  <td class="text-end"><button id="pwmDutyCycle${id}" type="button" class="btn btn-sm btn-light" onclick="toggle_duty_cycle(${id})" style="width: 60px">???</button></td>
  <td id="pwmCurrent${id}" class="text-end"></td>
</tr>
<tr id="pwmDutySliderRow${id}" style="display:none">
  <td colspan="4">
    <input type="range" class="form-range" min="0" max="100" id="pwmDutySlider${id}">
  </td>
</tr>
`;

const PWMEditRow = (id, name, soft_fuse) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fPWMEnabled${id}">
      <label class="form-check-label" for="fPWMEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fPWMName${id}" value="${name}">
      <label for="fPWMName${id}">Name</label>
    </div>
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
  </div>
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fPWMDimmable${id}">
      <label class="form-check-label" for="fPWMDimmable${id}">
        Dimmable?
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fPWMSoftFuse${id}" value="${soft_fuse}">
      <label for="fPWMSoftFuse${id}">Soft Fuse (Amps)</label>
    </div>
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be a number between 0 and 20</div>
  </div>
</div>
`;

const SwitchControlRow = (id, name) => `
<tr id="switch${id}" class="switchRow">
  <td class="text-center"><button id="switchState${id}" type="button" class="btn btn-sm" style="width: 80px"></button></td>
  <td class="switchName align-middle">${name}</td>
</tr>
`;

const SwitchEditRow = (id, name) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fSwitchEnabled${id}">
      <label class="form-check-label" for="fSwitchEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <div class="form-check form-switch">
      <select id="fSwitchMode${id}" class="form-select" aria-label="Switch Mode">
        <option value="direct">Direct</option>
        <option value="inverting">Inverting</option>
        <option value="toggle_rising">Toggle Rising</option>
        <option value="toggle_falling">Toggle Falling</option>
      </select>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <input type="text" class="form-control" id="fSwitchName${id}" value="${name}">
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
  </div>
</div>
`;

const RGBControlRow = (id, name) => `
<tr id="rgb${id}" class="rgbRow">
  <td class="text-center"><input id="rgbPicker${id}" type="text"></td>
  <td class="rgbName align-middle">${name}</td>
</tr>
`;

const RGBEditRow = (id, name) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fRGBEnabled${id}">
      <label class="form-check-label" for="fRGBEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <input type="text" class="form-control" id="fRGBName${id}" value="${name}">
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
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

const ADCEditRow = (id, name) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fADCEnabled${id}">
      <label class="form-check-label" for="fADCEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <input type="text" class="form-control" id="fADCName${id}" value="${name}">
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
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
let currentRGBPickerID = -1;

//our heartbeat timer.
// function send_heartbeat()
// {
//   //did we not get a heartbeat?
//   if (Date.now() - last_heartbeat > heartbeat_rate * 2)
//   {
//     yarrboard_log("Missed heartbeat: " + (Date.now() - last_heartbeat))
//     socket.close();
//     retry_connection();
//   }

//   //only send it if we're already open.
//   if (socket.readyState == WebSocket.OPEN)
//   {
//     client.send({"cmd": "ping"});
//     setTimeout(send_heartbeat, heartbeat_rate);
//   }
//   else if (socket.readyState == WebSocket.CLOSING)
//   {
//     yarrboard_log("she closing " + socket.readyState);
//     //socket.close();
//     retry_connection();
//   }
//   else if (socket.readyState == WebSocket.CLOSED)
//   {
//     yarrboard_log("she closed " + socket.readyState);
//     //socket.close();
//     retry_connection();
//   }
// }

function start_yarrboard()
{
  //main data connection
  start_websocket();

  //check our connection status.
  setInterval(check_connection_status, 100);

  //light/dark theme init.
  setTheme(getPreferredTheme());
  $("#darkSwitch").change(function (){
    if (this.checked)
      setTheme("dark");
    else
      setTheme("light");
  });
}

function load_configs()
{
  if (app_role == "nobody")
    return;

  if(app_role == "admin")
  {
    client.getNetworkConfig();
    client.getAppConfig();
    check_for_updates();
  }

  client.getConfig();
}

function check_connection_status()
{
  if (client)
  {
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

function start_websocket()
{
  //close any old connections
  // if (socket)
  //   socket.close();
  
  //do we want ssl?
  let use_ssl = false;
  if (document.location.protocol == 'https:')
    use_ssl = true;

  //open it.
  client = new YarrboardClient(window.location.host, "", "", false, use_ssl);

  client.onopen = function()
  {
    //figure out what our login situation is
    client.sayHello();
  };

  client.onmessage = function(msg)
  {
      if (msg.msg == 'hello')
    {
      yarrboard_log("hello");
      app_role = msg.role;
      default_app_role = msg.default_role;

      //light/dark mode
      if (msg.theme)
        setTheme(msg.theme);

      //auto login?
      if (Cookies.get("username") && Cookies.get("password")){
        yarrboard_log("auto login");
        client.login(Cookies.get("username"), Cookies.get("password"));
      } else {
        update_role_ui();
        load_configs();  
        open_default_page();  
      }
    }
    else if (msg.msg == 'config')
    {
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

      if (msg.pwm)
      {
        $('#pwmTableBody').html("");
        for (ch of msg.pwm)
        {
          if (ch.enabled)
          {
            $('#pwmTableBody').append(PWMControlRow(ch.id, ch.name));
            $('#pwmDutySlider' + ch.id).change(set_duty_cycle);

            //update our duty when we move
            $('#pwmDutySlider' + ch.id).on("input", set_duty_cycle);

            //stop updating the UI when we are choosing a duty
            $('#pwmDutySlider' + ch.id).on('focus', function(e) {
              let ele = e.target;
              let id = ele.id.match(/\d+/)[0];
              currentPWMSliderID = id;
            });

            //stop updating the UI when we are choosing a duty
            $('#pwmDutySlider' + ch.id).on('touchstart', function(e) {
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

        $('#pwmStatsTableBody').html("");
        for (ch of msg.pwm)
        {
          if (ch.enabled)
          {
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

      //populate our switch control table
      $('#switchControlDiv').hide();
      $('#switchStatsDiv').hide();
      if (msg.switches)
      {
        $('#switchTableBody').html("");
        for (ch of msg.switches)
        {
          if (ch.enabled)
            $('#switchTableBody').append(SwitchControlRow(ch.id, ch.name));
        }

        $('#switchStatsTableBody').html("");
        for (ch of msg.switches)
        {
          if (ch.enabled)
          {
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
      if (msg.rgb)
      {
        $('#rgbTableBody').html("");
        for (ch of msg.rgb)
        {
          if (ch.enabled)
          {
            $('#rgbTableBody').append(RGBControlRow(ch.id, ch.name));

            //init our color picker
            $('#rgbPicker' + ch.id).spectrum({
              color: "#000",
              showPalette: true,
              palette: [
                ["#000","#444","#666","#999","#ccc","#eee","#f3f3f3","#fff"],
                ["#f00","#f90","#ff0","#0f0","#0ff","#00f","#90f","#f0f"],
                ["#f4cccc","#fce5cd","#fff2cc","#d9ead3","#d0e0e3","#cfe2f3","#d9d2e9","#ead1dc"],
                ["#ea9999","#f9cb9c","#ffe599","#b6d7a8","#a2c4c9","#9fc5e8","#b4a7d6","#d5a6bd"],
                ["#e06666","#f6b26b","#ffd966","#93c47d","#76a5af","#6fa8dc","#8e7cc3","#c27ba0"],
                ["#c00","#e69138","#f1c232","#6aa84f","#45818e","#3d85c6","#674ea7","#a64d79"],
                ["#900","#b45f06","#bf9000","#38761d","#134f5c","#0b5394","#351c75","#741b47"],
                ["#600","#783f04","#7f6000","#274e13","#0c343d","#073763","#20124d","#4c1130"]
            ]
            });

            //update our color on change
            $('#rgbPicker' + ch.id).change(set_rgb_color);

            //update our color when we move
            $('#rgbPicker' + ch.id).on("move.spectrum", set_rgb_color);

            //stop updating the UI when we are choosing a color
            $('#rgbPicker' + ch.id).on('show.spectrum', function(e) {
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
      if (msg.adc)
      {
        $('#adcTableBody').html("");
        for (ch of msg.adc)
        {
          if (ch.enabled)
            $('#adcTableBody').append(ADCControlRow(ch.id, ch.name));
        }

        $('#adcControlDiv').show();
      }
      
      //only do it as needed
      if (!page_ready.config || current_page != "config")
      {
        //populate our pwm edit table
        $('#boardConfigForm').html(BoardNameEdit(msg.name));

        //validate + save control
        $("#fBoardName").change(validate_board_name);

        //edit controls for each pwm
        $('#pwmConfig').hide();
        if (msg.pwm)
        {
          $('#pwmConfigForm').html("");
          for (ch of msg.pwm)
          {
            $('#pwmConfigForm').append(PWMEditRow(ch.id, ch.name, ch.softFuse));
            $(`#fPWMDimmable${ch.id}`).prop("checked", ch.isDimmable);
            $(`#fPWMEnabled${ch.id}`).prop("checked", ch.enabled);
  
            //enable/disable other stuff.
            $(`#fPWMName${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMDimmable${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMSoftFuse${ch.id}`).prop('disabled', !ch.enabled);
  
            //validate + save
            $(`#fPWMEnabled${ch.id}`).change(validate_pwm_enabled);
            $(`#fPWMName${ch.id}`).change(validate_pwm_name);
            $(`#fPWMDimmable${ch.id}`).change(validate_pwm_dimmable);
            $(`#fPWMSoftFuse${ch.id}`).change(validate_pwm_soft_fuse);
          }  
          $('#pwmConfig').show();
        }

        //edit controls for each switch
        $('#switchConfig').hide();
        if (msg.switches)
        {
          $('#switchConfigForm').html("");
          for (ch of msg.switches)
          {
            $('#switchConfigForm').append(SwitchEditRow(ch.id, ch.name));
            $(`#fSwitchEnabled${ch.id}`).prop("checked", ch.enabled);
            $(`#fSwitchMode${ch.id}`).val(ch.mode.toLowerCase());
  
            //enable/disable other stuff.
            $(`#fSwitchName${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fSwitchMode${ch.id}`).prop('disabled', !ch.enabled);

            //validate + save
            $(`#fSwitchEnabled${ch.id}`).change(validate_switch_enabled);
            $(`#fSwitchName${ch.id}`).change(validate_switch_name);
            $(`#fSwitchMode${ch.id}`).change(validate_switch_mode);
          }  
          $('#switchConfig').show();
        }

        //edit controls for each rgb
        $('#rgbConfig').hide();
        if (msg.rgb)
        {
          $('#rgbConfigForm').html("");
          for (ch of msg.rgb)
          {
            $('#rgbConfigForm').append(RGBEditRow(ch.id, ch.name));
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
        if (msg.adc)
        {
          $('#adcConfigForm').html("");
          for (ch of msg.adc)
          {
            $('#adcConfigForm').append(ADCEditRow(ch.id, ch.name));
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

      //ready!
      page_ready.config = true;

      if (!current_page)
        open_page('control');
    }
    else if (msg.msg == 'update')
    {
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
      if (msg.pwm)
      {
        for (ch of msg.pwm)
        {
          if (current_config.pwm[ch.id].enabled)
          {
            if (ch.state)
            {
              $('#pwmState' + ch.id).html("ON");
              $('#pwmState' + ch.id).addClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
            }
            else if(ch.tripped)
            {
              $('#pwmState' + ch.id).html("TRIP");
              $('#pwmState' + ch.id).addClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
            }
            else
            {
              $('#pwmState' + ch.id).html("OFF");
              $('#pwmState' + ch.id).addClass("btn-secondary");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-danger");
            }
      
            //duty is a bit of a special case.
            let duty = Math.round(ch.duty * 100);
            if (current_config.pwm[ch.id].isDimmable)
            {
              if (currentPWMSliderID != ch.id)
              {
                $('#pwmDutySlider' + ch.id).val(duty); 
                $('#pwmDutyCycle' + ch.id).html(`${duty}%`);
                $('#pwmDutyCycle' + ch.id).show();
              }
            }
            else
            {
              $('#pwmDutyCycle' + ch.id).hide();
            }
      
            let current = ch.current.toFixed(1);
            let currentHTML = `${current}&nbsp;A`;

            if (msg.bus_voltage)
            {
              let wattage = ch.current * msg.bus_voltage;
              wattage = wattage.toFixed(0);
              
              currentHTML += `&nbsp;/&nbsp${wattage}&nbsp;W`;
            }

            $('#pwmCurrent' + ch.id).html(currentHTML);
          }
        }
      }

      //our switch info
      if (msg.switches)
      {
        for (ch of msg.switches)
        {
          if (current_config.switches[ch.id].enabled)
          {
            if (ch.state)
            {
              $('#switchState' + ch.id).html("ON");
              $('#switchState' + ch.id).removeClass("btn-secondary");
              $('#switchState' + ch.id).addClass("btn-success");
            }
            else
            {
              $('#switchState' + ch.id).html("OFF");
              $('#switchState' + ch.id).removeClass("btn-success");
              $('#switchState' + ch.id).addClass("btn-secondary");
            }
          }
        }
      }

      //our rgb info
      if (msg.rgb)
      {
        for (ch of msg.rgb)
        {
          if (current_config.rgb[ch.id].enabled && currentRGBPickerID != ch.id)
          {
            let _red = Math.round(255 * ch.red);
            let _green = Math.round(255 * ch.green);
            let _blue = Math.round(255 * ch.blue);

            $("#rgbPicker" + ch.id).spectrum("set", `rgb(${_red}, ${_green}, ${_blue}`);
          }
        }
      }

      //our adc info
      if (msg.adc)
      {
        for (ch of msg.adc)
        {
          if (current_config.adc[ch.id].enabled)
          {
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
      
      page_ready.control = true;
    }
    else if (msg.msg == "stats")
    {
      //yarrboard_log("stats");

      //we need this
      if (!current_config)
        return;

      $("#uptime").html(secondsToDhms(Math.round(msg.uptime/1000000)));
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

      if (msg.pwm)
      {
        let total_ah = 0.0;
        let total_wh = 0.0;
        let total_on_count = 0;
        let total_trip_count = 0;

        for (ch of msg.pwm)
        {
          if (current_config.pwm[ch.id].enabled)
          {
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

      if (msg.switches)
      {
        for (ch of msg.switches)
        {
          if (current_config.switches[ch.id].enabled)
          {
            $('#switchOnCount' + ch.id).html(ch.state_change_count.toLocaleString("en-US"));
          }
        }
      }    

      page_ready.stats = true;
    }
    //load up our network config.
    else if (msg.msg == "network_config")
    {
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
    else if (msg.msg == "app_config")
    {
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
      $("#default_role").change(function () {toggle_role_passwords($(this).val())});
      $("#app_enable_mfd").prop("checked", msg.app_enable_mfd);
      $("#app_enable_api").prop("checked", msg.app_enable_api);
      $("#app_enable_serial").prop("checked", msg.app_enable_serial);

      //for our ssl stuff
      $("#app_enable_ssl").prop("checked", msg.app_enable_ssl);
      $("#server_cert").val(msg.server_cert);
      $("#server_key").val(msg.server_key);

      //hide/show these guys
      if (msg.app_enable_ssl)
      {
        $("#server_cert_container").show();
        $("#server_key_container").show();
      }

      //make it dynamic too
      $("#app_enable_ssl").change(function (){
        if ($("#app_enable_ssl").prop("checked"))
        {
          $("#server_cert_container").show();
          $("#server_key_container").show();  
        }
        else
        {
          $("#server_cert_container").hide();
          $("#server_key_container").hide();  
        }
      });

      page_ready.settings = true;    
    }
    //load up our network config.
    else if (msg.msg == "ota_progress")
    {
      //yarrboard_log("ota progress");

      //OTA is blocking... so update our heartbeat
      //TODO: do we need to update this?
      //last_heartbeat = Date.now();

      let progress = Math.round(msg.progress);

      let prog_id = `#firmware_progress`;
      $(prog_id).css("width", progress + "%").text(progress + "%");
      if (progress == 100)
      {
        $(prog_id).removeClass("progress-bar-animated");
        $(prog_id).removeClass("progress-bar-striped");
      }

      //was that the last?
      if (progress == 100)
      {
        show_alert("Firmware update successful.", "success");

        //reload our page
        setTimeout(function (){
          location.reload(true);
        }, 2500); 
      }
    }
    else if (msg.status == "error")
    {
      yarrboard_log(msg.message);

      //did we turn login off?
      if (msg.message == "Login not required.")
      {
        Cookies.remove("username");
        Cookies.remove("password");    
      }

      //keep the u gotta login to the login page.
      if (msg.message == "You must be logged in.")
        open_page("login");
      else
        show_alert(msg.message);
    }
    else if (msg.msg == "login")
    {

      //keep the login success stuff on the login page.
      if (msg.message == "Login successful.")
      {
        //once we know our role, we can load our other configs.
        app_role = msg.role;

        yarrboard_log(`app_role: ${app_role}`);
        yarrboard_log(`current_page: ${current_page}`);

        //only needed for login page, otherwise its autologin
        if (current_page == "login")
        {
          //save user/pass to cookies.
          if (app_username && app_password)
          {
            Cookies.set('username', app_username, { expires: 365 });
            Cookies.set('password', app_password, { expires: 365 });
          }
        }
        
        //prep the site
        update_role_ui();
        load_configs();
        open_page("control");    
      }
      else
        show_alert(msg.message, "success");
    }
    else if (msg.msg == "set_theme")
    {
      //light/dark mode
      setTheme(msg.theme);
    }
    else if (msg.msg)
    {
      yarrboard_log("[socket] Unknown message: " + JSON.stringify(msg));
    }
  };

  //use custom logger
  client.log = yarrboard_log;

  client.start();
}

// function retry_connection()
// {
//   //bail if its good to go
//   if (socket.readyState == WebSocket.OPEN)
//     return;

//   //keep watching if we are connecting
//   if (socket.readyState == WebSocket.CONNECTING)
//   {
//     yarrboard_log("Waiting for connection");
    
//     retry_time++;
//     $("#retries_count").html(retry_time);

//     //tee it up.
//     setTimeout(retry_connection, 1000);

//     return;
//   }

//   //keep track of stuff.
//   retry_time = 0;
//   socket_retries++;
//   yarrboard_log("Reconnecting... " + socket_retries);

//   //our connection status
//   $(".connection_status").hide();
//   $("#retries_count").html(retry_time);
//   $("#connection_retrying").show();

//   //reconnect!
//   start_websocket();

//   //set some bounds
//   let my_timeout = 500;
//   my_timeout = Math.max(my_timeout, socket_retries * 1000);
//   my_timeout = Math.min(my_timeout, 60000);

//   //tee it up.
//   setTimeout(retry_connection, my_timeout);

//   //infinite retees
//   //our connection status
//   //  $(".connection_status").hide();
//   //  $("#connection_failed").show();
// }

function show_alert(message, type = 'danger')
{
  //we only need one alert at a time.
  $('#liveAlertPlaceholder').html(AlertBox(message, type))

  //make sure we can see it.
  $('html').animate({
      scrollTop: 0
    },
    750 //speed
  );
}

function toggle_state(id)
{
  //OFF or TRIP both switch it to off.
  let new_state = false;
  if ($("#pwmState" + id).text() == "OFF")
    new_state = true;

  client.setPWMChannelState(id, new_state, true);
}

function toggle_adc_details(id)
{
  $(`#adc${id}Details`).toggle();
}

function toggle_duty_cycle(id)
{
  $(`#pwmDutySliderRow${id}`).toggle();
}

function open_page(page)
{
  yarrboard_log(`opening ${page}`);

  if (!page_permissions[app_role].includes(page))
  {
    yarrboard_log(`${page} not allowed for ${app_role}`);
    return;
  }

  if (page == current_page)
  {
    yarrboard_log(`already on ${page}.`);
    //return;
  }

  current_page = page;

  //request our stats.
  if (page == "stats")
    get_stats_data();

  //request our control updates.
  if (page == "control")
    get_update_data();

  //hide all pages.
  $("div.pageContainer").hide();

  //special stuff
  if (page == "login")
  {
    //hide our nav bar
    $("#navbar").hide();

    //enter triggers login
      $(document).on('keypress',function(e) {
        if(e.which == 13)
            do_login();
    });
  }

  //sad to see you go.
  if (page == "logout")
  {
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
  else
  {
    //update our nav
    $('.nav-link').removeClass("active");
    $(`#${page}Nav a`).addClass("active");

    //is our new page ready?
    on_page_ready();
  }
}

function on_page_ready()
{
  //is our page ready yet?
  if (page_ready[current_page])
  {
    $("#loading").hide();
    $(`#${current_page}Page`).show();
  }
  else
  {
    $("#loading").show();
    setTimeout(on_page_ready, 100);
  }
}

function get_stats_data()
{
  if (client.isOpen() && (app_role == 'guest' || app_role == 'admin'))
  {
    //yarrboard_log("get_stats");

    client.getStats();

    //keep loading it while we are here.
    if (current_page == "stats")
      setTimeout(get_stats_data, app_update_interval);
  }
}

function get_update_data()
{
  if (client.isOpen() && (app_role == 'guest' || app_role == 'admin'))
  {
    //yarrboard_log("get_update");

    client.getUpdate();

    //keep loading it while we are here.
    if (current_page == "control")
      setTimeout(get_update_data, app_update_interval);
  }
}

function validate_board_name(e)
{
  let ele = e.target;
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
    $(ele).removeClass("is-invalid");
    $(ele).addClass("is-valid");

    //set our new board name!
    client.send({
      "cmd": "set_boardname",
      "value": value
    });
  }
}

function set_duty_cycle(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  //must be realistic.
  if (value >= 0 && value <= 100)
  {
    //update our button
    $(`#pwmDutyCycle${id}`).html(Math.round(value) + '%');

    //we want a duty value from 0 to 1
    value = value / 100;
  
    //update our duty cycle
    client.setPWMChannelDuty(id, value, false);
  }
}

function validate_pwm_name(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
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

function validate_pwm_dimmable(e)
{
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

function validate_pwm_enabled(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fPWMName${id}`).prop('disabled', !value);
  $(`#fPWMDimmable${id}`).prop('disabled', !value);
  $(`#fPWMSoftFuse${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_pwm_channel",
    "id": id,
    "enabled": value
  });
}

function validate_pwm_soft_fuse(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = parseFloat(ele.value);

  //real numbers only, pal.
  if (isNaN(value) || value <= 0 || value > 20)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
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

function validate_switch_name(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
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

function validate_switch_mode(e)
{
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

function validate_switch_enabled(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fSwitchName${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  client.send({
    "cmd": "config_switch",
    "id": id,
    "enabled": value
  });
}

function set_rgb_color(e, color)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];

  let rgb = color.toRgb();

  let red = rgb.r / 255;
  let green = rgb.g / 255;
  let blue = rgb.b / 255;

  client.setRGB(id, red.toFixed(4), green.toFixed(4), blue.toFixed(4), false);
}

function validate_rgb_name(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
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

function validate_rgb_enabled(e)
{
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

function validate_adc_name(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
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

function validate_adc_enabled(e)
{
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

function do_login(e)
{
  app_username = $('#username').val();
  app_password = $('#password').val();

  client.login(app_username, app_password);
}

function save_network_settings()
{
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
  setTimeout(function (){
    location.reload();
  }, 2500);  
}

function save_app_settings()
{
  //get our data
  let admin_user = $("#admin_user").val();
  let admin_pass = $("#admin_pass").val();
  let guest_user = $("#guest_user").val();
  let guest_pass = $("#guest_pass").val();
  let default_role = $("#default_role option:selected").val();
  yarrboard_log(default_role);
  let app_enable_mfd = $("#app_enable_mfd").prop("checked");
  let app_enable_api = $("#app_enable_api").prop("checked");
  let app_enable_serial = $("#app_enable_serial").prop("checked");
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
  if (default_app_role != "admin")
  {
    Cookies.set('username', admin_user, { expires: 365 });
    Cookies.set('password', admin_pass, { expires: 365 });
  }
  else
  {
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
    "app_enable_ssl": app_enable_ssl,
    "server_cert": server_cert,
    "server_key": server_key
  });

  //if they are changing from client to client, we can't show a success.
  show_alert("App settings have been updated.", "success");
}

function restart_board()
{
  if (confirm("Are you sure you want to restart your Yarrboard?"))
  {
    client.restart();

    show_alert("Yarrboard is now restarting, please be patient.", "primary");
    
    setTimeout(function (){
      location.reload();
    }, 5000);
  }
}

function reset_to_factory()
{
  if (confirm("WARNING! Are you sure you want to reset your Yarrboard to factory defaults?  This cannot be undone."))
  {
    client.factoryReset();

    show_alert("Yarrboard is now resetting to factory defaults, please be patient.", "primary");
  }
}

function check_for_updates()
{
  //did we get a config yet?
  if (current_config)
  {
    $.ajax({
      url: "https://raw.githubusercontent.com/hoeken/yarrboard-firmware/main/firmware.json",
      cache: false,
      dataType: "json",
      success: function(jdata) {
        //did we get anything?
        let data;
        for (firmware of jdata)
          if (firmware.type == current_config.hardware_version)
            data = firmware;

        if (!data)
        {
          show_alert(`Could not find a firmware for this hardware.`, "danger");
          return;
        }

        $("#firmware_checking").hide();

        //do we have a new version?
        if (compareVersions(data.version, current_config.firmware_version))
        {
          if (data.changelog)
          {
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

function update_firmware()
{
  $("#btn_update_firmware").prop("disabled", true);
  $("#progress_wrapper").show();

  //okay, send it off.
  client.startOTA();

  ota_started = true;
}

function toggle_role_passwords(role)
{
  if (role == "admin")
  {
    $(".admin_credentials").hide();
    $(".guest_credentials").hide();
  }
  else if (role == "guest")
  {
    $(".admin_credentials").show();
    $(".guest_credentials").hide();
  }
  else
  {
    $(".admin_credentials").show();
    $(".guest_credentials").show();
  }  
}

function update_role_ui()
{
  //what nav tabs should we be able to see?
  if (app_role == "admin")
  {
    $("#navbar").show();
    $(".nav-permission").show();
  }
  else if (app_role == "guest")
  {
    $("#navbar").show();
    $(".nav-permission").hide();
    page_permissions[app_role].forEach((page) => {
      $(`#${page}Nav`).show();
    });
  }
  else
  {
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

function open_default_page()
{
  if (app_role != 'nobody')
  {
    //check to see if we want a certain page
    if (window.location.hash)
    {
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

function secondsToDhms(seconds)
{
  seconds = Number(seconds);
  var d = Math.floor(seconds / (3600*24));
  var h = Math.floor(seconds % (3600*24) / 3600);
  var m = Math.floor(seconds % 3600 / 60);
  var s = Math.floor(seconds % 60);
  
  var dDisplay = d > 0 ? d + (d == 1 ? " day, " : " days, ") : "";
  var hDisplay = h > 0 ? h + (h == 1 ? " hour, " : " hours, ") : "";
  var mDisplay = (m > 0 && d == 0) ? m + (m == 1 ? " minute, " : " minutes, ") : "";
  var sDisplay = (s > 0 && d == 0 && h == 0 && m == 0) ? s + (s == 1 ? " second" : " seconds") : "";

  return (dDisplay + hDisplay + mDisplay + sDisplay).replace(/,\s*$/, "");
}

function formatAmpHours(aH)
{
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

function formatWattHours(wH)
{
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
function compareVersions(first, second)
{

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

function yarrboard_log(message)
{
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

function getPreferredTheme()
{
  const storedTheme = getStoredTheme();

  if (storedTheme)
    return storedTheme;

  return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
}

function setTheme(theme)
{
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