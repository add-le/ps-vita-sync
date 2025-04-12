function trailingZero(string) {
  return string.length === 1 ? "0" + string.toString() : string;
}

function init() {
  try {
    var user_code = "XXXX-XXXX";
    var verification_url = "https://www.google.com/device";
    var expires_in = 0;

    // Block non ps vita browser
    // 960x544 + User Agent...
    var isPSP2 = navigator.userAgent.indexOf("PlayStation Vita") !== -1;
    var PSP2_Width = 960;
    var PSP2_Height = 544;
    var rightDimensions =
      (window.innerWidth === PSP2_Width &&
        window.innerHeight === PSP2_Height) ||
      (window.screen.width === PSP2_Width &&
        window.screen.height === PSP2_Height);

    if (!isPSP2 || !rightDimensions) {
      document.body.innerHTML =
        '<object width="100%" height="100%" type="text/html" data="error.html"></object>';
      return;
    }

    var search = location.search.replace("?", "").split("&");
    search.forEach(function (param) {
      switch (param.split("=")[0]) {
        case "user_code":
          user_code = param.split("=")[1];
          break;
        case "expires_in":
          expires_in = param.split("=")[1];
          break;
        case "verification_url":
          verification_url = param.split("=")[1];
          break;
      }
    });

    // Handle reload to not reset expires time
    var reloader = localStorage.getItem(user_code);
    var timer;
    if (reloader) {
      var ts = JSON.parse(reloader).ts;
      var expire = JSON.parse(reloader).expires_in;
      var elps =
        expire * 1000 - (new Date().getTime() - new Date(ts).getTime());
      timer = new Date(elps < 0 ? 0 : elps);
    } else {
      localStorage.setItem(
        user_code,
        JSON.stringify({ ts: new Date(), expires_in: expires_in })
      );
      timer = new Date(expires_in * 1000);
    }

    var entry = document.getElementById("user_code");
    entry.innerHTML = user_code;

    var device_page = document.getElementById("verification_url");
    device_page.innerHTML = verification_url;
    device_page.href = verification_url;

    var display_timer = document.getElementById("display-timer");
    var timerInterval = setInterval(function () {
      if (timer.getTime() <= 0) {
        clearInterval(timerInterval);
        display_timer.parentElement.innerHTML =
          '<span class="expire-code">The code has expired, make a new request in the application.</span>';
        return;
      }

      timer.setTime(timer.getTime() - 1000);
      display_timer.innerHTML =
        trailingZero(timer.getUTCMinutes().toString()) +
        ":" +
        trailingZero(timer.getUTCSeconds().toString());
    }, 1000);
  } catch (error) {
    document.body.innerHTML = error;
  }
}
