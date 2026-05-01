(function(window, $) {
  "use strict";

  function EventEmitter() {
    this.listeners = {};
  }

  EventEmitter.prototype.on = function(eventName, listener) {
    if (!this.listeners[eventName]) {
      this.listeners[eventName] = [];
    }
    this.listeners[eventName].push(listener);
  };

  EventEmitter.prototype.emit = function(eventName) {
    var listeners = this.listeners[eventName];
    var args;
    var i;
    if (!listeners) {
      return;
    }
    args = Array.prototype.slice.call(arguments, 1);
    for (i = 0; i < listeners.length; i++) {
      listeners[i].apply(this, args);
    }
  };

  function WallClient(uri, username, password, qos) {
    var self = this;
    this.username = username;
    this.password = password;
    this.qos = typeof qos === "number" ? qos : 0;
    this.clientId = WallClient.generateClientId();
    this.client = new Paho.MQTT.Client(uri, this.clientId);
    this.client.onMessageArrived = function(message) {
      self.onMessage(message.destinationName, message.payloadString, message.retained, message.qos);
    };
    this.client.onConnectionLost = function(response) {
      console.info("Connection lost", response);
      if (WallClient.isNetworkError(response.errorCode)) {
        self._reconnect();
        return;
      }
      self.onError("Connection lost (" + response.errorMessage + ")", true);
    };
    this.currentTopic = null;
    this.firstConnection = true;
    this.attempts = 0;
    this.onConnected = $.noop;
    this.onMessage = $.noop;
    this.onError = $.noop;
    this.onStateChanged = $.noop;
    this._setState(WallClient.STATE.NEW);
  }

  WallClient.STATE = {
    NEW: 0,
    CONNECTING: 1,
    CONNECTED: 2,
    RECONNECTING: 3,
    ERROR: 99
  };

  WallClient.generateClientId = function() {
    var nowPart = Date.now() % 1000;
    var randPart = Math.round(Math.random() * 1000);
    return "wall-" + (nowPart * 1000 + randPart);
  };

  WallClient.isNetworkError = function(errorCode) {
    return [1, 2, 3, 4, 6, 7, 8, 9, 11, 12, 15, 16, 17].indexOf(errorCode) >= 0;
  };

  WallClient.prototype.subscribe = function(topic, onSuccess) {
    var self = this;

    if (this.currentTopic !== null && this.currentTopic !== topic) {
      (function(previousTopic) {
        self.client.unsubscribe(previousTopic, {
          onSuccess: function() {
            console.info("Unsubscribe '%s' success", previousTopic);
          },
          onFailure: function(response) {
            console.error("Unsubscribe '%s' failure", previousTopic, response);
          }
        });
      })(this.currentTopic);
    }

    this.client.subscribe(topic, {
      qos: this.qos,
      onSuccess: function(response) {
        console.info("Subscribe '%s' success", topic, response);
        self.currentTopic = topic;
        if (onSuccess) {
          onSuccess();
        }
      },
      onFailure: function(response) {
        console.error("Subscribe '%s' failure", topic, response);
        self.onError("Subscribe failure");
      }
    });
  };

  WallClient.prototype.connect = function() {
    var self = this;
    var options = {
      onSuccess: function() {
        console.info("Connect success");
        self.attempts = 0;
        self._setState(WallClient.STATE.CONNECTED);
        if (self.firstConnection) {
          self.firstConnection = false;
          self.onConnected();
        } else if (self.currentTopic) {
          self.subscribe(self.currentTopic);
        }
      },
      onFailure: function(response) {
        console.error("Connect fail", response);
        if (WallClient.isNetworkError(response.errorCode)) {
          self._reconnect();
          return;
        }
        self.onError("Fail to connect", true);
      }
    };

    if (this.username && this.password) {
      options.userName = this.username;
      options.password = this.password;
    }

    this._setState(this.firstConnection ? WallClient.STATE.CONNECTING : WallClient.STATE.RECONNECTING);
    this.client.connect(options);
  };

  WallClient.prototype._reconnect = function() {
    var self = this;
    var reconnectMs;
    this.attempts += 1;
    this._setState(this.firstConnection ? WallClient.STATE.CONNECTING : WallClient.STATE.RECONNECTING);
    reconnectMs = 2000 * (this.attempts - 1);
    reconnectMs = Math.max(Math.min(reconnectMs, 30000), 100);
    window.setTimeout(function() {
      self.connect();
    }, reconnectMs);
  };

  WallClient.prototype._setState = function(state) {
    this.state = state;
    if (this.onStateChanged) {
      this.onStateChanged(state);
    }
  };

  WallClient.prototype.toString = function() {
    return this.client._getURI();
  };

  function Toast(message, kind, persistent) {
    var self = this;
    this.$root = $("<div class='toast-item'>")
      .text(message)
      .addClass(kind || "info")
      .hide()
      .appendTo("#toast")
      .fadeIn();

    if (persistent) {
      this.$root.addClass("persistent");
    } else {
      window.setTimeout(function() {
        self.hide();
      }, 5000);
    }
  }

  Toast.prototype.hide = function() {
    var self = this;
    this.$root.slideUp().queue(function() {
      self.remove();
    });
  };

  Toast.prototype.remove = function() {
    this.$root.remove();
  };

  Toast.prototype.setMessage = function(message) {
    this.$root.text(message);
  };

  var UI = {
    setTitle: function(topic) {
      document.title = "MQTT Wall" + (topic ? " for " + topic : "");
    },
    toast: function(message, kind, persistent) {
      return new Toast(message, kind || "info", !!persistent);
    }
  };

  function MessageLine(topic) {
    this.topic = topic;
    this.counter = 0;
    this.isNew = true;
    this.init();
  }

  MessageLine.prototype.init = function() {
    var $header;
    this.$root = $("<article class='message'>");
    $header = $("<header>").appendTo(this.$root);
    $("<h2>").text(this.topic).appendTo($header);

    if (window.config.showCounter) {
      this.$counterMark = $("<span class='mark counter' title='Message counter'>0</span>").appendTo($header);
    }

    this.$retainMark = $("<span class='mark retain' title='Retain message'>R</span>").appendTo($header);
    this.$qosMark = $("<span class='mark qos' title='Received message QoS'>QoS</span>").appendTo($header);
    this.$payload = $("<p>").appendTo(this.$root);
  };

  MessageLine.prototype.animateHighlight = function($targets) {
    $targets.each(function() {
      var computed = window.getComputedStyle(this);
      var baseBackgroundColor = computed.backgroundColor;
      var baseColor = computed.color;
      $(this).stop(true, true)
        .css({
          backgroundColor: "rgb(224, 242, 254)",
          color: "#082f49"
        })
        .animate({
          backgroundColor: baseBackgroundColor,
          color: baseColor
        }, 2000);
    });
  };

  MessageLine.prototype.highlight = function(isNewTopic) {
    if (isNewTopic) {
      this.animateHighlight(this.$root.add(this.$root.children()));
      return;
    }
    this.animateHighlight(this.$payload);
  };

  MessageLine.prototype.setRetained = function(value) {
    this.$retainMark[value ? "show" : "hide"]();
  };

  MessageLine.prototype.setSystemPayload = function(value) {
    this.$payload.toggleClass("sys", value);
  };

  MessageLine.prototype.update = function(payload, retained, qos) {
    var text = payload;
    this.counter += 1;
    this.setRetained(retained);

    if (this.$counterMark) {
      this.$counterMark.text(this.counter);
    }

    if (this.$qosMark) {
      if (qos === 0) {
        this.$qosMark.hide();
      } else {
        this.$qosMark.show();
        this.$qosMark.text("QoS " + qos);
        this.$qosMark.attr("data-qos", qos);
      }
    }

    if (text === "") {
      text = "NULL";
      this.setSystemPayload(true);
    } else {
      this.setSystemPayload(false);
    }

    this.$payload.text(text);
    this.highlight(this.isNew);

    if (this.isNew) {
      this.isNew = false;
    }
  };

  function MessageContainer($parent) {
    this.sort = MessageContainer.SORT_APLHA;
    this.$parent = $parent;
    this.init();
  }

  MessageContainer.SORT_APLHA = "Alphabetically";
  MessageContainer.SORT_CHRONO = "Chronologically";

  MessageContainer.prototype.init = function() {
    this.reset();
  };

  MessageContainer.prototype.reset = function() {
    this.lines = {};
    this.topics = [];
    this.$parent.html("");
  };

  MessageContainer.prototype.update = function(topic, payload, retained, qos) {
    var line;
    if (!this.lines[topic]) {
      line = new MessageLine(topic);
      this["addLine" + this.sort](line);
      this.lines[topic] = line;
    }
    this.lines[topic].update(payload, retained, qos);
  };

  MessageContainer.prototype.addLineAlphabetically = function(line) {
    var topic = line.topic;
    var newIndex;
    var previousTopic;

    if (this.topics.length === 0) {
      this.addLineChronologically(line);
      return;
    }

    this.topics.push(topic);
    this.topics.sort();
    newIndex = this.topics.indexOf(topic);

    if (newIndex === 0) {
      this.$parent.prepend(line.$root);
      return;
    }

    previousTopic = this.topics[newIndex - 1];
    line.$root.insertAfter(this.lines[previousTopic].$root);
  };

  MessageContainer.prototype.addLineChronologically = function(line) {
    this.topics.push(line.topic);
    this.$parent.append(line.$root);
  };

  function Footer() {}

  Footer.prototype.setClientId = function(value) {
    $("#status-client").text(value);
  };

  Footer.prototype.setUri = function(value) {
    $("#status-host").text(value);
  };

  Footer.prototype.setState = function(value) {
    var text = "";
    var cssClass = "connecting";

    switch (value) {
      case WallClient.STATE.NEW:
        text = "connecting";
        cssClass = "connecting";
        break;
      case WallClient.STATE.CONNECTING:
        text = "connecting...";
        cssClass = "connecting";
        break;
      case WallClient.STATE.CONNECTED:
        text = "connected";
        cssClass = "connected";
        break;
      case WallClient.STATE.RECONNECTING:
        text = "reconnecting...";
        cssClass = "connecting";
        break;
      case WallClient.STATE.ERROR:
        text = "not connected";
        cssClass = "fail";
        break;
      default:
        throw new Error("Unknown WallClient.STATE");
    }

    if (this.reconnectAttempts > 1) {
      text += " (" + this.reconnectAttempts + ")";
    }

    $("#status-state").removeClass().addClass(cssClass);
    $("#status-state span").text(text);
  };

  function Toolbar($parent) {
    EventEmitter.call(this);
    this.$parent = $parent;
    this.$topic = $parent.find("#topic");
    this.initEventHandlers();
    this.initDefaultTopic();
  }

  Toolbar.prototype = Object.create(EventEmitter.prototype);
  Toolbar.prototype.constructor = Toolbar;

  Toolbar.prototype.initEventHandlers = function() {
    var self = this;
    var canceled = false;

    this.$topic.keyup(function(event) {
      if (event.which === 13) {
        self.$topic.blur();
      }
      if (event.keyCode === 27) {
        canceled = true;
        self.$topic.blur();
      }
    });

    this.$topic.focus(function() {
      canceled = false;
    });

    this.$topic.blur(function() {
      if (canceled) {
        self.updateUi();
      } else {
        self.inputChanged();
      }
    });
  };

  Toolbar.prototype.initDefaultTopic = function() {
    if (location.hash !== "") {
      this._topic = location.hash.substr(1);
    } else {
      this._topic = window.config.defaultTopic || "/#";
    }
    this.updateUi();
  };

  Toolbar.prototype.inputChanged = function() {
    var nextTopic = this.$topic.val();
    if (nextTopic !== this._topic) {
      this._topic = nextTopic;
      this.emit("topic", nextTopic);
    }
  };

  Toolbar.prototype.updateUi = function() {
    this.$topic.val(this._topic);
  };

  Toolbar.prototype.getTopic = function() {
    return this._topic;
  };

  function decodePassword(encodedPassword) {
    if (typeof encodedPassword !== "string") {
      return undefined;
    }
    return atob(encodedPassword);
  }

  function bootstrap() {
    var password = window.config.server.password !== undefined
      ? decodePassword(window.config.server.password)
      : undefined;
    var client = new WallClient(
      window.config.server.uri,
      window.config.server.username,
      password,
      window.config.qos
    );
    var messages = new MessageContainer($("section.messages"));
    var footer = new Footer();
    var toolbar = new Toolbar($("#header"));
    var reconnectToast = null;

    messages.sort = window.config.alphabeticalSort
      ? MessageContainer.SORT_APLHA
      : MessageContainer.SORT_CHRONO;

    footer.setClientId(client.clientId);
    footer.setUri(client.toString());
    footer.setState(WallClient.STATE.NEW);

    function subscribeCurrentTopic() {
      var topic = toolbar.getTopic();
      client.subscribe(topic, function() {
        UI.setTitle(topic);
        location.hash = "#" + topic;
      });
      messages.reset();
    }

    toolbar.on("topic", function() {
      subscribeCurrentTopic();
    });

    client.onConnected = function() {
      subscribeCurrentTopic();
      UI.toast("Connected to host " + client.toString());
    };

    client.onError = function(message, persistent) {
      UI.toast(message, "error", persistent);
    };

    client.onStateChanged = function(state) {
      footer.reconnectAttempts = client.attempts;
      footer.setState(state);

      if ((state === WallClient.STATE.CONNECTING || state === WallClient.STATE.RECONNECTING) && client.attempts >= 2) {
        var text = state === WallClient.STATE.CONNECTING
          ? "Fail to connect. Trying to connect... (" + client.attempts + " attempts)"
          : "Connection lost. Trying to reconnect... (" + client.attempts + " attempts)";
        if (reconnectToast === null) {
          reconnectToast = UI.toast(text, "error", true);
        } else {
          reconnectToast.setMessage(text);
        }
      }

      if (state === WallClient.STATE.CONNECTED && reconnectToast !== null) {
        reconnectToast.hide();
        reconnectToast = null;
        UI.toast("Reconnected");
      }
    };

    client.onMessage = function(topic, payload, retained, qos) {
      messages.update(topic, payload, retained, qos);
    };

    client.connect();
  }

  bootstrap();
})(window, jQuery);
