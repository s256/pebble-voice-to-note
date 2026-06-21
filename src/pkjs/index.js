var Keys = {
  TRANSCRIPTION: 0,
  STATUS: 1,
  CONFIG_URL: 2,
  CONFIG_METHOD: 3,
  CONFIG_HEADERS: 4,
  CONFIG_BODY: 5
};

// Default configuration
var DEFAULT_CONFIG = {
  url: '',
  method: 'POST',
  headers: '{"Content-Type": "application/json"}',
  body: '{"note": "{{note}}"}'
};

function getConfig() {
  var config = {};
  try {
    var stored = localStorage.getItem('voice_to_note_config');
    if (stored) {
      config = JSON.parse(stored);
    }
  } catch (e) {
    console.log('Error reading config: ' + e);
  }
  return {
    url: config.url || DEFAULT_CONFIG.url,
    method: config.method || DEFAULT_CONFIG.method,
    headers: config.headers || DEFAULT_CONFIG.headers,
    body: config.body || DEFAULT_CONFIG.body
  };
}

function saveConfig(config) {
  localStorage.setItem('voice_to_note_config', JSON.stringify(config));
}

function sendStatus(statusCode) {
  var msg = {};
  msg[Keys.STATUS] = statusCode;
  Pebble.sendAppMessage(msg, function () {
    console.log('Status ' + statusCode + ' sent to watch');
  }, function (e) {
    console.log('Failed to send status to watch: ' + JSON.stringify(e));
  });
}

function sendNote(transcription) {
  var config = getConfig();

  if (!config.url) {
    console.log('No webhook URL configured');
    sendStatus(0);
    return;
  }

  // Replace {{note}} placeholder in body template
  var bodyStr = config.body.replace(/\{\{note\}\}/g, transcription.replace(/"/g, '\\"'));

  // Parse headers
  var headers = {};
  try {
    headers = JSON.parse(config.headers);
  } catch (e) {
    console.log('Invalid headers JSON, using defaults');
    headers = { 'Content-Type': 'application/json' };
  }

  var request = new XMLHttpRequest();
  request.onload = function () {
    console.log('HTTP ' + this.status + ': ' + this.responseText.substring(0, 200));
    if (this.status >= 200 && this.status < 300) {
      sendStatus(200);
    } else {
      sendStatus(this.status);
    }
  };
  request.onerror = function () {
    console.log('HTTP request failed');
    sendStatus(0);
  };
  request.ontimeout = function () {
    console.log('HTTP request timed out');
    sendStatus(0);
  };
  request.timeout = 15000;
  request.open(config.method, config.url);

  // Set headers
  for (var key in headers) {
    if (headers.hasOwnProperty(key)) {
      request.setRequestHeader(key, headers[key]);
    }
  }

  // Send body for methods that support it
  if (config.method === 'GET' || config.method === 'HEAD') {
    request.send();
  } else {
    request.send(bodyStr);
  }
}

// Listen for when PebbleKit JS is ready
Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready');
});

// Receive transcription from watch
Pebble.addEventListener('appmessage', function (e) {
  console.log('Received message: ' + JSON.stringify(e.payload));
  var transcription = e.payload[Keys.TRANSCRIPTION];
  if (transcription) {
    sendNote(transcription);
  }
});

// Configuration page
Pebble.addEventListener('showConfiguration', function () {
  var config = getConfig();
  var configUrl = 'https://s256.github.io/pebble-voice-to-note/index.html';
  var params = '?config=' + encodeURIComponent(JSON.stringify(config));
  Pebble.openURL(configUrl + params);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (e && e.response && e.response.length > 0) {
    try {
      var config = JSON.parse(decodeURIComponent(e.response));
      saveConfig(config);
      console.log('Config saved: ' + JSON.stringify(config));
    } catch (err) {
      console.log('Error parsing config response: ' + err);
    }
  }
});
