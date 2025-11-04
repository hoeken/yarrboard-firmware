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
    }
  };

  // expose to global
  global.YB = YB;
})(this);