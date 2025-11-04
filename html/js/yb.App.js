(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  const YarrboardClient = window.YarrboardClient;

  YB.App = {
    config: {},

    messageHandlers: {},

    addMessageHandler: function (message, handler) {
      if (!YB.App.messageHandlers[message])
        YB.App.messageHandlers[message] = [];
      YB.App.messageHandlers[message].push(handler);
    },

    onMessage: function (msg) {
      if (msg.debug)
        YB.log(`SERVER: ${msg.debug}`);

      if (msg.msg) {
        const handlers = YB.App.messageHandlers[msg.msg];
        if (!handlers) {
          YB.log("No handler for: " + JSON.stringify(msg));
          return;
        }

        for (let handler of handlers) {
          try {
            handler(msg);
          } catch (err) {
            YB.log(`Handler for '${msg.msg}' failed:`, err);
          }
        }
      }
    },

    updateInterval: 500,
    updateIntervalId: null,

    username: null,
    password: null,
    role: "nobody",
    defaultRole: "nobody",

    currentPage: null,
    pageList: ["control", "config", "stats", "graphs", "network", "settings", "system"],
    pageReady: {
      "control": false,
      "config": false,
      "stats": false,
      "graphs": false,
      "network": false,
      "settings": false,
      "system": true,
      "login": true
    },

    pagePermissions: {
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
    },

    currentlyPickingBrightness: false,

    start: function () {
      YB.log.setupDebugTerminal();
      // YB.log("User Agent: " + navigator.userAgent);
      // YB.log("Window Width: " + window.innerWidth);
      // YB.log("Window Height: " + window.innerHeight);
      // YB.log("Window Location: " + window.location);
      // YB.log("Device Pixel Ratio: " + window.devicePixelRatio);
      // YB.log("Is canvas supported? " + YB.Util.isCanvasSupported());

      //main data connection
      YB.App.startWebsocket();

      //check our connection status.
      setInterval(YB.App.checkConnectionStatus, 100);

      //light/dark theme init.
      let theme = YB.App.getPreferredTheme();
      // YB.log(`preferred theme: ${theme}`);
      YB.App.setTheme(theme);
      $("#darkSwitch").change(YB.App.updateThemeSwitch);

      // preferred scheme watcher
      window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
        const storedTheme = YB.App.getStoredTheme();
        if (storedTheme !== 'light' && storedTheme !== 'dark')
          YB.App.setTheme(YB.App.getPreferredTheme());
      });

      //brightness slider callbacks
      $("#brightnessSlider").change(YB.App.setBrightness);
      $("#brightnessSlider").on("input", YB.App.setBrightness);

      //stop updating the UI when we are choosing
      $("#brightnessSlider").on('focus', function (e) {
        YB.App.currentlyPickingBrightness = true;
      });

      //stop updating the UI when we are choosing
      $("#brightnessSlider").on('touchstart', function (e) {
        YB.App.currentlyPickingBrightness = true;
      });

      //restart the UI updates when slider is closed
      $("#brightnessSlider").on("blur", function (e) {
        YB.App.currentlyPickingBrightness = false;
      });

      //restart the UI updates when slider is closed
      $("#brightnessSlider").on("touchend", function (e) {
        YB.App.currentlyPickingBrightness = false;
      });
    },

    startWebsocket: function () {
      //do we want ssl?
      let use_ssl = false;
      if (document.location.protocol == 'https:')
        use_ssl = true;

      //set it up
      YB.client = new YarrboardClient(window.location.host, "", "", false, use_ssl);

      //figure out what our login situation is
      YB.client.onopen = function () {
        YB.client.sayHello();
      };

      //app handles messages
      YB.client.onmessage = YB.App.onMessage;

      //use custom logger
      YB.client.log = YB.log;

      YB.client.start();
    },

    openPage: function (page) {
      //YB.log(`opening ${page}`);

      if (!YB.App.pagePermissions[YB.App.role].includes(page)) {
        YB.log(`${page} not allowed for ${YB.App.role}`);
        return;
      }

      YB.App.currentPage = page;

      //request our stats.
      if (page == "stats")
        YB.App.getStatsData();

      //request our historical graph data (if any)
      if (page == "graphs") {
        YB.App.getGraphData();
      }

      //request our control updates.
      if (page == "control")
        YB.App.startUpdateData();

      //we need updates for adc config page.
      if (page == "config" && YB.App.config && YB.App.config.hasOwnProperty("adc"))
        YB.App.startUpdateData();

      //hide all pages.
      $("div.pageContainer").hide();

      //special stuff
      if (page == "login") {
        //hide our nav bar
        $("#navbar").hide();

        //enter triggers login
        $(document).on('keypress', function (e) {
          if (e.which == 13)
            YB.App.doLogin();
        });
      }

      //sad to see you go.
      if (page == "logout") {
        Cookies.remove("username");
        Cookies.remove("password");

        YB.App.role = YB.App.defaultRole;
        YB.App.updateRoleUI();

        YB.client.logout();

        if (YB.App.role == "nobody")
          YB.App.openPage("login");
        else
          YB.App.openPage("control");
      }
      else {
        //update our nav
        $('#controlNav .nav-link').removeClass("active");
        $(`#${page}Nav a`).addClass("active");

        //is our new page ready?
        YB.App.onPageReady();
      }
    },

    isMFD: function () {
      if (YB.Util.getQueryVariable("mfd_name") !== null)
        return true;

      return false;
    },

    BoardNameEdit: (name) => `
      <div class="col-12">
        <h4>Board Name</h4>
        <input type="text" class="form-control" id="fBoardName" value="${name}">
        <div class="valid-feedback">Saved!</div>
        <div class="invalid-feedback">Must be 30 characters or less.</div>
      </div>`,

    AlertBox: (message, type) => `
      <div>
        <div class="mt-3 alert alert-${type} alert-dismissible" role="alert">
          <div>${message}</div>
          <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
        </div>
      </div>`,

    loadConfigs: function () {
      if (YB.App.role == "nobody")
        return;

      if (YB.App.role == "admin") {
        YB.client.getNetworkConfig();
        YB.client.getAppConfig();

        YB.client.send({
          "cmd": "get_full_config"
        });

        YB.App.checkForUpdates();
      }

      YB.client.getConfig();
    },

    checkConnectionStatus: function () {
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
    },

    showAlert: function (message, type = 'danger') {
      //we only need one alert at a time.
      $('#liveAlertPlaceholder').html(YB.App.AlertBox(message, type))

      console.log(`YB.App.showAlert: ${message}`);

      //make sure we can see it.
      $('html').animate({
        scrollTop: 0
      },
        750 //speed
      );
    },

    showAdminAlert: function (message, type = 'danger') {
      //we only need one alert at a time.
      $('#adminAlertPlaceholder').html(YB.App.AlertBox(message, type))

      console.log(`YB.App.showAdminAlert: ${message}`);

      //make sure we can see it.
      $('html').animate({
        scrollTop: 0
      },
        750 //speed
      );
    },

    addConfigurationDragDropHandler: function () {
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
    },

    onPageReady: function () {
      //is our page ready yet?
      if (YB.App.pageReady[YB.App.currentPage]) {
        $("#loading").hide();
        $(`#${YB.App.currentPage}Page`).show();
      }
      else {
        $("#loading").show();
        setTimeout(YB.App.onPageReady, 100);
      }
    },

    getStatsData: function () {
      if (YB.client.isOpen() && (YB.App.role == 'guest' || YB.App.role == 'admin')) {
        //YB.log("get_stats");

        YB.client.getStats();

        //keep loading it while we are here.
        if (YB.App.currentPage == "stats")
          setTimeout(YB.App.getStatsData, YB.App.updateInterval);
      }
    },

    getGraphData: function () {
      if (YB.client.isOpen() && (YB.App.role == 'guest' || YB.App.role == 'admin')) {
        YB.client.send({
          "cmd": "get_graph_data"
        });
      }
    },

    startUpdateData: function () {
      if (!YB.App.updateIntervalId) {
        //YB.log("starting updates");
        YB.App.updateIntervalId = setInterval(YB.App.getUpdateData, YB.App.updateInterval);
      } else {
        //YB.log("updates already running");
      }
    },

    getUpdateData: function () {
      if (YB.client.isOpen() && (YB.App.role == 'guest' || YB.App.role == 'admin')) {
        //YB.log("get_update");

        YB.client.getUpdate();

        //keep loading it while we are here.
        if (YB.App.currentPage == "control" || YB.App.currentPage == "graphs" || (YB.App.currentPage == "config" && YB.App.config && YB.App.config.hasOwnProperty("adc")))
          return;
        else {
          //YB.log("stopping updates");
          clearInterval(YB.App.updateIntervalId);
          YB.App.updateIntervalId = 0;
        }
      }
    },

    validateBoardName: function (e) {
      let ele = e.target;
      let value = ele.value;

      if (value.length <= 0 || value.length > 30) {
        $(ele).removeClass("is-valid");
        $(ele).addClass("is-invalid");
      } else {
        $(ele).removeClass("is-invalid");
        $(ele).addClass("is-valid");

        //set our new board name!
        YB.client.send({
          "cmd": "set_boardname",
          "value": value
        });
      }
    },

    setBrightness: function (e) {
      let ele = e.target;
      let value = ele.value;

      //must be realistic.
      if (value >= 0 && value <= 100) {
        //we want a duty value from 0 to 1
        value = value / 100;

        //update our duty cycle
        YB.client.setBrightness(value, false);
      }
    },

    updateThemeSwitch: function () {
      if (YB.App.checked)
        YB.App.setTheme("dark");
      else
        YB.App.setTheme("light");
    },

    doLogin: function (e) {
      YB.App.username = $('#username').val();
      YB.App.password = $('#password').val();
      YB.client.login(YB.App.username, YB.App.password);
    },

    saveNetworkSettings: function () {
      //get our data
      let wifi_mode = $("#wifi_mode").val();
      let wifi_ssid = $("#wifi_ssid").val();
      let wifi_pass = $("#wifi_pass").val();
      let local_hostname = $("#local_hostname").val();

      //we should probably do a bit of verification here

      //if they are changing from client to client, we can't show a success.
      YB.App.showAlert("Yarrboard may be unresponsive while changing WiFi settings. Make sure you connect to the right network after updating.", "primary");

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
    },

    saveAppSettings: function () {
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
      YB.App.updateInterval = update_interval;

      //remember it and update our UI
      YB.App.defaultRole = default_role;
      YB.App.updateRoleUI();

      //helper function to keep admin logged in.
      if (YB.App.defaultRole != "admin") {
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
        "app_update_interval": YB.App.updateInterval,
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

      YB.App.showAlert("App settings have been updated.", "success");
    },

    restartBoard: function () {
      if (confirm("Are you sure you want to restart your Yarrboard?")) {
        YB.client.restart();

        YB.App.showAlert("Yarrboard is now restarting, please be patient.", "primary");

        setTimeout(function () {
          location.reload();
        }, 5000);
      }
    },

    resetToFactory: function () {
      if (confirm("WARNING! Are you sure you want to reset your Yarrboard to factory defaults?  This cannot be undone.")) {
        YB.client.factoryReset();

        YB.App.showAlert("Yarrboard is now resetting to factory defaults, please be patient.", "primary");
      }
    },

    checkForUpdates: function () {
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
              //YB.App.showAlert(`Could not find a firmware for this hardware.`, "danger");
              return;
            }

            $("#firmware_checking").hide();

            //do we have a new version?
            if (YB.Util.compareVersions(data.version, YB.App.config.firmware_version)) {
              if (data.changelog) {
                $("#firmware_changelog").append(marked.parse(data.changelog));
                $("#firmware_changelog").show();
              }

              $("#new_firmware_version").html(data.version);
              $("#firmware_bin").attr("href", `${data.url}`);
              $("#firmware_update_available").show();

              YB.App.showAlert(`There is a <a onclick="YB.App.openPage('system')" href="/#system">firmware update</a> available (${data.version}).`, "primary");
            }
            else
              $("#firmware_up_to_date").show();
          }
        });
      }
      //wait for it.
      else
        setTimeout(YB.App.checkForUpdates, 1000);
    },

    updateFirmware: function () {
      $("#btn_update_firmware").prop("disabled", true);
      $("#progress_wrapper").show();

      //okay, send it off.
      YB.client.startOTA();
    },

    toggleRolePasswords: function (role) {
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
    },

    updateRoleUI: function () {
      //what nav tabs should we be able to see?
      if (YB.App.role == "admin") {
        $("#navbar").show();
        $(".nav-permission").show();
      }
      else if (YB.App.role == "guest") {
        $("#navbar").show();
        $(".nav-permission").hide();
        YB.App.pagePermissions[YB.App.role].forEach((page) => {
          $(`#${page}Nav`).show();
        });
      }
      else {
        $("#navbar").hide();
        $(".nav-permission").hide();
      }

      //show login or not?
      $('#loginNav').hide();
      if (YB.App.defaultRole == 'nobody' && YB.App.role == 'nobody')
        $('#loginNav').show();
      if (YB.App.defaultRole == 'guest' && YB.App.role == 'guest')
        $('#loginNav').show();

      //show logout or not?
      $('#logoutNav').hide();
      if (YB.App.defaultRole == 'nobody' && YB.App.role != 'nobody')
        $('#logoutNav').show();
      if (YB.App.defaultRole == 'guest' && YB.App.role == 'admin')
        $('#logoutNav').show();
    },

    openDefaultPage: function () {
      if (YB.App.role != 'nobody') {
        //check to see if we want a certain page
        if (window.location.hash) {
          let page = window.location.hash.substring(1);
          if (page != "login" && YB.App.pageList.includes(page))
            YB.App.openPage(page);
          else
            YB.App.openPage("control");
        }
        else
          YB.App.openPage("control");
      }
      else
        YB.App.openPage('login');
    },

    getStoredTheme: function () { localStorage.getItem('theme'); },
    setStoredTheme: function () { localStorage.setItem('theme', theme); },

    getPreferredTheme: function () {
      //did we get one passed in? b&g, etc pass in like this.
      let mode = YB.Util.getQueryVariable("mode");
      // YB.log(`mode: ${mode}`);
      if (mode !== null) {
        if (mode == "night")
          return "dark";
        else
          return "light";
      }

      //do we have stored one?
      const storedTheme = YB.App.getStoredTheme();
      if (storedTheme)
        return storedTheme;

      //prefs?
      return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
    },

    setTheme: function (theme) {
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
    },

    handleHelloMessage: function (msg) {
      YB.App.role = msg.role;
      YB.App.defaultRole = msg.default_role;

      //light/dark mode
      //let the mfd override with ?mode=night, etc.
      if (!YB.Util.getQueryVariable("mode")) {
        if (msg.theme)
          YB.App.setTheme(msg.theme);
      }

      //auto login?
      if (Cookies.get("username") && Cookies.get("password")) {
        YB.client.login(Cookies.get("username"), Cookies.get("password"));
      } else {
        YB.App.updateRoleUI();
        YB.App.loadConfigs();
        YB.App.openDefaultPage();
      }
    },

    handleConfigMessage: function (msg) {
      // YB.log("config");
      // YB.log(msg);

      YB.App.config = msg;

      //is it our first boot?
      if (msg.first_boot && YB.App.currentPage != "network")
        YB.App.showAlert(`Welcome to Yarrboard, head over to <a href="#network" onclick="YB.App.openPage('network')">Network</a> to setup your WiFi.`, "primary");

      //did we get a crash?
      if (msg.has_coredump)
        YB.App.showAdminAlert(`
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
        if (YB.App.currentPage == 'config')
          YB.App.startUpdateData();
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
        YB.App.pageList = YB.App.pageList.filter(p => p !== "graphs");
        delete YB.App.pageReady.graphs;
        for (const role in YB.App.pagePermissions) {
          YB.App.pagePermissions[role] = YB.App.pagePermissions[role].filter(p => p !== "graphs");
        }
      }

      //only do it as needed
      if (!YB.App.pageReady.config || YB.App.currentPage != "config") {

        //board name controls
        $('#boardConfigForm').html(YB.App.BoardNameEdit(msg.name));
        $("#fBoardName").change(YB.App.validateBoardName);

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
      if (msg.brightness && !YB.App.currentlyPickingBrightness)
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
      YB.App.pageReady.config = true;

      if (!YB.App.currentPage)
        YB.App.openPage('control');
    },

    handleUpdateMessage: function (msg) {
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
            if (YB.App.currentPage == "config") {
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

      YB.App.pageReady.control = true;
    },

    handleStatsMessage: function (msg) {
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
        let rssi_icon = YB.Util.getWifiIconForRSSI(msg.rssi);
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

      YB.App.pageReady.stats = true;
    },

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

    handleFullConfigMessage: function (msg) {
      //YB.log("full config");

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

      YB.App.addConfigurationDragDropHandler();
    },

    handleNetworkConfigMessage: function (msg) {
      //YB.log("network config");

      //YB.log(msg);
      $("#wifi_mode").val(msg.wifi_mode);
      $("#wifi_ssid").val(msg.wifi_ssid);
      $("#wifi_pass").val(msg.wifi_pass);
      $("#local_hostname").val(msg.local_hostname);

      YB.App.pageReady.network = true;
    },

    handleAppConfigMessage: function (msg) {
      //YB.log("app config");

      //update some ui stuff
      YB.App.updateRoleUI();
      YB.App.toggleRolePasswords(msg.default_role);

      //what is our update interval?
      if (msg.app_update_interval)
        YB.App.updateInterval = msg.app_update_interval;

      //YB.log(msg);
      $("#admin_user").val(msg.admin_user);
      $("#admin_pass").val(msg.admin_pass);
      $("#guest_user").val(msg.guest_user);
      $("#guest_pass").val(msg.guest_pass);
      $("#app_update_interval").val(msg.app_update_interval);
      $("#default_role").val(msg.default_role);
      $("#default_role").change(function () { YB.App.toggleRolePasswords($(this).val()) });
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

      YB.App.pageReady.settings = true;
    },

    handleOTAProgressMessage: function (msg) {
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
        YB.App.showAlert("Firmware update successful.", "success");

        //reload our page
        setTimeout(function () {
          location.reload(true);
        }, 2500);
      }
    },

    handleErrorMessage: function (msg) {
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
        YB.App.showAlert(msg.message);
    },

    handleLoginMessage: function (msg) {

      //keep the login success stuff on the login page.
      if (msg.message == "Login successful.") {
        //once we know our role, we can load our other configs.
        YB.App.role = msg.role;

        // YB.log(`YB.App.role: ${YB.App.role}`);
        // YB.log(`YB.App.currentPage: ${YB.App.currentPage}`);

        //only needed for login page, otherwise its autologin
        if (YB.App.currentPage == "login") {
          //save user/pass to cookies.
          if (YB.App.username && YB.App.password) {
            Cookies.set('username', YB.App.username, { expires: 365 });
            Cookies.set('password', YB.App.password, { expires: 365 });
          }
        }

        //prep the site
        YB.App.updateRoleUI();
        YB.App.loadConfigs();
        YB.App.openDefaultPage();
      }
      else
        YB.App.showAlert(msg.message, "success");
    },

    handleSetThemeMessage: function (msg) {
      //light/dark mode
      YB.App.setTheme(msg.theme);
    },

    handleSetBrightnessMessage: function (msg) {
      //did we get brightness?
      if (msg.brightness && !YB.App.currentlyPickingBrightness)
        $('#brightnessSlider').val(Math.round(msg.brightness * 100));
    },

  };

  //setup all of our message handlers.
  YB.App.addMessageHandler("hello", YB.App.handleHelloMessage);
  YB.App.addMessageHandler("config", YB.App.handleConfigMessage);
  YB.App.addMessageHandler("update", YB.App.handleUpdateMessage);
  YB.App.addMessageHandler("stats", YB.App.handleStatsMessage);
  YB.App.addMessageHandler("graph_data", YB.App.handleGraphDataMessage);
  YB.App.addMessageHandler("full_config", YB.App.handleFullConfigMessage);
  YB.App.addMessageHandler("network_config", YB.App.handleNetworkConfigMessage);
  YB.App.addMessageHandler("app_config", YB.App.handleAppConfigMessage);
  YB.App.addMessageHandler("ota_progress", YB.App.handleOTAProgressMessage);
  YB.App.addMessageHandler("error", YB.App.handleErrorMessage);
  YB.App.addMessageHandler("login", YB.App.handleLoginMessage);
  YB.App.addMessageHandler("set_theme", YB.App.handleSetThemeMessage);
  YB.App.addMessageHandler("set_brightness", YB.App.handleSetBrightnessMessage);

  // expose to global
  global.YB = YB;
})(this);