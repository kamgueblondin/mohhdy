/* MOHHDY embed.js - scaffold ASSIST-050
 * Snippet public : dessine une bulle de chat. Aucun secret, aucune cle API,
 * aucun jeton admin. Le chat IA, les sessions et les actes navigateur
 * ne sont pas implementes.
 */
(function () {
  "use strict";

  var script = document.currentScript;
  var siteId = "unspecified";
  if (script && script.getAttribute) {
    siteId = script.getAttribute("data-mohhdy-site") || siteId;
  }

  function ready(fn) {
    if (document.readyState === "loading") {
      document.addEventListener("DOMContentLoaded", fn);
    } else {
      fn();
    }
  }

  function paint() {
    if (document.getElementById("mohhdy-launcher")) {
      return;
    }

    var style = document.createElement("style");
    style.textContent = [
      "#mohhdy-launcher{position:fixed;right:20px;bottom:20px;z-index:2147483000;",
      "width:56px;height:56px;border:0;border-radius:50%;cursor:pointer;",
      "background:#1b4332;color:#f4f1ea;font:600 22px/1 ui-sans-serif,system-ui,sans-serif;",
      "box-shadow:0 4px 16px rgba(0,0,0,.28);}",
      "#mohhdy-launcher:focus{outline:2px solid #3d9a8a;outline-offset:3px;}",
      "#mohhdy-panel{position:fixed;right:20px;bottom:86px;z-index:2147483000;",
      "width:min(360px,calc(100vw - 32px));background:#fff;color:#1b1f24;",
      "border-radius:12px;box-shadow:0 8px 28px rgba(0,0,0,.22);",
      "font:14px/1.45 ui-sans-serif,system-ui,sans-serif;display:none;overflow:hidden;}",
      "#mohhdy-panel.open{display:block;}",
      "#mohhdy-panel header{background:#1b4332;color:#f4f1ea;padding:12px 14px;}",
      "#mohhdy-panel header strong{display:block;font-size:15px;}",
      "#mohhdy-panel header span{display:block;opacity:.8;font-size:12px;margin-top:2px;}",
      "#mohhdy-panel .mohhdy-body{padding:14px;}",
      "#mohhdy-panel .mohhdy-stub{background:#fff6d6;border:1px solid #e0c35a;",
      "border-radius:8px;padding:10px 12px;margin:0 0 10px;font-size:13px;}",
      "#mohhdy-panel .mohhdy-input{width:100%;box-sizing:border-box;padding:8px 10px;",
      "border:1px solid #c9d0d6;border-radius:8px;font:inherit;}",
      "#mohhdy-panel .mohhdy-input:disabled{background:#f3f5f7;color:#6a7380;}"
    ].join("");
    document.head.appendChild(style);

    var btn = document.createElement("button");
    btn.id = "mohhdy-launcher";
    btn.type = "button";
    btn.setAttribute("aria-label", "Ouvrir le chat MOHHDY (scaffold)");
    btn.setAttribute("aria-expanded", "false");
    btn.textContent = "M";

    var panel = document.createElement("section");
    panel.id = "mohhdy-panel";
    panel.setAttribute("role", "dialog");
    panel.setAttribute("aria-label", "Chat MOHHDY scaffold");
    panel.innerHTML =
      "<header><strong>MOHHDY</strong><span>site " +
      String(siteId).replace(/[<>&"]/g, "") +
      " - scaffold</span></header>" +
      '<div class="mohhdy-body">' +
      '<p class="mohhdy-stub">Stub ASSIST-050 : aucune session n\'est ouverte, ' +
      "le chat IA ne repond pas, aucun acte navigateur n'est execute. " +
      "Ce script public ne contient ni secret ni cle API.</p>" +
      '<label for="mohhdy-compose">Message (desactive)</label>' +
      '<input class="mohhdy-input" id="mohhdy-compose" type="text" ' +
      'disabled placeholder="Non implemente">' +
      "</div>";

    btn.addEventListener("click", function () {
      var open = panel.classList.toggle("open");
      btn.setAttribute("aria-expanded", open ? "true" : "false");
    });

    document.body.appendChild(panel);
    document.body.appendChild(btn);
  }

  ready(paint);
})();
