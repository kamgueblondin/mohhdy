/* MOHHDY embed.js - widget ASSIST-010 + 012/031/041
 * Snippet public : bulle de chat, session visiteur, sondage HTTP.
 * Aucun secret, aucune cle, aucun jeton admin, aucun prefixe ACL interne.
 * Les reponses sont un stub local (echo ou base autorisee), pas un LLM de production.
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
  var sessionStatus = "open";

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
          err.payload = data;
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
    var headerMeta = el("span", {
      id: "mohhdy-header-meta",
      text: "site " + siteId + " - stub local",
    });
    header.appendChild(headerMeta);

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
    var humanBtn = el("button", {
      class: "mohhdy-human",
      id: "mohhdy-human",
      type: "button",
      text: "Parler a un humain",
    });
    form.appendChild(input);
    form.appendChild(send);
    form.appendChild(humanBtn);

    panel.appendChild(header);
    panel.appendChild(stub);
    panel.appendChild(log);
    panel.appendChild(form);

    function addSystem(text) {
      log.appendChild(el("div", { class: "mohhdy-msg system", text: text }));
      log.scrollTop = log.scrollHeight;
    }

    function roleClass(role) {
      if (role === "visitor") {
        return "visitor";
      }
      if (role === "human") {
        return "human";
      }
      if (role === "system") {
        return "system";
      }
      return "agent";
    }

    function applyStatus(status) {
      if (!status) {
        return;
      }
      sessionStatus = status;
      var label = "stub local";
      if (status === "waiting_human") {
        label = "en attente d'un humain";
        humanBtn.disabled = true;
      } else if (status === "human_active") {
        label = "humain en ligne";
        humanBtn.disabled = true;
      } else {
        humanBtn.disabled = false;
      }
      headerMeta.textContent = "site " + siteId + " - " + label;
    }

    function renderMessage(msg) {
      if (!msg || !msg.id || knownMessageIds[msg.id]) {
        return;
      }
      knownMessageIds[msg.id] = true;
      var role = roleClass(msg.role || msg.speaker);
      var node = el("div", { class: "mohhdy-msg " + role, text: msg.content || "" });
      if (role === "human") {
        node.setAttribute("data-speaker", "human");
      }
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

    function applySession(data) {
      if (!data) {
        return;
      }
      applyStatus(data.status);
      renderMessages(data.messages);
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
            applySession(data);
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
        applyStatus(data.status || "open");
        addSystem("Session " + sessionId.slice(0, 8) + "... ouverte");
        return sessionId;
      });
    }

    function poll() {
      if (!sessionId) {
        return;
      }
      api("/api/sessions/" + encodeURIComponent(sessionId)).then(function (data) {
        applySession(data);
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
          applyStatus(data.status);
          if (data.agent_message) {
            renderMessage(data.agent_message);
          } else if (data.status === "human_active" || data.status === "waiting_human") {
            addSystem("Message transmis. L'agent ne repond plus automatiquement.");
          }
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

    humanBtn.addEventListener("click", function () {
      humanBtn.disabled = true;
      ensureSession()
        .then(function (id) {
          return api("/api/sessions/" + encodeURIComponent(id) + "/escalate", {
            method: "POST",
            body: {},
          });
        })
        .then(function (data) {
          applySession(data);
          addSystem("Demande transmise a un humain.");
        })
        .catch(function (err) {
          humanBtn.disabled = sessionStatus === "waiting_human" || sessionStatus === "human_active";
          addSystem((err && err.message) || "Escalade impossible.");
        });
    });

    document.body.appendChild(panel);
    document.body.appendChild(btn);
  }

  ready(paint);
})();
