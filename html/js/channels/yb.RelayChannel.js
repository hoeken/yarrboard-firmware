(function (global) { //private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

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

  function toggle_relay_state(id) {
    YB.client.send({
      "cmd": "toggle_relay_channel",
      "id": id,
      "source": YB.App.config.hostname
    }, true);
  }



  function RelayChannel() {
    YB.BaseChannel.call(this, "relay");
  }
  RelayChannel.prototype = Object.create(YB.BaseChannel.prototype);
  RelayChannel.prototype.constructor = RelayChannel;

  RelayChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.RelayChannel = RelayChannel;
  YB.ChannelRegistry.registerChannelType("relay", YB.RelayChannel)

  window.YB = YB; // <-- this line makes it global

})(this); //private scope