/* MOHHDY embed.js - widget ASSIST-010
 * Snippet public : bulle de chat, session visiteur, sondage HTTP.
 * Aucun secret, aucune cle, aucun jeton admin.
 * Les reponses sont un echo stub local, pas un LLM de production.
 */
(function () {
  "use strict";

  var script = document.currentScript;
  var siteId = "unspecified";
  if (script && script.getAttribute) {
    siteId = script.getAttribute("data-mohhdy-site") || "unspecified";
  }

  function instanceOrigin() {
    if (script && script.src) {
      try {
        return new URL(script.src, window.location.href).origin;
      } catch (err) {
        return window.location.origin;
      }
    }
    return window.location.origin;
  }

  var origin = instanceOrigin();
  var storageKey = "mohhdy.session." + siteId;
  var sessionId = null;
  var knownMessageIds = {};
  var pollTimer = null;

  function ready(fn) {
    if (document.readyState === "loading") {
      document.addEventListener("DOMContentLoaded", fn);
    } else {
      fn();
    }
  }

  function loadCss() {
    if (document.getElementById("mohhdy-embed-css")) {
      return;
    }
    var link = document.createElement("link");
    link.id = "mohhdy-embed-css";
    link.rel = "stylesheet";
    link.href = origin + "/embed.css";
    document.head.appendChild(link);
  }

  function el(tag, attrs) {
    var node = document.createElement(tag);
    var key;
    attrs = attrs || {};
    for (key in attrs) {
      if (Object.prototype.hasOwnProperty.call(attrs, key)) {
        if (key === "text") {
          node.textContent = attrs[key];
        } else {
          node.setAttribute(key, attrs[key]);
        }
      }
    }
    return node;
  }

  function api(path, options) {
    options = options || {};
    return fetch(origin + path, {
      method: options.method || "GET",
      headers: options.body
        ? { "Content-Type": "application/json" }
        : {},
      body: options.body ? JSON.stringify(options.body) : undefined,
    }).then(function (resp) {
      return resp.json().then(function (data) {
        if (!resp.ok) {
          var err = new Error((data && data.message) || "http " + resp.status);
          err.status = resp.status;
          throw err;
        }
        return data;
      });
    });
  }

  function paint() {
    if (document.getElementById("mohhdy-launcher")) {
      return;
    }
    loadCss();

    var btn = el("button", {
      id: "mohhdy-launcher",
      type: "button",
      "aria-label": "Ouvrir le chat MOHHDY",
      "aria-expanded": "false",
      text: "M",
    });

    var panel = el("section", {
      id: "mohhdy-panel",
      role: "dialog",
      "aria-label": "Chat MOHHDY",
    });

    var header = document.createElement("header");
    header.appendChild(el("strong", { text: "MOHHDY" }));
    header.appendChild(
      el("span", { text: "site " + siteId + " - echo stub local" })
    );

    var stub = el("p", {
      class: "mohhdy-stub",
      text:
        "Reponses locales stub : ce n'est pas un LLM de production. " +
        "Aucune cle n'est dans ce script.",
    });

    var log = el("div", {
      class: "mohhdy-log",
      id: "mohhdy-log",
      "aria-live": "polite",
    });

    var form = el("form", { class: "mohhdy-form" });
    var input = el("input", {
      class: "mohhdy-input",
      id: "mohhdy-compose",
      type: "text",
      maxlength: "4000",
      placeholder: "Ecrire un message",
      "aria-label": "Message visiteur",
    });
    var send = el("button", {
      class: "mohhdy-send",
      type: "submit",
      text: "Envoyer",
    });
    form.appendChild(input);
    form.appendChild(send);

    panel.appendChild(header);
    panel.appendChild(stub);
    panel.appendChild(log);
    panel.appendChild(form);

    function addSystem(text) {
      log.appendChild(el("div", { class: "mohhdy-msg system", text: text }));
      log.scrollTop = log.scrollHeight;
    }

    function renderMessage(msg) {
      if (!msg || !msg.id || knownMessageIds[msg.id]) {
        return;
      }
      knownMessageIds[msg.id] = true;
      var role = msg.role === "visitor" ? "visitor" : "agent";
      var node = el("div", { class: "mohhdy-msg " + role, text: msg.content || "" });
      log.appendChild(node);
      log.scrollTop = log.scrollHeight;
    }

    function renderMessages(messages) {
      var i;
      messages = messages || [];
      for (i = 0; i < messages.length; i += 1) {
        renderMessage(messages[i]);
      }
    }

    function ensureSession() {
      if (sessionId) {
        return Promise.resolve(sessionId);
      }
      try {
        sessionId = window.sessionStorage.getItem(storageKey) || null;
      } catch (err) {
        sessionId = null;
      }
      if (sessionId) {
        return api("/api/sessions/" + encodeURIComponent(sessionId)).then(
          function (data) {
            renderMessages(data.messages);
            return sessionId;
          },
          function () {
            sessionId = null;
            try {
              window.sessionStorage.removeItem(storageKey);
            } catch (err2) {
              /* ignore */
            }
            return createSession();
          }
        );
      }
      return createSession();
    }

    function createSession() {
      return api("/api/sessions", {
        method: "POST",
        body: { site_id: siteId },
      }).then(function (data) {
        sessionId = data.session_id;
        try {
          window.sessionStorage.setItem(storageKey, sessionId);
        } catch (err) {
          /* ignore */
        }
        addSystem("Session " + sessionId.slice(0, 8) + "... ouverte");
        return sessionId;
      });
    }

    function poll() {
      if (!sessionId) {
        return;
      }
      api("/api/sessions/" + encodeURIComponent(sessionId)).then(function (data) {
        renderMessages(data.messages);
      }).catch(function () {
        /* sondage silencieux */
      });
    }

    function startPoll() {
      if (pollTimer) {
        return;
      }
      poll();
      pollTimer = window.setInterval(poll, 2000);
    }

    function stopPoll() {
      if (pollTimer) {
        window.clearInterval(pollTimer);
        pollTimer = null;
      }
    }

    btn.addEventListener("click", function () {
      var open = panel.classList.toggle("open");
      btn.setAttribute("aria-expanded", open ? "true" : "false");
      if (open) {
        ensureSession()
          .then(function () {
            startPoll();
            input.focus();
          })
          .catch(function () {
            addSystem("Impossible d'ouvrir une session.");
          });
      } else {
        stopPoll();
      }
    });

    form.addEventListener("submit", function (event) {
      event.preventDefault();
      var text = (input.value || "").trim();
      if (!text) {
        return;
      }
      send.disabled = true;
      input.disabled = true;
      ensureSession()
        .then(function (id) {
          return api("/api/sessions/" + encodeURIComponent(id) + "/messages", {
            method: "POST",
            body: { content: text },
          });
        })
        .then(function (data) {
          input.value = "";
          renderMessage(data.visitor_message);
          renderMessage(data.agent_message);
        })
        .catch(function () {
          addSystem("Envoi impossible. Reessayez.");
        })
        .then(function () {
          send.disabled = false;
          input.disabled = false;
          input.focus();
        });
    });

    document.body.appendChild(panel);
    document.body.appendChild(btn);
  }

  ready(paint);
})();
