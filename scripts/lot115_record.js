#!/usr/bin/env node
/* Lot 115: record what a reference DOM gives for the compatibility
 * fixtures in tests/fixtures/lot115 (a developer tool, not run in CI).
 *
 * Each fixture page logs its results with console.log. This script loads
 * every page in jsdom (24.x, node v24) with scripts enabled, at the URL
 * http://10.0.2.2:8067/lot115/compat/NAME.html, waits for its timers,
 * and writes tests/fixtures/lot115/expected.txt: for each page a line
 * "== NAME" and then its console lines. test_lot115_compat.c runs the
 * same pages through the guest's page pass (wf_jspage_run) and compares
 * line by line.
 *
 * Usage: NODE_PATH=/path/to/node_modules node scripts/lot115_record.js
 * (jsdom is not a dependency of the repository). */
'use strict';
const fs = require('fs');
const path = require('path');
const { JSDOM, VirtualConsole } = require('jsdom');

const DIR = path.join(__dirname, '..', 'tests', 'fixtures', 'lot115');
const BASE = 'http://10.0.2.2:8067/lot115/compat/';

function record(name) {
  return new Promise((resolve) => {
    const lines = [];
    const vc = new VirtualConsole();
    vc.on('log', (...a) => lines.push(a.map(String).join(' ')));
    vc.on('jsdomError', (e) => lines.push('error: ' + e.message.split('\n')[0]));
    const html = fs.readFileSync(path.join(DIR, name + '.html'), 'utf8');
    const dom = new JSDOM(html, { url: BASE + name + '.html', runScripts: 'dangerously', virtualConsole: vc, pretendToBeVisual: false,
      /* browsers have structuredClone; jsdom 24 does not expose it, node does */
      beforeParse(window) { window.structuredClone = structuredClone; } });
    setTimeout(() => { dom.window.close(); resolve(lines); }, 300);
  });
}

(async () => {
  const names = fs.readdirSync(DIR).filter((f) => f.endsWith('.html')).map((f) => f.slice(0, -5)).sort();
  let out = '';
  for (const n of names) {
    const lines = await record(n);
    out += '== ' + n + '\n' + lines.map((l) => l.replace(/\n/g, '\\n')).join('\n') + (lines.length ? '\n' : '');
  }
  fs.writeFileSync(path.join(DIR, 'expected.txt'), out);
  process.stdout.write(out);
})();
