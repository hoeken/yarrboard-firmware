(function (global) { //private scope

  // work in the global YB namespace.
  var YB = global.YB || {};

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

  let currentServoSliderID = -1;

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

  function ServoChannel() {
    YB.BaseChannel.call(this, "servo");
  }
  ServoChannel.prototype = Object.create(YB.BaseChannel.prototype);
  ServoChannel.prototype.constructor = ServoChannel;

  ServoChannel.prototype.parseConfig = function (cfg) {
    YB.BaseChannel.prototype.parseConfig.call(this, cfg);
  };

  YB.ServoChannel = ServoChannel;
  YB.ChannelRegistry.registerChannelType("servo", YB.ServoChannel)

  window.YB = YB; // <-- this line makes it global

})(this); //private scope