(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  const ADC_TYPES = {
    "raw": "Raw Output",
    "digital_switch": "Digital Switching",
    "thermistor": "Thermistor",
    "4-20ma": "4-20mA Sensor",
    "high_volt_divider": "0-32v Input",
    "low_volt_divider": "0-5v Input",
    "ten_k_pullup": "10k Pullup"
  };

  function ADCChannel() {
    YB.BaseChannel.call(this, "adc", "ADC");

    this.runningAverage = [];

    // Bind UI handlers
    this.onAddCalibrationRow = this.onAddCalibrationRow.bind(this);
    // this.onRemoveCalibrationRow = this.onRemoveCalibrationRow.bind(this);
    this.onCopyAverage = this.onCopyAverage.bind(this);
    this.onResetAverage = this.onResetAverage.bind(this);
  }

  ADCChannel.prototype = Object.create(YB.BaseChannel.prototype);
  ADCChannel.prototype.constructor = ADCChannel;

  ADCChannel.prototype.getConfigSchema = function () {
    // copy base schema to avoid mutating the base literal
    var base = YB.BaseChannel.prototype.getConfigSchema.call(this);
    var schema = Object.assign({}, base);

    schema.type = {
      presence: { allowEmpty: false },
      type: "string",
      length: { minimum: 1, maximum: 32 }
    };

    schema.displayDecimals = {
      numericality: {
        onlyInteger: true,
        greaterThanOrEqualTo: 0,
        lessThanOrEqualTo: 4
      }
    };

    schema.useCalibrationTable = {
      inclusion: {
        within: [true, false],
        message: "^useCalibrationTable must be boolean"
      }
    };

    schema.calibratedUnits = {
      type: "string",
      length: { minimum: 0, maximum: 10 }
    };

    schema.calibrationTable = {
      type: "array"
    };

    return schema;
  };

  ADCChannel.prototype.generateControlUI = function () {
    // Adapted from table row to Card format to match RelayChannel
    return `
      <div id="adcControlCard${this.id}" class="col-xs-12 col-sm-6">
        <table class="w-100 h-100 p-2">
          <tr>
            <td>
              <div class="card p-2 text-center">
                 <div class="fw-bold">${this.name}</div>
                 <div class="text-muted small">${ADC_TYPES[this.cfg.type] || this.cfg.type}</div>
                 <h3 id="adcValue${this.id}">--</h3>
              </div>
            </td>
          </tr>
        </table>
      </div>
    `;
  };

  ADCChannel.prototype.updateControlUI = function () {
    // 1. Get Data
    // We assume the incoming data packet (this.data) contains 'value' and 'calibrated_value'
    // per your snippet's logic.
    let value = parseFloat(this.data.value);

    // Safety check if data hasn't arrived yet
    if (isNaN(value)) return;

    let calibrated_value = value;

    // 2. Load Defaults from Config
    let units = this.cfg.units || "";

    // Are we using a calibration table?
    if (this.cfg.useCalibrationTable) {
      units = this.cfg.calibratedUnits || "";
      if (this.data.calibrated_value !== undefined) {
        calibrated_value = parseFloat(this.data.calibrated_value);
      }
    }

    // 3. Update Running Average (Only on Config Page)
    if (YB.App.currentPage == "config") {
      // Manage sliding window using instance variable
      this.runningAverage.push(value);

      if (this.runningAverage.length > 1000) {
        this.runningAverage.shift();
      }

      // Calculate Average
      const average = this.runningAverage.reduce((acc, curr) => acc + curr, 0) / this.runningAverage.length;

      // Update Edit UI inputs (IDs matched to generateEditUI)
      $(`#f-adc-avg-output-${this.id}`).val(average.toFixed(4));
      $(`#f-adc-avg-count-${this.id}`).text(`(${this.runningAverage.length} points)`);
    } else {
      // Clear average when not on config page to save memory
      this.runningAverage = [];
    }

    // 4. Format the Value
    // We use YB.Util.formatNumber if available, otherwise fall back to toFixed
    const formatNum = (val, dec) => (YB.Util && YB.Util.formatNumber) ? YB.Util.formatNumber(val, dec) : val.toFixed(dec);

    if (this.cfg.displayDecimals !== undefined && this.cfg.displayDecimals !== null && this.cfg.displayDecimals >= 0) {
      calibrated_value = formatNum(calibrated_value, this.cfg.displayDecimals);
    } else {
      switch (this.cfg.type) {
        case "thermistor":
          calibrated_value = formatNum(calibrated_value, 1);
          break;
        case "raw":
        case "4-20ma":
        case "high_volt_divider":
        case "low_volt_divider":
          calibrated_value = formatNum(calibrated_value, 2);
          break;
        case "ten_k_pullup":
          calibrated_value = formatNum(calibrated_value, 0);
          break;
        default:
          calibrated_value = formatNum(calibrated_value, 3);
      }
    }

    // 5. Display the Value (Handle specific types)
    const $display = $(`#adcValue${this.id}`);

    switch (this.cfg.type) {
      case "digital_switch":
        if (value)
          $display.html(`HIGH`);
        else
          $display.html(`LOW`);
        break;

      case "4-20ma":
        if (value == 0)
          $display.html(`<span class="text-danger">ERROR: No Signal</span>`);
        else if (value < 4.0)
          $display.html(`<span class="text-danger">ERROR: ???</span>`);
        else
          $display.html(`${calibrated_value} ${units}`);
        break;

      case "ten_k_pullup":
        if (value == -2)
          $display.html(`<span class="text-danger">Error: No Connection</span>`);
        else if (value == -1)
          $display.html(`<span class="text-danger">Error: Negative Voltage</span>`);
        else
          $display.html(`${calibrated_value} ${units}`);
        break;

      default:
        $display.html(`${calibrated_value} ${units}`);
    }

    // Toggle card visibility based on enabled state
    $(`#adcControlCard${this.id}`).toggle(this.enabled);
  };

  ADCChannel.prototype.generateEditUI = function () {
    let standardFields = this.generateStandardEditFields();

    // Build options
    const typeOptions = Object.entries(ADC_TYPES)
      .map(([key, label]) => `<option value="${key}">${label}</option>`)
      .join("\n");

    return `
      <div id="adcEditCard${this.id}" class="col-xs-12 col-sm-6">
        <div class="p-3 border border-secondary rounded">
          <h5>ADC Channel #${this.id}</h5>
          ${standardFields}
          
          <div class="form-floating mb-3">
            <select id="f-adc-type-${this.id}" class="form-select" aria-label="Input Type">
              ${typeOptions}
            </select>
            <label for="f-adc-type-${this.id}">Input Type</label>
            <div class="invalid-feedback"></div>
          </div>

          <div class="form-floating mb-3">
            <select id="f-adc-decimals-${this.id}" class="form-select" aria-label="Display Format">
              <option value="0">123,456</option>
              <option value="1">123,456.1</option>
              <option value="2">123,456.12</option>
              <option value="3">123,456.123</option>
              <option value="4">123,456.1234</option>
            </select>
            <label for="f-adc-decimals-${this.id}">Display Decimals</label>
          </div>

          <div class="form-check form-switch">
            <input class="form-check-input" type="checkbox" id="f-adc-use-cal-${this.id}">
            <label class="form-check-label" for="f-adc-use-cal-${this.id}">Use Calibration Table</label>
          </div>

          <div id="f-adc-cal-ui-${this.id}" class="mt-3" style="display: none;">
             <div class="card card-body bg-light">
                <h6>Calibration Setup</h6>
                <div class="form-floating mb-3">
                  <input type="text" class="form-control" id="f-adc-cal-units-${this.id}">
                  <label for="f-adc-cal-units-${this.id}">Calibrated Units</label>
                </div>

                <label class="small">Live Average <span id="f-adc-avg-count-${this.id}"></span></label>
                <div class="input-group mb-3">
                  <input id="f-adc-avg-output-${this.id}" type="text" class="form-control" readonly>
                  <button id="btn-adc-copy-${this.id}" type="button" class="btn btn-outline-primary">Copy</button>
                  <button id="btn-adc-reset-${this.id}" type="button" class="btn btn-outline-secondary">Reset</button>
                </div>

                <table class="table table-sm table-striped">
                  <thead>
                    <tr>
                      <th>Raw Input</th>
                      <th>Calibrated</th>
                      <th></th>
                    </tr>
                  </thead>
                  <tbody id="adc-cal-table-body-${this.id}"></tbody>
                  <tfoot>
                    <tr>
                      <td><input type="number" step="any" class="form-control form-control-sm" id="f-adc-new-in-${this.id}" placeholder="Raw"></td>
                      <td><input type="number" step="any" class="form-control form-control-sm" id="f-adc-new-out-${this.id}" placeholder="Target"></td>
                      <td><button type="button" class="btn btn-sm btn-primary" id="btn-adc-add-row-${this.id}">Add</button></td>
                    </tr>
                  </tfoot>
                </table>
             </div>
          </div>

        </div>
      </div>
    `;
  };

  ADCChannel.prototype.setupEditUI = function () {
    YB.BaseChannel.prototype.setupEditUI.call(this);

    // Populate Data
    $(`#f-adc-type-${this.id}`).val(this.cfg.type);
    $(`#f-adc-decimals-${this.id}`).val(this.cfg.displayDecimals);
    $(`#f-adc-use-cal-${this.id}`).prop('checked', this.cfg.useCalibrationTable);
    $(`#f-adc-cal-units-${this.id}`).val(this.cfg.calibratedUnits);

    // Render Calibration Table
    const tableBody = $(`#adc-cal-table-body-${this.id}`);
    tableBody.empty();
    if (Array.isArray(this.cfg.calibrationTable)) {
      this.cfg.calibrationTable.forEach(row => this.renderCalibrationRow(row[0], row[1]));
    }

    // Toggle Visibility
    this.toggleCalibrationUI();

    // Event Listeners
    $(`#f-adc-type-${this.id}`).change(this.onEditForm);
    $(`#f-adc-decimals-${this.id}`).change(this.onEditForm);
    $(`#f-adc-use-cal-${this.id}`).change((e) => {
      this.toggleCalibrationUI();
      this.onEditForm(e);
    });
    $(`#f-adc-cal-units-${this.id}`).change(this.onEditForm);

    // Calibration Interactive Buttons
    $(`#btn-adc-add-row-${this.id}`).click(this.onAddCalibrationRow);
    $(`#btn-adc-copy-${this.id}`).click(this.onCopyAverage);
    $(`#btn-adc-reset-${this.id}`).click(this.onResetAverage);
  };

  ADCChannel.prototype.getConfigFormData = function () {
    let newcfg = YB.BaseChannel.prototype.getConfigFormData.call(this);

    newcfg.type = $(`#f-adc-type-${this.id}`).val();
    newcfg.displayDecimals = parseInt($(`#f-adc-decimals-${this.id}`).val());
    newcfg.useCalibrationTable = $(`#f-adc-use-cal-${this.id}`).is(':checked');
    newcfg.calibratedUnits = $(`#f-adc-cal-units-${this.id}`).val();

    // Scrape table data
    newcfg.calibrationTable = [];
    $(`#adc-cal-table-body-${this.id} tr`).each(function () {
      const raw = parseFloat($(this).find('.cal-raw').text());
      const cal = parseFloat($(this).find('.cal-target').text());
      if (!isNaN(raw) && !isNaN(cal)) {
        newcfg.calibrationTable.push([raw, cal]);
      }
    });

    return newcfg;
  };

  ADCChannel.prototype.onEditForm = function (e) {
    YB.BaseChannel.prototype.onEditForm.call(this, e);

    const enabled = this.enabled;
    $(`#f-adc-type-${this.id}`).prop('disabled', !enabled);
    $(`#f-adc-decimals-${this.id}`).prop('disabled', !enabled);
    $(`#f-adc-use-cal-${this.id}`).prop('disabled', !enabled);
    // Note: Calibration inner UI enablement is handled by toggleCalibrationUI + enabled check if needed
  };

  // --- Specific ADC Helper Methods ---

  ADCChannel.prototype.toggleCalibrationUI = function () {
    const useCal = $(`#f-adc-use-cal-${this.id}`).is(':checked');
    if (useCal && this.enabled) {
      $(`#f-adc-cal-ui-${this.id}`).show();
    } else {
      $(`#f-adc-cal-ui-${this.id}`).hide();
    }
  };

  ADCChannel.prototype.renderCalibrationRow = function (raw, target) {
    const rowId = `cal-row-${this.id}-${Date.now()}-${Math.random().toString(36).substr(2, 5)}`;
    const html = `
      <tr id="${rowId}">
        <td class="cal-raw">${raw}</td>
        <td class="cal-target">${target}</td>
        <td>
           <button type="button" class="btn btn-sm btn-outline-danger remove-cal-row">
             &times;
           </button>
        </td>
      </tr>
    `;
    $(`#adc-cal-table-body-${this.id}`).append(html);

    // Bind removal directly to the new button
    $(`#${rowId} .remove-cal-row`).click(() => $(`#${rowId}`).remove());
  };

  ADCChannel.prototype.onAddCalibrationRow = function () {
    const rawInput = $(`#f-adc-new-in-${this.id}`);
    const targetInput = $(`#f-adc-new-out-${this.id}`);

    const raw = parseFloat(rawInput.val());
    const target = parseFloat(targetInput.val());

    if (!isNaN(raw) && !isNaN(target)) {
      this.renderCalibrationRow(raw, target);
      rawInput.val('');
      targetInput.val('');
    }
  };

  ADCChannel.prototype.onCopyAverage = function () {
    const avg = $(`#f-adc-avg-output-${this.id}`).val();
    $(`#f-adc-new-in-${this.id}`).val(avg);
  };

  ADCChannel.prototype.onResetAverage = function () {
    this.runningAverage = [];
    $(`#f-adc-avg-output-${this.id}`).val(0);
  };

  // Export and Register
  YB.ADCChannel = ADCChannel;
  YB.ChannelRegistry.registerChannelType("adc", YB.ADCChannel);

  global.YB = YB;

})(this);