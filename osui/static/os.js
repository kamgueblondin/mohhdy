(function () {
  "use strict";

  var zIndex = 10;
  var visitorSession = null;
  var selectedAdminId = null;
  var statusFilter = "";
  var drag = null;
  var chatDrag = null;
  var CHAT_POS_KEY = "mohhdy.os.chat.pos";
  var PROGRAM_PANES = ["browser", "shell", "support", "admin", "status", "fs"];

  var COMMANDS = [
    { name: "help", aliases: ["?", "aide"], summary: "Liste les raccourcis du SE" },
    { name: "browser", aliases: ["nav", "browser-os"], summary: "Ouvre Browser-OS (simulateur DOM)", pane: "browser" },
    { name: "shell", aliases: ["sh", "terminal", "multiboot"], summary: "Ouvre le shell Multiboot (vocabulaire guest Ring 3)", pane: "shell" },
    { name: "admin", aliases: ["console"], summary: "Ouvre Admin (grant/revoke, takeover)", pane: "admin" },
    { name: "support", aliases: ["sessions"], summary: "Ouvre Support (sessions, escalade)", pane: "support" },
    { name: "status", aliases: ["sante", "health"], summary: "Ouvre Statut instance", pane: "status" },
    { name: "fs", aliases: ["files", "fichiers"], summary: "Ouvre le FS sandbox lecture", pane: "fs" },
    { name: "center", aliases: ["centre", "desktop"], summary: "Ferme les programmes et ramene le chat au centre" },
    { name: "close", aliases: ["fermer"], summary: "Ferme les programmes" },
    { name: "plan", aliases: ["mini-plan"], summary: "Mini-plan autonome stub sur #ai-stage" },
    { name: "draw", aliases: ["dessine"], summary: "Dessine sur la scene IA (stub HTML/SVG)" },
    { name: "stage", aliases: ["scene"], summary: "Met a jour la scene IA sans ouvrir de pane" },
    { name: "guest", aliases: ["guest-status"], summary: "Statut d'attache guest (bootstrap/live)" },
    { name: "ai-help", aliases: ["aihelp"], summary: "Aide IA Multiboot (stub llm=stub_echo)" }
  ];

  var PROMPT_OPEN = [
    { re: /\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(browser|navigateur|browser-os)\b/i, pane: "browser" },
    { re: /\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(shell|terminal|multiboot)\b/i, pane: "shell" },
    { re: /\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(admin|console)\b/i, pane: "admin" },
    { re: /\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(support|sessions)\b/i, pane: "support" },
    { re: /\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(status|statut|sante)\b/i, pane: "status" },
    { re: /\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(fs|fichiers|files|sandbox)\b/i, pane: "fs" },
    { re: /\b(montre|afficher|affiche)\b[\s\S]*\b(navigateur|browser)\b/i, pane: "browser" },
    { re: /\b(montre|afficher|affiche)\b[\s\S]*\b(shell|terminal)\b/i, pane: "shell" },
    { re: /\b(montre|afficher|affiche)\b[\s\S]*\b(admin)\b/i, pane: "admin" }
  ];

  function $(id) {
    return document.getElementById(id);
  }

  function paneWindow(name) {
    return document.querySelector('.os-window[data-pane="' + name + '"]');
  }

  function chatEl() {
    return $("os-chat");
  }

  function getChatMode() {
    return chatEl().getAttribute("data-mode") || "center";
  }

  function storedChatPos() {
    try {
      var raw = window.sessionStorage.getItem(CHAT_POS_KEY);
      if (!raw) {
        return null;
      }
      var pos = JSON.parse(raw);
      if (typeof pos.left === "number" && typeof pos.top === "number") {
        return pos;
      }
    } catch (err) {
      return null;
    }
    return null;
  }

  function saveChatPos(left, top) {
    try {
      window.sessionStorage.setItem(CHAT_POS_KEY, JSON.stringify({ left: left, top: top }));
    } catch (err) {
      /* ignore */
    }
  }

  function applyFloatPosition() {
    var chat = chatEl();
    var pos = storedChatPos();
    if (pos) {
      chat.style.left = pos.left + "px";
      chat.style.top = pos.top + "px";
      chat.style.right = "auto";
      chat.style.bottom = "auto";
    } else {
      chat.style.left = "";
      chat.style.top = "";
      chat.style.right = "";
      chat.style.bottom = "";
    }
    chat.style.transform = "";
  }

  function setChatMode(mode) {
    var chat = chatEl();
    var next = mode === "float" ? "float" : "center";
    chat.setAttribute("data-mode", next);
    if (next === "float") {
      applyFloatPosition();
    } else {
      chat.style.left = "";
      chat.style.top = "";
      chat.style.right = "";
      chat.style.bottom = "";
      chat.style.transform = "";
    }
  }

  function visiblePrograms() {
    return PROGRAM_PANES.filter(function (name) {
      var win = paneWindow(name);
      return win && !win.classList.contains("hidden");
    });
  }

  function restoreChatIfIdle() {
    if (!visiblePrograms().length) {
      setChatMode("center");
    }
  }

  function closeAllPrograms() {
    PROGRAM_PANES.forEach(function (name) {
      var win = paneWindow(name);
      if (win) {
        win.classList.add("hidden");
        win.classList.remove("active");
      }
    });
    setChatMode("center");
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
    if (name === "chat") {
      if (!visiblePrograms().length) {
        setChatMode("center");
      }
      $("os-chat-input").focus();
      return;
    }
    var win = paneWindow(name);
    if (!win) {
      return;
    }
    setChatMode("float");
    focusWindow(win);
    if (name === "status") {
      loadStatus();
    }
    if (name === "admin") {
      loadAdmin();
    }
    if (name === "browser") {
      refreshBrowserLabel();
    }
    if (name === "shell") {
      ensureShellWelcome();
    }
    if (name === "fs") {
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
          win.classList.remove("active");
          restoreChatIfIdle();
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
      if (chatDrag) {
        var desk = $("os-desktop").getBoundingClientRect();
        var left = Math.max(0, event.clientX - chatDrag.dx - desk.left);
        var top = Math.max(0, event.clientY - chatDrag.dy - desk.top);
        var chat = chatEl();
        var maxL = Math.max(0, desk.width - chat.offsetWidth);
        var maxT = Math.max(0, desk.height - chat.offsetHeight);
        left = Math.min(left, maxL);
        top = Math.min(top, maxT);
        chat.style.left = left + "px";
        chat.style.top = top + "px";
        chat.style.right = "auto";
        chat.style.bottom = "auto";
        chat.style.transform = "none";
        return;
      }
      if (!drag) {
        return;
      }
      drag.win.style.left = Math.max(0, event.clientX - drag.dx) + "px";
      drag.win.style.top = Math.max(0, event.clientY - drag.dy) + "px";
    });
    document.addEventListener("mouseup", function () {
      if (chatDrag) {
        var chat = chatEl();
        $("os-chat-header").style.cursor = "grab";
        saveChatPos(parseInt(chat.style.left, 10) || 0, parseInt(chat.style.top, 10) || 0);
        chatDrag = null;
      }
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

  function bindChatDrag() {
    var header = $("os-chat-header");
    header.addEventListener("mousedown", function (event) {
      if (getChatMode() !== "float") {
        return;
      }
      if (event.target && (event.target.id === "os-chat-center" || event.target.getAttribute("data-chat-center") !== null)) {
        return;
      }
      var rect = chatEl().getBoundingClientRect();
      chatDrag = {
        dx: event.clientX - rect.left,
        dy: event.clientY - rect.top,
      };
      header.style.cursor = "grabbing";
      event.preventDefault();
    });
    $("os-chat-center").addEventListener("click", function (event) {
      event.stopPropagation();
      closeAllPrograms();
    });
    var closer = header.querySelector("[data-chat-center]");
    if (closer) {
      closer.addEventListener("click", function (event) {
        event.stopPropagation();
        closeAllPrograms();
      });
    }
  }

  function appendChat(role, text, meta) {
    var container = $("os-chat-log");
    var wrap = document.createElement("div");
    wrap.className = "msg" + (role === "human" ? " human" : role === "sys" ? " sys" : "");
    var metaEl = document.createElement("div");
    metaEl.className = "meta";
    metaEl.textContent = meta || role;
    var body = document.createElement("div");
    body.textContent = text;
    wrap.appendChild(metaEl);
    wrap.appendChild(body);
    container.appendChild(wrap);
    container.scrollTop = container.scrollHeight;
  }

  function helpText() {
    var lines = [
      "Raccourcis du SE (chat central). llm=stub_echo.",
      "Un programme ouvert deplace le chat en panneau flottant (coin, draggable).",
    ];
    COMMANDS.forEach(function (cmd) {
      lines.push("/" + cmd.name + "  " + cmd.summary);
    });
    lines.push("Equivalent : \"ouvre le navigateur\", \"open shell\", \"dessine un cercle\".");
    lines.push("/plan lance un mini-plan autonome stub (reflecting -> acting -> presenting).");
    lines.push("Un prompt hors slash met a jour #ai-stage (llm=stub_echo).");
    return lines.join("\n");
  }

  function findCommand(token) {
    var name = String(token || "").replace(/^\//, "").toLowerCase();
    var i;
    for (i = 0; i < COMMANDS.length; i += 1) {
      if (COMMANDS[i].name === name) {
        return COMMANDS[i];
      }
      if ((COMMANDS[i].aliases || []).indexOf(name) !== -1) {
        return COMMANDS[i];
      }
    }
    return null;
  }

  function runCommand(cmd) {
    if (cmd.name === "help") {
      appendChat("sys", helpText(), "os /help · llm=stub_echo");
      return true;
    }
    if (cmd.name === "center") {
      closeAllPrograms();
      appendChat("sys", "Chat au centre. Programmes fermes.", "os /center");
      return true;
    }
    if (cmd.name === "close") {
      closeAllPrograms();
      appendChat("sys", "Programmes fermes. Chat au centre.", "os /close");
      return true;
    }
    if (cmd.name === "plan") {
      return sendRoutedPrompt("/plan " + (cmd.rest || "mini-plan autonome stub"));
    }
    if (cmd.name === "draw") {
      return sendRoutedPrompt("/draw " + (cmd.rest || "dessine trois boites"));
    }
    if (cmd.name === "stage") {
      return sendRoutedPrompt("/stage " + (cmd.rest || ""));
    }
    if (cmd.name === "guest") {
      return sendRoutedPrompt("/guest");
    }
    if (cmd.name === "ai-help") {
      return sendRoutedPrompt("/ai-help");
    }
    if (cmd.pane) {
      openPane(cmd.pane);
      appendChat("sys", "Programme ouvert : /" + cmd.name + " (" + cmd.summary + ")", "os /" + cmd.name);
      return true;
    }
    return false;
  }

  function parseLine(text) {
    var trimmed = String(text || "").trim();
    if (!trimmed) {
      return { kind: "empty" };
    }
    if (trimmed.charAt(0) === "/") {
      var parts = trimmed.split(/\s+/);
      var token = parts[0];
      var cmd = findCommand(token);
      if (!cmd) {
        return { kind: "unknown_slash", token: token };
      }
      cmd = {
        name: cmd.name,
        aliases: cmd.aliases,
        summary: cmd.summary,
        pane: cmd.pane,
        rest: parts.slice(1).join(" ")
      };
      return { kind: "slash", command: cmd };
    }
    var i;
    for (i = 0; i < PROMPT_OPEN.length; i += 1) {
      if (PROMPT_OPEN[i].re.test(trimmed)) {
        return { kind: "prompt_open", pane: PROMPT_OPEN[i].pane };
      }
    }
    return { kind: "prompt", text: trimmed };
  }

  function handleChatLine(text) {
    var parsed = parseLine(text);
    if (parsed.kind === "empty") {
      return Promise.resolve();
    }
    if (parsed.kind === "unknown_slash") {
      appendChat("human", text, "vous");
      appendChat("sys", "Commande inconnue " + parsed.token + ". Tapez /help.", "os");
      return Promise.resolve();
    }
    if (parsed.kind === "slash") {
      appendChat("human", text, "vous");
      return Promise.resolve(runCommand(parsed.command));
    }
    if (parsed.kind === "prompt_open") {
      appendChat("human", text, "vous");
      openPane(parsed.pane);
      appendChat("sys", "Programme ouvert via prompt : " + parsed.pane, "os");
      return Promise.resolve();
    }
    return sendRoutedPrompt(parsed.text);
  }

  function showSupportSession(session) {
    visitorSession = session;
    $("support-error").hidden = true;
    $("os-chat-error").hidden = true;
    $("support-meta").textContent =
      "session_id=" + session.session_id +
      " / llm=" + (session.llm || "stub_echo") +
      " / status=" + session.status +
      " / origin=" + (session.document_origin || "") +
      " / binding=" + (session.origin_binding || "");
    $("support-caps").textContent = "Droits : " + ((session.capabilities || []).join(", ") || "(aucun)");
    $("os-chat-hint").textContent =
      "session=" + session.session_id.slice(0, 8) + " llm=" + (session.llm || "stub_echo");
    renderMessages($("support-log"), session);
    refreshBrowserLabel();
  }

  function seedWelcome(container) {
    var wrap = document.createElement("div");
    wrap.className = "msg sys welcome";
    var meta = document.createElement("div");
    meta.className = "meta";
    meta.textContent = "os · llm=stub_echo";
    var body = document.createElement("div");
    body.textContent =
      "Mohhdy OS — SE Multiboot dirige par prompts.\n" +
      "llm=stub_echo (pas un LLM de production). phase3_complete=false. us031_complete=false.\n" +
      "Le bureau est la scene IA. Tapez /help, /shell, ou un prompt.";
    wrap.appendChild(meta);
    wrap.appendChild(body);
    container.appendChild(wrap);
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
    return api("/api/sessions", { method: "POST", body: { site_id: site } }).then(showSupportSession).catch(function (err) {
      $("support-error").hidden = false;
      $("support-error").textContent = formatError(err);
      $("os-chat-error").hidden = false;
      $("os-chat-error").textContent = formatError(err);
      throw err;
    });
  }

  function ensureSession() {
    if (visitorSession) {
      return Promise.resolve(visitorSession);
    }
    return createSupportSession().then(function () {
      return visitorSession;
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

  function sendOsPrompt(text) {
    return sendRoutedPrompt(text);
  }

  var planTimer = null;

  function stopPlanTimer() {
    if (planTimer) {
      window.clearInterval(planTimer);
      planTimer = null;
    }
  }

  function setPlanLabel(payload) {
    var el = $("ai-stage-plan");
    if (!el) {
      return;
    }
    if (payload && payload.autonomous) {
      var idx = (payload.step_index || 0) + 1;
      var total = payload.step_count || 3;
      el.textContent = "plan stub " + idx + "/" + total;
      el.hidden = false;
    } else {
      el.textContent = "";
      el.hidden = true;
    }
  }

  function playPlan(payload) {
    applyStage(payload);
    stopPlanTimer();
    if (!payload || !payload.autonomous || payload.done) {
      return;
    }
    var delay = payload.delay_ms || 450;
    planTimer = window.setInterval(function () {
      api("/api/os/stage/tick", { method: "POST", body: {} }).then(function (next) {
        applyStage(next);
        if (next && next.done) {
          stopPlanTimer();
        }
      }).catch(function () {
        stopPlanTimer();
      });
    }, delay);
  }

  function sendRoutedPrompt(text) {
    var trimmed = String(text || "").trim();
    if (!trimmed) {
      return Promise.resolve();
    }
    if (trimmed.charAt(0) !== "/") {
      appendChat("human", trimmed, "vous");
    }
    $("os-chat-error").hidden = true;
    setStageMode("reflecting");
    return api("/api/os/prompt", {
      method: "POST",
      body: { text: trimmed },
    }).then(function (routed) {
      if (routed.kind === "open_pane" && routed.pane) {
        openPane(routed.pane);
        appendChat("sys", "Programme ouvert via prompt : " + routed.pane, "os");
        return routed;
      }
      if (routed.kind === "shell") {
        if (routed.open_shell) {
          openPane("shell");
        }
        if (routed.shell && routed.shell.output) {
          appendChat("sys", String(routed.shell.output).replace(/\s+$/, ""), "os shell · llm=stub_echo");
        }
        if (routed.stage) {
          playPlan(routed.stage);
        }
        return routed;
      }
      if (routed.stage) {
        playPlan(routed.stage);
      }
      return ensureSession().then(function (session) {
        if (!session) {
          throw new Error("session indisponible");
        }
        return api("/api/sessions/" + encodeURIComponent(session.session_id) + "/messages", {
          method: "POST",
          body: { content: trimmed },
        });
      }).then(function (data) {
        $("os-chat-input").value = "";
        var llm = (data && data.llm) || "stub_echo";
        if (data && data.agent_message && data.agent_message.content) {
          appendChat("assistant", data.agent_message.content, "assistant · llm=" + llm);
        } else if (data && data.auto_reply === false) {
          appendChat("sys", "Pas de reponse auto (session prise par un humain).", "os · llm=" + llm);
        }
        refreshSupport();
        return routed;
      });
    }).catch(function (err) {
      $("os-chat-error").hidden = false;
      $("os-chat-error").textContent = formatError(err);
      appendChat("sys", formatError(err), "os erreur");
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
      $("browser-act").textContent = "Envoyez d'abord un prompt dans le chat, ou ouvrez Support.";
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
      : "Session : (prompt dans le chat ou Support)";
  }

  function fsQuery(pathId, outId) {
    var path = ($(pathId).value || "demo").trim();
    api("/api/browser/fs?path=" + encodeURIComponent(path), { admin: true }).then(function (data) {
      $(outId).textContent = JSON.stringify(data, null, 2);
    }).catch(function (err) {
      $(outId).textContent = formatError(err);
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
        ["primary", (os.interaction && os.interaction.primary) || "center_chat"],
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

  var STAGE_ALLOWED_TAGS = {
    DIV: 1, SPAN: 1, P: 1, H1: 1, H2: 1, H3: 1, UL: 1, OL: 1, LI: 1,
    STRONG: 1, EM: 1, SECTION: 1, ARTICLE: 1, HEADER: 1, SVG: 1, RECT: 1,
    CIRCLE: 1, LINE: 1, PATH: 1, TEXT: 1, G: 1, POLYLINE: 1, POLYGON: 1,
    ELLIPSE: 1, SMALL: 1, CODE: 1, PRE: 1, BR: 1
  };
  var STAGE_ALLOWED_ATTR = {
    "class": 1, id: 1, role: 1, "aria-label": 1, "data-mode": 1, "data-kind": 1,
    viewBox: 1, viewbox: 1, width: 1, height: 1, x: 1, y: 1, cx: 1, cy: 1, r: 1,
    rx: 1, ry: 1, d: 1, fill: 1, stroke: 1, "stroke-width": 1, transform: 1,
    points: 1, x1: 1, y1: 1, x2: 1, y2: 1, "text-anchor": 1, opacity: 1,
    "font-size": 1
  };

  function setStageMode(mode) {
    var stage = $("ai-stage");
    var next = mode === "acting" || mode === "presenting" ? mode : "reflecting";
    if (!stage) {
      return next;
    }
    stage.setAttribute("data-mode", next);
    var label = $("ai-stage-mode");
    if (label) {
      label.textContent = next;
    }
    return next;
  }

  function sanitizeStageHtml(html) {
    var parser = new window.DOMParser();
    var doc = parser.parseFromString("<div id=\"mh-wrap\">" + String(html || "") + "</div>", "text/html");
    var wrap = doc.getElementById("mh-wrap") || doc.body;
    function clean(node) {
      var children = Array.prototype.slice.call(node.childNodes);
      children.forEach(function (child) {
        if (child.nodeType === 8 || (child.nodeType !== 1 && child.nodeType !== 3)) {
          node.removeChild(child);
          return;
        }
        if (child.nodeType === 3) {
          return;
        }
        var tag = (child.tagName || "").toUpperCase();
        if (!STAGE_ALLOWED_TAGS[tag]) {
          node.removeChild(child);
          return;
        }
        Array.prototype.slice.call(child.attributes || []).forEach(function (attr) {
          var name = attr.name;
          if (name.slice(0, 2).toLowerCase() === "on" || !STAGE_ALLOWED_ATTR[name]) {
            child.removeAttribute(name);
            return;
          }
          if (/^\s*(javascript:|data:text\/html|vbscript:)/i.test(attr.value) || /expression\s*\(/i.test(attr.value)) {
            child.removeAttribute(name);
          }
        });
        clean(child);
      });
    }
    clean(wrap);
    return wrap.innerHTML;
  }

  function applyStage(payload) {
    if (!payload) {
      return;
    }
    setStageMode(payload.mode || "presenting");
    setPlanLabel(payload);
    var host = $("ai-stage-content");
    if (!host) {
      return;
    }
    host.innerHTML = sanitizeStageHtml(payload.html || "");
  }

  var shellReady = false;

  function shellPrint(text) {
    var out = $("shell-out");
    out.textContent += text + "\n";
    out.scrollTop = out.scrollHeight;
  }

  function ensureShellWelcome() {
    if (shellReady) {
      return;
    }
    shellReady = true;
    $("shell-out").textContent = "";
    shellPrint("MOHHDY Shell v6.0 — Multiboot Ring 3 (bootstrap osui)");
    shellPrint("llm=stub_echo  phase3_complete=false  us031_complete=false");
    shellPrint("attachment=bootstrap  live_guest=false  qemu_serial=false");
    shellPrint("Vocabulaire guest : help, ai, vfs-list, ls. Pas un bash Linux.");
    shellPrint("Live : MOHHDY_SHELL_ATTACH=live + serie/HMP (voir docs/osui_shell_live.md).");
    shellPrint("");
  }

  function runShellLine(line) {
    ensureShellWelcome();
    var raw = String(line || "");
    shellPrint("MOHHDY> " + raw.trim());
    if (!raw.trim()) {
      return;
    }
    api("/api/os/shell", { method: "POST", body: { line: raw } }).then(function (data) {
      if (data && data.clear) {
        shellReady = false;
        ensureShellWelcome();
        return;
      }
      if (data && data.output) {
        var text = String(data.output);
        if (text.charAt(text.length - 1) === "\n") {
          text = text.slice(0, -1);
        }
        shellPrint(text);
      }
      if (data && data.open_pane) {
        openPane(data.open_pane);
      }
      if (data && data.stage) {
        applyStage(data.stage);
      }
      var attach = $("shell-attach");
      if (attach && data) {
        attach.textContent =
          "attachment=" + (data.attachment || "bootstrap") +
          " live_guest=" + String(!!data.live_guest) +
          " qemu_serial=" + String(!!data.qemu_serial) +
          " prompt=" + (data.prompt || "MOHHDY>");
      }
    }).catch(function (err) {
      shellPrint(formatError(err));
    });
  }

  function boot() {
    bindWindowChrome();
    bindChatDrag();
    tickClock();
    setInterval(tickClock, 1000);
    setChatMode("center");
    seedWelcome($("os-chat-log"));
    var existing = storedToken();
    if (existing) {
      $("admin-token").value = existing;
    }
    $("support-create").addEventListener("click", function () {
      createSupportSession().catch(function () { /* deja affiche */ });
    });
    $("support-form").addEventListener("submit", function (event) {
      event.preventDefault();
      sendVisitor();
    });
    $("support-escalate").addEventListener("click", escalateVisitor);
    $("os-chat-form").addEventListener("submit", function (event) {
      event.preventDefault();
      var text = ($("os-chat-input").value || "").trim();
      $("os-chat-input").value = "";
      handleChatLine(text);
    });
    $("os-chat-input").addEventListener("keydown", function (event) {
      if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        $("os-chat-form").dispatchEvent(new Event("submit", { cancelable: true }));
      }
    });
    $("shell-form").addEventListener("submit", function (event) {
      event.preventDefault();
      var line = $("shell-input").value;
      $("shell-input").value = "";
      runShellLine(line);
    });
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
    $("fs-list").addEventListener("click", function () { fsQuery("fs-path", "fs-out"); });
    $("fs-read").addEventListener("click", function () { fsQuery("fs-path", "fs-out"); });
    $("fs-pane-list").addEventListener("click", function () { fsQuery("fs-pane-path", "fs-pane-out"); });
    $("fs-pane-read").addEventListener("click", function () { fsQuery("fs-pane-path", "fs-pane-out"); });
    loadStatus();
  }

  window.MohhdyOS = {
    version: "chat-desktop-stage-live",
    commands: COMMANDS.map(function (cmd) { return "/" + cmd.name; }),
    parseLine: parseLine,
    openPane: openPane,
    setChatMode: setChatMode,
    getChatMode: getChatMode,
    closeAllPrograms: closeAllPrograms,
    chatPosKey: CHAT_POS_KEY,
    setStageMode: setStageMode,
    applyStage: applyStage,
    sanitizeStageHtml: sanitizeStageHtml,
    playPlan: playPlan,
  };

  boot();
})();
