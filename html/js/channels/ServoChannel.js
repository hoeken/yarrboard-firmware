(function (global) { //private scope

  // work in the global YB namespace.
  var YB = global.YB || {};

  function ServoChannel() {
    YB.BaseChannel.call(this, "servo", "Servo");

    this.setAngle = this.setAngle.bind(this);
  }

  ServoChannel.prototype = Object.create(YB.BaseChannel.prototype);
  ServoChannel.prototype.constructor = ServoChannel;

  ServoChannel.currentSliderID = -1;

  ServoChannel.prototype.getConfigSchema = function () {
    // copy base schema to avoid mutating the base literal
    var base = YB.BaseChannel.prototype.getConfigSchema.call(this);

    var schema = Object.assign({}, base);
    return schema;
  };

  ServoChannel.prototype.generateControlUI = function () {
    return `
      <div id="servoControlCard${this.id}" class="col-xs-12 col-sm-6">
        <div class="p-3 bg-secondary border border-secondary rounded text-white">
        <table class="w-100 h-100 p-2">
            <tr>
              <td width="75%" id="servoName${this.id}">${this.name}</td>
              <td id="servoAngle${this.id}"></td>
            </tr>
            <tr>
              <td colspan="2">
                <input type="range" class="form-range" min="0" max="180" step="1" id="servoSlider${this.id}" list="servoMarker${this.id}">
                <datalist id="servoMarker${this.id}">
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
      </div>
    `;
  };

  ServoChannel.prototype.setupControlUI = function () {
    YB.BaseChannel.prototype.setupControlUI.call(this);

    $('#servoSlider' + this.id).change(this.setAngle);

    //update our duty when we move
    $('#servoSlider' + this.id).on("input", this.setAngle);

    //stop updating the UI when we are choosing a duty
    $('#servoSlider' + this.id).on('focus', function (e) {
      let ele = e.target;
      let id = ele.id.match(/\d+/)[0];
      ServoChannel.currentSliderID = id;
    });

    //stop updating the UI when we are choosing a duty
    $('#servoSlider' + this.id).on('touchstart', function (e) {
      let ele = e.target;
      let id = ele.id.match(/\d+/)[0];
      ServoChannel.currentSliderID = id;
    });

    //restart the UI updates when slider is closed
    $('#servoSlider' + this.id).on("blur", function (e) {
      ServoChannel.currentSliderID = -1;
    });

    //restart the UI updates when slider is closed
    $('#servoSlider' + this.id).on("touchend", function (e) {
      ServoChannel.currentSliderID = -1;
    });
  };

  ServoChannel.prototype.setAngle = function (e) {
    let ele = e.target;
    let value = ele.value;

    //must be realistic.
    if (value >= 0 && value <= 180) {
      //update our button
      $(`#servoAngle${this.id}`).html(`${value}°`);

      YB.client.send({
        "cmd": "set_servo_channel",
        "id": this.id,
        "angle": value
      });
    }
  }

  ServoChannel.prototype.generateEditUI = function () {

    let standardFields = this.generateStandardEditFields();

    return `
      <div id="servoEditCard${this.id}" class="col-xs-12 col-sm-6">
        <div class="p-3 border border-secondary rounded">
          <h5>Servo Channel #${this.id}</h5>
          ${standardFields}
        </div>
      </div>
    `;
  };

  ServoChannel.prototype.setupEditUI = function () {
    YB.BaseChannel.prototype.setupEditUI.call(this);
  };

  ServoChannel.prototype.updateControlUI = function () {
    if (ServoChannel.currentSliderID != this.id) {
      $('#servoSlider' + this.id).val(this.data.angle);
      $('#servoAngle' + this.id).html(`${this.data.angle}°`);
    }

    $(`#servoControlCard${this.id}`).toggle(this.enabled);
  };

  YB.ServoChannel = ServoChannel;
  YB.ChannelRegistry.registerChannelType("servo", YB.ServoChannel)

  global.YB = YB; // <-- this line makes it global

})(this); //private scope