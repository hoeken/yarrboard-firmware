(function (global) { // private scope
  // work in the global YB namespace.
  var YB = global.YB || {};

  // Channel registry + factory under YB
  YB.Util = {

    formatAmpHours: function (aH) {
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
    },

    formatWattHours: function (wH) {
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
    },

    formatBytes: function (bytes, decimals = 1) {
      if (!+bytes) return '0 Bytes'

      const k = 1024
      const dm = decimals < 0 ? 0 : decimals
      const sizes = ['Bytes', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB']

      const i = Math.floor(Math.log(bytes) / Math.log(k))

      return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`
    },

    formatNumber: function (num, decimals) {
      return Number(num).toLocaleString("en-US", {
        minimumFractionDigits: decimals,
        maximumFractionDigits: decimals
      });
    },

    secondsToDhms: function (seconds) {
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
    },

    getQueryVariable: function (name) {
      const query = window.location.search.substring(1);
      const vars = query.split("&");
      for (let i = 0; i < vars.length; i++) {
        const pair = vars[i].split("=");
        if (decodeURIComponent(pair[0]) === name) {
          return decodeURIComponent(pair[1] || "");
        }
      }
      return null; // Return null if the variable is not found
    },

    // return true if 'first' is greater than or equal to 'second'
    compareVersions: function (first, second) {

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
    },

    isCanvasSupported: function () {
      var elem = document.createElement('canvas');
      return !!(elem.getContext && elem.getContext('2d'));
    },

    // Function to update the progress bar
    updateProgressBar: function (ele, progress) {
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
    },

    getWifiIconForRSSI: function (rssi) {
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
    },

    getBootstrapColors: function () {
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
    },

    showFormValidationResults: function (data, errors) {
      //clear our errors
      for (const field of Object.keys(data)) {
        const el = $(`#${field}`);
        el.removeClass("is-valid is-invalid");

        if (errors && errors[field]) {
          //add our invalid class
          el.addClass("is-invalid");

          // Find the right feedback element (works for form-floating and form-switch)
          const msg = errors[field][0];
          const $fb = el.siblings(".invalid-feedback")
            .add(el.closest(".form-check, .form-floating").find(".invalid-feedback"))
            .first();
          $fb.text(msg);
        } else {
          YB.Util.flashClass(el, "is-valid");
        }
      }
    },

    flashClass: function (el, myclass, ms = 1000) {
      el.addClass(myclass);
      setTimeout(() => el.removeClass(myclass), ms);
    },
  };

  // expose to global
  global.YB = YB;
})(this);