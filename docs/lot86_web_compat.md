# Lot 86 - pages measured on 9 November 2026

The guest fetch path was not driven from QEMU for these four (CI may have
no route, and a full Google body is a script). The same URLs were read
with curl, User-Agent `Mozilla/5.0 (compatible; Mohhdy/86)`. The compat
scanner (`wf_compat_scan`) then classified a short fixture of each shape.
Live sizes are the curl byte counts. Fixtures are smaller on purpose.

| URL | Live bytes | Verdict | What the bytes are |
|---|---|---|---|
| https://www.google.com/search?q=mohhdy&gbv=1 | 92874 | js-wall | title "Google Search", a script, and a noscript refresh to `/httpservice/retry/enablejs`. Same wall as lot 85 (92893 bytes that morning). |
| https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=harbor&format=json | 1182 | reader | JSON. First hit title Harbor. totalhits 85989, suggestion "harpor". |
| https://example.com/ | 577 | reader | "Example Domain" and one paragraph. No script wall. |
| https://news.ycombinator.com/ | 33820 | table | A stories table, a search form (`name=q`), and `hn.js` at the end. The guest reader can align the table; it does not run the script. |

`www.google.com` with `gbv=1` is still not a result list. The guest keeps
using the Wikipedia API and the other fallbacks for a Google form, as
lot 85 did. Nothing here is a JavaScript engine.
