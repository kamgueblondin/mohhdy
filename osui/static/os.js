(function () {
  "use strict";

  var zIndex = 10;
  var visitorSession = null;
  var selectedAdminId = null;
  var statusFilter = "";
  var drag = null;

  function $(id) {
    return document.getElementById(id);
  }

  function paneWindow(name) {
    return document.querySelector('.os-window[data-pane="' + name + '"]');
  }

  function focusWindow(win) {
    if (!win) {
      return;
    }
    win.classList.remove("hidden");
    zIndex += 1;
    win.style.zIndex = String(zIndex);
    Array.prototype.forEach.call(document.querySelectorAll(".os-window"), function (other) {
      other.classList.toggle("active", other === win);
    });
  }

  function openPane(name) {
    focusWindow(paneWindow(name));
    if (name === "status") {
      loadStatus();
    }
    if (name === "admin") {
      loadAdmin();
    }
    if (name === "browser") {
      refreshBrowserLabel();
    }
  }

  function storedToken() {
    try {
      return window.sessionStorage.getItem("mohhdy.os.admin.token") || "";
    } catch (err) {
      return "";
    }
  }

  function saveToken(value) {
    try {
      if (value) {
        window.sessionStorage.setItem("mohhdy.os.admin.token", value);
      } else {
        window.sessionStorage.removeItem("mohhdy.os.admin.token");
      }
    } catch (err) {
      /* ignore */
    }
  }

  function adminHeaders() {
    var token = (($("admin-token").value) || storedToken()).trim();
    var out = {};
    if (token) {
      out.Authorization = "Bearer " + token;
    }
    return out;
  }

  function api(path, options) {
    options = options || {};
    var hdrs = options.admin ? adminHeaders() : {};
    if (options.body) {
      hdrs["Content-Type"] = "application/json";
    }
    return fetch(path, {
      method: options.method || "GET",
      headers: hdrs,
      body: options.body ? JSON.stringify(options.body) : undefined,
    }).then(function (resp) {
      return resp.json().then(function (data) {
        if (!resp.ok) {
          var err = new Error((data && (data.message || data.error)) || "http " + resp.status);
          err.status = resp.status;
          err.payload = data;
          throw err;
        }
        return data;
      });
    });
  }

  function formatError(err) {
    var payload = err.payload || {};
    var rid = payload.request_id ? " request_id=" + payload.request_id : "";
    return (payload.error || err.message || "erreur") + rid;
  }

  function tickClock() {
    var now = new Date();
    var hh = String(now.getUTCHours()).padStart(2, "0");
    var mm = String(now.getUTCMinutes()).padStart(2, "0");
    var ss = String(now.getUTCSeconds()).padStart(2, "0");
    var el = $("os-clock");
    el.textContent = hh + ":" + mm + ":" + ss + " UTC";
    el.setAttribute("datetime", now.toISOString());
  }

  function bindWindowChrome() {
    Array.prototype.forEach.call(document.querySelectorAll(".os-window"), function (win) {
      var bar = win.querySelector(".os-titlebar");
      var closer = win.querySelector("[data-close]");
      win.addEventListener("mousedown", function () {
        focusWindow(win);
      });
      if (closer) {
        closer.addEventListener("click", function (event) {
          event.stopPropagation();
          win.classList.add("hidden");
        });
      }
      bar.addEventListener("mousedown", function (event) {
        if (event.target && event.target.getAttribute("data-close") !== null) {
          return;
        }
        drag = {
          win: win,
          dx: event.clientX - win.offsetLeft,
          dy: event.clientY - win.offsetTop,
        };
        bar.style.cursor = "grabbing";
        event.preventDefault();
      });
    });
    document.addEventListener("mousemove", function (event) {
      if (!drag) {
        return;
      }
      drag.win.style.left = Math.max(0, event.clientX - drag.dx) + "px";
      drag.win.style.top = Math.max(0, event.clientY - drag.dy) + "px";
    });
    document.addEventListener("mouseup", function () {
      if (drag) {
        var bar = drag.win.querySelector(".os-titlebar");
        bar.style.cursor = "grab";
      }
      drag = null;
    });
    Array.prototype.forEach.call(document.querySelectorAll("[data-open]"), function (btn) {
      btn.addEventListener("click", function () {
        openPane(btn.getAttribute("data-open"));
      });
    });
  }

  function renderMessages(container, session) {
    container.innerHTML = "";
    (session.messages || []).forEach(function (msg) {
      var wrap = document.createElement("div");
      wrap.className = "msg" + (msg.role === "human" ? " human" : "");
      var meta = document.createElement("div");
      meta.className = "meta";
      meta.textContent = (msg.speaker || msg.role) + " · " + (msg.kind || "") + " · " + (msg.created_at || "");
      var body = document.createElement("div");
      body.textContent = msg.content || "";
      wrap.appendChild(meta);
      wrap.appendChild(body);
      container.appendChild(wrap);
    });
    container.scrollTop = container.scrollHeight;
  }

  function showSupportSession(session) {
    visitorSession = session;
    $("support-error").hidden = true;
    $("support-meta").textContent =
      "session_id=" + session.session_id +
      " / llm=" + (session.llm || "stub_echo") +
      " / status=" + session.status +
      " / origin=" + (session.document_origin || "") +
      " / binding=" + (session.origin_binding || "");
    $("support-caps").textContent = "Droits : " + ((session.capabilities || []).join(", ") || "(aucun)");
    renderMessages($("support-log"), session);
    refreshBrowserLabel();
  }

  function refreshSupport() {
    if (!visitorSession) {
      return;
    }
    api("/api/sessions/" + encodeURIComponent(visitorSession.session_id)).then(showSupportSession).catch(function (err) {
      $("support-error").hidden = false;
      $("support-error").textContent = formatError(err);
    });
  }

  function createSupportSession() {
    var site = ($("support-site").value || "osui_demo").trim();
    api("/api/sessions", { method: "POST", body: { site_id: site } }).then(showSupportSession).catch(function (err) {
      $("support-error").hidden = false;
      $("support-error").textContent = formatError(err);
    });
  }

  function sendVisitor() {
    if (!visitorSession) {
      $("support-error").hidden = false;
      $("support-error").textContent = "Creez une session d'abord.";
      return;
    }
    var text = ($("support-input").value || "").trim();
    if (!text) {
      return;
    }
    api("/api/sessions/" + encodeURIComponent(visitorSession.session_id) + "/messages", {
      method: "POST",
      body: { content: text },
    }).then(function () {
      $("support-input").value = "";
      refreshSupport();
    }).catch(function (err) {
      $("support-error").hidden = false;
      $("support-error").textContent = formatError(err);
    });
  }

  function escalateVisitor() {
    if (!visitorSession) {
      return;
    }
    api("/api/sessions/" + encodeURIComponent(visitorSession.session_id) + "/escalate", {
      method: "POST",
      body: { reason: "demande depuis le shell OS" },
    }).then(function () {
      refreshSupport();
      loadAdmin();
    }).catch(function (err) {
      $("support-error").hidden = false;
      $("support-error").textContent = formatError(err);
    });
  }

  function renderAdminList(sessions) {
    var tbody = document.querySelector("#admin-sessions tbody");
    tbody.innerHTML = "";
    if (!sessions.length) {
      var tr = document.createElement("tr");
      var td = document.createElement("td");
      td.className = "empty";
      td.colSpan = 3;
      td.textContent = "Aucune session";
      tr.appendChild(td);
      tbody.appendChild(tr);
      return;
    }
    sessions.forEach(function (row) {
      var tr = document.createElement("tr");
      if (row.session_id === selectedAdminId) {
        tr.className = "active";
      }
      ["session_id", "site_id", "status"].forEach(function (key) {
        var td = document.createElement("td");
        td.textContent = row[key] == null ? "" : String(row[key]);
        tr.appendChild(td);
      });
      tr.addEventListener("click", function () {
        selectedAdminId = row.session_id;
        loadAdminDetail(row.session_id);
        loadAdmin();
      });
      tbody.appendChild(tr);
    });
  }

  function renderJournal(entries) {
    var body = document.querySelector("#admin-journal tbody");
    body.innerHTML = "";
    if (!entries.length) {
      var tr = document.createElement("tr");
      var td = document.createElement("td");
      td.className = "empty";
      td.colSpan = 3;
      td.textContent = "Aucun acte";
      tr.appendChild(td);
      body.appendChild(tr);
      return;
    }
    entries.slice(0, 20).forEach(function (row) {
      var tr = document.createElement("tr");
      ["request_id", "tool", "outcome"].forEach(function (key) {
        var td = document.createElement("td");
        td.textContent = row[key] == null ? "" : String(row[key]);
        tr.appendChild(td);
      });
      body.appendChild(tr);
    });
  }

  function renderAdminDetail(session) {
    $("admin-detail").hidden = false;
    $("admin-detail-meta").textContent =
      session.session_id + " / " + session.site_id + " / " + session.status +
      (session.handoff ? " / handoff" : "");
    $("admin-detail-caps").textContent = "Droits : " + ((session.capabilities || []).join(", ") || "(aucun)");
    renderMessages($("admin-log"), session);
  }

  function loadAdmin() {
    var path = "/api/admin/sessions";
    if (statusFilter) {
      path += "?status=" + encodeURIComponent(statusFilter);
    }
    return api(path, { admin: true }).then(function (data) {
      $("admin-note").textContent = data.auth === "open_stub"
        ? "Mode stub ouvert : ADMIN_TOKEN absent. Ne pas exposer sur Internet."
        : "Jeton accepte. Liste de l'instance.";
      renderAdminList(data.sessions || []);
      return api("/api/admin/journal", { admin: true }).then(function (journal) {
        renderJournal(journal.journal || []);
      }).catch(function () {
        renderJournal([]);
      });
    }).catch(function (err) {
      $("admin-note").textContent = formatError(err);
    });
  }

  function loadAdminDetail(id) {
    api("/api/admin/sessions/" + encodeURIComponent(id), { admin: true }).then(renderAdminDetail).catch(function (err) {
      $("admin-detail").hidden = false;
      $("admin-detail-meta").textContent = formatError(err);
    });
  }

  function changeCaps(kind) {
    if (!selectedAdminId) {
      return;
    }
    var name = ($("admin-cap-name").value || "").trim();
    if (!name) {
      return;
    }
    var body = {};
    body[kind] = [name];
    api("/api/admin/sessions/" + encodeURIComponent(selectedAdminId) + "/capabilities", {
      method: "POST",
      admin: true,
      body: body,
    }).then(function () {
      loadAdminDetail(selectedAdminId);
    }).catch(function (err) {
      $("admin-detail-meta").textContent = formatError(err);
    });
  }

  function runTool(tool, args) {
    if (!visitorSession) {
      $("browser-act").textContent = "Ouvrez d'abord une session dans Support.";
      return Promise.resolve();
    }
    return api("/api/sessions/" + encodeURIComponent(visitorSession.session_id) + "/tools", {
      method: "POST",
      body: {
        tool: tool,
        origin: window.location.origin,
        args: args || {},
      },
    }).then(function (data) {
      $("browser-act").textContent =
        "ok tool=" + tool +
        " harness=" + (data.harness || "dom_simulator") +
        " request_id=" + (data.request_id || "");
      $("browser-frame").src = "/demo-app";
      return data;
    }).catch(function (err) {
      $("browser-act").textContent = formatError(err);
    });
  }

  function refreshBrowserLabel() {
    $("browser-session-label").textContent = visitorSession
      ? "Session : " + visitorSession.session_id
      : "Session : (utiliser Support d'abord)";
  }

  function fsQuery(which) {
    var path = ($("fs-path").value || "demo").trim();
    api("/api/browser/fs?path=" + encodeURIComponent(path), { admin: true }).then(function (data) {
      $("fs-out").textContent = JSON.stringify(data, null, 2);
    }).catch(function (err) {
      $("fs-out").textContent = formatError(err);
    });
  }

  function tryNavigate() {
    api("/api/browser/navigate", {
      method: "POST",
      admin: true,
      body: { url: "/demo-app" },
    }).then(function (data) {
      $("browser-act").textContent = JSON.stringify(data);
    }).catch(function (err) {
      $("browser-act").textContent = formatError(err);
    });
  }

  function loadStatus() {
    Promise.all([api("/health"), api("/api/os")]).then(function (pair) {
      var health = pair[0];
      var os = pair[1];
      var dl = $("status-dl");
      dl.innerHTML = "";
      [
        ["service", health.service],
        ["shell", health.shell],
        ["backend", health.backend],
        ["llm", health.llm],
        ["harness", health.harness],
        ["phase3_complete", String(health.phase3_complete)],
        ["us031_complete", String(health.us031_complete)],
        ["chromium_session_engine", String(health.chromium_session_engine)],
        ["browser_engine", health.browser_engine],
        ["panes", (os.panes || []).join(", ")],
      ].forEach(function (row) {
        var dt = document.createElement("dt");
        dt.textContent = row[0];
        var dd = document.createElement("dd");
        dd.textContent = row[1] == null ? "" : String(row[1]);
        dl.appendChild(dt);
        dl.appendChild(dd);
      });
      $("status-json").textContent = JSON.stringify(health, null, 2);
    }).catch(function (err) {
      $("status-json").textContent = formatError(err);
    });
  }

  function boot() {
    bindWindowChrome();
    tickClock();
    setInterval(tickClock, 1000);
    var existing = storedToken();
    if (existing) {
      $("admin-token").value = existing;
    }
    $("support-create").addEventListener("click", createSupportSession);
    $("support-form").addEventListener("submit", function (event) {
      event.preventDefault();
      sendVisitor();
    });
    $("support-escalate").addEventListener("click", escalateVisitor);
    $("admin-load").addEventListener("click", function () {
      saveToken($("admin-token").value.trim());
      loadAdmin();
    });
    Array.prototype.forEach.call(document.querySelectorAll("#admin-filters button"), function (btn) {
      btn.addEventListener("click", function () {
        statusFilter = btn.getAttribute("data-status") || "";
        Array.prototype.forEach.call(btn.parentNode.children, function (other) {
          other.classList.toggle("active-filter", other === btn);
        });
        loadAdmin();
      });
    });
    $("admin-grant").addEventListener("click", function () { changeCaps("grant"); });
    $("admin-revoke").addEventListener("click", function () { changeCaps("revoke"); });
    $("admin-takeover").addEventListener("click", function () {
      if (!selectedAdminId) {
        return;
      }
      api("/api/admin/sessions/" + encodeURIComponent(selectedAdminId) + "/takeover", {
        method: "POST",
        admin: true,
        body: {},
      }).then(function (session) {
        renderAdminDetail(session);
        loadAdmin();
        refreshSupport();
      }).catch(function (err) {
        $("admin-detail-meta").textContent = formatError(err);
      });
    });
    $("admin-human-form").addEventListener("submit", function (event) {
      event.preventDefault();
      if (!selectedAdminId) {
        return;
      }
      var text = ($("admin-human-input").value || "").trim();
      if (!text) {
        return;
      }
      api("/api/admin/sessions/" + encodeURIComponent(selectedAdminId) + "/messages", {
        method: "POST",
        admin: true,
        body: { content: text },
      }).then(function () {
        $("admin-human-input").value = "";
        loadAdminDetail(selectedAdminId);
        refreshSupport();
      }).catch(function (err) {
        $("admin-detail-meta").textContent = formatError(err);
      });
    });
    $("browser-click").addEventListener("click", function () {
      runTool("dom.click", { selector: "#menu-toggle" });
    });
    $("browser-invoice").addEventListener("click", function () {
      runTool("mcp.invoice.create", { customer: "OSUI", amount: "12.00" });
    });
    $("browser-navigate").addEventListener("click", tryNavigate);
    $("fs-list").addEventListener("click", function () { fsQuery("list"); });
    $("fs-read").addEventListener("click", function () { fsQuery("read"); });
    openPane("support");
    openPane("status");
    loadStatus();
  }

  boot();
})();
