(function (global) { //private scope

  // work in the global YB namespace.
  var YB = global.YB || {};

  function ServoChannel() {
    YB.BaseChannel.call(this, "servo", "Servo");

    this.onEditForm = this.onEditForm.bind(this);
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
      <div id="servo${this.id}" class="col-xs-12 col-sm-6">
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
    `;
  };

  ServoChannel.prototype.setupControlUI = function () {

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
      ServoChannel.currentSliderID = id;
    });

    //restart the UI updates when slider is closed
    $('#servoSlider' + this.id).on("touchend", function (e) {
      ServoChannel.currentSliderID = id;
    });
  };

  ServoChannel.prototype.setAngle = function () {
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
    $(`#fRelayEnabled${this.id}`).prop("checked", this.enabled);

    //enable/disable other stuff.
    $(`#fRelayName${this.id}`).prop('disabled', !this.enabled);

    //validate + save
    $(`#fRelayEnabled${this.id}`).change(this.onEditForm);
    $(`#fRelayName${this.id}`).change(this.onEditForm);
  };

  ServoChannel.prototype.onEditForm = function (e) {

    //layer our form data over our existing config.
    let newcfg = this.cfg;
    newcfg.name = $(`#f-relay-name-${this.id}`).val()
    newcfg.enabled = $(`#f-relay-enabled-${this.id}`).prop("checked");
    newcfg.key = $(`#f-relay-key-${this.id}`).val();

    this.handleEditForm(newcfg, e);

    //ui updates
    $(`#f-relay-name-${this.id}`).prop('disabled', !this.enabled);
    $(`#f-relay-key-${this.id}`).prop('disabled', !this.enabled);
    $(`#f-relay-type-${this.id}`).prop('disabled', !this.enabled);
  };

  RelayChannel.prototype.updateControlUI = function () {
    if (RelayChannel.currentSliderID != this.id) {
      $('#servoSlider' + this.id).val(ch.angle);
      $('#servoAngle' + this.id).html(`${ch.angle}°`);
    }
  };

  YB.ServoChannel = ServoChannel;
  YB.ChannelRegistry.registerChannelType("servo", YB.ServoChannel)

  global.YB = YB; // <-- this line makes it global

})(this); //private scope