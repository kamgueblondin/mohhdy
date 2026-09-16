(function () {
  "use strict";

  var PROGRAM_PANES = ["browser", "shell", "support", "admin", "status", "fs"];
  var lastJson = "";
  var drag = null;

  function $(id) {
    return document.getElementById(id);
  }

  function paneWindow(name) {
    return document.querySelector('.os-window[data-pane="' + name + '"]');
  }

  function tickClock() {
    var el = $("os-clock");
    if (!el) return;
    var d = new Date();
    function pad(n) { return n < 10 ? "0" + n : String(n); }
    el.textContent = pad(d.getUTCHours()) + ":" + pad(d.getUTCMinutes()) + ":" + pad(d.getUTCSeconds()) + " UTC";
  }

  function sendLine(line) {
    if (!line) return Promise.resolve();
    return fetch("/api/line", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ line: line })
    }).then(function (res) {
      return res.json().catch(function () { return {}; });
    }).then(function () {
      return refresh();
    }).catch(function (err) {
      var box = $("os-chat-error");
      if (box) {
        box.hidden = false;
        box.textContent = String(err);
      }
    });
  }

  function setChatMode(mode) {
    var chat = $("os-chat");
    var next = mode === "float" ? "float" : "center";
    chat.setAttribute("data-mode", next);
    if (next !== "float") {
      chat.style.left = "";
      chat.style.top = "";
      chat.style.right = "";
      chat.style.bottom = "";
      chat.style.transform = "";
    }
  }

  function showPane(name) {
    PROGRAM_PANES.forEach(function (pane) {
      var win = paneWindow(pane);
      if (!win) return;
      var open = pane === name;
      win.classList.toggle("hidden", !open);
      win.classList.toggle("active", open);
    });
  }

  function renderMessages(messages) {
    var log = $("os-chat-log");
    if (!log) return;
    log.innerHTML = "";
    if (!messages || !messages.length) {
      var empty = document.createElement("div");
      empty.className = "msg sys";
      empty.innerHTML = "<span class=\"meta\">os · llm=stub_echo</span><div>Mohhdy OS — SE Multiboot dirige par prompts. llm=stub_echo (pas un LLM de production). phase3_complete=false, us031_complete=false. Tapez /help, /shell, ou un prompt.</div>";
      log.appendChild(empty);
      return;
    }
    messages.forEach(function (text) {
      var row = document.createElement("div");
      row.className = "msg";
      var meta = document.createElement("span");
      meta.className = "meta";
      meta.textContent = "session locale, stub";
      var body = document.createElement("div");
      body.textContent = text;
      row.appendChild(meta);
      row.appendChild(body);
      log.appendChild(row);
    });
    log.scrollTop = log.scrollHeight;
  }

  function stageMarkup(kind, mode) {
    if (kind === "circle") {
      return "<svg viewBox=\"0 0 200 120\" aria-hidden=\"true\"><circle cx=\"100\" cy=\"60\" r=\"42\" fill=\"none\" stroke=\"#7ad4c4\" stroke-width=\"3\"/><circle cx=\"100\" cy=\"60\" r=\"6\" fill=\"#3d9a8a\"/></svg>";
    }
    if (kind === "boxes") {
      return "<div class=\"ai-row\"><div class=\"ai-box\">A</div><div class=\"ai-box\">B</div><div class=\"ai-box\">C</div></div>";
    }
    if (kind === "graph") {
      return "<svg viewBox=\"0 0 220 120\"><polyline fill=\"none\" stroke=\"#7ad4c4\" stroke-width=\"3\" points=\"10,90 50,40 90,70 130,20 170,55 210,30\"/></svg>";
    }
    if (kind === "tree") {
      return "<ul class=\"ai-tree\"><li>root<ul><li>shell.c</li><li>osui_runtime.c</li><li>vfs</li></ul></li></ul>";
    }
    if (kind === "clock") {
      return "<svg viewBox=\"0 0 160 160\"><circle cx=\"80\" cy=\"80\" r=\"54\" fill=\"none\" stroke=\"#7ad4c4\" stroke-width=\"3\"/><line class=\"ai-clock-hour\" x1=\"80\" y1=\"80\" x2=\"80\" y2=\"48\" stroke=\"#e8eef4\" stroke-width=\"4\"/><line class=\"ai-clock-min\" x1=\"80\" y1=\"80\" x2=\"110\" y2=\"80\" stroke=\"#3d9a8a\" stroke-width=\"3\"/></svg>";
    }
    if (kind === "sim" || mode === "acting") {
      return "<div class=\"ai-sim\"><div class=\"ai-sim-box\">1</div><div class=\"ai-sim-box delay\">2</div><div class=\"ai-sim-box delay2\">3</div></div>";
    }
    return "<div class=\"ai-row\"><div class=\"ai-box" + (mode === "reflecting" ? "" : " muted") + "\">reflexion</div><div class=\"ai-box" + (mode === "acting" ? "" : " muted") + "\">action</div><div class=\"ai-box" + (mode === "presenting" ? "" : " muted") + "\">resultats</div></div>";
  }

  function renderStage(state) {
    var stage = $("ai-stage");
    var mode = state.stage_mode || "reflecting";
    var kind = state.stage_kind || "plan";
    stage.setAttribute("data-mode", mode);
    stage.setAttribute("data-kind", kind);
    $("ai-stage-mode").textContent = mode;
    var caption = $("ai-scene-caption");
    if (mode === "reflecting") caption.textContent = "llm=stub_echo · en attente d'un prompt";
    else if (mode === "acting") caption.textContent = "llm=stub_echo · action stub";
    else caption.textContent = "llm=stub_echo · resultats (kind=" + kind + ")";
    $("ai-scene-body").innerHTML = stageMarkup(kind, mode);
  }

  function applyState(state) {
    if (!state) return;
    setChatMode(state.chat_mode);
    showPane(state.pane && state.pane !== "none" ? state.pane : "");
    renderMessages(state.messages);
    renderStage(state);
    var hint = $("os-chat-hint");
    if (hint) hint.textContent = "session " + (state.session_id || "s0001") + ", stub";
    var flags = $("os-flags");
    if (flags) {
      flags.innerHTML = "<span>llm=" + (state.llm || "stub_echo") + "</span>" +
        "<span>phase3_complete=false</span>" +
        "<span>us031_complete=false</span>";
    }
    var status = $("status-json");
    if (status) status.textContent = JSON.stringify(state, null, 2);
    var support = $("support-out");
    if (support && state.pane === "support") support.textContent = (state.messages || []).join("\n") || "session " + (state.session_id || "");
    var admin = $("admin-out");
    if (admin && state.pane === "admin") admin.textContent = "session_id=" + (state.session_id || "") + "\ngrant/revoke/takeover dans le guest C";
    var shell = $("shell-out");
    if (shell && state.pane === "shell") shell.textContent = (state.messages || []).join("\n") || "prompt=MOHHDY> live_guest=true";
    var fs = $("fs-pane-out");
    if (fs && state.pane === "fs") fs.textContent = (state.messages || []).join("\n") || "fs-list / fs-read";
  }

  function refresh() {
    return fetch("/api/state").then(function (res) { return res.json(); }).then(function (state) {
      var raw = JSON.stringify(state);
      if (raw === lastJson) return;
      lastJson = raw;
      applyState(state);
    }).catch(function () { /* helper pas encore pret */ });
  }

  function bind() {
    document.addEventListener("click", function (ev) {
      var t = ev.target;
      if (!t) return;
      var btn = t.closest ? t.closest("[data-cmd]") : null;
      if (btn && btn.getAttribute("data-cmd")) {
        ev.preventDefault();
        sendLine(btn.getAttribute("data-cmd"));
      }
    });
    $("os-chat-form").addEventListener("submit", function (ev) {
      ev.preventDefault();
      var input = $("os-chat-input");
      var line = (input.value || "").trim();
      input.value = "";
      sendLine(line);
    });
    $("os-chat-input").addEventListener("keydown", function (ev) {
      if (ev.key === "Enter" && !ev.shiftKey) {
        ev.preventDefault();
        $("os-chat-form").dispatchEvent(new Event("submit", { cancelable: true }));
      }
    });
    $("shell-form").addEventListener("submit", function (ev) {
      ev.preventDefault();
      var input = $("shell-input");
      var line = (input.value || "").trim();
      input.value = "";
      sendLine(line);
    });

    var header = $("os-chat-header");
    header.addEventListener("mousedown", function (ev) {
      if ($("os-chat").getAttribute("data-mode") !== "float") return;
      drag = {
        x: ev.clientX - $("os-chat").offsetLeft,
        y: ev.clientY - $("os-chat").offsetTop
      };
    });
    document.addEventListener("mousemove", function (ev) {
      if (!drag) return;
      var chat = $("os-chat");
      chat.style.left = (ev.clientX - drag.x) + "px";
      chat.style.top = (ev.clientY - drag.y) + "px";
      chat.style.right = "auto";
      chat.style.bottom = "auto";
      chat.style.transform = "none";
    });
    document.addEventListener("mouseup", function () { drag = null; });
  }

  bind();
  tickClock();
  setInterval(tickClock, 1000);
  refresh();
  setInterval(refresh, 400);
})();
