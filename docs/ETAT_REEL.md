# État réel de MOHHDY

**Date de constat :** 3 octobre 2026 (rejeu local de `make test-all` 675/675 apres la liberation de l'instantane GGUF et le refus strict des syscalls 109/110, 674/674 apres la tranche session GGUF de `aiworker` (`test_gpt2_gguf_session`, `test_gguf_session`), 662/662 apres la tranche GGUF de `aiworker`, 661/661 avant, 648/648 avant, et `make qemu-smoke`, six scenarios en 547,9 s apres l'image). `python3 tests/integration/test_qemu_ai_gguf.py` : exit 0 en 255 s, binaires deja construits. `make qemu-foundation-steps`, `make qemu-ata-driver` 295,1 s, `make qemu-net-worker` 209,3 s, `make qemu-net-wire` 86,8 s et `make integration-qemu` 7/7 en 800,2 s decrivent la pointe precedente du meme jour. Le smoke du 2 octobre 2026 et les 800,6 s decrivent une mesure anterieure. Ponctuation de cette mise a jour : ASCII et accents francais.

**Reference :** prototype guest i386 32-bit, BIOS/Multiboot, QEMU et Ring 3. Ce n'est pas une distribution Linux.

**Produit.** Mohhdy est **un seul** SE agentique autonome Multiboot. Cette page mesure la **tranche guest i386** (chemin pedagogique sous QEMU), y compris la surface OS-UI Ring 3 (`userspace/osui_runtime.c` + `userspace/osui_gui.c` + `kernel/gfx_desktop.c` : chat stub, scene IA pixel, commande `gui` VBE 1024x768, sessions, droits, MCP, FS sandbox). Elle ne decrit pas un navigateur-OS Chromium, un LLM de production, ni l'ancien scaffold Python `agent/` / `osui/` (retires en OS-UI-3). Docker / PC / hyperviseur bootent l'instance QEMU, ce n'est pas un second produit. Le guest n'execute pas HTML.

**Role :** cette page decrit uniquement les fonctions observables dans le code et les tests **du guest**. Elle prevaut sur les diagnostics historiques et sur les specs de vision pour tout ce qui concerne QEMU i386. Les paragraphes historiques conservent les compteurs de leur livraison ; l'encadre ci-dessous decrit l'etat courant. Lexique : [vocabulaire.md](vocabulaire.md).

## État courant vérifié

## Etat courant du 30 October 2026 (lot 63)

US-031 partial, phase3_complete=false. The same bounded guest engine adds
::before/::after boxes with content strings, attr(), counter()/counters()
(counter-reset/increment/set, CSS Lists 3 scoping) and quotes, numeric and
named character references mapped to ASCII, overflow-x/overflow-y with
clip/auto/scroll clipping, the white-space longhands, word-wrap and word-
break keep-all/break-word, radial-gradient explicit sizes and "at
<position>" centres, rotate by any whole degree (deg/grad/rad/turn) and
skew painted as at most 8 row bands per rectangle with nested matrices
composed when no clip separates them, rounded dashed and dotted borders
(solid arcs, patterned straight parts) rounded overflow clips under
transforms, preserved tabs with tab-size and white-space break-spaces,
collapsed table bands per grid line, soft hyphens with hyphens
none/manual, transform matrix() and background-image url() of initrd
images with bounded repeat/position/size. Kernel, rights and shared ABI unchanged. No Chromium, WebKit,
network loading, JavaScript VM, new font engine or margin collapsing.
US-059/060/063/074 unchanged; US-G19..US-G24 remain partial. Margin
collapsing remains absent. Roadmap statuses and the complete non-done
assertion stay unchanged. Subset, remaining gaps, line counts and measured
results: docs/lot63_plan.md.

## Etat courant du 30 October 2026 (lot 62)

US-031 partial, phase3_complete=false. The same bounded guest engine adds
alpha colours (rgb/rgba/hsl/hsla, #rgba, #rrggbbaa, transparent) with
engine-side source-over compositing of translucent paint and opacity
groups (the kernel painter still receives opaque colours), shaded
groove/ridge/inset/outset borders, paint-time transforms (translate,
scale, quarter-turn rotate) about transform-origin, outline and
box-shadow (outer, inset, spread, two-layer blur), text-decoration,
text-transform, text-shadow, text-overflow ellipsis, percentage margins and
paddings against the containing block width (0 in intrinsic sizes),
linear-gradient backgrounds as at most 8 bands, background-clip/origin,
border-radius as row slices, CSS 2.1 appendix E order of block
backgrounds, floats, inline content and positioned boxes, and ol
start/reversed/value/type with alpha and roman markers. A second round
adds rounded overflow clipping, rounded double/3D borders and shadows,
letter-spacing, word-spacing, text-align justify and text-align-last,
radial-gradient rings, and grid-area / padding-box percentage containing
blocks. Kernel, rights and
shared ABI unchanged. No Chromium, WebKit, network loading, JavaScript VM,
new font engine or margin collapsing. US-059/060/063/074 unchanged;
US-G19..US-G24 remain partial. Margin collapsing remains absent.
Roadmap statuses and the complete non-done assertion stay unchanged.
Subset, remaining gaps, line counts and measured results:
docs/lot62_plan.md.

## Etat courant du 30 October 2026 (lot 61)

US-031 partial, phase3_complete=false. The same bounded guest engine adds
last baselines (row flex, grid with row spans), nested flex/grid/table
baseline export through a bounded probe, vertical-align:baseline for table
cells and inline-block, grid line occurrences, named spans and area aliases,
repeat(auto-fill/auto-fit) bounded to 1..8 tracks, a bounded subgrid on both
axes, an explicit viewport with position:fixed, static positions and bounded
GUI scroll, partial glyph clipping from kernel/gfx_font8.h and per-edge collapsed table borders (CSS 2.1 width, style, origin with col/colgroup and the table outer border, half-border content insets), dashed/dotted/double border painting, subgrid own gaps with parent content distribution and shared parent baselines, column flex and rowspan cell baselines, and bounded z-index and opacity stacking contexts with negative z below in-flow content (opacity 0 hides paint; partial opacity paints opaque). Kernel,
rights and shared ABI unchanged. No
Chromium, WebKit, network loading, JavaScript VM, new font engine or margin
collapsing. US-059/060/063/074 unchanged; US-G19..US-G24 remain partial.
Alpha blending, transform and margin collapsing remain absent. Roadmap statuses and the complete non-done
assertion stay unchanged. Subset, remaining gaps, line counts and measured
results: docs/lot61_plan.md.

## Etat courant du 29 October 2026 (lot 60)

US-031 partial, phase3_complete=false. The same bounded guest engine adds
first-baseline alignment for row flex and single-row grid items, plus
named grid lines/aliases and negative explicit-line indices on both axes.
Measurement reuses the existing text formatter; all name/request tables
are caller-owned and fixed. Kernel/rights/shared ABI/shell unchanged.
No Chromium, WebKit, network loading, JavaScript VM or new font engine.
US-059/060/063/074 unchanged; US-G19..US-G24 remain partial. Roadmap statuses
and the complete non-done assertion stay unchanged. Subset, remaining gaps,
line counts and measured results: docs/lot60_plan.md.

## Etat courant du 29 October 2026 (lot 59)

US-031 stays partial, phase3_complete=false. The same bounded guest engine
adds intrinsic width constraints and fit-content(length-percentage),
intrinsic flex bases, anywhere/break-all text breaks, bounded relative
right/bottom and atomic physical/logical insets. One shared measurement,
layout and imgdec path; i386 caller document 554816 bytes under its 1 MiB
cap. Kernel/rights/shared ABI unchanged. No Chromium, WebKit, network
loading, JavaScript VM or new font engine. US-059/060/063/074 unchanged;
US-G19..US-G24 stay partial. Roadmap statuses and complete non-done list
unchanged. Subset, remaining gaps and results: docs/lot59_plan.md.

## Etat courant du 29 October 2026 (lot 58)

US-031 stays partial, phase3_complete=false. The same bounded guest engine
adds typed integer calc/min/max/clamp for dimensions, constraints, flex
bases, gaps and positioned offsets; aspect-ratio; horizontal block/grid
auto margins; horizontal LTR logical dimensions and spacing aliases.
Measure/paint share the grid item width. Local images reuse one imgdec
workspace and its existing ceilings. The caller-owned document has a
compile-time 1 MiB cap (i386 actual 554176 bytes); no heap is added.
Kernel/rights/shared ABI unchanged. No Chromium, WebKit, network loading,
JavaScript VM or new font engine. US-059/060/063/074 unchanged;
US-G19..US-G24 remain partial. Roadmap statuses and the exact complete
non-done list are unchanged. Subset, limits and results: docs/lot58_plan.md.

## Etat courant du 29 October 2026 (lot 57)

US-031 stays partial, phase3_complete=false. The same bounded guest webeng
adds grid minmax/capped intrinsic tracks, mixed repeat groups, dense and
column placement, named areas and implicit track descriptors; explicit
object-fit/position and natural image ratio sizing through the one imgdec;
flex cross-axis auto margins; bounded combinator/attribute/structural CSS
selectors and !important cascade. Kernel/rights/syscall ABI unchanged.
No Chromium, WebKit, network loading, JavaScript VM or new font engine.
US-059/060/063/074 unchanged; US-G19..US-G24 remain partial. Roadmap statuses
and the exact complete non-done id list are unchanged. Detailed subset,
limits and measured results: docs/lot57_plan.md.

## Etat courant du 28 octobre 2026 (lot 56)

US-031 remains partial, phase3_complete=false. One guest webeng.c increment:
position absolute against the nearest positioned padding edge; flex
row-reverse, column-reverse and wrap-reverse; order -8..8; auto margins on
the flex main axis; table rowspan 1..8 and two-axis border-spacing;
white-space pre, pre-wrap and pre-line; clear; line-height; text-indent;
inline-block; table vertical-align; absolute z-index.
Kernel/rights/syscall ABI unchanged. No Chromium, WebKit, network loading,
JavaScript VM or new font engine. US-059/060/063/074 unchanged; US-G19..US-G24
remain partial. Measured limits and gates: [lot56_plan.md](lot56_plan.md).

## Etat courant du 28 octobre 2026 (lot 55)

US-031 remains partial, phase3_complete=false. One large guest webeng.c
layout increment: bounded intrinsic text/box sizes and content bases;
flex alignment/packing and axis gaps; fixed/weighted/auto grid tracks,
placement and spans; percentage heights and box-sizing; table colspan;
nowrap/break-word, clipping, visibility and matching link hits.
Measurement reuses the painter's formatter. Kernel/rights/syscall ABI
unchanged. No Chromium, WebKit, network loading, JavaScript VM or new
font engine. US-059/060/063/074 unchanged; US-G19..US-G24 remain partial.
Measured limits, useful line counts and gates: [lot55_plan.md](lot55_plan.md).

## Etat courant du 28 octobre 2026 (lot 54)

US-031 stays partial, phase3_complete=false. Column flex-basis/shrink now
uses the same bounded row solver, with px min/max-height limits, frozen
minima, capped grow and wrapping. Percent bases require definite height;
intrinsic sizing is still absent. Console and GUI share webeng.c.
No kernel, ABI or rights change; no Chromium, WebKit, network loading,
JavaScript VM or new font engine. Other story statuses unchanged;
US-G19..US-G24 remain partial. Details: [lot54_plan.md](lot54_plan.md).

## Etat courant du 28 octobre 2026 (lot 53)

US-031 stays partial and phase3_complete=false. Row flex-basis (px, percent,
auto) and weighted flex-shrink are in the same webeng.c. Minimum freezes,
rounding and unavoidable overflow are tested; column basis/shrink is
explicitly refused. Legacy defaults remain. No kernel/rights change,
Chromium, WebKit, network loading, JavaScript VM or new font engine.
US-059, US-060, US-063, US-074 and US-G19..US-G24 unchanged.
Details and measured gates: [lot53_plan.md](lot53_plan.md).

## Etat courant du 28 octobre 2026 (lot 52)

US-031 stays **partial**, `phase3_complete=false`. Plan and measured checks:
[lot52_plan.md](lot52_plan.md). Base: `origin/main` `303f23d` (PR #208).

- The same `userspace/webeng.c` lays out flex wrap/grow, grid row tracks,
  left/right floats, bounded tables and local images through shared imgdec.
- Also implemented, in the requested order: min/max widths, relative
  offsets bounded to the containing box, and list markers. No margin
  collapse. Relative vertical movement requires a definite parent height.
- Images use one static workspace, bounded in `webeng.h`. Output uses
  existing rectangle items; excessive source size, dimensions, decode
  scratch or display complexity produces a diagnostic and the alt.
- The image reader is the shell FS adapter in console and GUI, with the
  same kernel rights. No second shell, no kernel/ABI change, no new slash.
- Final local `make test-all`: 1344 passed, 0 failed, 0 skipped, 149s.
  `test_webeng`: 203/203, also ASan/UBSan. QEMU parity, web layout (1536
  image pixels) and production (71s) passed. Useful diff count: 4044; see plan.
- No Chromium, no WebKit, no network loading and no JavaScript VM.
  Fixed bitmap font only. US-059, US-060, US-063 and US-074 unchanged.
  US-G19 through US-G24 remain partial. Roadmap statuses unchanged.

## Etat courant du 28 octobre 2026 (lot 51)

Living baseline. Plan: [lot51_plan.md](lot51_plan.md). Base: `origin/main` `839d47a` (pull request #207 merged).

- US-G22, US-G23, US-G24 are `partial`. `/child`, `/release` and `/keys` run existing shell commands. `/p2p`, `/prod`, `/collab` and `/ci` keep their previous slash.
- `/release` opens on `deploy-verify` with no manifest: the shell prints `manifest not found` and writes nothing. `deploy-apply`, `prod-restore` and `prod-rollback` need a second Enter.
- `/keys` opens on `collab-keys`. While the node is down the shell prints `error p2p down`. That is not a live ledger.
- Phase 9: done 18, partial 6, simule 0, open 0, progress 75%, 69 entries. Next: US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24.
- US-059, US-060, US-063, US-074 unchanged. No JavaScript VM. No native compiler. No second shell.
- `make test-all`: 1150 passed, 0 failed, 0 skipped, 64 s. Journal `test_logs/test_results_20261028_051041.log` (28 October 2026, 09:10 Mauritius, UTC+4). `make tree-clean-check`, `make secrets-check`, `make roadmap-check` and `make strict-image-check` passed on `72aabc8` before that suite. Guest commands stay 447. `check_gui_parity` families=27 entries=161.
- `make qemu-gui-parity` passed (about 261 s, started 09:12 Mauritius, UTC+4). Serial: `osui panel out=child-exit-count ok 0`, `osui panel out=deploy-verify error manifest not found`, `osui panel out=collab-keys error p2p down (p2p-up first)`. `img-info /images/mire-prog.jpg` still prints `img-info ok fmt=jpeg w=48 h=32 bpp=24`.
- `make qemu-production` passed in 71 s (same run, finished 09:17 Mauritius, UTC+4). Guest printed `prod-roadmap phase 9 (gui-parity) done 18 partial 6 simule 0 open 0 progress 75%`, `prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24`, and `entries 69 bad 0`.
- `make qemu-osui-gui` passed (about 290 s, 09:18 to 09:23 Mauritius, UTC+4). `/p2p` still reaches `pane=p2p`, then `/collab` reaches `pane=collab` and `/prod` reaches `pane=prod`. ESC, shell-equals-console and the terminal window passed in the same target.
- CI run 40637781657 on the docs commit failed one job: `make qemu-ai-worker` missed `aiclient tokens ... end`. The serial log had the line, cut by `yield ok` (`aiclient token` + `yield ok` + `s 0 1 2 ... end`). `normalized_log` now glues that cut the same way it already glues a `spawn ok` cut. No character is dropped. Local re-run passed (10:35 to 10:38 Mauritius, UTC+4): stall 15.0 s, ESC cancel -149, same tokens on worker, kernel and fallback.

## Etat courant du 28 octobre 2026 (lot 50)

Living baseline. Plan: [lot50_plan.md](lot50_plan.md).

- US-G19, US-G20, US-G21 are `partial`. `/pm`, `/proc` and `/mesh` run existing shell commands from a window (same `handle_line`, same kernel rights). `/p2p` stays the desktop pane. pm-ide stays terminal-only. The mesh window does not bring a node up.
- US-G03 stays `done`. `qemu-gui-parity` expects `img-info ok` on `/images/mire-prog.jpg` and draws it in the viewer. Arithmetic SOF9 stays refused (no sample in the initrd). The explorer opens a `.jpg` in that same viewer. `.jpeg` is not recognized.
- Phase 9 of `/roadmap.txt`: done 18, partial 3, simule 0, open 0, progress 85%, 66 entries. Next non-done ids: US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21.
- US-059, US-060, US-063, US-074 unchanged. No JavaScript VM. No native code generator. `pm-assist` stays rules.
- This lot is a menu over the shell, not a second implementation. It is not tens of thousands of new lines: copying `shell.c` into the GUI would break the one-code-path rule.
- `make test-all` on the first Lot 50 tree: 1149 passed, 0 failed, 0 skipped, 64 s. Journal `test_logs/test_results_20261028_003631.log` (28 October 2026, 04:36 Mauritius, UTC+4). `make qemu-gui-parity` passed (about 248 s) on that tree, which still used the slash `/p2p` for the mesh menu.
- CI run 40610642239 on `d54e2c0` (04:44 Mauritius, UTC+4): gui-parity passed. Three jobs failed. `qemu-production` printed `prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21` (US-081 partial and US-097 simule were already in the file; the old expected string was only a prefix). `qemu-osui-gui` timed out on `pane=p2p` and `qemu-gui-apps` saw no P2P pane pixels, because `/p2p` had been taken by the new menu. The menu slash is now `/mesh`. `/p2p` is the pane again.
- Re-run after that fix (journal `test_logs/test_results_20261028_025410.log`, 28 October 2026, 06:54 Mauritius, UTC+4): `make test-all` 1149 passed, 0 failed, 0 skipped, 114 s. `make qemu-gui-parity`, `make qemu-gui-apps`, `make qemu-production` and `make qemu-osui-gui` passed.

## Etat courant du 28 octobre 2026 (lot 49)

Living baseline. Plan: [lot49_plan.md](lot49_plan.md). Pull request #206.

- US-120 `done`. `make roadmap-check` compares every phase 6-9 status with the US master tables.
- US-031 stays `partial`. Flex (row or column, gap) and grid (`repeat(N, 1fr)`, N at most 8) in `userspace/webeng.c`. No JavaScript VM. `phase3_complete` stays false.
- US-055 stays `partial`. Second host target: `pm-cross FILE py` (Python 3, same result as the VM). C99 remains. No native code generator.
- US-058 stays `partial`. `pm-assist` is still rules and names a missing `end`. No language model.
- US-059, US-060, US-063, US-074 unchanged (`partial`; US-060 seal key per boot and US-074 NAT are `simule`).
- US-G03 stays `done`. Progressive Huffman JPEG (SOF2, spectral selection, successive approximation) in the shared imgdec. Arithmetic JPEG stays refused. Scratch is caller-owned (pixels, plus int16 coefficients for SOF2).
- GUI: `/docs`, `/home`, `/home/user` and `/docs/readme.txt` are copied into the kernel VFS when `gui` starts, so the explorer and `ls` list the same tree. They are not created at shell init (that made `vfs-list overlay/` non-empty; the vfs-service job failed on commit 65bb9fb). Maximize paints once per event line. Terminal blue is light blue. `make qemu-gui-lot49` passed (explorer `/docs`, maximize to editor resize 0.00 s). `make integration-qemu-vfs` passed in 542.2 s (`vfs-list ok count 0`).
- `make test-all` on this commit: 1148 passed, 0 failed, 0 skipped, 67 s. Journal `test_logs/test_results_20261027_235517.log` (28 October 2026, 03:55 Mauritius, UTC+4). Coverage is not measured by that runner.
- `make coverage COVERAGE_MIN=78` on this commit: 81.91% lines executed (42464/51842) across 211 source files, 813 `.gcda`. Floor OK (81.91% >= 78%). Host unit-test translation units only, not the guest image. The previous CI run (65bb9fb) measured 81.93% (42143/51438).

## Etat courant du 24 octobre 2026 (UX 2: edition de ligne, fenetres, grille variable)

- Edition de ligne unique (`kernel/line_edit.c`) pour le mode texte et la fenetre terminal: historique (fleches haut/bas, 16 lignes, sans doublon consecutif), Tab complete le premier mot parmi les commandes du shell (prefixe commun, liste au second Tab), gauche/droite, Debut/Fin, Suppr. PgUp/PgDn font toujours defiler l'ecran. Test QEMU `make qemu-line-edit` (les deux modes) et test unitaire.
- Un seul gestionnaire de fenetres (`kernel/gfx_wm.c`) pour le terminal, les volets et le chat: boutons fermer (rouge), agrandir (vert), reduire (jaune), deplacement par la barre de titre. Une fenetre reduite a une marque ambre sous son icone du dock; l'icone (ou le menu) la restaure. Le terminal ne peut pas etre detruit (c'est la console): reduire et fermer reviennent au bureau, reduire garde la ligne en cours de saisie, fermer l'efface. Le chat se ferme et se rouvre par le menu Chat ou l'icone. La barre de menus surligne l'element actif. IDE et WEB gardent leur ancien bouton de fermeture seul. Test QEMU `make qemu-gui-windows`.
- Statut, FS et Admin montrent des donnees reelles lues par appels systeme (`userspace/osui_panes.c`): uptime, memoire, liste des taches (Statut), `ls /` (FS), etat de 9 services connus, actif ou absent (Admin). Rafraichi toutes les 100 ticks. Aucune donnee inventee.
- La grille de la console n'est plus fixe: `vga_console_resize` reconstruit les lignes logiques (marques de retour automatique, historique compris) et les remet en page. Fenetre: 80x25 (ou moins si l'ecran est trop petit). Agrandie: tout l'ecran (127x46 a 1024x768). Une taille d'ecran recue sur COM2 pendant que le terminal est affiche (`make run-gui-fit`) change le mode VBE et la grille. Le changement est applique au point sur de la boucle d'attente de `SYS_GETS`. Le mode texte et le bureau reviennent a 80x25. `term-dump` donne la grille (`grid=CxR`). Test QEMU `make qemu-gui-term-resize`.
- Limites connues (audit du meme jour): apres reduction d'un volet, la scene centrale (carte "Scene IA") et la fenetre de chat se chevauchent; `make run-gpt2` n'utilise que le zoom GTK, la grille ne suit donc la fenetre QEMU qu'avec `make run-gui-fit`; les volets Browser, Support, Prod, P2P et Collab restent des textes descriptifs; la bande d'etat (`llm=stub_echo usb31_complete=false`) est un texte de developpeur; le texte du chat lateral est coupe au lieu d'etre renvoye a la ligne; le service `vfs` apparait absent (aucun service de ce nom n'est enregistre aupres du superviseur); les icones du dock ne sont que des lettres (C, B, sh, A, S, i, F) sans infobulle; pas de pointeur texte ni de selection a la souris dans le terminal; une seule fenetre de volet a la fois.

## Etat courant du 24 octobre 2026 (terminal du bureau = la console)

- Le Shell du bureau n'est plus un volet avec son propre editeur de ligne: c'est la console elle-meme. `/shell`, l'icone Shell, le menu Shell, ou une ligne `ai ...` ou `!commande` tapee sur le bureau terminent la boucle du bureau; le shell (`userspace/shell.c`) revient a son invite normale et le noyau dessine la meme grille de cellules 80x25 de `kernel/vga_console.c` dans une fenetre VBE (`kernel/gfx_term.c`, avec la police VGA sauvegardee du plan 2). Une ligne tapee sur le bureau est executee a cette invite.
- Un seul coeur: memes cellules, couleurs, curseur, defilement, historique d'ecran (PgUp/PgDn), meme saisie de ligne (`SYS_GETS`), memes commandes, meme `ai` (Echap annule la generation comme en mode texte). Le pilote Ring 3 vgadriver n'est pas utilise pendant que la fenetre est affichee: la trame va au second backend.
- Supprime: l'evaluation de ligne du volet Shell (`run_program_line`, `seed_shell_term`, `osui_gui_set_program_eval`), la capture de sortie de shell.c (`putc` capture, `shell_gui_eval`, branches `g_gui_eval`).
- Sortir: `gui` revient au bureau, `console` (ou `gui-exit`) revient au mode texte. Echap va a l'application de la console, il ne quitte pas la fenetre.
- `term-dump` ecrit la grille visible sur la ligne serie (`TERM-DUMP BEGIN backend=text|vbe`). Test QEMU `make qemu-gui-shell`: la meme suite (clear, p2p-status, session-status, help qui defile) donne la meme grille de 25 lignes en mode texte et dans la fenetre, PgUp fait defiler la fenetre, `console` revient en 720x400. Tests unitaires: meme grille sur les deux backends a partir du meme flux, rendu des glyphes et couleurs, decoupage par cellules modifiees.
- Limites: la grille reste 80x25. La fenetre est centree; un cadre deux fois plus grand est utilise si l'ecran VBE fait au moins 1288x848. La taille VBE ne suit pas la fenetre QEMU tant que le terminal est affiche (elle est relue au retour au bureau). Pas de pointeur souris dans la fenetre. La console n'a ni rappel d'historique par fleche haute ni completion par Tab (fleche haute fait defiler d'une ligne), donc le terminal du bureau non plus: c'est la meme console. Le volet Shell decrit ci-dessous (meme jour, PR #194) est remplace par cette fenetre.

## Etat courant du 24 octobre 2026 (bureau: Echap et shell unique)

- Echap sur le bureau VBE ne laisse plus des bandes magenta et vertes. Cause: la sortie du bureau ne remettait que le registre misc et le sequenceur; le CRTC, le controleur graphique et le controleur d'attributs restaient en 1024x768 graphique, donc les cellules texte de 0xB8000 etaient lues comme des pixels. Le LFB ecrase aussi la police (plan 2) et les cellules texte, alors que le pilote Ring 3 vgadriver ne repeignait que les cellules qu'il croyait changees. Correction: jeu complet de registres VGA mode 3, police sauvegardee avant le premier passage en VBE puis restauree, et compteur `vram_epoch` dans la trame du vgadriver pour tout repeindre. Test QEMU: `make qemu-gui-esc` (deux allers-retours gui puis Echap, capture 720x400 sans pixels satures).
- Le volet Shell du bureau (`/shell` ou l'icone Shell) est le shell de la console: chaque ligne passe par `handle_line` de `userspace/shell.c`, y compris les commandes OS-UI et les lignes `/...`, que shell.c transmet lui-meme a `osui_dispatch_line` comme dans la console. Seuls `console`, `gui-exit`, `web`, `pm-ide` et `mobile-ui` restent geres par la boucle du bureau. Point unique de decision: `osui_gui_route_line`. Les autres volets (chat) gardent la ligne OS-UI, `ai ...` et `!commande` y passent aussi par shell.c. Test unitaire `test_gui_shell_pane_uses_console_shell`, test QEMU `make qemu-gui-shell` (meme sortie pour `p2p-status` et `session-status` en console et dans le volet Shell).
- Correctif en route: ce chemin faisait une faute de page (pile utilisateur epuisee sous la boucle du bureau). Les tampons de 1 a 3 Kio de la boucle du bureau et de `handle_line` sont maintenant statiques.
- Limites: le volet Shell affiche 14 lignes de 64 caracteres; une sortie plus longue est tronquee a l'ecran (elle reste complete sur la ligne serie). Les commandes qui lisent le clavier en continu ne sont pas prouvees dans le volet.

## Etat courant du 24 octobre 2026 (lot 43)

Living baseline. Plan: [lot43_plan.md](lot43_plan.md).

```text
make test-all : 1001 passed, 0 failed, 0 skipped
```

- US-015 / US-048: Storage VFS PCIe NVMe-over-Fabrics (NVMe-oF) Engine (`vfs_nvmf_engine_init`, `vfs_nvmf_register_target`, `vfs_nvmf_submit_sq`, `vfs_nvmf_process_cq`) and Multi-Tenant Resource Allocator Engine (`vfs_res_allocator_init`, `vfs_res_allocator_register_tenant`, `vfs_res_allocator_allocate_io`).
- US-065 / US-090: Web Runtime WebAssembly SIMD Vectorized LLM Mixture-of-Experts (MoE) Sparse Router (`webrt_moe_router_init`, `webrt_moe_route_tokens`) and WebGPU Multi-Pipeline Async Compute Engine (`webrt_webgpu_multi_pipeline_init`, `webrt_webgpu_multi_pipeline_create`, `webrt_webgpu_multi_pipeline_dispatch_parallel`).
- US-078 / US-090: P2P Mesh Network Cross-WAN Geo-Distributed BFT Anti-Entropy Engine (`p2p_bft_anti_entropy_init`, `p2p_bft_anti_entropy_add_node`, `p2p_bft_anti_entropy_reconcile`) and Post-Quantum ZK-STARK Batch Aggregator (`p2p_zk_stark_batch_init`, `p2p_zk_stark_batch_add_proof`, `p2p_zk_stark_batch_verify_all`).
- US-126 / US-135: Production Daemon Autonomous AI Workload Rebalancer Engine (`prod_ai_rebalancer_init`, `prod_ai_rebalancer_record_node_load`, `prod_ai_rebalancer_rebalance_and_isolate`) and Predictive Power/Thermal Core Affinity Scheduler (`prod_predictive_power_sched_init`, `prod_predictive_power_sched_bind_affinity`, `prod_predictive_power_sched_tick`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 23 octobre 2026 (lot 35)

Living baseline. Plan: [lot35_plan.md](lot35_plan.md).

```text
make test-all : 979 passed, 0 failed, 0 skipped
```

- US-008 / US-041: Storage VFS PCIe NVMe Direct-I/O Ring 3 Driver Engine (`vfs_nvme_direct_io_init`, `vfs_nvme_direct_io_submit`) and Multi-Stream Transactional Journaling Engine (`vfs_nvme_multi_stream_journal_init`, `vfs_nvme_multi_stream_journal_append`, `vfs_nvme_multi_stream_journal_flush`).
- US-056 / US-058: Web Runtime Service Workers PWA Offscreen Cache Engine (`webrt_service_worker_register`, `webrt_offscreen_cache_put`, `webrt_offscreen_cache_match`) and WebAssembly SIMD Vector Acceleration Engine (`webrt_wasm_simd_vec_add_f32`, `webrt_wasm_simd_vec_dot_i32`).
- US-071 / US-083: P2P Mesh Cross-WAN Resilient Raft Group Sharding (`p2p_cross_wan_raft_shard_init`, `p2p_cross_wan_raft_propose_sharded`, `p2p_cross_wan_raft_sync_groups`) and Zero-Knowledge Bulletproof Inner-Product Proof Validation (`p2p_zk_bulletproof_inner_product_create`, `p2p_zk_bulletproof_inner_product_verify`).
- US-119 / US-128: Production Daemon Autonomous Fault Mitigation Engine (`prod_autonomous_fault_mitigation_init`, `prod_autonomous_fault_mitigation_eval`) and Real-Time Predictive AI Anomaly Remediation Engine (`prod_ai_anomaly_remediation_init`, `prod_ai_anomaly_remediation_record`, `prod_ai_anomaly_remediation_eval`, `prod_ai_anomaly_remediation_execute`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 23 octobre 2026 (lot 33)

Living baseline. Plan: [lot33_plan.md](lot33_plan.md).

```text
make test-all : 971 passed, 0 failed, 0 skipped
```

- US-006 / US-039: Storage VFS CRDT Replicated Conflict-Free Resolution (`vfs_crdt_merge`, `vfs_crdt_resolve_conflict`) and 2PL/MVCC Distributed Transaction Isolation Engine (`vfs_dist_tx_begin`, `vfs_dist_tx_isolation_check`, `vfs_dist_tx_commit_iso`, `vfs_dist_tx_rollback_iso`).
- US-053 / US-055: Web Runtime WebGPU 16.16 fixed-point Compute Shader dispatch (`webrt_webgpu_compute_init`, `webrt_webgpu_dispatch_compute`) and WebAssembly Threads Atomic Futex Engine (`webrt_wasm_threads_init`, `webrt_wasm_atomic_op`, `webrt_futex_wait`, `webrt_futex_wake`).
- US-068 / US-080: P2P Mesh Cross-WAN Geo-Sharded Multi-Region Consensus (`p2p_geo_shard_init`, `p2p_geo_consensus_propose`, `p2p_geo_cross_region_sync`) and Zero-Knowledge Bulletproof Range Proof Validation (`p2p_zk_bulletproof_create_commitment`, `p2p_zk_bulletproof_verify`).
- US-117 / US-126: Production Daemon Autonomous SLA Self-Healing Engine (`prod_sla_self_heal_init`, `prod_sla_self_heal_eval`) and Predictive Anomaly Risk Mitigation Engine (`prod_anomaly_predict_init`, `prod_anomaly_predict_record`, `prod_anomaly_predict_eval`, `prod_anomaly_mitigate_action`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 23 octobre 2026 (lot 31)

Living baseline. Plan: [lot31_plan.md](lot31_plan.md).

```text
make test-all : 963 passed, 0 failed, 0 skipped
```

- US-004 / US-037: VFS Distributed Replicated State & Write-Ahead Log Engine (`vfs_wal_log_append`, `vfs_wal_checkpoint_apply`), multi-node distributed lock manager with dynamic lease and orphan detection (`vfs_distributed_lock_acquire`, `vfs_distributed_lock_release`), and distributed inode table integrity analysis with post-partition reconciliation (`vfs_distributed_reconcile`).
- US-049 / US-051: Web Runtime WebGL 3D Vector Pipeline in 16.16 fixed-point with 3D projection, integer z-buffering, and Phong illumination (`webrt_webgl3d_init`, `webrt_webgl3d_set_matrix`, `webrt_webgl3d_render_mesh`), AssemblyScript JIT bytecode VM with host function registration (`webrt_assemblyscript_exec`, `webrt_assemblyscript_register_host_fn`), and multi-thread synchronization with Shared ArrayBuffer & Atomics (`webrt_arraybuffer_share`, `webrt_atomics_wait_notify`).
- US-066 / US-078: P2P WAN Mesh Multi-Topology routing (Ring, Star, Mesh) with dynamic EWMA selection (`p2p_topology_select_route`), Byzantine Fault Tolerance consensus supporting up to f malicious nodes (`p2p_bft_preprepare`, `p2p_bft_prepare`, `p2p_bft_commit`), and distributed Bloom filter anti-entropy diff synchronization (`p2p_bloom_sync_init`, `p2p_bloom_sync_diff`).
- US-115 / US-124: Production Chaos Engineering fault injector daemon (`prod_chaos_inject_fault`, `prod_chaos_clear_faults`), incident auto-remediation playbook engine (`prod_remediation_run_playbook`), and FNV-1a signed post-mortem incident audit report generator (`prod_audit_postmortem_report`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 23 octobre 2026 (lot 30)

Living baseline. Plan: [lot30_plan.md](lot30_plan.md).

```text
make test-all : 959 passed, 0 failed, 0 skipped
```

- US-003 / US-036: Sandbox VFS Virtual Layer & Multi-Mount Overlay Engine (`vfs_mount_table_t`, `vfs_mount_point_add`, `vfs_mount_point_remove`), multi-layer Overlay FS Copy-On-Write write & file lookup (`vfs_overlay_cow_write`, `vfs_overlay_file_lookup`), and VFS namespace fragmentation analysis & compaction (`vfs_defrag_compact`).
- US-048 / US-050: Web Runtime Vector Canvas 2D engine in 16.16 fixed-point (`webrt_canvas2d_init`, `webrt_canvas2d_draw_path`, `webrt_canvas2d_transform`), MVP stack-based WebAssembly bytecode VM interpreter (`webrt_wasm_vm_init`, `webrt_wasm_exec_module`, `webrt_wasm_stack_push`, `webrt_wasm_stack_pop`), and animation frame tick event loop execution (`webrt_request_animation_frame`, `webrt_render_frame_tick`).
- US-065 / US-077: P2P WAN Multi-Region Mesh routing with region-weighted EWMA (`p2p_region_route_calculate`), Raft Log Sharding / Multi-Raft group consensus (`p2p_multi_raft_propose`, `p2p_multi_raft_commit`), and Merkle tree state anti-entropy diff synchronization (`p2p_merkle_sync_tree`, `p2p_merkle_diff_sync`).
- US-114 / US-123: Production predictive health matrix evaluation (`prod_predictive_health_eval`), worker auto-quarantine daemon (`prod_worker_auto_quarantine`), and global FNV-1a signed diagnostic audit reporting (`prod_audit_diagnostic_report`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 22 octobre 2026 (lot 27)

Living baseline. Plan: [lot27_plan.md](lot27_plan.md).

```text
make test-all : 945 passed, 0 failed, 0 skipped
```

- US-035: VFS journaled mutations (`vfs_journal_begin`, `vfs_journal_commit`, `vfs_journal_rollback`) and transactional snapshots (`vfs_snapshot_create`, `vfs_snapshot_restore`) in Ring 3.
- US-047 / US-063: DOM MutationObserver (`webrt_dom_observe_mutation`, `webrt_dom_notify_mutation`) and CSS Flexbox Light layout engine (`webrt_flex_compute_layout`) in Ring 3. Agent pipeline streaming (`pm_stream_init`, `pm_stream_append`, `pm_stream_read`) in PromptProgram.
- US-080: Multi-Cluster P2P Mesh inter-cluster routing (`p2p_cluster_route_forward`) and Raft dynamic fast recovery (`p2p_raft_fast_recovery`).
- US-122: Production AI SLA Monitor (`prod_ai_sla_init`, `prod_ai_sla_record`, `prod_ai_sla_eval`) and predictive hardware/service fault engine (`prod_fault_predict_init`, `prod_fault_predict_record`, `prod_fault_predict_eval`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 22 octobre 2026 (lot 26)

Living baseline. Plan: [lot26_plan.md](lot26_plan.md).

```text
make test-all : 940 passed, 0 failed, 0 skipped
```

- US-033 / US-034: VFS quota capacity manager (`vfs_quota_set`, `vfs_quota_check`), access rights control (`vfs_access_grant`, `vfs_access_control`), atomic inter-file swap (`vfs_atomic_swap`), and file locking (`vfs_flock`).
- US-043 / US-046 / US-062: Ring 3 JS/DOM Event Loop timers (`webrt_timer_schedule`, `webrt_event_loop_tick`), X.509 KeyUsage/EKU verification (`webrt_x509_verify_key_usage`), simulated OCSP revocation check (`webrt_ocsp_check_status`), and PromptProgram Multi-Agent Async Message Queue & Event Bus (`pm_bus_init`, `pm_bus_publish`, `pm_bus_subscribe`, `pm_bus_dequeue`).
- US-071 / US-079: Multi-Region P2P routing and hierarchical clustering (`p2p_region_set`), chunked Raft snapshot streaming (`raft_stream_snapshot_chunk`), and adaptive BBR-like congestion controller (`p2p_qos_bbr_step`).
- US-119 / US-121: Automated service auto-restarts (`auto_restarts`) in `prod_perf_monitor_eval` and `prod_support_bundle_text`, business KPI calculator (`prod_biz_kpi_eval`), and FNV-1a overlay persistence integrity validation.
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 22 octobre 2026 (lot 25)

Living baseline. Plan: [lot25_plan.md](lot25_plan.md).

```text
make test-all : 938 passed, 0 failed, 0 skipped
```

- US-032 / US-033: VFS multi-level subdirectories (`ramfs_mkdir_p`, `ramfs_list_recursive`), recursive directory copy (`ramfs_cp`), cross-directory move (`ramfs_mv`), and recursive deletion (`ramfs_rm_r` / `rm -r`) under capacity mutate access control.
- US-041 / US-045 / US-061: Ring 3 TLS handshake state machine and multi-level X.509 certificate chain validation (`webrt_tls_verify_cert_chain`), DOM event manager and async callback dispatch (`webrt_dom_add_event_listener`, `webrt_dom_dispatch_event`), multi-agent PromptProgram session context and instruction pipeline chaining (`pm_context_*`, `pm_pipe_exec`).
- US-070 / US-078: Raft log automatic partition reconciliation (`p2p_raft_reconcile_partition`), gRPC-Wire header compression (`grpcwire_compress_header`), and dynamic leader lease adjustment under high RTT / packet loss (`p2p_raft_update_lease`).
- US-118 / US-120: Automatic auto-correction for stalled/blocked workers (`prod_autofix_stalled_workers`), consolidated predictive maintenance risk engine, and FNV-1a checksum verification.
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 22 octobre 2026 (lot 24)

Living baseline. Plan: [lot24_plan.md](lot24_plan.md).

```text
make test-all : 935 passed, 0 failed, 0 skipped
```

- US-039 / US-041: Dynamic CSP evaluation (`webrt_csp_enforce`, `webrt_csp_validate_display_list_resource`) and origin isolation (`webrt_site_storage_validate_origin`) consolidated for all resource types. Security audit logging (`osui-audit`) for CSP blocked resources (`csp_blocked`), unauthorized origin access (`origin_denied`), and MCP credentials management (`mcp_auth`, `mcp_credentials`, `mcp_invoke`).
- US-070 / US-076: Raft log anti-entropy synchronization refined post-partition reconnects and snapshot compaction (`p2p_raft_compact`, `raft_install_snapshot`). EWMA link cost calculation (`p2p_calc_link_cost`), reactive AIMD rate control (`p2p_qos_note_loss`), and automatic P2P cache invalidation on peer disconnect (`p2p_cache_on_peer_down`).
- US-110 / US-120: Dynamic workload autoscaling under extreme stress scenarios (`prod_autoscale_workload_eval`), global performance monitor (`prod_perf_monitor_eval`), FNV-1a roadmap evaluation (`/roadmap.txt`, `prod_roadmap_eval`), and health score inclusion in support bundles (`prod-support-bundle`, `prod_support_bundle_text`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 21 octobre 2026 (lot 22)

Living baseline. Plan: [lot22_plan.md](lot22_plan.md).

```text
make test-all : 929 passed, 0 failed, 0 skipped
```

- US-041 / US-060: Consolidated sandboxed PromptMessage execution (`cmd_browser_pm_run`), CA certificate validation, Schnorr signatures (`csig_verify`), and extended security audit trail (`osui-audit`, `audit_log_add`) for CA key rotation (`pm_ca_rotate_status`) and user session lifecycle (`session_new`, `session_use`, `session_end`, `session_ttl`, `session_restore`, `session_cleanup`).
- US-075 / US-082: Consolidated P2P state synchronization (`p2p_sync`), Raft election/compaction (`p2p_raft_elect`, `p2p_raft_compact`, `raft_step_down`), Last-Writer-Wins (LWW) collaborative transaction resolution, and AES-128-GCM signing key sealing (`g_collab_locked`, `shell_persist_line`).
- US-112 / US-116: Refined predictive maintenance risk calculation (`prod_maint_model_risk`), workload auto-scaling (`prod_autoscale_workload_eval`), performance monitoring (`prod_perf_monitor_eval`), dynamic roadmap evaluation (`prod_roadmap_eval`, `checksum_fnv`), and diagnostic support bundle generation (`prod_support_bundle_text`, `prod-support-bundle`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 21 octobre 2026 (lot 17)

Living baseline. Plan: [lot17_plan.md](lot17_plan.md).

```text
make test-all : 914+ passed, 0 failed, 0 skipped
```

- US-040 / US-081: Evolved Web Runtime DOM/CSS media query evaluation (`aspect-ratio`, `min-aspect-ratio`, `max-aspect-ratio`) in `userspace/webrt.c` and consolidated VFS sandbox security audit logging (`vfs_read`, `vfs_scope_denied`, `vfs_traversal_denied`, `vfs_fs_save`, `vfs_fs_load`, `fetch_allowed`) in `userspace/osui_runtime.c`.
- US-070 / US-076: Reinforced P2P Raft consensus resilience by purging active proposals (`n->prop.active = 0`, `n->prop.result = -2`) on term step-down / network partition, and cleaning up uncommitted log slots and cache fences during snapshot installation (`raft_install_snapshot`) and compaction (`p2p_raft_compact`).
- US-110 / US-121: Extended production autoscaling workload evaluation (`prod_autoscale_workload_eval`) for high CPU and queue depth stress scenarios, and integrated system health score & status into diagnostic support bundles (`prod_support_bundle_text`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 20 octobre 2026 (lot 15)

Living baseline. Plan: [lot15_plan.md](lot15_plan.md).

```text
make test-all : 913+ passed, 0 failed, 0 skipped
```

- US-039 / US-031: Evolved Web Runtime CSP enforcement (`webrt_csp_enforce`, `webrt_csp_validate_display_list_resource`) across scripts, styles, images, media streams, and fetch/connect directives, logging all CSP violations into the `osui-audit` security audit trail.
- US-063 / US-070: Consolidated P2P link cost calculations (`p2p_calc_link_cost`) using sliding window EWMA RTT (`rtt_ewma_ms`), packet loss percentage, and bucket load. Improved mesh network resilience with automatic cache invalidation and waiting queue cleanup on peer disconnection (`p2p_cache_on_peer_down`).
- US-120 / Garde 1: Updated roadmap evaluation (`prod_roadmap_eval`, `prod_roadmap_parse`) with FNV-1a checksum integrity verification (`checksum_fnv`), exposing phase progress and checksum telemetry in `prod-roadmap` and OS-UI telemetry outputs.
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 20 octobre 2026 (lot 14)

Living baseline. Plan: [lot14_plan.md](lot14_plan.md).

```text
make test-all : 913+ passed, 0 failed, 0 skipped
```

- US-043 / US-045: Evolved browser performance telemetry (`web-perf`, `browser-telemetry perf`) with cycle latency calculation and scheduling tick overhead tracking (`telemetry_sched_cycles`, `perf_min_us`, `perf_max_us`, `perf_avg_us`, `perf_last_us`, `render_ms`).
- US-045 / VFS-as-Web: Extended VFS/DOM tree inspection in `/browser` and `/fs` for target node inspection, DOM hierarchy display (`browser-dom-tree`), and sandboxed expression evaluation (`browser-eval`).
- Garde 3 / GGUF: Consolidated multi-turn KV cache handling in `kernel/llm/gpt2_gguf_infer.c` (`gpt2_gguf_cache_match_prefix`, `gpt2_gguf_align_cache_prefix`) to rewind matching prefix KV entries on multi-turn prompt changes. Validated KVM latency benchmark protocol (`make gguf-kvm-benchmark-check`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest (`python3 scripts/extract_guest_commands.py --check`).

## Etat courant du 20 octobre 2026 (lot 13)

Living baseline. Plan: [lot13_plan.md](lot13_plan.md).

```text
make test-all : 912+ passed, 0 failed, 0 skipped
```

- US-034: Evolved PWA engine with isolated origin execution (`pwa://<app_id>`), strict manifest validation, required install verification before execution, and MCP connector access control (`mcp-auth`, `mcp-credentials`, `mcp-invoke`).
- US-048: Enhanced freestanding media stream handling (`browser-media`, `media-play`, `media-pause`, `media-stop`, `media-stream`) with autonomous state management, tick telemetry (`telemetry_media_ticks`), and status header display on the QEMU VBE desktop.
- US-081: Extended structured security audit logging (`osui-audit`, `audit_log_add`) tracking origin denials (`origin_denied`), MCP capability & auth violations (`mcp_capability_denied`, `mcp_auth_required`), and PWA lifecycle events (`pwa_manifest`, `pwa_install`, `pwa_run`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest.

## Etat courant du 19 octobre 2026 (lot 11)

Living baseline. Plan: [lot11_plan.md](lot11_plan.md).

```text
make test-all : 907+ passed, 0 failed, 0 skipped
```

- US-039 / US-031: Extended CSP verification in `userspace/webrt.c` and `userspace/webrt.h` with dynamic origin validation for image, style, and script display list resources (`webrt_csp_validate_display_list_resource`). Unauthorized load attempts logged to the audit log (`osui-audit`).
- US-063 / US-070: Reinforced degraded link detection in P2P routing table using smoothed EWMA RTT sliding window (`rtt_ewma_ms` in `link_cost`). Automatic stale cache entry invalidation (`p2p_cache_on_peer_down`) triggered on peer disconnections.
- US-081: Integrated power profile manager (`plat_power_schedule_tick`) into OS-UI runtime scheduling tick (`osui_stage_tick` in `userspace/osui_runtime.c`) to dynamically adapt background activity levels (`background_ms`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest.

## Etat courant du 19 octobre 2026 (lot 10)

Living baseline. Branched from `600fcfa` (merge of pull request #158), branch `cursor/lot10-geant`, pull request #159. Plan: [lot10_plan.md](lot10_plan.md).

```text
make test-all : 902/902 passed, 0 failed, 0 skipped
```

- US-039: Full CSP directive enforcement in `userspace/webrt.c` (`webrt_csp_enforce`). Evaluates script, style, image and fetch resource permissions against origin.
- US-059/060: CA Key Rotation and Revocation status helper in `userspace/promptmessage.c` (`pm_ca_rotate_status`).
- US-078/081: Dynamic sensor discharge simulation on tick cycles in `userspace/platform.c` (`plat_sensor_tick`).
- US-097: Dynamic points conversion with fee percentage computation in `userspace/collab.c` (`collab_pay_convert`).
- US-120: Dynamic roadmap evaluation with FNV checksum calculation in `userspace/prod.c` (`prod_roadmap_eval`).
- Command alignment: 442 commands aligned and verified across shell builtins and JSON manifest.

## Etat courant du 17 octobre 2026 (lot 9)

Living baseline. Branched from `0b6114c` (merge of pull request #157), branch `cursor/lot9-geant`, pull request #158. The lot 8 section below keeps its own counter (894/894). Plan and full inventory: [lot9_plan.md](lot9_plan.md).

```text
make test-all : 901/901 passed, 0 failed, 0 skipped
```

- CI headroom: jobs rebalanced (qemu-platform in coverage, qemu-net-worker in ai-gguf, test-all in production, new job `collab` for `make qemu-collab`); every job finishes in about 10 min or less on the 15 min budget.
- US-067 closed: gRPC (HTTP/2 frames, HPACK subset, protobuf) carried in the application records of the guest TLS 1.2 session between two guests (`grpc-tls-serve`, `grpc-tls-call`, AES-128-GCM); the hub sees no gRPC preface, path or payload in clear. Proof: `make qemu-net-peer-tls-worker`. HPACK stays a subset.
- US-099 closed: contract language MCL in the ledger (`collab-contract-deploy`, `collab-signoff`), compiled and evaluated deterministically on every replica (pay, refund, split, conditions on sign-offs, payee, arbiter, elapsed Lamport time). Proof: `test_collab`, `make qemu-collab`. No real money (US-097 stays simule).
- US-104 closed as a verified self-check: six computed checks, signatures re-verified with the bound keys, erasure verified, GDPR article map. It is not a certification.
- US-112 closed: offline signature database (built in or file), hex patterns and FNV hashes, severities (`prod-sec-db`). No online CVE feed.
- US-115 closed: Q12 logistic regression trained in the guest on labelled windows (`prod-maint-train`, `prod-maint-predict` learned verdict).
- US-083 closed: `devsync-pull` between two guests over the sealed P2P channel (version vector request, delta reply, last-writer-wins, equal digests, no plaintext on the wire). Proof: `make qemu-p2p` scenario 1b.
- US-077 closed: `mobile-ui` typed on the OS-UI VBE desktop paints the phone frame through the web pane display list (`mui_display_list`), checked pixel by pixel against the pixel model in `test_mobile_ui` and on the framebuffer in `make qemu-osui-gui`. Touch input stays simule (no touch hardware).
- US-103 closed: Schnorr NIZK in a 2048-bit group with a 256-bit subgroup (`csig_zk_*`, `collab-zk-strong`), parameters from `scripts/gen_zk_group.py` (fixed labels, re-checked in the guest), known answer test against an independent Python run. Not constant time. The 62-bit toy command `collab-zk-prove` is kept.
- Root causes fixed: the shell prompt is written in one call (a kernel line from another task split the `ESC[0m : ` needle in the four-guest job), the collab job creates its log directory.
- Still partial or open: see the inventory in [lot9_plan.md](lot9_plan.md) (phase 2 open, US-038/US-039, US-050, US-055, US-058, US-059/US-060, US-063, US-068/US-073, US-069, US-070, US-074, US-075, US-076, US-080, US-081, US-120 partial, US-078 and US-097 simule).

## Etat courant du 17 octobre 2026 (lot 8)

Living baseline. Branched from `1de7236` (merge of pull request #156), branch `cursor/lot8-geant`, pull request #157. The lot 7 section below keeps its own counter (867/867).

```text
make test-all : 894/894 passed, 0 failed, 0 skipped
```

New unit files: `test_usb_dma`, `test_arm_mmu`, `test_mobile_ui`, `test_devsync`; new cases in `test_rtc` (long update-in-progress, torn snapshot, tick between snapshots), `test_device_service` (USB owner and counters), `test_collab` (Schnorr proof), `test_prod` (roadmap parser).

- NE2000 TLS multipair flake, root cause: `rtc_read_utc` failed when a read hit the CMOS update window (measured in QEMU: about 5000 failed reads in 6 million, all next to a second boundary), and the TLS poll of the Ring 3 networker treated a failed clock read as fatal (`ai-tls-poll rc -96` right after server_hello, when the certificate time check runs). Fix in the guest: the reader waits for the update flag with a bound and requires two equal snapshots (up to 16 attempts); `SYS_RTC_UTC` exposes it; the relay log names the failing stage (`tls-fail clock` or `tls-fail handshake`). Regression: three `test_rtc` cases and the guest `rtc-check` command in qemu-smoke (loops across 3 second boundaries, `fail 0`; the old reader shows 133 to 183 failures per run). Loading the host (taskset, busy loops, 6 parallel guests) never reproduced the stall; the measurement did.
- US-001 closed on the default image (decision for the owner to confirm). The UHCI USB tablet driver runs in Ring 3 (`bin/usbdriver`, service `usb-driver` pinned to that binary, spawned at boot when a controller exists). Bounded DMA without an IOMMU: the kernel finds and stops the controller at boot, owns the frame list and the schedule page (one QH, 8 TDs, mapped read-only in the driver), and writes every TD itself after `usbdma_build_td` and `usbdma_chain_check` (buffer inside the driver data page, links inside the schedule, no cycle). The driver gets the data page writable and only USBSTS, USBINTR, FRNUM, PORTSC1/2 in its IOPB (not USBCMD, not FLBASEADD). When the driver dies the kernel stops the controller first, then the supervisor respawns it (budget 4). No Ring 0 tablet polling remains (timer and keyboard hooks removed); without the driver only the PS/2 mouse moves the pointer. Kept in the kernel by design: PIC, PIT, IRQ entry stubs, early boot console, panic path, early boot PCI probes (UHCI reset, NE2000 presence). The legacy comparison image keeps its Ring 0 NIC path (not shipped). Proof: `make qemu-usb-driver` (24 s: bounds probe refused with -181 for a buffer leaving the page and for a looping schedule, QMP tablet reports, `devrogue` refused -180, kill: `stops` 1, respawn, re-enumeration, reports again), `make qemu-osui-gui` (dock clicks go through the Ring 3 driver), `test_usb_dma`, `test_device_service`.
- US-076 partial: ARMv7-A kernel core in `arch/arm` (boot, PL011, GICv2, generic timer at 100 Hz, MMU with 1 MB sections: kernel and devices PL1-only, one user section; USR-mode tasks enter by SVC with the i386 syscall numbers; round-robin preemption; user pointers checked; a user read of kernel memory takes a permission fault and only that task dies). `kernel/ipc.c` and `kernel/sha256.c` are compiled unchanged. Not ported: drivers, filesystem, AI engine, shell, OS-UI. Isolated: own CI job `arm-port` (gcc-arm-none-eabi, qemu-system-arm), nothing in the i386 build or images changes. Proof: `make qemu-arm-boot` (2 s), `test_arm_mmu`.
- US-077 partial: mobile shell model of the guest (`userspace/mobile_ui.c`, command `mobile-ui`): phone and tablet layouts from the US-080 form factor, touch targets of at least 48 px, nav bar (back, home, recents), back stack, notification shade, gestures through the US-079 classifier, hashed pixel model. No touch hardware (input simule) and it is not painted on the VBE screen. Proof: `test_mobile_ui`, `make qemu-platform`.
- US-083 partial: multi-device sync (`userspace/devsync.c`, commands `devsync-*`): last-writer-wins map (Lamport clock, device id tie break, tombstones), version vectors, deltas carrying only unseen entries, FNV-checked text format (a tampered delta changes nothing). Three device replicas live in one guest (simule devices); the transport between guests is not wired. Proof: `test_devsync`, `make qemu-platform` (replicas converge to the same digest).
- US-103 partial, stronger: Schnorr zero-knowledge proof of knowledge (Fiat-Shamir with SHA-256, bound to a message, subgroup checks) in `collab.c` (`collab-zk-prove`). The 62-bit safe-prime group is a toy size (simule strength). Proof: `test_collab`, `make qemu-collab`.
- US-120 partial: `prod-roadmap` computes per-phase progress from `/roadmap.txt` in the initrd (one line per story); the file is maintained by hand. Proof: `test_prod`, `make qemu-production`.
- Unchanged and still partial: US-099 (fixed escrow templates), US-104 (computed checks), US-112 (no CVE database), US-115 (heuristic). US-067 TLS for gRPC not done in this lot. US-078 and US-097 stay simule.

## Etat courant du 17 octobre 2026 (lot 7)

Living baseline. Branched from `a44e95c` (merge of pull request #155), branch `cursor/lot7-geant`, pull request #156. The lot 6 section below keeps its own counter (848/848).

```text
make test-all : 867/867 passed, 0 failed, 0 skipped
```

New unit files: `test_dosemu`, `test_cicd`; new cases in `test_device_service`, `test_io_bitmap`, `test_vga_console` (VGA driver), `test_grpcwire` (multi-call, flow control), `test_osui_gui`, `test_webrt` (sanitizer), `test_collab` (escrow, disputes, auctions), `test_prod` (scanner, tuning, maintenance, bundle, metrics), `test_platform`. Inventory of the Phases 6-8 rows that were hard-coded: `docs/lot7_inventory.md`.

- US-001 (still open, narrower): the VGA text console is painted by the Ring 3 driver `bin/vgadriver` (service `vga-driver`, pinned to that binary, spawned at boot, priority HIGH). After `OS_DEV_OP_VGA_CLAIM` the kernel maps the text page 0xB8000 writable at `OS_VGA_DRIVER_WINDOW` in that task only (borrowed page) and opens the CRTC ports 0x3D4/0x3D5 in its TSS IOPB; the driver takes each frame (`OS_DEV_OP_VGA_TAKE`) and writes cells and cursor at CPL 3. The kernel keeps the console model (rows, scrollback, cursor) because the kernel itself prints. Ring 0 paints: before the claim (the whole early boot log), after a driver death until the supervised respawn claims (budget 4), on a 5-tick stall and after a kernel exception; post-handoff Ring 0 frames are counted (`devices vga owner .. ring3 .. ring0 .. stalls .. respawns ..`). Proof: `test_device_service`, `test_io_bitmap`, `test_vga_console`, `make qemu-input-drivers` step 8 (a line echoed by the shell is read back from guest video memory with QMP `pmemsave`, the driver is killed, Ring 0 paints, the respawned driver paints the next line; `devrogue` is refused the claim and the take). Decision on closure: US-001 is NOT closed. Remaining Ring 0: the UHCI USB tablet driver (`kernel/input/usb_tablet.c`, polled from IRQ0, DMA frame list: moving it needs DMA memory for a user task, not done), the NE2000 path of the legacy comparison image only. Kept in the kernel on purpose (irreducible core of this design): PIC (IRQ routing and EOI) and PIT (the tick that drives preemption and every driver watchdog), the IRQ entry stubs, the early boot console painting and the panic path.
- Phase 3: US-039 the page sanitizer runs first in the web pipeline (`osui_web_pipeline`: script, `on*`, `javascript:` URLs, `@import` and `expression()` removed, style kept and filtered); the status line prints `sanitized N`; `webpages/hostile.html` (9 removals) is rendered in QEMU without the injected content. CSP is parsed (`webrt_csp_*`) but the engine loads no external resource, so it gates nothing: US-039 stays partial. US-038 unchanged (origin `simule`). `phase3_complete` stays false. Proof: `test_osui_gui`, `test_webrt`, `make qemu-osui-gui`.
- Phase 5, US-067: HTTP/2 connection engine in `grpcwire.c` (`gw_conn_*`): several calls per connection on odd streams (up to 8 open, beyond: REFUSED_STREAM), connection and stream flow-control windows, WINDOW_UPDATE replenishment, SETTINGS INITIAL_WINDOW_SIZE applied to open streams, DATA held back and split by the window, PING, GOAWAY, RST_STREAM, FLOW and PROTOCOL errors. `grpc-serve PORT [CALLS] [SECONDS]` keeps one engine per connection; `grpc-calls LOCAL REMOTE PORT CALLS WINDOW SERVICE METHOD [PAYLOAD]` pipelines CALLS calls on one TCP connection with a small window. Limits: cleartext h2c, HPACK subset, no TLS. Proof: `test_grpcwire`, `make qemu-grpc-guests` (3 calls on one connection with a 16-byte window, 5 DATA frames per stream, served 5 failed 0 connections 3).
- Phase 6: US-086 real 8086 subset interpreter (`dosemu.c`) runs .COM programs from `/dos` (`legacy-emul FILE [MAX_STEPS]`), replacing the `ok_simulated` stub; not a full DOS or BIOS. US-078 `sensor-read` says `simule` (no sensor hardware). Proof: `test_dosemu`, `make qemu-platform`.
- Phase 7: escrow contracts (optional arbiter), disputes settled by the arbiter or a majority of uninvolved members, auctions (best bid locked, outbid refunded, low bid rejected) are ledger entries folded identically on every guest (US-094, US-099 partial: fixed templates, US-101). Privacy proof is a hash commitment (US-103 partial), compliance checks are computed (US-104 partial), point conversion labelled `simule` (US-097 partial). Proof: `test_collab`, `make qemu-collab` (new phase: 33 entries, same digest on 3 guests, balances 60/105/135).
- Phase 8: `prod-sec-scan`/`prod-sec-baseline` (ELF32 W^X, executable stack, entry, size+FNV baseline, no CVE database: US-112 partial), `prod-cicd` (real lint, build, test, deploy, verify of a PromptMessage program: US-108), `prod-perf-tune` (measured rates, mode from the busy average: US-106), `prod-maint-predict` (heuristic: alerts plus memory trend: US-115 partial), `prod-support-bundle` (plain-text bundle written, read back, FNV verified: US-116), `prod-biz-metrics` (observed counters: US-119). Proof: `test_prod`, `test_cicd`, `make qemu-production`.
- Still open in Phases 6-8 (not feasible offline here): US-076 ARM port, US-077 mobile UI, US-078 real sensors, US-083 device sync, US-097 real payment, US-120 live roadmap.

## Etat courant du 17 octobre 2026 (lot 6)

Living baseline. Branched from `ca04007` (merge of pull request #154), branch `cursor/lot6-geant`, pull request #155. The lot 5 section below keeps its own counter (830/830).

```text
make test-all : 848/848 passed, 0 failed, 0 skipped
```

New unit files: `test_key_seal` (5), `test_webeng` (7); new cases in `test_gfx_desktop` (web pane), `test_osui_gui` (web pane, page pipeline), `test_p2p` (shaping, per-peer anomaly, Raft cache), `test_grpcwire` (connection-level stream), `test_ps2_decode` (MODS events, mouse).

- US-001 (still open, narrower): the PS/2 mouse is proven in QEMU, not only by unit tests. `make qemu-input-drivers` sends QMP relative motion and a button: the packets go through `bin/ps2driver` (aux port), the kernel sums them (`devices` prints `mouse packets N dx X dy Y buttons B mods M`), motion is checked (+20, -10) and the button release. Shift desync on driver death fixed: the driver reports Shift/Caps in `OS_INPUT_EV_MODS` events, the kernel keeps them across a driver death (`keyboard_driver_lost` drops only a half read E0 sequence or mouse packet) and the respawned driver seeds its decoder from the kernel state. QEMU: Shift is held (QMP key down) while `kill` of the driver runs, the respawned driver gives `B`, after the release `b`. Remaining Ring 0: PIC, PIT, VGA text, early boot probes.
- Phase 3, US-031 delivered as a minimal web engine in guest C (`userspace/webeng.c`, no libc, no Chromium/WebKit, no JavaScript):
  - HTML tokenizer and tree builder (void elements, implied closes, entities, comments, `<script>` and `on*` ignored and counted), CSS parser (type, class, id, descendant and child selectors, specificity, inline `style`, `!important`), cascade with inheritance, block and inline layout (8 px characters, 10 px lines, margins, padding, borders, width/height, word wrap, `display:none`), display list of rectangles and text runs.
  - OS-UI: `web FILE` in the `gui` desktop paints the display list in the new pane `OS_FB_PANE_WEB` (scene version 4, `kernel/gfx_desktop.c`); arrows scroll, ESC closes. Text console: `web-render`, `web-dom`, `web-inspect FILE ID`, `web-console`, `web-perf FILE [N]`. Pages in initrd `/web/` (`demo.html`, `theme.html`).
  - Limits: fixed 8x10 bitmap font, no images (the `alt` text is drawn), no floats, no flex/grid, no margin collapse, no position, no JavaScript, local files only (no fetch in the engine).
  - US-043 delivered (minimal): `web-perf` times parse, style and layout over N runs (PIT ticks), prints ms per render and a 100 ms budget verdict, with node, rule, declaration, box and item counts.
  - US-045 delivered (minimal): `web-dom` (tree), `web-inspect` (computed style and box of an element by id), `web-console` (engine diagnostics: unknown properties, skipped scripts, dropped nodes, plus the accessibility audit).
  - US-040, US-041, US-042, US-044 now run inside the engine pipeline (they were standalone commands in lot 5): `<template>`/`<slot>` expanded, `@media` flattened for the 640x480 viewport, `var()` resolved, then parse, style, layout. The status line prints `components C media_kept K media_dropped D vars V`; `web-console` prints `a11y issues`. US-038 (cache, origin `simule`) and US-039 (CSP, sanitizer; the engine ignores scripts and `on*` but does not run the sanitizer) stay partial.
  - `phase3_complete` stays false (no network fetch in the engine, no script).
  Proof: `test_webeng`, `test_gfx_desktop`, `test_osui_gui`, `make qemu-osui-gui` (text commands plus a screendump pixel check of a 208x48 box, and the `theme.html` pipeline counters).
- Phase 4, US-059/US-060: the CA private key is no longer stored in clear. `SYS_KEY_SEAL` (159, `kernel/key_seal.c`) seals it with AES-128-GCM under a kernel key; the additional data binds the blob to the caller task name, so another task (`bin/devrogue`) is refused (`-191`). `pm-ca-init` writes `pmca2 sealed=HEX`, `pm-ca-sign` unseals in the kernel and refuses an old clear `pmca1 key=` record, `pm-ca-seal` prints the counters. Simule: the seal key is derived per boot from rdtsc/PIT entropy (no TPM, no persistent hardware key), so a sealed key does not survive a reboot (`pm-ca-init` again). The strict image now links `sha256.o` and `aes_gcm.o`. Still a local CA and a local catalogue (not a public authority, not a network marketplace): both stay partial. Proof: `test_key_seal`, `make qemu-promptmessage`.
- Phase 5:
  - US-067 (lot 6; lot 7 adds several calls per connection and flow control, see above): one gRPC unary call over a real TCP connection between two guests. `grpc-serve PORT [N] [SECONDS]` (guest B, plain TCP listening slots of its Ring 3 networker) and `grpc-call LOCAL_IP REMOTE_IP PORT SERVICE METHOD [PAYLOAD]` (guest A, `SYS_SOCKET_CONNECT`/`SEND`/`RECEIVE` relayed to its networker). The client writes the HTTP/2 preface, SETTINGS, HEADERS and DATA; the server answers SETTINGS, SETTINGS ACK, HEADERS, DATA and trailers (`grpcwire.c` connection helpers). The web table now ACKs client data at once (RFC 1122), so a request split in relay-sized segments (72 bytes) is not stuck. Limits: cleartext h2c with prior knowledge, one call per connection, stream 1 only, no flow control, no PING/GOAWAY, HPACK subset, no TLS. Proof: `test_grpcwire`, `make qemu-grpc-guests` (two QEMU guests on the shared hub, the hub never answers TCP; it counts the guest SYN/SYN-ACK, the preface and the HEADERS back; one OK call and one UNIMPLEMENTED call; CI p2p-mesh job).
  - US-068/US-073: messages over budget are queued (shaping queue of 8) instead of dropped, sent when the window allows; measured byte rates (EWMA) next to the window. `p2p-send` prints `queued ... (shaped)`, `p2p-burst PEER N`, `p2p-qos` adds `meas_tx_bps meas_rx_bps queued shaped_sent shaped_dropped`. QEMU scenario 1b: a burst of 6 under a limit of 2/s is shaped and all 6 arrive.
  - US-070: cache lines through the Raft log (`p2p-cache raft KEY VAL`): the line is applied on commit with version = log index and a fence, so all replicas agree. QEMU scenario 2b.
  - US-075: per-peer learned baseline, so an anomaly names its source; a reply burst the node asked for (sync, snapshot catch-up) is skipped and counted. QEMU scenario 1c: `p2p-flood` (a sender that ignores its own QoS controller, proof tool only) from gamma is attributed to gamma by beta. Still statistical (EWMA, thresholds), no machine learning.
  - These three stay partial in the US list sense (lab network of 3 to 4 guests, no real WAN), but each capability above has a unit test and a QEMU proof.

## Etat courant du 17 octobre 2026 (lot 5)

Living baseline. Branched from `c4fb0c8` (merge of pull request #153), branch `cursor/lot5-geant`, pull request #154. The lot 4 section below keeps its own counter (802/802).

```text
make test-all : 830/830 passed, 0 failed, 0 skipped
```

New unit files: `test_device_service` (6), `test_ps2_decode` (5), `test_webrt` (8), `test_grpcwire` (4); new cases in `test_io_bitmap` (PS/2 and COM1 ports), `test_promptmessage` (C99 target built by the host compiler, assistant, revocation list) and `test_osui_runtime` (web commands).

- CI rebalance: four contracts moved from the vfs-service job to the p2p-mesh job (the vfs-service job was at 13 min).
- US-001 stays open, narrower. The PS/2 keyboard/mouse controller and COM1 are driven by supervised Ring 3 drivers: `bin/ps2driver` (service `ps2-driver`) and `bin/serialdriver` (service `serial-driver`), pinned to their binaries, spawned at boot with priority HIGH, and given only ports 0x60/0x64 and 0x3F8-0x3FF in their TSS I/O bitmap after `SYS_DEVICE_SERVICE` claims. IRQ1 only wakes the PS/2 driver, which reads the controller and submits the bytes; the kernel keeps the keyboard decoder and the input queue. Kernel serial output goes into an 8 KiB queue that the serial driver writes; COM1 receive is polled by the driver. A rogue task (`bin/devrogue`) is refused the claim (-180) and faults on the ports. The supervisor respawns a dead driver (budget 4). Honest Ring 0 fallback: before the drivers claim (early boot), after a driver death until the respawn claims, when a driver stalls (PS/2 IRQ pending 5 ticks, serial queue idle 5 ticks), when the queue is full, and on CPU faults (panic mode writes COM1 directly). A chunk the serial driver took is never bypassed by a Ring 0 flush (order kept; the stall is counted). The scheduler resumes the interrupted task after a HIGH driver turn while its quantum lasts, and after a quantum expiry rotates from the last lower priority task (a `spin` task no longer starves the shell). `devices` prints owners, IRQs, events, Ring 0 bytes, stalls, overflows and respawns. Mouse packets: decoder proven by unit tests only. Known limit: if the PS/2 driver dies between a shift press and its release, the shift state can desynchronise until the next key. Proof: `make qemu-input-drivers` (CI production job), `test_device_service`, `test_ps2_decode`, `test_io_bitmap`, plus `qemu-irq0-preemption` and `qemu-foundation-steps`.
- Phase 3, partial, not a web engine (no JavaScript, no layout engine, no Chromium/WebKit; US-031 not delivered, `phase3_complete` stays false). `userspace/webrt.c` (guest C, no libc) with OS-UI runtime commands:
  - US-038 partial: HTTP cache (Cache-Control max-age, no-store, no-cache, ETag, revalidation by 304, LRU eviction). `web-cache` runs it against a scripted origin (`origin=simule`), not the network.
  - US-039 partial: Content-Security-Policy parser and check (default-src fallback, 'self', 'none', *, scheme, host, *.host, 'unsafe-inline') and an HTML sanitizer (script, iframe, object, embed and style elements, on* attributes, javascript: URLs). `web-csp`, `web-sanitize`.
  - US-040 partial: media queries (min/max width/height, orientation, prefers-color-scheme, and/comma) and flattening of `@media` blocks for a viewport. `web-responsive [w h]`.
  - US-041 partial: CSS custom properties (`--x`, `var(--x, fallback)`; an unknown variable without fallback becomes empty and is counted as unresolved) with dark/light themes. `web-theme [dark|light]`.
  - US-042 partial: `<template id=x-name>` components with `<slot>` expanded in the page. `web-components`. Not Custom Elements with script.
  - US-044 partial: accessibility audit (lang, img alt, heading order, link and button text, form labels). `web-a11y`.
  - Not done: US-031, US-043, US-045. US-032 to US-037 unchanged from the earlier lots.
  Proof: `test_webrt`, `test_osui_runtime`, `make qemu-osui-runtime`.
- Phase 4:
  - US-055 partial: `pm-cross FILE` translates the PMC1 bytecode to one portable C99 program (same output, step budget, path rule; `./app PHRASE` runs the program then the trigger). The guest prints the C between markers (the file store caps a file at 1 KB); the host compiler builds it. Proof: `test_cross_compile_c99_matches_vm` builds the C with `cc` and compares with the VM output, and `make qemu-promptmessage` builds the C printed by the guest and runs it. One target only (C99), no native guest code generation.
  - US-058 partial: `pm-assist FILE` is a rule based assistant (nearest keyword by edit distance, missing to/then/times, variables set but never read, triggers without effect). No language model (`engine=rules`).
  - US-059 partial: `pm-catalog-fetch`/`pm-install` refuse a revoked source and verify a local `NAME.ca` record when present (`ca=verified`, else `ca=unsigned`). Still local, no network marketplace.
  - US-060 partial: `pm-ca-revoke FILE` adds the source digest to `/etc/pm_ca.crl`; `pm-ca-verify` and the fetch refuse it. Local CA only, private key in plaintext, not a public authority.
  Proof: `test_promptmessage`, `make qemu-promptmessage`.
- Phase 5:
  - US-067 partial: `userspace/grpcwire.c` encodes one unary gRPC call as HTTP/2 frames (HEADERS, DATA, trailers on stream 1, client preface and SETTINGS), HPACK subset (static table, literals without indexing, no Huffman, no dynamic table), gRPC Length-Prefixed-Message and protobuf fields. `grpc-wire SERVICE METHOD [PAYLOAD]` serves and parses it in guest memory (`transport=memory`). Not over a TCP HTTP/2 connection; the P2P `p2p-grpc` path keeps its typed RPC datagrams. Proof: `test_grpcwire` (protobuf and RFC 7541 C.2 vectors, round trip); the frames were also decoded once by a reference HPACK decoder (Python `hpack`, outside CI); `make qemu-promptmessage` runs `grpc-wire`.
  - US-068/US-073, US-070, US-075: unchanged in this lot (see lot 3 and lot 4). Not closed.

## Etat courant du 17 octobre 2026 (lot 4)

Living baseline. Branched from `0254a81` (merge of pull request #152), branch `cursor/lot4-geant`, pull request #153. The lot 3 section below keeps its own counter (783/783).

```text
make test-all : 802/802 passed, 0 failed, 0 skipped
```

New cases: 2 `test_net_tcp` (segment classification, sequence wrap), 1 `test_ne2k` (TLS loss recovery), 4 `test_pci_service`, 1 `test_io_bitmap` (PCI ports), 3 `test_p2p` (crash/reroute/rejoin, STUN behind a simulated NAT), 4 `test_stun`, 4 `test_osui_ide`. Guest command register: 370 names (`lspci`, `pci-rescan`).

- Guest TCP/TLS client loss recovery (root cause fix, no test relaxed). `net_tcp_connection_classify_segment` sorts every received segment (in order, duplicate, future, overlap, foreign port; modular sequence arithmetic, wrap tested). The NE2000 socket path (`kernel/ne2k.c`, also linked by the Ring 3 networker) drops duplicates and out-of-order segments with a duplicate ACK and drains them in the same poll, trims overlaps, and retransmits the unacked flight (ClientHello, then the client Finished flight) with a poll-count backoff 2/4/8/16. The test peer (`tests/scripts/qemu_ne2k_controlled_peer.py`) now behaves like a TCP sender: RTO 1 s, up to 3 resends, re-ACK and resend on a guest duplicate. Proof: `make qemu-net-loss-recovery` (CI): the hub drops the first ClientHello and the first Certificate and duplicates ServerKeyExchange; the guest sends 2 ClientHello frames, the peer RTO resends the Certificate, the duplicate is ignored, TLS and HTTP complete. Limits: poll-count timers, not a wall-clock RTO; no SACK, no congestion window on the guest client.
- US-001 stays open, narrower. PCI enumeration is a Ring 3 worker: `bin/pcidriver` (registered service `pci-driver`, pinned to that binary) is spawned at boot, gets only ports 0xCF8-0xCFF in its TSS I/O bitmap, enumerates buses 0-7 and publishes the inventory (`SYS_PCI_SERVICE` 157). After the handoff Ring 0 config reads are refused and counted (`ring0denied`), and the kernel reads BARs (VGA framebuffer) from the published inventory. Supervision is lazy: a dead publisher is detected on the next request and respawned (budget 4). `lspci` prints the inventory and counters, `pci-rescan` asks the driver to enumerate again (IPC). A rogue binary (`pcirogue`) is refused. Proof: `test_pci_service`, `test_io_bitmap`, `make qemu-pci-driver` (CI: 6 functions, kill and respawn to generation 2, rescan to generation 3, rogue refused). Still Ring 0: the early boot probes that run before the driver publishes (USB tablet, NE2000 detection on the legacy image), PIC, PIT, PS/2, VGA text, serial.
- US-063 partial, closer: proven in QEMU with two relays. `make qemu-p2p-mesh` (CI job `p2p-mesh`) runs 4 guests on the hub; wire cuts make a line alpha-beta-gamma-delta and alpha reaches delta through beta and gamma (`relayed_multi` on beta, `relayed` on gamma). Still at most 3 hops and a destination must have been keyed once.
- US-069 partial, closer: node crash and rejoin in QEMU. Same contract: diamond topology, beta's QEMU process is killed, alpha and delta detect it, alpha reaches delta through gamma; beta reboots with an empty state and a new identity under the same name, the peers reuse the stale slot (`p2p peer beta rejoined id ...`, counter `rejoins`) and push the replicated store back (beta receives the key put during its crash). Not a complete fault-tolerance system (no quorum change on crash, identity is not persisted without a disk).
- US-074 partial (was simule). `userspace/stun.c` is a real RFC 5389 Binding codec (XOR-MAPPED-ADDRESS, transaction id checked per server, RFC 5769 test vector) carried inside the sealed P2P link; the server answers with the IPv4 source of the received frame; two servers classify the mapping as `direct`, `endpoint_independent` or `endpoint_dependent`. QEMU (scenario 0 of `qemu-p2p-mesh`): delta asks gamma and beta, `nat_type direct`. The NAT itself is simule (address rewrite on the unit-test bus, `test_stun_behind_simulated_nat`): there is no real NAT on the QEMU hub, no public STUN server, and `p2p-punch` is still a simulated pinhole. The UDP source port is not visible through the datagram API, so the mapped port is `P2P_PORT`.
- US-050 partial, closer. The full-screen `pm-ide` now colours PromptMessage syntax (keywords, strings, numbers, comments, gutter; scene version 3 carries per character classes, `kernel/gfx_desktop.c` draws them), completes the word before the cursor with Tab (keywords plus words of the buffer, strings and comments skipped, common prefix then the candidate list in the status line), and shows the program output of `ESC r` in an output pane (last 16 lines kept, 4 on screen, `ESC o` toggles). Proof: `test_osui_ide`, `test_gui_pm_ide_fullscreen`, `make qemu-osui-gui` (Tab completes `sa` to `say`, the run prints `output 1`, snapshot `ide_out=1 ide_pane=1`). Limits: no debugger, no semantic completion (no types), no undo.
- CI: `qemu-osui-web` second HTTPS client now waits up to 240 s for its SYN-ACK: the networker computes a server handshake in one step, so a second SYN can wait for the whole first handshake (59-110 s measured on CI runners). The spawn smoke counts the boot `pcidriver` task.

## Etat courant du 17 octobre 2026 (lot 3)

Living baseline. Branched from `7cfcf0b` (merge of pull request #151), branch `cursor/lot3-geant`, pull request #152. The lot 2 section below keeps its own counter (770/770).

```text
make test-all : 783/783 passed, 0 failed, 0 skipped
```

Detail: 54/54 kernel, 10/10 userspace, 1/1 performance, 1/1 robustness. New cases: 2 CSS, 4 P2P, 5 `test_osui_ide`, 1 `test_gui_pm_ide_fullscreen`. `tests/scripts/run_all_tests.sh` now counts a unit test that does not compile as a failure (it was counted as skipped, so a broken test left `make test-all` green). Guest command register: 368 names (`p2p-qos` added).

Coverage: `make coverage COVERAGE_MIN=78` runs in CI (job `coverage`, timeout 15 min) and fails below the floor. Measured this lot: 79.82% lines executed (25224/31603) across 145 source files. Host unit-test translation units only, not the guest image.

- US-001 stays open, narrower. The mount journal (LBA 4222-4223) and the event journal (LBA 4224) are now read by the Ring 3 `atadriver` and handed over with `OS_ATA_FS_INSTALL` kinds 4 and 5 (once per boot; the kernel does not flush a journal before it is handed over). `ata-status` prints `jkread` (journal sectors read by Ring 0 PIO) and `jinst`. Without an IDE disk the kernel does no journal port access. The boot overlay snapshot was already installed by Ring 3. The legacy image now links `boot_storage_strict.o`: the boot FAT mount, the overlay and the journals come from Ring 3 on both images, and `kernel/boot_storage_legacy.c` is gone. The legacy image keeps the Ring 0 NIC path (`ne2k_probe`, `net_tls_record_`) on purpose: it is the comparison kernel for `NET_RING0_FALLBACK`, not a shipped image. Still Ring 0 on both images: PIC, PIT, PS/2, VGA, serial, PCI, `usb_tablet`. Proof: `make strict-image-check` (rejects `kernel_ring0_boot_fat_mount` in both images), `make qemu-ata-driver` (`jkread 0 jinst 2`), the same contract with `MOHHDY_ATA_KERNEL=build/mohhdy-netlegacy.bin` (by hand, passed), `make qemu-foundation-steps` (both journals `(restored)` by Ring 3 after reboot, `alias/` and the unacked event survive).
- US-063 partial, closer. HELLO carries a distance vector (peer id plus hop count, at most 3 hops, also the frame TTL); frames are relayed hop by hop. Proof: `test_multi_relay_chain` (4 nodes in a line, alpha to delta through beta and gamma). QEMU still runs 3 guests, so QEMU proves one relay only. A destination must have been discovered and keyed once.
- US-068 and US-073 partial, closer. Adaptive controller: EWMA smoothed RTT, minimum RTT, window cut by a quarter when RTT is above twice the minimum, halved on loss (one loss per outage), plus one per success. `p2p-qos` prints it. Proof: `test_qos_adaptive_controller` and QEMU scenario 3 (`window < 16`, `ping_losses >= 1` after the gamma cut). Not a bandwidth estimator.
- US-069 unchanged (partial): hub wire cut, reroute, resync.
- US-070 partial, closer. Versioned entries, fences against stale writes (`p2p-cache stale refused`), invalidation acked per peer, re-sent every second up to 5 times, `incomplete` when acks never arrive. Proof: `test_cache_versioned_acked_invalidation` and QEMU scenario 4 (gamma's INV and ACK relayed through beta). Not a consensus-backed cache.
- US-075 partial, closer. Learned per-type receive baseline (EWMA, 5 windows of warm-up, anomaly above 4x baseline plus 8 frames/s). This is an EWMA baseline, not machine learning. A rate anomaly ranks as `degraded`, not `suspicious`: QEMU showed a solicited raft snapshot burst tripping it. Proof: `test_traffic_baseline_anomaly`, QEMU scenario 3 `p2p-analyze`.
- US-050 partial, closer. `pm-ide FILE` typed on the OS-UI desktop (`gui`) opens a full-screen editor (`userspace/osui_ide.c`, pane `OS_FB_PANE_IDE` drawn by `kernel/gfx_desktop.c`): 96 lines x 72 columns, arrows, Enter, Backspace, 12 visible rows with scrolling, ESC then `w` save, `c` check, `r` run, `q` quit (refused if unsaved), `Q` discard. Save, check and run use the shell store and the PromptMessage VM. Proof: `test_osui_ide`, `test_gui_pm_ide_fullscreen`, `make qemu-osui-gui` (types a program, saves, checks, runs, quits, then `pm-check` reads the file). Limits: no syntax colouring, no completion, no debugger, the program output is not shown in the editor (only the verdict line), the cursor is shown as `_` only on a space or past the end (Ln/Col in the title). `pm-ide` typed in the text console is still the line editor.
- US-059 and US-060 unchanged (partial).
- OS-UI CSS (Phase 3): `browser-html-render` now parses CSS rules (type, `.class`, `#id`, `*`, compounds, descendant combinator up to 3 compounds, comma groups, inline style, `!important`), matches them on the DOM, applies the cascade by specificity then source order, inherits color, font-weight and text-transform, and honours `display:none`, over a user-agent sheet. `browser-css-demo` prints `author_rules=7 total_rules=19 errors=0 hidden=1`. Proof: `test_css_cascade_engine`, `test_css_parser_rules_and_errors`, `make qemu-osui-runtime`. Still a text renderer: no layout engine, no box model drawing.

## Etat courant du 15 octobre 2026 (lot 2)

Previous baseline for lot 2, branched from `708917b` (merge of pull request #149). Superseded by the lot 3 section above. The lot 1 section below keeps its own counter (768/768). US-001 stays open. US-064 is closed for the guest Raft log described here. It is not Paxos.

```text
make test-all : 770/770 passed, 0 failed, 0 skipped
```

Detail: 54/54 kernel, 9/9 userspace, 1/1 performance, 1/1 robustness. The runner sums Unity `Tests Run` lines (770; was 768 before the snapshot case and the QoS case). Journal: `test_logs/test_results_20261015_112957.log`. Guest command register: 367 names (unchanged).

Coverage, measured by `make coverage` (not part of `make test-all` or CI): 79.55% lines executed (24434/30716) across 143 source files, 572 `.gcda`. Flags: `--coverage` (`-fprofile-arcs -ftest-coverage`) on the host unit-test binaries that `tests/scripts/run_all_tests.sh` compiles and runs. Scope: those translation units. Not the guest image. Report: `test_logs/coverage_report.txt`.

Measured in this lot:

- US-001 stays open. Both images link `kernel/ata_identify_absent.c` and `kernel/fat_pio_fallback_absent.c`. `kernel_ring0_ata_identify` and `kernel_ring0_fat_pio_fallback` are absent from `build/mohhdy.bin` and from `build/mohhdy-netlegacy.bin`. The legacy image still links the Ring 0 NIC (`ne2k_probe`, `net_tls_record_`) and `kernel_ring0_boot_fat_mount`. That boot FAT mount is the network-legacy contract. The Ring 3 `atadriver` sends IDENTIFY. After it dies, the kernel resets the channel when BSY or DRQ is set. An idle channel is not reset, so a clean kill does not bump `resets`. The kernel parks the store and respawns `bin/atadriver` (budget 4). The budget returns to 0 when that pid publishes the store (`[ATA] supervised respawn budget restored`). The blocked FAT operation is retried in Ring 3. `fatkpio` stays 0. A sector the driver already committed is not replayed (`OS_ATA_FS_ABORTED`). Steady-state journal sector I/O (`kernel_journal_sector_io`) uses the driver RPC when the driver is live and the caller can block. A boot journal read, before any driver exists, stays Ring 0. If the driver is live but the caller cannot block, the write returns -1 and stays in RAM. Overlay snapshot load at boot stays Ring 0 because no task can run yet. Still Ring 0 on both images: that boot journal read, that boot overlay load, PIC, PIT, PS/2, VGA, serial, PCI, `usb_tablet`, and the legacy boot FAT mount. Proof this run: `make strict-image-check` and `make qemu-ata-driver` in 269 s (boot `fatkpio` 0, kill path, mid-job #GP with `aborts` 0 to 1, `resets` 0 to 1, `fatkpio` 0 to 0).
- US-064 is closed for this guest Raft log. Not Paxos. `userspace/p2p.c`: pre-vote, membership quorum, physical window 16, compaction (`p2p-raft-compact`), InstallSnapshot (`P2P_I_RAFT_SNAPSHOT`) for a follower whose match is behind `snapshot_index`, and a leader lease of 10 s (`P2P_RAFT_LEASE_TICKS`). A heartbeat that reaches a peer renews the leader lease. A follower that still holds the lease does not start an election (`p2p raft lease held`). Term and vote stay in the p2pkv trailer. Proof: `test_raft_snapshot_installs_for_a_lagging_follower`, `test_raft_prevote_does_not_bump_a_healthy_term`, and `make qemu-p2p` in 183 s. Scenario 2 cuts the hub wire (`cut_pair`, not `p2p-block`, not `set_link`) and syncs after heal. Scenario 2b compacts index 2 while gamma is cut, then gamma installs that snapshot and commits index 3 after the wire is healed.
- US-068 and US-073 stay partial. `admit()` caps data when a rate limit is set or the window is below 16. The window starts at 16, halves on a rejected raft append, and grows by one on success. Control frames are not capped. Proof: `test_qos_window_and_cache_invalidate`. This is not a full adaptive controller.
- US-069 stays partial. Scenario 2 is a real hub wire cut. Scenario 3 still detects a blocked link and relays. Not a complete fault-tolerance system.
- US-070 stays partial. `p2p_cache_invalidate` clears the local entry and sends `P2P_I_CACHE_INV`. Remote get was already in QEMU. Not a coherent distributed cache. Proof: the same unit test.
- US-075 stays partial. `p2p-analyze` adds a fixed rule `congestion` when `qos.loss_count` is greater than 0. No learning. QEMU scenario 3 still names the dead link.
- US-067 stays typed RPC, not HTTP/2. US-074 stays simulated STUN/NAT. US-063 stays one relay.

## Etat courant du 15 octobre 2026 (lot 1)

Previous baseline for lot 1, branched from `e932c904` (merge of pull request #148). Superseded by the lot 2 section above. The 14 October section below keeps its own counter. US-001 and US-064 were still open in this section.

```text
make test-all : 768/768 passed, 0 failed, 0 skipped
```

Detail: 54/54 kernel, 9/9 userspace, 1/1 performance, 1/1 robustness. The runner sums Unity `Tests Run` lines (768, was 766 before the two Raft cases). Coverage: not measured. `tests/Makefile` CFLAGS do not pass `-fprofile-arcs` or `-ftest-coverage`. Journal: `test_logs/test_results_20261015_084902.log`. Guest command register: 367 names (`p2p-raft-compact` added). `make strict-image-check` rejects `ne2k_probe`, `kernel_ring0_boot_fat_mount`, `kernel_ring0_ata_identify` and `kernel_ring0_fat_pio_fallback` in `build/mohhdy.bin` and requires those symbols in `build/mohhdy-netlegacy.bin`.

Measured in this lot:

- US-001 stays open. On the strict image, ATA IDENTIFY (command 0xEC) and the fallback FAT sector PIO are not linked (`kernel/ata_identify_absent.c`, `kernel/fat_pio_fallback_absent.c`). The Ring 3 `atadriver` sends IDENTIFY and prints `atadriver identify ok`. If that driver dies, the strict kernel resets the channel when a transfer was open, parks the store in RAM, and respawns `bin/atadriver` (budget 4). The blocked FAT operation is retried in Ring 3. `fatkpio` stays 0. A sector the driver already committed is not replayed (`OS_ATA_FS_ABORTED`). A guest with no IDE disk does not spawn that driver: overlay mkdir stays in the kernel RAM store. Proof this run: `make qemu-ata-driver` in 269 s (boot `fatkpio` 0 at store ready, kill path, mid-job #GP with `aborts` 0 to 1, `resets` 0 to 1, `fatkpio` 0 to 0, payload on disk after reboot). The legacy image still links Ring 0 IDENTIFY and the Ring 0 FAT redo. Still Ring 0 on both images: journal and overlay persist PIO, PIC, PIT, PS/2, VGA, serial, PCI, `usb_tablet`.
- US-064 stays open. Raft in `userspace/p2p.c`: pre-vote (a refused pre-vote does not bump the term), membership quorum, physical window 16 (`P2P_RAFT_LOG`), compaction of the applied prefix (`p2p-raft-compact`), logical index 17 in `test_raft_compact_past_physical_window`. No InstallSnapshot and no leader lease. Partition proof: `make qemu-p2p` scenario 2b cuts the hub wire between gamma and the other two guests (not `p2p-block`, not `set_link`, no public Internet), then compacts and commits index 3. Scenario 2 still uses `p2p-block` for replication. This run: `make qemu-p2p` PASS in 188 s. Not Paxos.
- Phase 5 was not hardened further. US-067 stays typed RPC, not HTTP/2 gRPC. US-074 stays simulated STUN/NAT. US-063 stays one relay. US-069, US-070 and US-075 keep the QEMU proofs they already had (`make qemu-p2p` scenarios 2 and 3, including `p2p-analyze`). US-068 and US-073 stay unit-level budgets, not an adaptive controller. Those stories are not closed.

## Etat courant du 14 octobre 2026

Previous baseline (14 October 2026), kept for its counter. Superseded by the 15 October section above. It described `main` at `7ae6bb1` (merge of pull request #146) plus pull request #147. Historical paragraphs below keep their own counters. Lot 1 was not delivered then.

```text
make test-all : 766/766 passed, 0 failed, 0 skipped
```

Detail: 54/54 kernel, 9/9 userspace, 1/1 performance, 1/1 robustness. The runner sums Unity `Tests Run` lines. Coverage is not measured: `tests/Makefile` CFLAGS do not pass `-fprofile-arcs` or `-ftest-coverage`. `ENABLE_COVERAGE=1` still runs `gcov`, which finds no `.gcda` and reports no percentage. Journal: `test_logs/test_results_20261014_165218.log`. Guest command register: 366 names. `make strict-image-check` rejects `ne2k_probe` and `kernel_ring0_boot_fat_mount` in `build/mohhdy.bin` and requires both in `build/mohhdy-netlegacy.bin`. It does not reject ATA IDENTIFY: that command still runs inside `ata_init` in `kernel/ata.c`, and there is no `ata_identify` symbol. `make tree-clean-check` fails if git status shows a path outside `build/`.

Pull requests #140 to #146, merged, titles only:

- #140 `d3637db` 2026-10-12T15:57:59Z Consolidate systemic performance, P2P mesh, and endurance.
- #141 `2d10afd` 2026-10-12T22:21:29Z Harden authentication crypto, cross-origin security, and VBE layout.
- #142 `0141084` 2026-10-13T00:34:17Z Implement AI Engine, Web Runtime, P2P Mesh, and Production Engine Batch.
- #143 `8ffc341` 2026-10-13T02:41:20Z Harden microkernel VFS delegation, OSUI GUI, P2P, and HAL power policy.
- #144 `ec621da` 2026-10-13T04:45:19Z Complete feature lot for Mohhdy OS.
- #145 `12ac33e` 2026-10-13T07:22:28Z Consolidate P2P, Platform HAL, Collab Economy and Production modules.
- #146 `7ae6bb1` 2026-10-14T07:00:46Z Guest Raft log, typed RPC, and Ring 3 boot mount.

Lot 1 is not delivered. Pull request #147 does not finish US-001, US-064, or Phase 5:

- US-001: the strict image does not link `ne2k_probe` or `kernel_ring0_boot_fat_mount`. The Ring 3 `atadriver` reads the FAT boot sector and the overlay window and installs them (`OS_ATA_FS_INSTALL`). The first `OS_AI_ENGINE_GGUF_OPEN` loads `GPT2.GGU` through the live driver. Still Ring 0: ATA IDENTIFY (`ata_init` sends `ATA_CMD_IDENTIFY` 0xEC during boot) and fallback FAT sector PIO after the driver dies. Also still Ring 0: PIC, PIT, PS/2, VGA, serial, PCI, `usb_tablet`. If `bin/atadriver` is missing, the strict image does not mount FAT at boot. There is no supervised respawn of `bin/atadriver`.
- US-064: Raft log in `userspace/p2p.c` (append entries, commit index, follower catch-up after a dropped-frame partition, term and vote stored in the p2pkv blob trailer). Not Paxos. Log cap 8 (`P2P_RAFT_LOG`). No pre-vote, no leader lease, no compaction, no `p2p-raft-compact`. Election timeout 30 s so a short partition does not start a rival term. Proof: `test_raft_log_catchup_and_typed_rpc` and `make qemu-p2p` scenario 2b (`p2p-block` frame drop, not a NIC cut, not `set_link`).
- US-067: typed RPC on the sealed channel (request id, type, status, deadline). Status 0 ok, 3 unimplemented, 4 deadline exceeded, 5 not found. Not HTTP/2 gRPC. Same response text as before, plus `id N`.
- US-074: still simulated STUN/NAT (`p2p-stun`, `p2p-punch`). No real NAT.
- Still partial: US-063 routing (one relay chosen through neighbors, QEMU), US-069 fault tolerance (detection, reroute, resync, QEMU; key/value store, identity, term and vote persisted), US-070 distributed cache (remote read, QEMU), US-068 bandwidth and US-073 QoS (per-peer budget, priority control; unit tests only, not adaptive), US-075 traffic analysis (per-type counters plus `p2p-analyze` with fixed rules and a verdict, QEMU; no learning).

What pull request #147 delivers for Phase 4, in `userspace/shell.c` (Ring 3). US-050, US-059 and US-060 stay partial:

- US-050: `pm-ide` is an alias of the line editor `pm-edit`. Not a full-screen IDE, and not in `userspace/osui_runtime.c`. Both commands accept lines, `.l` prints line numbers, `.c` compiles the buffer, `.r` runs it on the bounded VM, `.` saves and checks, `.q` quits without saving. Success lines use the command's own tag (`pm-ide check ok` or `pm-edit check ok`). Proof: `make qemu-promptmessage`.
- US-059: `pm-catalog-search QUERY` lists local `/pm/*.pm` whose file name or `##` line contains QUERY. The printed lines still start with `pm-catalog`. `pm-catalog-fetch NAME` reads `/pm/NAME`, requires it to compile, and writes a copy in the working directory (same store as `pm-install`). No network marketplace, no remote package, no publishing.
- US-060: `pm-ca-init` runs with no filename, creates `/etc` if needed, and writes a local Schnorr key pair (same RFC 5114 1024/160 group as the collab ledger) to `/etc/pm_ca.pub` and `/etc/pm_ca.key`. The private key is plaintext. Short or non-hex records are rejected. `pm-ca-sign FILE` writes `FILE.ca` over the source digest, and only after the file compiles. `pm-ca-verify FILE` checks that signature against `/etc/pm_ca.pub` and rejects a tampered source. This is a local trust record. It is not a public certificate authority and it has no revocation. `pm-certify` / `pm-verify` (FNV-1a record, and `.sig` when a node key exists) are unchanged.

Still not delivered in Phase 4: US-055 cross compiler, US-058 AI assistance. US-046 to US-049, US-051 to US-054, US-056 and US-057 stay as the 10 October 2026 lot below.

What pull request #147 adds in Phase 3. This is not a web engine and it does not close US-031. `phase3_complete` stays false. Chromium and WebKit are not present.

- `browser-html-render` (alias `browser-render`) in `userspace/osui_runtime.c` is a text-mode printer. With HTML arguments it builds a flat list of at most 32 tags (name, id, class, text). Closing tags are skipped. There are no parent links. `osui_css_apply_styles` is not a selector engine: the CSS text is ignored. Hardcoded rules set h1 and b to font-weight 700, links to blue, buttons to a gray background and padding 4, and any node whose id is `app` to margin 8 and padding 8 when the CSS pointer is non-NULL. The printer then writes headings, list items, links, buttons, inputs, and other text, plus `margin=N` when that rule applied. With no HTML arguments it prints the banner only. Proof: `test_html_css_rendering`.
- `bigint_modexp` in `kernel/bigint.c` skips leading zero bits of the highest exponent limb and skips the first square (the accumulator starts at 1). It is the shared modular exponentiation primitive. It is not, by itself, a change to the RSA or TLS handshake. Proof: `test_bigint_modexp_leading_zeros` against `pow(base, exp, mod)` vectors, including a zero exponent, a single set bit, a high limb, and a zero high limb.
- `net_wire_bind` / `net_wire_demux` in `kernel/net_wire.c` accept one listen binding (remote `0.0.0.0` and remote port 0 together), refuse a half wildcard, refuse a second listen on the same local address and port, and prefer a specific 4-tuple over that listen. Proof: `test_demux_listen_beside_specific`. This is the worker wire table. The OS-UI `web-serve` pump in `kernel/net_llm_client.c` is a separate table and still drops frames whose destination port is not its own listen port.

## Lot GPT-2 local du 15 octobre 2026

Base: `main` at `1b55273` (merge of pull request #147).

- Regression found on main after #146: the boot `aiworker` was spawned only when a FP32 checkpoint or a resident GGUF existed. Since the FAT is mounted by the Ring 3 `atadriver`, no GGUF is resident at that point, so a GGUF-only initrd answered -148 to every 109/110 step. `kernel/kernel.c` now also spawns the worker when the tokenizer and the boot `atadriver` are present; the worker reads `GPT2.GGU` itself and exits cleanly if no model is usable. Proof: `make qemu-ai-gguf-only` (synthetic tokenizer only, synthetic GGUF on disk), also run by `make qemu-ai-gguf` in CI. The contract fails without the fix (`missing output: [AI] boot aiworker spawned`).
- `make qemu-gguf-smoke` waited for the Ring 0 boot line `GGUF: profil local FAT16 pret`, which strict boot no longer prints. It now waits for `aiworker gguf ready bytes` and `[AI] ai-engine GGUF ready in Ring 3`, and still requires a non-empty real completion and `ai-continue`. Same marker change in `test_gpt2_sentences.py` and the two GGUF benchmarks (boot timeout 300 s).
- The release `gpt2-124m-assets` cited by the README did not exist. It now holds `gpt2_124M.bin`, `gpt2_tokenizer.bin`, `gpt2-Q3_K_M.gguf` and `gpt2-124m-assets.sha256`.
- `make run-gpt2` (`scripts/run_gpt2.sh`): fetches missing assets, checks SHA-256, builds the image and the GGUF FAT16 disk, launches QEMU GTK with KVM when `/dev/kvm` is usable, else TCG. Real GGUF Q3_K_M under TCG on the dev box: about 71 s per sentence, 67 s for `ai-continue`. Not run in CI (no real weights in CI).

## Baseline vivante du 6 octobre 2026

La branche `main` a été réalignée sur `origin/main` au commit `dcc248a` (`Merge pull request #106 from kamgueblondin/cursor/gpt2-phrases-lisibles-1988`). La baseline a été exécutée depuis un arbre Git propre :

```text
make all       : succès
make test-all  : 741/741 réussis, 0 échec, 0 ignoré
```

Détail de `make test-all` : **53/53 tests kernel**, **4/4 tests userspace** et **1/1 test robustness**. Le runner indique que la couverture n'est pas mesurée (aucune instrumentation gcov/lcov). Les répertoires `unit/fs`, `integration`, `system` et `performance` ne contiennent pas de tests pour ce lanceur. Le journal reproductible est `test_logs/test_results_20261006_180024.log`.

Ce résultat remplace le compteur historique `675/675` pour cette baseline ; les anciens compteurs restent dans les paragraphes de livraison afin de préserver la traçabilité. `make qemu-smoke` n'a pas été relancé dans cette baseline : les 678 tests sont une validation de compilation et de tests locaux, pas une nouvelle preuve QEMU.

## Constat d'écran du 11 octobre 2026

Guest lancé sans poids GPT-2 dans l'initrd. La bannière et les messages du shell impriment `MOHHDY v6.0`. Les messages de `make` impriment `MOHHDY v7`.

Le prompt du shell texte est le répertoire courant suivi de ` : `. La commande `gui` se tape à ce prompt. `MOHHDY>` est le prompt du panneau Shell une fois le bureau ouvert. `echo` imprime ses arguments, puis la ligne `echo ok`. `write note.txt bonjour` a affiché `atadriver snapshot flush ok` et `write ok note.txt` ; `cat note.txt` a relu `bonjour`.

Dans le bureau, le bouton `action` envoie `simule action` et passe la scène en `acting` (pastilles 1, 2 et 3). Le bouton `resultats` envoie `resultats` et laisse le mode `reflecting`. Le clic recopie la commande dans le champ du chat. Sans poids, le bureau affiche `llm=stub_echo` et `us031_complete=false`. `console` et Echap quittent le bureau lorsque la saisie arrive au bureau, pas lorsqu'elle reste dans un panneau.

## Lot IA 1 valide le 7 octobre 2026

Le shell du guest expose maintenant `ai-runtime json`. Cette commande publie le contrat machine du moteur local : profil FP32 ou GGUF, fournisseur, modele, execution dans le guest QEMU, contexte de 64 jetons, maximum de 24 nouveaux jetons, sortie de complétion et absence de garantie factuelle, ainsi que l'etat reseau. La commande humaine `ai-runtime` affiche les memes limites en clair et refuse les arguments inconnus.

Le test `python3 tests/scripts/test_gpt2_shell.py` demontre d'abord `ai-runtime json`, puis la generation GPT-2 dans QEMU avec des actifs de test. Resultat : compilation reussie, test QEMU reussi et `make test-all` a **678/678** sans echec ni test ignore. Journal : `test_logs/test_results_20261007_175037.log`.

## Lot IA 2 valide le 8 octobre 2026

Le chat OS-UI reconnait maintenant le chemin `chat ai ...` comme une vraie requete locale : il appelle `SYS_GPT2_GENERATE`, conserve l'etat `idle`, `generating`, `ready` ou `error` dans la session, ajoute la reponse a l'historique borne et expose `response=...` avec `ai_status=ready`. La scene VGA indique `llm=gpt2_local` pendant ce parcours ; les messages ordinaires restent explicitement `stub_echo`. En cas d'echec du modele, le chat retourne `ai_generation_failed` sans fabriquer de reponse.

La preuve QEMU `python3 tests/scripts/test_qemu_osui_runtime.py` couvre `chat ai a` et passe avec la generation GPT-2 reelle dans le guest. Le timeout du contrat est fixe a 120 secondes pour tenir compte de la latence mesuree du moteur local. Le runner de non-regression utilise un fixture uniquement pour les tests unitaires hote, jamais dans QEMU. Validation finale : `make all` reussi, contrat QEMU OS-UI reussi, `make test-all` **678/678**, 0 echec, 0 ignore ; journal : `test_logs/test_results_20261008_044200.log`.

## Lot #134 valide le 11 octobre 2026

Consolidation globale et finalisation des phases ouvertes (Phase 5 P2P, Phase 6 Multi-Platform, Phase 7 Collaborative, Phase 8 Production). Verification et couverture complete des scenarios de repli, de persistance et des cas d'erreurs sur l'ensemble des modules Ring 3 (`test_p2p`, `test_platform`, `test_collab`, `test_prod`).

Resultat : compilation reussie, validation de la suite unitaire et robustesse (`make test-all`) a **748/748** tests reussis (0 echec, 0 ignore), alignement du registre de commandes a 329 noms (`python3 scripts/extract_guest_commands.py --check`), et validation de tous les contrats QEMU de bout en bout (`make qemu-p2p`, `make qemu-platform`, `make qemu-collab`, `make qemu-production`, `make qemu-persistence`).

## Lot #136 valide le 12 octobre 2026

Integration du moteur promptmessage dans l'OS-UI Runtime C freestanding (`userspace/osui_runtime.c`). Ajout de la commande `browser-pm-run`, execution automatique lors des navigations d'onglets vers des fichiers/URL `.pm` ou `pm://`, et routage unifie des workflows agentiques (`workflow=pm`, `/pm`, `pm-run`). Attribution d'origine par onglet (`origin`), isolation stricte du stockage local par onglet, filtrage des requetes `browser-fetch` et des sockets P2P/Peer (`SYS_PEER_DATA`). Acces inter-origines non autorises retournent 403 (`origin_denied`) et enregistres dans le journal d'audit (`osui-audit`). Ajout de `browser-origin-status` (`/origin-status`).

## Lot #137 merged le 12 octobre 2026

Merged. Pull request #137, head `a3cd28f`, merged into `main` as `46f04ea` at 2026-10-12T12:26:44Z.

Branch CI on `a3cd28f` was green: pull_request run 38239440621 and workflow_dispatch run 38239549781, all 8 jobs success. The earlier red run 38232561645 was on `eebc324` (6 jobs green, `make qemu-osui-runtime` and `make qemu-p2p` red). That failure was fixed before the merge (`id=p0002` instead of `id=pwa`, and no false "peer down" during shell key reads).

Post-merge main run 38249068719:
- Attempt 1: seven jobs green. `QEMU production + persistence across reboot` was cancelled at `timeout-minutes: 20`. Install dependencies took 12 min 0 s (12:26:54Z to 12:38:53Z). Build took 21 s. `make qemu-production` passed in 6 min 28 s. `make qemu-persistence` was still running when the job was killed (started 12:45:42Z, cancelled 12:47:06Z). `make qemu-osui-web` did not start.
- Attempt 2 (failed-job re-run, apt fast at 16 s): the production job succeeded. Wall 13 min 33 s (13:47:28Z to 14:01:01Z). `make qemu-production` 6 min 28 s, `make qemu-persistence` 4 min 4 s, `make qemu-osui-web` 2 min 15 s. The run is green. It does not have room for another 12 min apt install. Lot #138 is that headroom.

Local proofs recorded on `a3cd28f` before the merge (not re-run as part of the merge commit itself):
- `make qemu-p2p`: PASS (scenario 1 included, `p2p peers ok 2 up 2` unchanged), 127 s, frames=661 hello=159 sealed=502 plaintext=0.
- `make qemu-osui-runtime`: contract passed, including `pwa-manifest ok id=p0002 name=demo` and `media-play ok type=audio title=track01 status=playing`.
- `make test-all`: 756 passes, 0 fail, 0 skip; guest command register 353 names.
- `make strict-image-check`: OK. `make secrets-check`: OK, no secret in 3 images.
- `make tree-clean-check`: target absent from the Makefile.

Telemetry dashboard, simulated PWA manifests, and freestanding audio/video stream handlers in the OS-UI VBE GUI and the C web runtime:
- OS-UI telemetry: command, fetch, IPC, media tick, PWA execution, and error counters. Command `browser-telemetry` (`/telemetry`, `osui-telemetry`) supports `status`, `export` (JSON), and `reset`.
- Simulated PWA manifests: `pwa-manifest`, `pwa-install`, `pwa-list`, `pwa-run`. Default manifest `MohhdyApp` (`pwa0001`).
- Freestanding media: `browser-media`, `media-play`, `media-pause`, `media-stop`, `media-stream`, with ticks in the render loop.
- VBE desktop: media status in `userspace/osui_gui.c` and in screen snapshots (`OSUI-SNAP`).
- Guest command register: 353 names (`python3 scripts/extract_guest_commands.py --check`).

## Lot #138 le 12 octobre 2026

CI headroom for the production job. Not a product feature. No new guest command.

Root cause of run 38249068719 attempt 1, same code as the green attempt 2:
- Apt: `sudo apt-get update` plus install took 12 min on that runner (14 s to 39 s on the other jobs of the same attempt, 16 s on the re-run).
- Typing: `make qemu-production` sleeps 0.3 s per scancode (about 1300 characters, about 6.5 min). `make qemu-persistence` sleeps 0.2 s per scancode (about 1000 characters, about 4 min). The guest assertions are the same work either way.
- Initrd: `pack-initrd` was phony via `userspace-all`, so `make qemu-production`, `make qemu-persistence`, and `make qemu-osui-web` each repacked the initrd after `make all`.

What this lot changes, without dropping a contract or raising `timeout-minutes`:
- `scripts/ci-install-deps.sh` plus `actions/cache` on `.apt-cache`. A warm cache uses `dpkg -i` and does not call `apt-get update`. A cache miss still installs, with a 30 s acquire stall timeout and 3 retries. The script installs the superset used by every job (including `python3-cryptography`).
- `tests/scripts/qemu_key_pace.py` types on the SYS_GETS / SYS_GETC serial echo and waits at least `MOHHDY_KEY_GAP` (default 0.03 s, the sendkey hold is 10 ms). A doubled scancode is backspaced before Enter. Production still requires the submitted line to match (`send_exact`). Persistence still requires each command needle, both boots, the encrypted key, the ledger digest, and the corrupt-chunk refusal.
- `build/.initrd-stamp`: `pack-initrd` runs the userspace Makefile every time (incremental) and retars only when an initrd input is newer than the stamp.

`make qemu-osui-web` stays in the same job (HTTPS handshake, not the key gap). The Build job on main attempt 1 was 10 min 39 s (smoke about 9 min, already echo-paced at 0.05 s plus a duplicate settle). It is under 15 min when apt is fast. The apt cache covers that job too. Smoke was not retimed and its assertions were not changed.

Measured on this revision (local workspace, then the pull_request run):

Local:
- `make all` cold: 24 s. The next `make pack-initrd` prints `initrd unchanged` in under 1 s.
- `make test-all`: 756 passes, 0 fail, 0 skip. The runner reports 67 s. Wall including make was 110 s. Guest command register 353 names. Coverage: not measured.
- `make strict-image-check` and `make secrets-check`: OK, wall 8 s.
- `make qemu-production`: PASS in 47 s. The line check did not retype any command.
- `make qemu-persistence`: PASS in 72 s (boot 1 saved at 44 s). Digest and signing key matched across the reboot.

GitHub pull_request run 38260083464, job `QEMU production + persistence across reboot` (114848038552), success. This was a cache miss (apt still ran). Job wall 4 min 55 s (14:03:11Z to 14:08:06Z):
- Install dependencies 24 s.
- Build 20 s.
- `make qemu-production` 47 s (was 6 min 28 s on main).
- `make qemu-persistence` 1 min 13 s (was 4 min 4 s on main).
- `make qemu-osui-web` 2 min 5 s (was 2 min 15 s on main; not retimed by the key pacer).

A representative run is 4 min 55 s, about 10 min under a 15 min wall. A 12 min apt plus the 4 min 31 s of build and contracts on this run is about 16.5 min, inside `timeout-minutes: 20`. The cache post-step saved `.apt-cache` for the next run. The 4 min 55 s figure does not depend on a warm cache.

Follow-up pull_request run 38261049744 (this documentation revision) finished with all 8 jobs success. Production job wall 4 min 42 s, install dependencies 6 s from the apt cache. Build job 11 min 15 s, install 4 s. The longest job was the vfs integration shard at 11 min 59 s. Local `make qemu-osui-web` on the same tree: PASS in 123 s (the two HTTPS handshakes took 101 s).

Still absent, unchanged by this lot: Chromium/WebKit, `phase3_complete=false`, `us031_complete=false`, public Internet, a real conversational model, NAT traversal as a delivered network, real gRPC, real payment, a certificate authority. Coverage is not measured.

## Lot Consolidation Systemique et Endurance valide le 12 octobre 2026

Consolidation systemique finale et stabilisation du maillage P2P (gRPC simule, consensus Raft, antientropie), du Web Runtime et du moteur de production sous charge :
- Creation de la suite unitaire de performance, d'endurance, de charge et de repli `tests/performance/test_osui_p2p_prod_perf.c` integree a `tests/Makefile` et `scripts/run_all_tests.sh`.
- Validation unitaire des metriques de latence de l'OS-UI (`browser-telemetry status/export`), execution multi-onglets, PWA et PromptProgram sous charge.
- Validation unitaire du maillage P2P multi-noeuds (4 noeuds simules) : appels gRPC (`p2p_grpc_call`), consensus Raft (`p2p_raft_elect`), antientropie (`p2p_sync`), traversee STUN/NAT (`p2p_stun_probe`, `p2p_nat_punch`), et routage par repli (`p2p_route_find`).
- Validation unitaire du moteur de production : apprentissage de seuils (`prod_alert_learn_update`), evaluation du moniteur de performance (`prod_perf_monitor_eval`), autoscaling de charge (`prod_autoscale_workload_eval`), pipeline CI/CD, scanner de securite, et sauvegarde/restauration persistance avec refus de corruption.
- Creation et validation du contrat QEMU multi-noeuds d'endurance `make qemu-perf-endurance` (`tests/scripts/test_qemu_perf_endurance.py`) : boot de 2 invites QEMU sur hub Ethernet partage, execution complete du Web Runtime, du maillage P2P (gRPC, Raft, sync, STUN/punch, analyze) et du moteur de production avec persistance sur disque IDE.
- Resultat : validation complete de la suite de tests (`make test-all`) portee a **759/759** tests reussis (0 echec, 0 ignore), alignement du registre de commandes a 353 noms (`python3 scripts/extract_guest_commands.py --check`), et succes des verifications d'images (`make strict-image-check`, `make secrets-check`).

## Feuille de route opérationnelle après la baseline

L'ordre suivant est la suite d'implémentation retenue ; il ne transforme pas les fonctions futures en fonctions livrées :

1. **Contrat IA local** : distinguer explicitement la complétion GPT-2, l'assistant instruction-tuned et le fournisseur réseau ; documenter le modèle, le contexte, la latence, la non-garantie factuelle et les erreurs.
2. **Parcours OS-UI réel** : remplacer le chat stub par `gui -> session IA -> worker -> réponse -> affichage`, avec statut du fournisseur, historique borné, annulation et test QEMU de bout en bout.
   - Lot du 10 octobre 2026 : contrat runtime IA (latence, erreurs distinguees, raison d'abandon, vivacite du worker par battement, blocage detecte en 15 s), annulation Echap sans rejeu Ring 0, etats OS-UI distincts (absence de modele, erreur, annulation) et `session-end`. Correction de regression : depuis #106, SYS_GETS cooperatif laissait tourner les enfants du shell pendant la saisie (contrats spawn, net-worker, vfs-service, OS-UI rouges sur main) ; seuls les services de boot (atadriver, networker, aiworker) tournent desormais pendant l'attente clavier.
   - Lot 2 du 10 octobre 2026 : etape 2 close (contrat QEMU GUI de bout en bout : ouverture, requete IA, reponse ou etat sans modele, fin de session, fermeture, `gui_live=false`). Reseau : sur le noyau strict par defaut, le noyau ne sonde plus et ne reinitialise plus la NE2000 ; le networker Ring 3 fait la sonde, le reset, la lecture MAC et le rapporte (`OS_NET_NIC_REPORT`), et apres perte du worker le noyau ne touche plus la carte (le networker suivant la reinitialise). Le noyau legacy (`NET_RING0_FALLBACK=1`, `build/mohhdy-netlegacy.bin`) garde l'ancien chemin. La pile reseau Ring 0 reste liee dans l'image stricte : `mohhdy.bin` et `mohhdy-netlegacy.bin` partagent les memes objets (seul `syscall.c` differe) et les chemins `kernel_net_wire_*` / `net_llm_client` y sont encore references ; la retirer demande un second jeu d'objets, non fait dans ce lot. Etape 3 : expiration, restauration, nettoyage de sessions, confirmation explicite des mutations, revocation et journal.
   - Lot 3 du 10 octobre 2026 (empile sur #108) : (a) worker IA perdu prouve en QEMU depuis OS-UI : contrat `qemu-ai-worker` scenario 10, worker affame, job relaye en panne (stalled en 15 s), reponse par le chemin Ring 0, et OS-UI affiche `ai_status=ready worker=stalled fallback=ring0` (et sur la scene `etat_ia=reponse (repli Ring 0, worker IA perdu)`, verifie en test unitaire) ; scenario 11, worker tue, OS-UI servi directement par Ring 0 sans drapeau perime. L'etat `ai_status=worker_lost` (aucune reponse du tout) reste prouve en test unitaire seulement, car en QEMU le repli Ring 0 du GPT-2 repond toujours quand le modele est present. (b) L'image stricte par defaut ne lie plus la pile reseau Ring 0 : `NET_STACK_OBJECTS` (ARP, DHCP, IPv4/UDP, DNS, TCP, sockets, TLS client et serveur, X.509, bigint, ECDSA, X25519, RSA, SHA-256, client HTTP LLM, moteur wire, glue `ne2k.c`) est retire et remplace par `kernel/net_ring0_absent.c` (refus `-127` journalise, compteur de refus, statut wire, heure UTC pour le TLS du worker, E/S et IRQ NE2000). Taille `.text` : 850053 octets (legacy, identique a l'ancien defaut) contre 578737 (strict), fichier 929672 -> 641116 octets. `make strict-image-check` (CI) verifie l'absence des symboles de la pile ; `build/mohhdy-netlegacy.bin` garde tout. Les contrats net-no-ring0, net-loopback-worker, net-worker, net-wire et net-peer-tls-worker passent sur cette image. (c) Etape 4 sans Internet public : fournisseur OS-UI `peer` (pair QEMU local via le networker), etat reseau lu dans le rapport publie par le networker (`no_net_worker`, `no_nic`, `no_dhcp_lease`, `no_trust_anchor`, `ready`), repli local explicite dans chaque reponse (`provider=peer fallback=local reason=...`), y compris quand le repli echoue ; `make secrets-check` (CI) cherche des formes de secrets dans les images et l'initrd. Limite levee au lot 4 (voir ci-dessous). TLS, requete, streaming, fermeture et erreurs restent prouves separement par les contrats shell `qemu-ne2k-tls-*` et `qemu-net-peer-tls-worker`. (d) Etape 5, API locale controlee dans OS-UI (pas de socket, pas de moteur navigateur, `phase3_complete=false`) : `api <get|post> <route> [jeton] [corps]`, routes `/status` (publique), `/sessions` (seulement la session du jeton), `/vfs/<chemin>` (memes scopes `fs.read:` que la console), `/ai/chat` (meme chemin de chat et memes capacites) ; jeton `api-token` sous la capacite `web.api`, `api-revoke` ; erreurs 400/401/403/404/405/413/429 (8 requetes par 10 s, corps 64 octets) ; chaque requete journalisee. Le jeton est un identifiant local de session, pas un secret cryptographique. Preuves : `test_osui_runtime` et contrat QEMU OS-UI.
   - Lot 4 du 10 octobre 2026 (empile sur #109) : etape 4, le chat OS-UI parle vraiment au pair QEMU local : fournisseur `peer`, meme session SYS_LLM que le shell (acquisition DHCP/DNS/SYN, TLS, requete, reponse), relayee au client TLS du networker Ring 3 sur le noyau strict ; echange borne (30 s et budgets de sondage), session TLS rearmee pour le tour suivant au lieu d'etre fermee ; chaque etape en echec donne sa raison de repli local (`acquire_failed`, `tls_failed`, `tls_timeout`, `request_failed`, `response_failed`, `response_timeout`, `http_error`). Preuves : `test_osui_runtime` (chaque raison, reutilisation de session) et contrat QEMU `qemu-osui-peer` en CI (deux tours `llm=peer_qemu response=ok`, puis pair arrete : `reason=response_timeout`). Pas d'Internet public, pas d'hote reel. Etape 5 : route `/supervision` de l'API locale (vivacite des workers IA et reseau, derniere raison d'abandon IA, NIC, fin du journal) sous `admin.observe`. Reste etape 5 : servir l'API et la console sur le reseau (HTTP via le networker) ; le serveur pair Ring 3 actuel ne sait servir que l'echange METIER.
   - Lot 5 du 10 octobre 2026 (empile sur #110) : le serveur pair est generalise : nouvel appel `SYS_PEER_DATA` (156) qui recoit, emet (64 octets par appel, limite du relais) et ferme des donnees TCP en clair sur la connexion acceptee par `SYS_PEER_LISTEN`/`SYS_PEER_ACCEPT` ; sur le noyau strict il est relaye au networker Ring 3 (op 156 dans le journal du networker), sur le noyau legacy il tourne en Ring 0. OS-UI `web-serve [port] [requetes]` sert l'API locale en HTTP/1.0 (memes routes, jeton `Authorization: Bearer`, memes limites et erreurs) et une page console minimale sur `/`. Preuves : `test_web_serve` (unitaire) et contrat QEMU `qemu-osui-web` en CI (client HTTP hote sur le pair Ethernet controle, sans TAP ni Internet : `/status` 200, `/` 200, `/sessions` 401 puis 200 avec jeton, `POST /ai/chat` 202). Limites honnetes : HTTP en clair (pas de TLS sur ce port), une connexion a la fois, commande bloquante, pas de moteur de rendu (`phase3_complete=false`). CI : `qemu-ata-driver` deplace de OS-UI guest C (15:04) vers le job TLS invite-invite.
   - Lot 6 du 10 octobre 2026 (empile sur #111) : restes web de l'etape 5. `SYS_PEER_DATA` passe par le canal bulk du relais (structure entiere, 1 Kio par appel au lieu de 64 octets). Table web dans le networker : jusqu'a 4 connexions TCP sur un meme port (une socket par connexion, demultiplexage par adresse et port distants, une socket toujours en ecoute tant qu'un slot est libre ; capacite des sockets 4 -> 8). HTTPS : TLS 1.2 ECDHE-RSA-AES128-GCM-SHA256 par connexion, avec le code serveur TLS invite existant et le certificat de test du depot (pas un certificat de confiance) ; un record ClientHello en version 3.1 est accepte (RFC 5246 E.1). OS-UI : `web-serve [port] [n] [tls]` (bloquant), `web-serve start [port] [tls]` / `status` / `stop` (arriere-plan : le shell lit les touches sans bloquer via SYS_GETC et fait avancer le serveur entre deux touches, la console reste utilisable). Preuves : `test_web_serve` et `test_peer_data_bulk` (unitaires), contrat QEMU `qemu-osui-web` en CI (3 connexions HTTP ouvertes en meme temps sur 3 slots, 2 echanges HTTPS concurrents par un client Python ssl sur le pair Ethernet controle, puis serveur en arriere-plan avec commandes console entre deux requetes). Limites honnetes : pendant que la table web est ouverte, les trames TCP vers d'autres ports sont jetees (comptees) ; la poignee de main TLS est lente (signature RSA 2048 logicielle, environ une minute pour deux en QEMU TCG) ; le client de test retransmet son flight TLS si une trame est perdue ; pas de retransmission cote invite ; pas de moteur de rendu (`phase3_complete=false`).
3. **Sessions et capacités agentiques** : restaurer les préférences, gérer plusieurs sessions, puis ajouter progressivement lecture VFS, recherche, commandes allowlistées, confirmation avant mutation et journal d'audit.
4. **Réseau utile** : conserver les pairs QEMU comme preuves locales, puis isoler secrets, TLS, streaming, erreurs et repli local avant toute affirmation d'accès Internet public.
5. **Web Runtime** : seulement après le parcours IA réel, exposer une API locale contrôlée, la supervision web, le VFS et la console IA ; `phase3_complete=false` et Chromium/WebKit restent hors périmètre.

6. **Phase 4 PromptMessage** (ouverte le 10 octobre 2026) : langage, compilateur, interpreteur et outils dans le shell guest.
   - Lot phase 4 du 10 octobre 2026 : `userspace/promptmessage.c` (C autonome, Ring 3) : grammaire a mots fixes (`create file`, `append`, `show file`, `print`, `set`, `if/else/end`, `repeat`, `when user says`, `expect`, `use`, `stop`), compilation en bytecode a pile, image PMC1 avec somme FNV-1a (image alteree refusee), VM bornee (20000 pas, pile 12, 16 variables, 8 declencheurs), ecritures refusees sur `..`, `/bin/`, `/models/`. Shell : `pm`, `pm-check` (ligne et colonne), `pm-run`, `pm-say`, `pm-test`, `pm-doc`, `pm-debug` (trace ou point d'arret avec variables), `pm-disasm`, `pm-compile`, `pm-exec`, `pm-version(s)`, `pm-edit` (editeur de lignes), `pm-catalog`/`pm-install` (catalogue local `/pm`), `pm-certify`/`pm-verify` (enregistrement local, somme FNV-1a et tests `expect` passes). Optimiseur : repli des constantes entieres et des conditions constantes. Exemples `/pm/*.pm` dans l'initrd. Preuves : `test_promptmessage` (9 tests) et contrat QEMU `qemu-promptmessage` (CI, environ 1 min). Limites honnetes : pas de comprehension du langage naturel ni d'IA (les mots sont une grammaire fixe), pas de compilateur croise, catalogue local seulement (pas de marketplace reseau), la somme de controle n'est pas une signature, l'editeur n'est pas un IDE plein ecran.

Chaque étape devra ajouter ses critères d'acceptation et ses tests avant d'être marquée comme livrée.

| Domaine | État au 3 octobre 2026 |
|---|---|
| FAT16/FAT32, capacités et IPC VFS | FAT16 et FAT32 valident leurs BPB, lisent, listent, paginent et statent la racine ainsi que les sous-répertoires multi-niveaux (ex. `SUB1/SUB2/FILE.TXT`). `vfsserver` délègue à `vfsvirtual` les mutations de montages fixes, les I/O des quatre alias dynamiques **et** les I/O `read`/`stat`/liste/pages/observation des montages protégés, toujours avec PID et `request_id` corrélés (AOS-2163...2170 ; AOS-2171 refuse le repli local ATA/FAT quand le worker est vivant ; AOS-2172 ferme le contournement proprietaire FAT16/FAT32 tant que `vfs-virtual` est publie ; AOS-2173 ferme le meme contournement pour initrd/overlay ; AOS-2174 ferme le contournement SOURCE_ALL / combinaisons ATA-backed tant que `vfs-virtual` est publie ; AOS-2175 restreint overlay read/stat au PID worker vivant ; AOS-2177 gate SYS_READFILE/SYS_WRITEFILE historiques sur la partie overlay, initrd RAM restant lisible ; AOS-2178 ferme les autres points d'entree historiques overlay et la liste overlay backend). Le worker est l'autorité bornée de ses alias ; le médiateur ne garde qu'un miroir volatile de sélection. Le registre backend attache chaque capability à un masque de sources (`initrd`, `overlay`, `fat16`, `fat32`) et gère l'octroi cumulatif dynamique (`rights |= new_rights`, `sources |= new_sources`) : une transaction worker reçoit une source avec `read` ou `mutate`, puis cette capacité est révoquée à la réponse, au délai ou à la disparition. Les primitives FAT brutes sont elles aussi backend-protégées. `vfs-backend-scope <pid>` fournit en outre un instantané IPC corrélé. Le miroir est purgé à tout changement, disparition ou expiration du PID worker ; toute I/O ou mutation incertaine retourne `INVALID`, sans repli local ni rejeu. `vfs-write`, `vfs-remove` et `vfs-rename` refusent l'écrasement ; `rmdir` refuse un répertoire non vide. |
| LFN FAT16/FAT32 | Le checksum 8.3, la publication multi-entrée, la reconstruction, la lecture par nom long, le renommage et la suppression sont livrés à la racine et dans les sous-répertoires multi-niveaux. La conversion UTF-8/UTF-16LE couvre les caractères BMP et les paires substituts. |
| Réseau utilisateur | Le registre TCP statique expose les cycles actif et passif et la gestion multi-sockets jusqu'à 4 slots simultanés (`NET_SOCKET_CAPACITY`) avec réinitialisation propre à la fermeture (`net_socket_close`) ; les syscalls 99-108 valident les requêtes et buffers page-par-page dans l'espace utilisateur VMM. Sur une NE2000 QEMU et un pair Ethernet local déterministe, `ai-acquire` puis `ai-tls-poll` valident DHCP/DNS/ARP, SYN, SYN-ACK, ClientHello et ServerHello minimal (`TLS_STARTED`). `make qemu-ne2k-tls-http` prolonge ce pair : handshake TLS 1.2 authentifié (`0xc02f`, X25519, RSA/SHA-256, ancre de test `example.com`), `TLS_COMPLETE`, POST ollama chiffre et extraction HTTP 200 `{"response":"ok"}`. `make qemu-ne2k-tls-sse` envoie ensuite un POST `"stream":true` et lit un flux HTTP chunked : premier record `data: {"response":"ok"}`, second record `data: [DONE]` puis le terminateur chunked 0. Le decodeur SSE reste ouvert tant que le chunked n'est pas clos et que `[DONE]` n'est pas vu. `make qemu-ne2k-tls-close` remplace le terminateur `[DONE]` par un `close_notify` TLS 1.2 authentifié du pair : l'adaptateur socket le propage sans rollback, le noyau accuse le segment TCP, émet son propre `close_notify` AES-GCM de façon transactionnelle puis publie la réponse comme terminée. Le pair exige cette réponse déchiffrable avant le succès du contrat. `make qemu-ne2k-tls-next` rearme la session (`ai-next`) puis envoie un second POST chiffre (HTTP 200 puis flux SSE) sans refaire le handshake. `make qemu-ne2k-tls-multipair` exécute ensuite deux cycles indépendants et séquentiels, chacun avec un pair `127.0.0.1`, une MAC NE2000 et un journal propres ; les deux prouvent DHCP, ARP, DNS, TCP, TLS authentifié et HTTP 200. Un contrat séparé `make qemu-ne2k-shared-topology` branche ensuite deux invités simultanés sur un hub socket `127.0.0.1` (injection PS/2 mutexée, `nic=detected` des deux côtés, DHCP Discover de A observé pendant que B reste vivant) ; ce n'est pas un TLS multi-invités simultané. Le caractère séquentiel de `qemu-ne2k-tls-multipair` évite la contention PS/2 connue lors d'injections chevauchées sous TCG. Ce n'est pas Internet public, ni un TLS vers un hote reel, ni OpenAI. Tranche 5 : si `networker` est enregistre comme `net-driver`, les syscalls LLM reseau, socket et peer sont reserves a son PID (autres taches : `OS_NET_WORKER_REQUIRED` -59, `net-status` reste lisible, `make qemu-net-worker`) ; sans worker rien ne change ; relais IPC et TCP sur le fil via le worker : voir la ligne Tranche 5 ; depuis la Tranche 5 suite, un `networker` qui reclame la carte pilote lui-meme la NE2000 en Ring 3 (ports 0x300-0x31F via l'IOPB, IRQ3), les chemins LLM/TLS/peer noyau etant alors refuses (-141). |
| Latence GGUF QEMU | Le benchmark local répète `ai bonjour` puis `ai-continue`, sérialise minimum/médiane/maximum/dispersion en JSON et isole le temps de commande du boot. La campagne TCG de trois échantillons obtient une médiane de **48,739 s** pour le premier token et **22,781 s** pour la continuation. Une campagne KVM séparée, un seul échantillon (`GGUF_BENCH_RUNS=1`), mesurait **1407,034 s** puis **477,200 s** avant le transfert PIO groupé. Apres la copie residente des poids au boot, la meme commande mesure **43,916 s** puis **20,224 s**. Le passage PIO groupe precedent mesurait **52,319 s** puis **23,227 s**. `sub_second_claim_allowed` reste faux. Le TCG n'est pas substitué au KVM. |
| Validation | La suite complete passe **675/675** (cette tranche : `test_disk_range_ignores_resident_bytes` ; 674/674 apres la session GGUF ; **662/662** avant, tranche GGUF : `test_gguf_step` dans `test_ai_relay` ; 661/661 avant ; 56 binaires : 51 kernel, 4 userspace, 1 robustesse ; rejeu local du 3 octobre 2026 avec `test_ai_relay` et `test_gpt2_generate`, 0 ignore ; un premier rejeu annoncait 657/657 alors que `test_gpt2_generate` etait ignore faute de sources dans `tests/scripts/run_all_tests.sh`, corrige ; 648/648 avant, avec `test_ata_pio`, `test_sched_quantum` et `test_ipc_wait` ; 632/632 avant ; 630/630 plus tot le meme jour avant le tampon du journal ; 629/629 avant `children` ; 628/628 le 2 octobre ; 622/622 plus tot le meme jour ; 605/605 au 27 septembre ; 522/522 au 21 septembre). La couverture n'est pas mesuree : `tests/scripts/run_all_tests.sh` affichait des constantes (85/72/78 %) et dit maintenant "Coverage: not measured". Les dossiers C `unit/fs`, `integration`, `system` et `performance` sont vides pour ce lanceur ; les contrats QEMU sont des scripts Python a part. Le contrat QEMU VFS valide les fixtures FAT16 et FAT32 sur deux disques IDE, les cycles 8.3 et LFN de racine, puis pour chaque volume `mkdir`, refus de doublon, écriture enfant, `stat`, liste, lecture, renommage, refus `rmdir` non vide, suppression et `rmdir` final. Il prouve la délégation corrélée des ajouts/retraits et des I/O d'alias vers `vfsvirtual`, les pages et l'observation, ainsi que le diagnostic, pendant une lecture d'alias enfant suspendue, de l'unique scope `read initrd` : la ligne publique n'admet que droits, sources et identifiant de requête corrélé, sans préfixe ; puis la révocation complète de la capacité worker après expiration. Les tests unitaires prouvent qu'un droit `read` sur `initrd` n'autorise ni `overlay`, ni FAT, ni une primitive générique ; un droit `mutate` sur FAT32 reste aussi isolé. Le smoke CI ajoute les deux paires TLS locales ; le smoke cœur conserve sa confirmation caractère par caractère tandis que les contrats VFS et IRQ0 réconcilient la ligne entière, retirent localement les divergences avant `ret` et ne rejouent aucune ligne ou requête après soumission. `make integration-qemu` conserve ses sept contrats, est séquentiel par défaut pour éviter la contention PS/2 et doit rester sous le budget de 25 minutes. |
| OS-UI Ring 3 | Chat stub (`llm=stub_echo`), slash, pieges Linux, scene IA pixel (modes reflecting/acting/presenting, constructions VBE, pas HTML `#ai-stage`), commande canonique `gui` (aliases `graphics`, `desktop`; `console` / ESC quitte ; `chrome=qemu_fb` `display_surface=vbe_lfb`), chat flottant quand un pane s'ouvre (`/center` restaure), sessions `s0001+` isolees, grant/revoke, escalate/takeover, `origin_denied` 403, gestes simulateur allowlistes, MCP `mcp.invoice.create` / `tool_undeclared`, FS lecture sandbox (traversal et write refuses). `python_facade=false`, `phase3_complete=false`, `chromium=false`. US-031 (moteur Chromium/WebKit) n'est pas livre. Les chaines du guest (aide, `browser-status`, `os-status`, ligne serie `gui`, canvas) impriment `us031_complete=false`. `chromium=false` et `phase3_complete=false` restent. Preuves : Unity `test_osui_runtime.c` / `test_osui_gui.c` / `test_gfx_desktop.c`. UHCI `usb-tablet` enum verifie (`USB Tablet: Controller UHCI initialise et enumere`); smoke clic dock via monitor (`osui gui line=/shell`). Grab GTK hote peut encore exiger Ctrl+Alt+G en interactif. |

MOHHDY demarre sans OS preinstalle dans QEMU, charge une archive initrd TAR, lance un shell ELF en Ring 3 (dont OS-UI C) et peut executer localement GPT-2 124M si les deux actifs binaires sont integres a l'image. Il demeure un **prototype de noyau guest**, non un systeme d'exploitation generaliste, non un navigateur-OS Chromium ni un LLM de production. La facade Python `agent/` / `osui/` est retiree.

### Maintenance des gates - 25 août 2026

Le contrat de contrôle IA vérifie désormais le catalogue réellement déclaré par le shell (`gpt2_124M.bin` et `gpt2.gguf`) ainsi que son diagnostic de sélection de profil. Les contrats IPC, VFS et le smoke cœur attendent le premier tour coopératif nécessaire et filtrent uniquement les diagnostics d'ordonnancement asynchrones. Les contrats VFS et IRQ0 préparent puis réconcilient l'écho PS/2 de la ligne entière avant `ret` : un doublon ou une ligne partielle est effacé et retapé tant qu'elle n'a pas été soumise. Le contrat TLS réutilise la même discipline pour chaque ligne réseau. Après l'entrée, une opération IPC, VFS ou réseau n'est jamais rejouée. Le contrat de transfert de service consomme aussi l'événement de tâche best-effort qui peut précéder l'événement de purge du service dans la FIFO du shell. Le contrat de transfert normalise également ces diagnostics lorsqu'ils fragmentent une réponse `service-find`, avant d'appliquer son assertion complète de propriétaire. Ces garde-fous empêchent que des scancodes PS/2 dupliqués, une FIFO mixte ou une trace d'ordonnancement soient interprétés comme des régressions noyau.

| Périmètre | État vérifié |
|---|---|
| Démarrage et espace utilisateur | Noyau Multiboot i386, VGA/série, clavier PS/2 (lettres en Maj XOR Verr Maj ; `0xAA` est le relachement de Maj gauche), shell ELF Ring 3, curseur bloc et historique d'écran (Page Up/Down) |
| IA locale | GPT-2 124M `llm.c v3`, BPE, cache KV, SSE2 et top-k, sans réseau au boot. Inventaire point 4 : quand `aiworker` (service `ai-engine`) est vivant, la generation FP32 de `SYS_GPT2_GENERATE` tourne en Ring 3 sur les poids initrd projetes en lecture seule sans copie, avec etat FPU/SSE sauve par tache ; le noyau relaie (sonnette IPC, appelant bloque), compte `kernel_infer_while_live` (0 mesure) et garde le repli FP32 Ring 0 (worker absent, tue ou bloque 300 s). Preuve `make qemu-ai-worker` sur fixture synthetique seulement ([ai_worker_ring3.md](ai_worker_ring3.md)) ; le runtime GGUF prépare et exécute la génération quantifiée Q3_K/Q4_K/Q6_K sur buffers statiques ou caller-owned. Tranche GGUF : quand `aiworker` a charge sa copie du GGUF (lecture bulk reservee au worker) et declare `GGUF_READY`, chaque pas 109-110 tourne en Ring 3 ; depuis la tranche session, le worker tokenise le prompt normalise, possede la session (jetons, graine) et decode le texte (`OS_AI_JOB_GGUF_SESSION`, memes jetons que le chemin noyau, reprise par un nouveau worker prouvee) ; restent Ring 0 la normalisation du prompt, un miroir de la session, le tokenizer et la session de repli, le chargement resident au boot. Depuis cette pointe, la lecture bulk est `fat16_read_file_range_disk` et `GGUF_READY` libere l'instantane PMM (`[AI] GGUF resident released pages N`). Le defaut `GGUF_RING0_FALLBACK=0` refuse 109/110 sans worker GGUF (`OS_AI_GGUF_NO_WORKER`, -148, `path none`) au lieu du tokenizer et de la session Ring 0. Le repli FP32 reste. Preuve locale `python3 tests/integration/test_qemu_ai_gguf.py` en 255 s sur GGUF synthetique. Le GGUF reel Q3_K_M a ete rejoue hors CI (`make qemu-gpt2-sentences`, TCG, 67,4 s) : `the capital of france is` repond `the city of the city of the capital, which is called "the city of france" and is known as "`. Le chemin FP32, meme checkpoint que l'export Hugging Face `openai-community/gpt2` (sha256 `3da8b207584030bcdcd207cf7a99952e3421dce92da218b351071857511bf162`, 497904640 octets), utilise le GELU OpenAI. Le guest a repondu a `ai my name is` par `a little bit different, but I'm not sure if it's because of the fact that I'm from the same country`. En glouton, `The capital of France is` continue par `the capital of the French Republic, and the capital of`. Ce n'est pas une reponse factuelle. Les 43,916 s / 20,224 s ne sont pas rejoues. Detail : [ai_worker_gguf.md](ai_worker_gguf.md), [gpt2_baremetal_deployment.md](gpt2_baremetal_deployment.md). |
| Stockage | Archive initrd TAR en lecture seule, overlay AIOV V2 sur ATA PIO aux LBA 0-63, volumes FAT16 et FAT32 avec LFN UTF-8 à la racine et sous-répertoires multi-niveaux ; le VFS route lectures, listages paginés et statuts, et permet sous capacité `mutate` les fichiers 8.3 et LFN en sous-répertoires, `mkdir` et `rmdir` vide, sans écrasement ni remplacement transactionnel. |
| Stockage Tranche 4 (pilote Ring 3, logique FAT/overlay comprise) | Le pilote Ring 3 `atadriver` (service `ata-driver`) execute lui-meme `in`/`out` sur 0x1F0-0x1F7 et 0x3F6 a CPL 3 grace au bitmap I/O du TSS, ouvert par l'ordonnanceur pour ce seul PID ; il sert au seul client `ataclient` des fenetres de secteur de 64 octets par IPC et refuse d'ecrire les LBA 0-63. Toute autre tache voit tous les ports refuses (#GP, seule la tache fautive est tuee). Slice 2 : pilote vivant, le flush du snapshot overlay passe par le pilote (blocs de 8 secteurs copies par le noyau, compteur `flushes` cote pilote, compteur PIO noyau inchange) et survit a un redemarrage a froid ; un nouveau pilote recharge le snapshot via son PIO ; un claim noyau exclut le PIO noyau (-85) pendant les acces du pilote ; les ecritures client sont refusees sur l'overlay, FAT16 et un esclave FAT32. Slice 3 : le noyau lance `atadriver` au boot si un disque IDE est present (PID 2 apres le shell, seul le shell racine peut le tuer) ; pilote vivant, les E/S secteur FAT16/FAT32 passent par lui par RPC synchrone (tache appelante bloquee dans son syscall via une continuation noyau, seul le pilote tourne pendant ce temps) : compteurs `rd`/`wr` cote pilote en hausse, compteur PIO noyau FAT inchange et `fatkpiolive 0` ; apres `kill` du pilote, FAT et overlay repassent en PIO Ring 0 et persistent au redemarrage. Les lectures GGUF passent par les memes callbacks FAT16 (non prouve en QEMU faute de checkpoint en CI). Suite (27 septembre 2026, `SYS_ATA_FS` 145) : le pilote vivant execute **sa propre** logique FAT16/FAT32/overlay (montage via son PIO, operations, persistance incrementale LBA 0-63) ; chaque operation VFS/FAT/overlay d'une tache devient une RPC dont le noyau ne fait que copier les octets (`fskernel 0`, compteur PIO FAT noyau 17 -> 17, `rd=65 wr=4` cote pilote) ; a sa mort le store revient en Ring 0 depuis le miroir publie par le pilote, une operation en vol n'est refaite en Ring 0 que si aucun secteur n'etait commis (sinon `OS_ATA_FS_ABORTED`), un store detenu mais indisponible repond -137 sans executer la copie noyau perimee ; un pilote relance reprend le store. Restent Ring 0 par conception : chargement overlay et montage FAT au boot (aucune tache n'existe encore) et la copie de repli sans pilote. Preuve : `make qemu-ata-driver`, [tranche4_ata_ring3_driver.md](tranche4_ata_ring3_driver.md). |
| Reseau Tranche 5 (gate + relais IPC + TCP sur le fil + pilote NE2000 Ring 3) | Slice 1 : avec `networker` (service `net-driver`) enregistre, les syscalls LLM reseau (91-98), socket (99-108) et peer (128-130) sont reserves a son PID (-59 sinon). Slice 2 : les syscalls socket 99-108 des autres taches sont relayes au worker : le noyau lui envoie un message IPC (expediteur 0, `OS_IPC_NET_RELAY_REQUEST`), le worker execute l'appel par son chemin privilegie et repond par `SYS_NET_RELAY_REPLY` ; une requete a la fois, timeout -87, abort -88 si le worker meurt, jamais de rejeu ; compteurs `SYS_NET_RELAY_STATUS` / `net-relay-status`. LLM et peer restent -59. Les syscalls socket sont un codec de segments TCP sur le registre Ring 0 : la preuve est un echange TCP entre deux sockets du registre dans le meme invite, pas du TCP sur le fil. Slice 3 : TCP sur le fil via le worker. `SYS_SOCKET_CONNECT` (144, relaye) et, pour une socket liee au fil, send/receive/close relayes sont executes par le worker avec `SYS_NET_WIRE_CONNECT/SEND/RECV/CLOSE` (139-142, reserves au PID worker meme sans worker enregistre, -59 sinon) : ARP, trames IPv4/TCP via `ne2k_tcp_segment`, demux RX par 4-uplet vers la socket liee. Preuve `make qemu-net-wire` : `netwire` (tache ordinaire) echange 16 octets en echo avec un pair local 10.32.0.2:7 branche par `-netdev socket` (pas de user-net ni d'Internet) ; 7 trames invite->pair, 4 pair->invite, 4 appels relayes, 0 erreur de checksum cote pair, 9 acces directs refuses ; compteurs `SYS_NET_WIRE_STATUS` / `net-wire-status`. Connexion active seulement, sans retransmission de donnees. Suite (27 septembre 2026, `SYS_NET_NIC` 146) : le worker reclame la carte (CLAIM, seul PID `net-driver` vivant, NE2000 sondee au boot sinon -143) ; l'ordonnanceur ouvre 0x300-0x31F dans le bitmap I/O du TSS pour lui seul (#GP ailleurs) ; `networker` execute sonde, PROM/MAC (`prom-match 1`), anneaux, TX par DMA distant, poll RX et acquittement ISR a CPL 3 (`kernel/ne2k_hw.c` compile aussi en Ring 3) ; IRQ3 n'est plus que comptee par le noyau et servie par le worker ; tout acces noyau a la carte passe par une porte qui le refuse et le compte (0 mesure) et les syscalls LLM 91-98 / peer 128-130 repondent -141 ; le moteur `net_wire.c` est devenu reprenable : les appels fil repondent -142 et le worker le pompe (`PUMP`), recoit les trames a emettre et lui fournit celles qu'il a recues ; a la mort du worker le noyau ferme la fenetre, annule l'operation (sans rejeu), reinitialise la carte en Ring 0 et un nouveau worker peut la reclamer. Mesure `make qemu-net-wire` (NIC declaree `irq=3`) : 7 trames emises par le pilote Ring 3 = 7 recues par le pair, 4 recues, 0 echec TX, IRQ3 servie 9 fois (10 comptees), 0 acces noyau, 0 appel noyau refuse, reprise puis re-claim par un nouveau PID. Pile (27 septembre 2026, suite) : des que le worker detient la carte, ARP/IPv4/TCP/TLS tournent dans `networker` a CPL 3. Il lie sa propre copie de la pile (`net_socket`, `net_tcp`, `net_ethernet_arp`, `net_ipv4_udp`, `net_dhcp`, `net_dns`, `net_wire`, `ne2k`, TLS record, AES-GCM, X25519, RSA, X.509, HTTP) et l'executeur partage `kernel/net_stack_exec.c` : les syscalls socket relayes tournent sur son registre, `connect`/`send`/`recv`/`close` fil sur son moteur (trames construites et decodees en Ring 3, plus de `PUMP` : 0 pump, 0 trame noyau mesures), et les LLM 91-98 des autres taches lui sont relayes (canal bulk `SYS_NET_RELAY_BULK` 147 pour les structures > 96 octets, delai 60 s) et executes par `kernel/net_llm_client.c` (session DHCP/DNS/TCP/TLS 1.2/HTTP extraite de `kernel.c`, liee au noyau pour le repli et au worker). `SYS_NET_NIC` gagne LOG (ligne atomique), UTC (validite X.509), PUBLISH/STACK (compteurs Ring 3 fusionnes dans `SYS_NET_WIRE_STATUS`, mot de session renvoye par `SYS_LLM_SESSION_STATUS`). Mesures : `make qemu-net-wire` 7 trames construites en Ring 3 = 7 recues par le pair, 4 decodees, 0 pump noyau ; `make qemu-net-worker` 18 appels relayes (16 socket + 2 LLM repondus par le client TLS Ring 3), 2 refus peer ; `make qemu-net-tls-worker` handshake TLS authentifie, TLS_COMPLETE et HTTP 200 avec 15 syscalls LLM relayes. Restent Ring 0 : repli sans worker (session LLM et moteur fil noyau), sonde au boot, serveur TLS pair 128-130 (-141 tant que le worker detient la carte) ; `networker` n'est pas lance au boot. Build strict (3 octobre 2026) : `NET_RING0_FALLBACK=0` (seul `syscall.c` differe) refuse -59 tout syscall reseau (91-98, 99-108, 144, 128-130, 139-142) non relaye au worker, worker compris : la pile socket/LLM/peer/fil noyau n'est plus atteignable depuis l'espace utilisateur ; `make qemu-net-no-ring0` prouve TLS/HTTP et une boucle TCP de 14 appels socket servis par `networker`, puis, worker tue, -59 sans repli Ring 0. Sans NE2000, le worker d'un noyau strict sert les sockets en boucle locale depuis son registre Ring 3 et repond LLM/peer "NE2000 absent" depuis le Ring 3 (`make qemu-net-loopback-worker`) ; le serveur pair (128 ecoute, 130 poll) passe par le worker dans `make qemu-net-no-ring0`. Depuis le 3 octobre 2026 ce noyau strict est le noyau par defaut (`NET_RING0_FALLBACK ?= 0`, `networker` lance au boot meme sans carte) ; le noyau historique reste disponible pour comparaison (`make kernel-netlegacy`, `build/mohhdy-netlegacy.bin`, ou `make NET_RING0_FALLBACK=1`) ; les objets noyau restent lies (sonde, statuts, reinitialisation de la carte). Preuves : `make qemu-net-worker`, `make qemu-net-wire`, `make qemu-net-tls-worker`, `make qemu-net-no-ring0`, [tranche5_net_worker_gate.md](tranche5_net_worker_gate.md), [RAPPORT_T5_NET_RING3.md](RAPPORT_T5_NET_RING3.md). |
| Ordonnancement et télémétrie | Coopératif par syscall et quantum IRQ0 sûr entre tâches utilisateur ; instantané de ticks, sélections, priorité CPU, parent et enfants directs ; terminaison, réattribution, attente et capacité de création limitées à la filiation directe |
| IPC/VFS Foundation MOHHDY | Boîte aux lettres FIFO entre tâches Ring 3 (envoi non bloquant ; reception non bloquante `SYS_IPC_RECV` ou bloquante `SYS_IPC_RECV_WAIT` avec delai optionnel, tache en `TASK_BLOCKED_IPC` jamais planifiee, reveillee par un message ou par IRQ0 a l'echeance), 4 entrées par tâche, charge de 96 octets et `request_id` corrélé ; capacité de deux messages clients et état borné pour un propriétaire de service, médiateur VFS, métadonnées et listage source-spécifiques, worker virtuel Ring 3 redécouvert à chaque requête, avec récupération corrélée bornée d'une transaction interrompue, génération, décision `stale`, pagination, vue locale `vfs-worker` de PID/absence, compteur de récupérations après retrait et compteur d'expirations après huit tours, quatre montages protégés et quatre alias dynamiques. `vfsvirtual` est l'autorité de validation, stockage et routage effectif des aliases **et** des I/O de montages protégés ; `vfsserver` n'en garde qu'un miroir de sélection purgé à toute issue incertaine. Les I/O d'alias et de montages protégés sont externalisées (`read`, `stat`, liste, pages et observation), sont réauthentifiées contre la table du worker et ne basculent jamais silencieusement sur le miroir. Chaque transaction worker reçoit un droit, une source et un préfixe relatif canonique minimal : le noyau le contrôle avant toute primitive portant un chemin ; les primitives FAT sans chemin ne passent que sous la racine. Le contrat QEMU force un renommage depuis un sous-répertoire vers un voisin hors préfixe, exige le refus backend corrélé avant mutation, puis vérifie la source intacte et l'absence de cible. Le contrat QEMU consulte aussi le worker suspendu pendant une lecture enfant et exige que la réponse publique soit une ligne exacte droit-source-requête corrélée, sans préfixe, chemin ni alias. |
| Découverte Foundation MOHHDY | Registre volatile de 8 services nommés ; retrait, transfert par propriétaire, révocation VFS, abonnements de service best-effort et nettoyage à la terminaison |
| Réseau | Pilote NE2000 ISA (sonde `0x300`, anneaux, IRQ3), codecs caller-owned ICMP (Echo Request/Reply, checksum)/Ethernet/ARP/IPv4/UDP/DHCP/DNS/TCP et TLS record ; `net-status` publie la NIC. Sur `qemu-ne2k-acquire`, un bail DHCP, l'ARP de prochain saut, DNS A, SYN, SYN-ACK, ClientHello, ServerHello minimal et ACK sont observés. `qemu-ne2k-tls-http` ajoute le handshake authentifié local et un POST HTTP 200 JSON ; `qemu-ne2k-tls-sse` ajoute un flux SSE chunked Ollama local ; `qemu-ne2k-tls-close` vérifie la fermeture `close_notify` réciproque authentifiée après le premier événement SSE ; `qemu-ne2k-tls-next` rearme la session et enchaine un second POST. `qemu-ne2k-tls-multipair` répète le cycle TLS/HTTP complet sur deux paires socket `127.0.0.1` isolées, avec MAC et journaux distincts, de manière séquentielle ; **pas** de TAP, DHCP public, TLS vers un hote Internet ni d'OpenAI réel. |

> Les poids GPT-2 ne sont pas versionnés dans Git. Une image utilisable sans réseau les embarque dans l'initrd au moment du build.

## Prochaines etapes de developpement

Constat de pilotage du 3 octobre 2026. Les pas 1 a 19 ci-dessous sont livres. Le pas 0 historique citait `make test-all` 648/648. Le rejeu de cette pointe est 675/675. `python3 tests/integration/test_qemu_ai_gguf.py` : exit 0 en 255 s. Le pas 0 : `make test-all` 648/648. `make qemu-foundation-steps` imprime `cap-token 1`, `id-key 2`, `task-metrics id` avec cette cle, `children` / `ps` / `task-metrics` du fils avec la cle 4, `spill-drops ok service 0 supervision 0` encore au second boot, `right-token demo 3` puis refus, et `right-token ata-driver` refuse. `make qemu-ata-driver` passe en 295,1 s, `make qemu-net-worker` en 209,3 s, `make qemu-net-wire` en 86,8 s. Ces quatre contrats et `make integration-qemu` (7/7 en 800,2 s) sont la pointe precedente. `make qemu-smoke` de cette pointe : six scenarios verts en 547,9 s apres l'image. Le `make` complet, relink compris, dure 676,3 s depuis `build/boot.o`. La mesure 800,6 s est anterieure. La CI distante du commit `e8c3ff6`, run 37094146840, a les quatre jobs verts, dont le smoke et les contrats d'integration. Elle couvre le code de cette tranche, pas ce commit de documentation. Le run `8264b62` / 37032910130 reste anterieur, comme le mur des poids residents `9999514`, run 37029710466.

| Ordre | Etat | Preuve |
|---:|---|---|
| 0 | tenu en local | `make test-all` : 648 passes, 0 echec. `make qemu-smoke` : six scenarios verts en 547,9 s apres l'image. `make qemu-foundation-steps` (pointe precedente) : `cap-token 1`, `id-key 2`, cle fils 4 dans `children`, `ps` et `task-metrics`, `spill-drops ok service 0 supervision 0` au second boot, `right-token demo 3` puis refus, `right-token ata-driver` refuse. `make qemu-ata-driver` 295,1 s, `make qemu-net-worker` 209,3 s, `make qemu-net-wire` 86,8 s. `make integration-qemu` : 7/7 en 800,2 s, pointe precedente. |
| 1 | livre | `us031_complete=false` dans `userspace/osui_runtime.c`, `userspace/osui_gui.c` et les tests Unity. `chromium=false` et `phase3_complete=false` restent. Ce n'est pas un moteur Chromium. |
| 2 | livre | `make qemu-ne2k-guest-tls-metier` : les deux invites impriment `METIER ok` (facture chiffree, sans TAP, sans Internet, sans secret). Les syscalls pair 128-130 du shell sont relays au `networker` Ring 3 pendant qu'il tient la NE2000. En CI depuis le 3 octobre 2026 sur le noyau strict par defaut : `make qemu-net-peer-tls-worker` (job `net-peer-tls`) verifie en plus que chaque appel 91/92/128/129/130 est servi par le worker (`net-driver relay op`, compteurs `net-relay-status` sans abandon), aucun refus Ring 0 apres le debut du shell, un record applicatif TLS dans chaque sens et aucun `METIER` en clair sur le fil ; PKI de test (racine et feuille du depot). |
| 3 | livre, pas sous 1 s | Avant optimisation : 1407,034 s puis 477,200 s. Apres PIO groupe : 52,319 s puis 23,227 s. Apres copie residente des poids (`fat16_load_resident`, 100 Mio, survit a `fat16_invalidate_caches`) : `BOOT_TIMEOUT=300 GGUF_KVM_REQUIRE=1 GGUF_BENCH_RUNS=1 GGUF_GENERATION_TIMEOUT=900 make gguf-kvm-benchmark` (exit 0). Mediane **43,916 s** puis **20,224 s**, dispersion 0. `sub_second_claim_allowed` est faux. Le guest imprime `metal` puis `enfranch`. Un essai a 64 secteurs par job ATA a ete retire : le repli FAT Ring 0 ecrivait alors le fichier dans l'overlay. Les jobs restent a 8 secteurs. Le TCG (48,739 s / 22,781 s) reste une autre mesure. Le defaut de trois runs n'est pas rejoue. |
| 4 | livre | Le jeton de capability et le `right_token` du nom sont deux compteurs. Enregistrer `ata-driver` ou `net-driver` ne consomme plus le jeton backend. Le premier grant vfs imprime `cap-token 1`. Preuves `test_backend_token_is_nonzero_unique_and_required` et `test_right_token_does_not_consume_backend_token`. Pas le microkernel US-001. |
| 5 | livre | `service-event-pull` rend le plus ancien evenement non accuse et pose `acked`. Le canal ne depend pas de la boite IPC de quatre entrees. Si l'envoi IPC echoue, la copie attend dans un deversoir de huit places, lu par `ipc-recv` apres la boite (`sender_pid` 0). Un deversoir plein rend `OS_IPC_FULL` et cette copie est perdue. Deux compteurs saturent chacun a `0xFFFFFFFF` : `OS_IPC_TASK_SUPERVISION_EVENT` incremente la supervision, tout autre type (dont `OS_IPC_SERVICE_EVENT`) incremente le service. `OS_IPC_BAD_MESSAGE` ne compte pas. `spill-drops` (syscall 152) imprime `service N supervision N`. `make qemu-foundation-steps` imprime `spill-drops ok service 0 supervision 0`. Les evenements encore non acquittes sont ecrits au LBA 4224 (magic `EVNT`), hors AIOV et hors volume FAT du fixture. Au boot suivant, `service-watch` rattache le nom au nouveau PID. Un volume FAT qui couvre ce LBA (disque GGUF) garde le pull en RAM. Le deversoir, lui, est en RAM. Pas un second journal disque. Preuve unitaire `test_event_journal_survives_without_ata_and_rebinds` et `test_ipc_spill_keeps_event_when_mailbox_is_full`. QEMU : `make qemu-foundation-steps` relit `keep` au second boot. |
| 6 | livre | Journal `MNTJ` aux LBA 4222-4223, distinct de l'overlay AIOV (LBA 0-63). `make qemu-foundation-steps` : `alias/` est encore la au second boot, avec `initrd/` et `overlay/`. |
| 7 | livre | Cle d'identite (`identity_key`) gravee a la creation de la tache, distincte du PID, de la sequence et de la generation, jamais nulle. Un PID reutilise qui recopie la sequence et la generation est refuse. Sans tache vivante, une cle stockee a 0 reste acceptee (tests sur PID nu). Le shell lit la cle de la tache courante avec `id-key` (syscall 151). `ps`, `task-metrics`, `children` et l'instantane d'enfants montrent sequence, generation et cle seulement pour soi ou un enfant direct (`identity_visible` 1). Ailleurs les trois temoins restent a 0. Un enfant ne voit pas la cle de son parent. Apres `task-delegate`, l'ancien parent ne les voit plus et le nouveau parent direct les voit. Apres la mort du parent, meme regle : pas 17. Le journal local les recopie a l'evenement : pas 18. Elle meurt avec la tache. `make qemu-foundation-steps` imprime `id-key 2`, puis la cle 4 du fils dans `children`, `ps` et `task-metrics`. Au second boot, `spill-drops` est encore `service 0 supervision 0` et `right-token ata-driver` est refuse. Preuves unitaires `test_identity_key_blocks_cloned_sequence`, `test_task_identity_visible_to_self_and_child` et `test_task_identity_follows_current_parent`. Pas un certificat. |
| 8 | livre | Le nom de service porte un `right_token` non nul et la cle du titulaire. Les ports ATA et l'entree reseau, quand `net-driver` est vivant, exigent ce droit (`service_right_held`). Un transfert de nom recopie la cle du beneficiaire. `right-token <nom>` (syscall 153) lit ce droit seulement pour le titulaire. Il reste distinct de `cap-token`, non nul, et il ne se transfere pas. Apres `service-grant`, l'ancien titulaire recoit `right-token: droit absent`. `make qemu-foundation-steps` imprime `right-token demo 3`, different de `cap-token 1`. Sans `net-driver`, le mode degrade reste ouvert a tous. Pas US-001. |
| 9 | livre | `atadriver` et `networker` restent Ring 3 au runtime, et leurs ports passent par le droit du pas 8. Le programme des registres NE2000 (anneaux, TX, RX) est deja dans `networker`. Restent Ring 0 : montage ATA au boot, repli sans worker, sonde de presence de la carte, IRQ NIC, miroir de la session GGUF, normalisation du prompt, repli FP32 GPT-2 sans `aiworker` (la generation FP32 tourne en Ring 3 quand `aiworker` est vivant, inventaire point 4), et les pilotes PIC, PIT, PS/2, VGA, serie, PCI et `usb_tablet`. Le repli tokenizer/session des pas 109-110 est retire par defaut : pas 19. |
| 10 | livre | Les notifications de supervision (sortie, suspension, reprise, delegation) attendent dans le meme deversoir de huit places si la boite est pleine. Une livraison reussie par la boite ou par le deversoir compte comme remise. La perte n'est comptee que si les deux sont pleins, et elle incremente le compteur supervision, pas le compteur service. Le journal local de quatre places reste. Le tampon d'identite de ce journal est le pas 18. L'avis IPC reste sur 24 octets. |
| 11 | livre | Registre des commandes guest : 206 noms, dont `id-key`, `spill-drops` et `right-token`. |
| 12 | livre | Visibilite de l'identite : soi et enfant direct seulement. Voir le pas 7. |
| 13 | livre | Lecture shell du droit de nom : `right-token`, syscall 153. Voir le pas 8. |
| 14 | livre | Compteurs de pertes separes, service et supervision. Voir le pas 5. Au second boot ils sont encore a zero. |
| 15 | livre | `children` imprime ` id SEQ GEN KEY` pour chaque enfant direct. `children ok N` reste. `make qemu-foundation-steps` : la cle du `vfsserver` est 4 dans `children`, `ps` et `task-metrics`. |
| 16 | livre | Apres `task-delegate`, la cle suit le parent courant. Preuve `test_task_identity_follows_current_parent`. |
| 17 | livre | Apres la mort du parent, la cle suit le nouveau parent direct. Un etranger ne la voit pas. Un petit-enfant reste cache tant que son parent intermediaire vit. Un orphelin dont le parent vaut -1 ne montre pas sa cle a un tiers. Preuve `test_task_identity_follows_parent_death`. Pas un certificat. La cle meurt avec la tache. |
| 18 | livre | Le journal RAM du parent copie `child_sequence`, `child_generation` et `identity_key` au moment de l'evenement. Les lignes `task-event` ajoutent ` id SEQ GEN KEY` apres les ticks. `task-events ok GEN COUNT` ne change pas. L'afficheur `ipc-recv` ne change pas. L'avis IPC reste 24 octets et `os_task_parse_supervision_event` remet les trois temoins a zero. `make qemu-smoke` : suspend et exit de `sleeper` portent `id 5 1 4`, exit de `waitchild` porte `id 6 2 5`, y compris apres la mort et dans `task-events-observe`. Preuve `test_task_supervision_journal_keeps_child_identity`. Pas un second journal disque. |
| 19 | livre | Instantane GGUF en pages PMM a la taille du fichier (plus de BSS de 100 Mio). `GGUF_READY` imprime `[AI] GGUF resident released pages N` et rend les pages. La lecture bulk est `fat16_read_file_range_disk`. Defaut `GGUF_RING0_FALLBACK=0` : 109/110 rendent -148, chemin `none`, sans worker GGUF ou si le job est abandonne. `kernel` et `ring0` restent 0. Normalisation et miroir restent Ring 0. Repli FP32 inchange. `make test-all` 675/675. Contrat local 255 s : 983 pages, jetons worker `0 1 2 14 3 4 \| 1 14 6 14 0 5 13 7`, reprise `resumed` 1, fwd 33 done 32 kernel 0 fallback 0 aborted 1 rogue 1, session worker 32 ring0 0, libres 257599/257599. Pas de rejeu KVM. Pas de CI distante de cette pointe. |

Phases 4 a 8 : ouvertes le 10 octobre 2026 avec l'accord ecrit de l'utilisateur, en tres gros lots. Phase 4 (PromptMessage) livree (PR #116, voir [promptmessage.md](promptmessage.md)). Phase 5 (P2P, US-061 a US-075) : noeud P2P invite (`userspace/p2p.c`, commandes `p2p-*`) sur datagrammes UDP diffuses via `SYS_PEER_DATA` `OS_PEER_P2P_SEND/RECV`, relayes au `networker` Ring 3 sur le noyau strict ; decouverte HELLO authentifiee, liens chiffres par paire (X25519 + AES-128-GCM), replication LWW, synchronisation anti-entropie, routage multi-sauts avec couts de chemin (`p2p-route`, US-063), consensus Raft (`p2p-raft`, US-064), simulation gRPC (`p2p-grpc`, US-067), cache distribue TTL (`p2p-cache`, US-070), traversée NAT / STUN virtuel (`p2p-stun`, `p2p-punch`, US-074), analyse de trafic (`p2p-analyze`, US-075). Preuves : `tests/unit/userspace/test_p2p.c`. Detail dans `docs/p2p.md`.

Phase 6 (multi-plateforme, US-076 a US-090) : livre dans l'invite i386 (`userspace/platform.c`) : rapport HAL (`hal-info`), adaptation d'ecran et redimensionnement dynamique (`screen-adapt`, `screen-resize`, US-080), registre de capteurs simules (`sensor-read`, US-078), gestes tactiles simple et multi-pointeur (`gesture`, `gesture-multi`, US-079), profils d'energie (`power-profile`, `power-status`, US-081), file de notifications push avec TTL (`notify-push`, US-082), protocole de continuite inter-appareils (`continuity-export`, `continuity-import`, US-084), gestionnaire de peripheriques avec regles de hotplug (`dev-list`, `dev-rules`, `dev-hotplug`, US-085), couche d'emulation legacy (`legacy-emul`, US-086), compatibilite ELF (`compat-check`), vue d'administration multi-noeuds (`admin-all`, `admin-nodes`, `admin-node-add`, US-089), migration ramfs (`migrate-*`), deploiement (`deploy-*`). Preuves : `tests/unit/userspace/test_platform.c`. Detail dans `docs/platform.md`.

Phase 7 (collaboratif, US-091 a US-105) : registre local replique sur le noeud P2P (`userspace/collab.c`) : points (`collab-pay`), conversion de points et sequestres avances (`collab-pay-convert`, `collab-escrow-*`, US-097), offres et reservations avec recherche marketplace, encheres et calcul de prix moyen (`collab-market-*`, US-094), taches distribuees avec sequestre (`collab-task`), moteur de modeles de contrats intelligents (`collab-contract-*`, US-099), flux d'arbitrage de litiges (`collab-dispute-*`, US-101), preuves de confidentialite ZK et anonymisation (`collab-privacy-proof`, US-103), rapports d'audit de conformite GDPR (`collab-compliance`, US-104), support communautaire avec routage P2P (`collab-ticket-*`, US-105), reputation (`collab-rate`), gouvernance (`collab-vote`), profil (`collab-profile`), signatures Schnorr par noeud (`collab-keys`). Preuves : `tests/unit/userspace/test_collab.c`. Detail dans `docs/collab.md`.

Phase 8 (production, US-106 a US-120) : outils `prod-*` dans le shell invite (`userspace/prod.c`) : metriques echantillonnees, alertes a seuil avec apprentissage (`prod-alert-learn`, US-114), moniteur de performance consolide (`prod-perf-monitor`, US-106/107), simulation de pipeline CI/CD (`prod-cicd`, `prod-pipeline`, US-108), autoscaling avance avec chargeur de travail (`prod-autoscale-workload`, US-110), scanner de securite et de vulnerabilites (`prod-sec-scan`, US-112), auto-tuning (`prod-perf-tune`, US-106), maintenance predictive (`prod-maint-predict`, US-115), bundle support (`prod-support-bundle`, US-116), tutoriel (`prod-tutorial`, US-117), kpi metriques (`prod-biz-metrics`, US-119), matrice roadmap (`prod-roadmap`, US-120). Preuves : `tests/unit/userspace/test_prod.c`. Detail dans `docs/production.md`. Lot 7 : plusieurs de ces sorties etaient figees (pipeline, scanner, MTBF, bundle, utilisateurs actifs) ; elles sont remplacees par du code reel ou marquees partielles, voir l encadre lot 7 et `docs/lot7_inventory.md`.

Lot de consolidation (PR #121, empile sur #120 avec #119 et #117 fusionnes) : persistance sur disque (`persist-save/load/status` : etat production, registre collab avec cles, graine du noeud) via l'overlay vidange par le pilote Ring 3 `atadriver`, prouvee par deux demarrages sur la meme image (`make qemu-persistence`) ; mise a l'echelle qui lance et arrete de vraies taches (`prod-scale-run`, programme `idle`) ; signatures par noeud des entrees collab (Schnorr RFC 5114 1024/160, confiance au premier usage), entrees forgees rejetees dans le contrat trois invites (`make qemu-collab`). Detail : `docs/production.md`, `docs/collab.md`. Suite du lot : sauvegarde automatique (60 s par defaut avec disque, blobs modifies seulement) et a l'arret propre (`shutdown`/`reboot`/`exit`), cle de signature chiffree au repos (PBKDF2-SHA256 + AES-128-GCM, `persist-passphrase`/`persist-unlock`, verrouillee sinon), magasin cle/valeur P2P persiste (US-069), analyse automatique du trafic `p2p-analyze` (US-075), certificats PromptMessage signes par la cle du noeud (US-060). Toujours absents : NAT, gRPC, paiement reel, autorite de certification.

Hors de cette file, ne pas ouvrir : TensorFlow Lite, comprehension du langage a 90 %, facturation, corps physique, OpenAI public (sauf accord ecrit, secret hors image, hors CI), ecrasement FAT, renommage inter-repertoire, remplacement atomique.

`networker` est lance au boot quand la sonde NE2000 a vu une carte. Le shell qui redemande `spawn networker` recoit le PID deja vivant. Le tuer rend la carte au Ring 0. Restent Ring 0, par conception : montage ATA au boot, repli sans `atadriver`, repli sans `networker` sur le noyau historique, sonde de presence de la carte au boot. Pour les syscalls 109-110, restent Ring 0 la normalisation du prompt et le miroir de session. Le defaut refuse ces syscalls sans worker GGUF (-148) au lieu d'y tokeniser ([ai_worker_gguf.md](ai_worker_gguf.md)). Le repli FP32 `SYS_GPT2_GENERATE` quand `aiworker` est absent reste (la generation FP32 est servie en Ring 3 par `aiworker` quand il est vivant, [ai_worker_ring3.md](ai_worker_ring3.md)). Sans pilote Ring 3, restent aussi les pilotes PIC, PIT, PS/2, VGA, serie, PCI et `usb_tablet`. Le chemin noyau des syscalls pair 128-130 reste refuse (`-141`) s'il n'a pas ete relaye ; le shell, lui, passe par le worker, qui execute ces appels dans sa pile Ring 3.

L'ACL prefixee (garde 1) est a tenir, pas a reouvrir : preuves de voisin, de racine et de voie sans chemin ; le diagnostic public reste droit-source et ne divulgue pas le prefixe interne.

## Fonctions livrées

### GGUF et kernels de quantification

Le runtime possède des produits scalaires freestanding pour les super-blocs GGML **Q3_K**, **Q4_K** et **Q6_K**, en plus de Q8_0. Les blocs sont contrôlés sur 256 valeurs et leurs tailles binaires sont explicites : 110, 144 et 210 octets. Le parseur GGUF classe séparément les tenseurs Q3_K, Q4_K et Q6_K au lieu de les signaler comme types quantifiés inconnus. `gpt2_gguf_find_tensor` vérifie la forme, le type, la taille de bloc et la plage avant utilisation.

Le lot 69 ajoute `gpt2_gguf_build_index`, une table caller-owned bornée à 512 tenseurs, puis `gpt2_gguf_index_find` pour les recherches répétées sans rescanner les métadonnées. `gpt2_gguf_map_role` associe les cinq tenseurs structurants GPT-2 (`token_embd`, `position_embd`, `output_norm` et `output`) à des rôles stables pour le futur forward quantifié. La suite `make test-all` atteint désormais **263 tests réussis**, dont les fixtures d'index, de mapping et de chargement FAT16.

Le lot 70 ajoute `gpt2_gguf_load_fat16`, qui lit un profil GGUF dans un buffer caller-owned depuis le volume FAT16, puis construit l'index et vérifie le rôle `output.weight`. La fixture de bout en bout utilise le nom FAT16 8.3 `gpt2.ggu`, charge 320 octets et traverse le montage, la chaîne de clusters, le parseur et le mapping sans allocation dynamique.

Le lot 71 ajoute `fat16_read_file_range`, qui saute jusqu'au cluster demandé et copie uniquement la fenêtre utile dans un buffer caller-owned. Le test lit `ell` au milieu de `FATOK.TXT` et rejette un offset hors taille; `make test-all` atteint désormais **264 tests réussis**, sans échec ni test ignoré. Cette primitive prépare la lecture paginée des tenseurs GGUF, mais le forward GPT-2 réel n'est toujours pas annoncé.

Le lot 72 ajoute `fat16_open_file` et `fat16_file_read`, un curseur caller-owned qui conserve cluster, position et garde FAT entre lectures successives. Le test lit `hel`, puis `lo`, puis la fin de `FATOK.TXT`; `make test-all` atteint désormais **265 tests réussis**, sans échec ni test ignoré. Le curseur prépare la lecture progressive des fenêtres GGUF, mais ne fournit pas encore de seek ou de forward GPT-2 complet.

Le lot 73 ajoute `gpt2_gguf_read_tensor_fat16`, qui calcule l'offset absolu d'un tenseur indexé, contrôle les additions 32-bit et lit une fenêtre plafonnée via FAT16. La fixture Q4_K vérifie la lecture de quatre octets depuis `GPT2.GGU`; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Le lot 74 ajoute `gpt2_gguf_read_quant_block_fat16`, qui refuse les types non K-quant, calcule les tailles Q3_K/Q4_K/Q6_K et lit un super-bloc complet avec une capacité vérifiée. La fixture Q4_K valide 144 octets depuis `GPT2.GGU` et rejette une capacité insuffisante; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Le lot 75 ajoute `gpt2_gguf_dot_quant_block_fat16`, qui lit un super-bloc Q3_K/Q4_K/Q6_K dans un scratch caller-owned puis appelle le kernel quantifié avec exactement 256 activations. Le test Q4_K vérifie un produit nul sur `GPT2.GGU` et rejette une longueur invalide; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Le lot 76 ajoute `gpt2_gguf_dot_quant_tensor_fat16`, qui lit et accumule plusieurs super-blocs Q3_K/Q4_K/Q6_K avec un scratch caller-owned. La fixture Q4_K contient deux blocs pour 512 valeurs et vérifie une accumulation nulle; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Le lot 77 ajoute `gpt2_gguf_dot_quant_row_fat16`, qui valide une forme 1D/2D, exige `count == shape[0]`, calcule l'offset de ligne en 64-bit et accumule les super-blocs depuis FAT16. La fixture Q4_K valide une ligne 1D de 512 valeurs et rejette une seconde ligne; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Le lot 78 valide une forme GGUF 2D pour `output.weight` (`shape[0]=512`, `shape[1]=1`). `gpt2_gguf_dot_quant_row_fat16` contrôle `row_index`, calcule l'offset de ligne borné et réutilise l'accumulation Q4_K; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Le lot 79 transforme la fixture en matrice Q4_K multi-lignes `shape[0]=256`, `shape[1]=2`. Les lignes 0 et 1 sont lues depuis FAT16, la ligne 2 est rejetée et l'accumulation des deux blocs reste validée; la suite reste à **265 tests réussis**, sans échec ni test ignoré.

Les kernels GGUF Q3_K/Q4_K/Q6_K, un index et un mapping de couches existent ; le chemin ne réalise pas encore le parcours complet d'un checkpoint réel ni une génération autoregressive. Les valeurs de fixture sont nulles et aucune latence inférieure à une seconde n'est annoncée sans mesure native ou KVM reproductible.


### Noyau, stockage et tâches

Le noyau configure GDT, IDT, PIC 8259, PIT 100 Hz, i8042 PS/2, PMM/VMM/heap, paging, chargement ELF 32-bit, syscalls et initrd TAR. L'EOI de l'IRQ0 est envoyé **avant** le gestionnaire C : lorsqu'un changement de contexte effectue un `iret` sans revenir dans le stub, IRQ1 clavier n'est donc pas laissée bloquée.

La console VGA 80x25 programme le curseur matériel (bloc clignotant) à la position de saisie. Un historique de 80 lignes retient les lignes éjectées par le défilement automatique. **Page Up** / flèche haut remontent d'un écran ou d'une ligne ; **Page Down** / flèche bas redescendent. Une écriture à l'écran ramène à la vue courante. Ce n'est pas un terminal Unix : pas de scrollbar, pas de sélection souris.

L'overlay RAM persistant utilise le format snapshot **AIOV V2** : 64 nœuds, chemins de 80 octets, contenu de 384 octets et 64 secteurs ATA PIO (LBA 0-63). `overlay_restore()` reconnaît également le format V1 afin de restaurer les images déjà créées. Le volume FAT16 est monté à partir du LBA 64, avec BPB, table d'allocation et chaînes de clusters bornés ; FAT32 est monté sur le second IDE de test. Les deux volumes publient les LFN à la racine et un seul niveau de sous-répertoire 8.3 via le VFS, sans écrasement, LFN enfant, niveau imbriqué ni remplacement atomique. Conception : [aos_fat_volume.md](aos_fat_volume.md) ; livraison initiale : [mohhdy_foundation_increment_68_fat16_volume.md](mohhdy_foundation_increment_68_fat16_volume.md).

Les appels `spawn`, `yield` et `exec` continuent de changer de contexte depuis le cadre utilisateur de `int 0x80`. En complément, IRQ0 déclenche un round-robin toutes les 20 interruptions uniquement si le cadre interrompu est Ring 3 et si une **autre** tâche utilisateur est `READY`. Cela évite les cadres noyau incomplets et empêche un quantum inutile en mono-tâche. Le contrat QEMU lance `spin`, une boucle utilisateur sans syscall, puis exige une nouvelle commande du shell : la réactivité obtenue démontre la préemption réelle.

Le premier incrément MOHHDY ajoute une boîte aux lettres **IPC Foundation** par tâche utilisateur. `SYS_IPC_SEND` transmet un payload borné de 96 octets vers un PID utilisateur valide et le noyau inscrit lui-même le PID d'émetteur ; `SYS_IPC_RECV` retire le plus ancien message de la FIFO. Chaque endpoint tient quatre messages, rejette explicitement la saturation et ne bloque jamais le destinataire. Après un envoi réussi, le noyau réalise un handoff coopératif lorsqu'une autre tâche utilisateur est prête : cela permet au destinataire de traiter le message avant que le shell ne retourne dans `SYS_GETS`, cadre Ring 0 non préemptable par IRQ0. Le contrat QEMU lance `ipcserver`, envoie `bonjour`, vérifie l'émetteur puis confirme une boîte aux lettres vide. Ce mécanisme prépare l'externalisation ultérieure des services ; il ne constitue ni un microkernel ni un IPC à capabilities.

Le deuxième incrément construit un premier **médiateur VFS Ring 3** sur cet endpoint. `vfsserver` reçoit une requête `OS_IPC_VFS_READ`, valide un chemin NUL-terminé de 47 octets utiles au plus et refuse `..`, lit au plus 80 octets via l'ABI existante, puis répond au PID expéditeur avec statut et données. Le backend initrd/overlay/ATA demeure néanmoins noyau, et un client peut encore appeler `SYS_READFILE` directement : il s'agit d'un médiateur de politique, non d'un VFS isolé.

Le troisième incrément retire le PID codé en dur de ce parcours. `SYS_SERVICE_REGISTER` associe un nom de 15 octets utiles au plus au PID de l'appelant Ring 3 et `SYS_SERVICE_LOOKUP` retourne le PID d'un propriétaire utilisateur vivant. `vfsserver` publie `vfs`, puis `vfs-read <chemin>` le résout avant l'envoi IPC. Le registre contient huit entrées, refuse les noms invalides et les conflits actifs. Le client VFS cède au plus trois fois le CPU puis relit sa boîte aux lettres, ce qui préserve un protocole non bloquant tout en absorbant le handoff d'ordonnancement.

Le quatrième incrément fixe le cycle de vie : `SYS_SERVICE_UNREGISTER` ne retire qu'un nom détenu par son appelant Ring 3 ; `SYS_EXIT` et `SYS_KILL` retirent toutes les entrées du PID avant la terminaison ou le retrait de la tâche. La purge lazy à la recherche demeure une défense secondaire. Le registre ne confère toujours aucun droit : tout processus utilisateur peut tenter de réserver un nom libre, les capabilities, badges et registre persistant restent absents.

Le cinquième incrément ajoute `request_id` à `os_ipc_payload_t` et `os_ipc_message_t`, sans modifier la charge utile de 96 octets ni les numéros de syscall. La FIFO noyau le copie de bout en bout. `vfs-read` génère un identifiant non nul monotone par session, et `vfsserver` le recopie dans la réponse ; le parseur VFS refuse type, taille ou identifiant discordant. Le client retire et ignore une réponse non corrélée pendant sa fenêtre de trois cessions CPU, ce qui prépare la gestion de plusieurs requêtes mais ne conserve pas encore les messages ignorés ni n'offre un RPC bloquant.

Le sixième incrément introduit `SYS_SERVICE_GRANT` : le propriétaire courant d'un nom peut transférer ce nom à une tâche Ring 3 vivante. Le registre remplace son PID en place, et le nettoyage à `exit` ou `kill` s'applique ensuite au nouveau propriétaire. Un handoff coopératif laisse le bénéficiaire constater immédiatement le transfert avant que le shell ne retourne dans son attente Ring 0. Cela reste un transfert unique, volatile et sans secret : l'ancien propriétaire ne peut plus le révoquer, la découverte demeure publique et aucun token de capability, contrôle d'identité ou audit n'est fourni.

Le septième incrément conserve les messages non corrélés retirés de l'endpoint du shell : `vfs-read` les place dans une file Ring 3 de quatre entrées, puis `ipc-recv` les restitue FIFO. Une réponse VFS dont le type et le `request_id` correspondent est extraite directement de cette file avant toute attente. La capacité est strictement bornée : une saturation renvoie une erreur au lieu d'écraser un message, et aucune file partagée, persistance ou attente bloquante n'est ajoutée.

Le huitième incrément ajoute `vfs-info`, une source synthétique servie directement par `vfsserver` Ring 3 sans `SYS_READFILE`. Il déplace ainsi une première politique de chemin et la construction de sa réponse hors du noyau. Les autres chemins sûrs restent relayés vers le backend initrd/overlay noyau ; il ne s'agit ni d'un montage général ni d'une externalisation des pilotes ou du stockage ATA.

Le neuvième incrément réserve `SYS_VFS_BACKEND_READ` au propriétaire utilisateur vivant du nom `vfs`. `vfsserver` l'utilise pour ses chemins ordinaires et le shell reçoit `OS_VFS_BACKEND_DENIED` par la commande de diagnostic. Cette voie isole le médiateur du syscall générique pour le protocole VFS, sans retirer `SYS_READFILE` des commandes historiques, sans capabilities ni externalisation du backend initrd/overlay.

Le dixième incrément permet à `vfsserver` de transférer son propre nom via une requête IPC `OS_IPC_VFS_GRANT` et la commande shell `vfs-grant <pid>`. Le contrôle backend suit immédiatement le nouveau propriétaire : `vfsclaim` prouve la lecture autorisée, tandis que l'arrêt de l'ancien serveur ne supprime plus le nom. L'arrêt du nouveau propriétaire le purge. Cette révocation repose sur un PID de registre volatile, pas sur un token de capability, une identité ou une autorité de révocation indépendante.

Le onzième incrément ajoute un registre de montages borné au médiateur. `vfsserver` déclare actuellement le préfixe `initrd/`, l'expose en lecture virtuelle via `vfs-read vfs-mounts` et délègue seulement le suffixe relatif d'un chemin correspondant au backend réservé. Ainsi `vfs-read initrd/hello.txt` atteint `hello.txt`, tandis que `vfs-read hello.txt` retourne `OS_VFS_STATUS_NOT_MOUNTED` et un diagnostic de chemin hors montage. Les sources `vfs-info` et `vfs-mounts` sont toujours produites en Ring 3. La liste est statique, ne monte ni autre backend ni répertoire, et le backend initrd/overlay/ATA reste noyau.

Le douzième incrément ajoute `SYS_SERVICE_NOTIFY` et huit abonnements `(nom, PID)` au plus. Lors d'une publication, d'un transfert, d'un retrait ou d'une purge, le noyau émet un `OS_IPC_SERVICE_EVENT` vers chaque abonné actif avec `sender_pid = 0`, l'ancien PID, le nouveau PID et une raison bornée. Le shell offre `service-watch <nom>` et rend l'événement visible par `ipc-recv`; `serviceclaim` s'abonne à `demo` au lieu de retenter continuellement l'inscription. Cette livraison est best-effort : la boîte IPC de quatre entrées peut être saturée, sans bloquer le registre ni garantir la reprise de l'événement.

Le treizième incrément ajoute `SYS_VFS_BACKEND_WRITE`, réservé au propriétaire vivant du nom `vfs`, et le protocole `OS_IPC_VFS_WRITE`. `vfsserver` annonce désormais `initrd/ ro` et `overlay/ rw`. La commande `vfs-write overlay/nom.txt texte` valide le préfixe, transmet seulement `nom.txt` au backend overlay et exige une réponse corrélée. Une écriture vers `initrd/` ou hors montage est refusée par le médiateur ; le shell ne peut pas appeler directement la voie backend réservée. Chaque requête transporte au plus 44 octets, l'overlay/ATA reste noyau, et le syscall historique d'écriture demeure accessible hors du protocole VFS.

Le quatorzième incrément sépare les lectures backend par source. `SYS_VFS_INITRD_READ` sert exclusivement l'archive initrd pour le montage `initrd/`, tandis que `SYS_VFS_OVERLAY_READ` sert exclusivement l'overlay ATA pour `overlay/`. Les deux voies sont réservées au propriétaire de `vfs`. Le médiateur ne délègue plus les lectures montées au backend générique à repli implicite ; `vfs-read overlay/nom.txt` lit ainsi un fichier créé par `vfs-write` depuis l'overlay, alors que `vfs-read initrd/nom.txt` ne consulte jamais l'overlay.

Le quinzième incrément ajoute `SYS_VFS_OVERLAY_UNLINK`, réservé au propriétaire de `vfs`, ainsi que `OS_IPC_VFS_REMOVE`. `vfs-remove overlay/nom.txt` retire seulement le suffixe relatif `nom.txt` de l'overlay. Une suppression sous `initrd/` ou hors montage est refusée avant l'appel backend ; la commande `vfs-backend-remove-probe` démontre que le shell n'accède pas directement à cette primitive. La réponse est corrélée au `request_id`, mais l'opération reste non transactionnelle et les erreurs d'overlay sont renvoyées au client.

Le seizième incrément ajoute `SYS_VFS_OVERLAY_RENAME` et `OS_IPC_VFS_RENAME`. Les deux chemins de 48 octets remplissent exactement la charge IPC de 96 octets et doivent appartenir au même montage `overlay/`. `vfs-rename overlay/ancien.txt overlay/nouveau.txt` ne transmet donc que les deux suffixes relatifs au backend réservé. Les changements inter-montages ou les chemins non sûrs sont refusés ; `vfs-backend-rename-probe` prouve que le shell ne peut pas appeler la primitive directe. La réponse transporte le statut et le `request_id` originel.

Le dix-septième incrément ajoute des compteurs locaux d'observabilité dans `vfsserver`, exposés sans nouveau syscall par la source virtuelle `vfs-stats` et la commande `vfs-stats`. Les compteurs 32 bits `reads`, `writes`, `removes` et `renames` sont remis à zéro au démarrage du serveur. Toute requête VFS reconnue est comptée avant validation, de sorte que les refus de politique et l'interrogation `vfs-stats` elle-même restent visibles. Ces compteurs ne sont ni persistants, ni atomiques, ni des métriques de sécurité ou de performance.

Le dix-huitième incrément remplace la liste de montages fixe par une table locale de cinq entrées : `initrd/` et `overlay/` sont protégés, auxquels peuvent s'ajouter trois alias dynamiques de 13 octets utiles au plus. Les requêtes IPC `OS_IPC_VFS_MOUNT_ADD` et `OS_IPC_VFS_MOUNT_REMOVE`, et leurs réponses corrélées, restent dans le payload de 96 octets et n'ajoutent aucun syscall. `vfs-mount-add <prefixe/> <initrd|overlay>` installe un alias non recouvrant ; `vfs-mount-remove <prefixe/>` le retire. La source `initrd` reste lecture seule, tandis qu'un alias de source `overlay` permet les mutations VFS existantes. La source virtuelle `vfs-mounts` est générée depuis la table. Les entrées sont volatiles, non persistantes et ne transfèrent pas les aliases lors d'un nouveau serveur `vfs`.

Le dix-neuvième incrément réserve deux emplacements clients de FIFO à toute tâche Ring 3 qui possède au moins un nom de service publié. Le troisième envoi client retourne `OS_IPC_SERVICE_FULL` (`-44`), que le shell affiche comme une capacité de service atteinte ; une tâche non publiée conserve la limite brute de quatre messages et `OS_IPC_FULL`. La politique est appliquée dans `sys_ipc_send` après validation de la cible, sans nouveau syscall et sans modifier la livraison best-effort des notifications synthétiques noyau. Elle reste globale au PID propriétaire, volatile et sans priorité ni isolation par nom.

Le vingtième incrément ajoute `SYS_SERVICE_STATUS` (36). `service-status <nom>` remplit un instantané public de 16 octets : PID propriétaire, profondeur totale de FIFO, limite client de deux et capacité brute de quatre. Un nom absent ou dont le propriétaire n'est plus vivant retourne `OS_SERVICE_NOT_FOUND`. Cette lecture n'est ni atomique, ni une réservation, ni une capability ; les notifications noyau best-effort peuvent contribuer à la profondeur observée.

Le vingt-et-unième incrément ajoute `SYS_VFS_INITRD_STAT` (37) et `SYS_VFS_OVERLAY_STAT` (38). `vfs-stat <chemin>` transporte une requête VFS corrélée de 48 octets et retourne `status`, taille et drapeaux dans une réponse de 12 octets. Le médiateur utilise exclusivement la source déclarée du montage, y compris un alias dynamique. Un chemin hors montage est refusé ; cet instantané ne fournit ni verrouillage, ni propriétaire, ni horodatage, ni métadonnées persistantes pour les sources virtuelles.

Le vingt-deuxième incrément ajoute `SYS_VFS_INITRD_LISTDIR` (39) et `SYS_VFS_OVERLAY_LISTDIR` (40), réservés au propriétaire vivant de `vfs`. `vfs-list <prefixe/>` liste la racine dans la source unique associée : aucun repli ni mélange initrd/overlay n'est effectué, y compris pour un alias dynamique. La réponse corrélée de 80 octets porte un statut, au plus quatre noms séparés par `\n` dans 72 octets et un compte. Un statut `OS_VFS_STATUS_TRUNCATED` signifie qu'une page ne montre pas toutes les entrées ; il n'existe pas encore de curseur de pagination.

Le vingt-troisième incrément étend ce même protocole, sans nouveau syscall ni nouveau type IPC, à `vfs-list <repertoire/>`. Le chemin sûr doit se terminer par `/` et peut désigner la racine d'un montage ou un sous-répertoire, par exemple `initrd/bin/`. Le serveur retire le préfixe de montage, transmet exclusivement ce suffixe au backend de la source associée et vérifie qu'il désigne effectivement un répertoire via `initrd_is_dir` ou `overlay_is_dir`. Un fichier suffixé par `/`, un chemin hors montage ou un `..` est refusé ; aucune fusion entre initrd et overlay n'est introduite. La page demeure limitée à quatre noms, sans ordre contractuel ni instantané atomique.

Le vingt-quatrième incrément ajoute `OS_IPC_VFS_LIST_PAGE` et `OS_IPC_VFS_LIST_PAGE_REPLY`, ainsi que `SYS_VFS_INITRD_LISTDIR_PAGE` (41) et `SYS_VFS_OVERLAY_LISTDIR_PAGE` (42). `vfs-list-page <repertoire/> <depart>` transporte un index logique et retourne au plus quatre noms, un index suivant ou la sentinelle de fin. Le routage reste strictement source-spécifique ; le curseur est volatil, non persistant et non atomique face aux mutations.

Le vingt-cinquième incrément ajoute `OS_IPC_VFS_LIST_OBSERVE` et sa réponse corrélée. `vfs-list-observe <repertoire/> <depart> <generation>` reçoit une génération volatile de 32 bits ; une génération attendue non nulle et différente retourne `OS_VFS_STATUS_STALE` avec la génération actuelle. Celle-ci est incrémentée après les mutations VFS et montages réussis, sans verrouillage, persistence ni snapshot.

Le vingt-sixième incrément ajoute `vfs-mkdir <chemin>`, `OS_IPC_VFS_MKDIR` et `SYS_VFS_OVERLAY_MKDIR` (43). La requête de 48 octets est corrélée et le serveur ne route que les préfixes overlay déclarés vers `overlay_mkdir`; initrd et les chemins hors montage sont refusés. Une création réussie incrémente la génération volatile de page.

Le vingt-septième incrément ajoute `vfs-rmdir <chemin>`, `OS_IPC_VFS_RMDIR` et `SYS_VFS_OVERLAY_RMDIR` (44). La requête de 48 octets et la réponse de statut de quatre octets sont corrélées. Le médiateur ne transmet que le suffixe relatif d'un montage overlay déclaré; initrd et les chemins hors montage sont refusés. Le syscall backend vérifie que la cible est un répertoire avant d'appeler `overlay_unlink`, ce qui sépare strictement cette opération de `vfs-remove` pour les fichiers. Un répertoire non vide conserve le refus `OV_ERR_NOTEMPTY` (`-5`) et une suppression réussie invalide la génération de listage. Les compteurs historiques `removes` ne sont pas incrémentés, car ils comptent seulement les requêtes de suppression de fichiers.

Le vingt-huitième incrément ajoute `SYS_SERVICE_BACKEND_GRANT` (45), `vfs-backend-grant <pid>` et un droit backend VFS noyau borné à quatre bénéficiaires. `vfsserver` reste propriétaire du nom public `vfs` et délègue par une requête IPC corrélée un droit aux primitives backend, démontré par `vfscapclaim`. Une délégation est retirée au transfert ou retrait du nom, à la purge du propriétaire et à la terminaison du bénéficiaire. Ce droit est volatile et global à toutes les primitives backend VFS : il n'est ni une identité vérifiée, ni une ACL par chemin, ni une capability persistante ou transférable.

Le vingt-neuvième incrément ajoute `SYS_SERVICE_BACKEND_REVOKE` (46) et `vfs-backend-revoke <pid>`. Seul le propriétaire public vivant de `vfs` peut retirer une délégation backend active pour un PID donné. La requête IPC et sa réponse sont corrélées; une délégation absente ou un mandant différent est refusé sans toucher au nom public. `vfscapclaim` confirme la perte immédiate d'accès après révocation. Cette opération reste un droit local volatile par PID, sans ACL, identités vérifiées, durée, journal durable ni permission par chemin.

Le trentième incrément ajoute `SYS_SERVICE_BACKEND_GRANT_SCOPED` (47), `vfs-backend-grant-read <pid>` et un profil backend lecture seule. Le registre conserve un masque par délégation : lecture, `stat` et listage demandent le droit lecture; écriture, suppression, renommage, `mkdir` et `rmdir` demandent le droit mutation. Le propriétaire public de `vfs` garde tous les droits. L'octroi historique reste complet et la révocation/purge retire l'entrée entière. Le client `vfsreadclaim` prouve en QEMU qu'une lecture est admise tandis qu'une écriture backend est refusée. Le droit reste global par opération, sans ACL par chemin, durée, identité vérifiée ni journal durable.

Le trente-et-unième incrément ajoute `vfs-backend-grant-mutate <pid>`, qui emploie le masque scoped mutation (`2`) déjà livré. Ce profil admet écriture, suppression, renommage, `mkdir` et `rmdir` backend, mais refuse lecture, métadonnées et listage. `vfsmutateclaim` prouve ce comportement en refusant la lecture, créant puis supprimant un fichier de démonstration. Le contrat QEMU arrête explicitement chaque client de preuve après son assertion pour libérer les emplacements de tâches avant le transfert VFS final. La granularité reste par opération globale, sans ACL par chemin, identité vérifiée, expiration, quota ou journal durable.

Le trente-deuxième incrément ajoute `SYS_SERVICE_BACKEND_STATUS` (48), le protocole corrélé `OS_IPC_VFS_BACKEND_STATUS` et `vfs-backend-status <pid>`. Seul le propriétaire public vivant de `vfs` peut lire le masque d'une délégation backend active ; le médiateur retourne `read` (`1`), `mutate` (`2`) ou `full` (`3`), et normalise le masque à zéro en cas d'erreur. Une capacité révoquée, purgée ou absente retourne `OS_SERVICE_NOT_FOUND`; un autre mandant retourne `OS_SERVICE_NOT_OWNER`. Cette réponse est un instantané non atomique de contrôle, sans réservation, ACL par chemin, identité vérifiée, expiration, audit durable ni transfert de capacité.

Le trente-troisième incrément ajoute `SYS_SERVICE_BACKEND_LIST` (49), `OS_IPC_VFS_BACKEND_LIST` et `vfs-backend-list`. Le propriétaire public vivant de `vfs` reçoit un instantané corrélé de taille fixe : statut, compteur et au plus quatre couples PID/masque. Les profils affichés restent `read`, `mutate` et `full`; une erreur impose une réponse vide. Le registre efface la sortie avant le contrôle de propriété et ne restitue plus une entrée après sa révocation, sa purge, le transfert ou la fin du bénéficiaire. Cet inventaire est borné, non atomique et non autorisant : il ne fournit ni capability, ni réservation, ni ACL par chemin, ni identité vérifiée, ni audit durable.

Le trente-quatrième incrément ajoute `SYS_SERVICE_BACKEND_OBSERVE` (50), le protocole corrélé `OS_IPC_VFS_BACKEND_OBSERVE` et `vfs-backend-observe <generation>`. La génération backend démarre à 1 et progresse après chaque mutation effective de délégation, purge ou retrait associé. Une génération attendue différente retourne `OS_SERVICE_STALE`, la génération courante et une liste vide. Elle reste un indicateur non atomique : elle ne réserve rien et ne remplace pas le contrôle backend effectué à chaque opération.

Le trente-cinquième incrément porte cette génération à la granularité du service publié. Une mutation backend de `demo` ne rend plus l'observation de `vfs` obsolète ; seules les mutations, révocations, transferts et purges associés au même nom font avancer sa génération. Aucun nouveau syscall ni droit n'est ajouté.

Le trente-sixième incrément lie les réponses corrélées des commandes `vfs-backend-*` au PID du médiateur `vfs` résolu avant l'envoi. Une réponse du bon type et du bon `request_id` issue d'un autre PID n'est pas décodée comme valide. Cette protection couvre l'octroi, les profils scoped, la révocation, le statut, l'inventaire et l'observation backend.

Le trente-septième incrément étend ce même contrôle aux réponses VFS de fichiers, répertoires, pagination, observation de répertoire, métadonnées et montages. Toutes les commandes VFS corrélées du shell exigent maintenant le PID du médiateur résolu avant le décodage de leur réponse. Aucun syscall, format IPC ni droit supplémentaire n'est introduit.

Le trente-huitième incrément ajoute `SYS_TASK_METRICS` (51), `os_task_metrics_t`, `OS_TASK_NOT_FOUND` (`-62`) et `task-metrics <pid>`. Toute tâche expose son PID, état, type, tick de création, âge en ticks, temps d'exécution cumulé et nombre de sélections. L'ordonnanceur accumule le quantum sortant et l'instantané d'une tâche `RUNNING` ajoute le quantum courant sans modifier le compteur stocké. La lecture est non atomique, volatile et à granularité PIT ; le PMM affiché par le shell reste global, sans mesure de mémoire privée par processus.

Le trente-neuvième incrément ajoute `SYS_TASK_SET_PRIORITY` (52), `OS_TASK_BAD_PRIORITY` (`-63`) et `task-priority <pid> <1|2|3>`. La priorité est bornée à bas (`1`), normal (`2`) et haut (`3`), visible dans les métriques et initialisée à normal pour les nouvelles tâches. L'ordonnanceur privilégie une tâche Ring 3 `READY` de priorité la plus haute, tout en conservant le parcours circulaire comme round-robin entre priorités égales. Le réglage est local et prend effet au prochain passage sûr dans l'ordonnanceur ; il n'est ni un quota, ni une garantie CPU, ni un mécanisme anti-famine, ni une politique autorisée par capability.

Le quarantième incrément ajoute `parent_pid` à l'instantané de tâche et `OS_TASK_CONTROL_DENIED` (`-64`). `spawn` et `exec` lient chaque enfant à son créateur direct ; `task-priority` n'autorise désormais que la tâche cible ou son parent direct. Une tâche sans lien, un enfant ou un pair reçoit un refus explicite. Cette filiation est volatile, directe et locale : elle n'est ni une identité vérifiée, ni une capacité, ni une ACL, ni une délégation générale.

Le quarante-et-unième incrément étend ce garde-fou au cycle de vie. `ps` expose désormais le parent (`PPID`) de chaque tâche et `kill` n'autorise que le parent direct à terminer son enfant. Les protections de la tâche noyau et de l'auto-terminaison restent en place ; le nettoyage des services ne se produit qu'après un `kill` autorisé. Les ancêtres, enfants, pairs et tâches sans lien ne reçoivent aucun droit implicite de terminaison.

Le quarante-deuxième incrément réattribue les enfants directs avant le retrait de leur parent, sur `SYS_EXIT` comme sur un `kill` autorisé. L'enfant hérite du parent du sortant ; si celui-ci est une racine, il devient racine avec `parent_pid = -1`. Cette règle conserve une filiation directe cohérente sans créer d'adoption, de droit d'ancêtre indirect, de terminaison récursive ou de superviseur automatique.

Le quarante-troisième incrément ajoute `SYS_TASK_WAIT` (53), `OS_TASK_NOT_CHILD` (`-65`) et `wait <pid>`. Un parent peut attendre un enfant direct existant : le noyau le place en attente, réutilise le waiter unique de l'enfant et le réveille sur `SYS_EXIT` ou `kill` autorisé. `task-metrics` expose aussi le compteur volatil `direct_children`. Ce mécanisme n'est pas un `waitpid` : il ne retourne aucun statut de sortie et ne conserve ni zombie, ni historique, ni attente rétrospective.

Le quarante-quatrième incrément borne à quatre le nombre d'enfants directs par parent (`OS_TASK_CHILD_CAPACITY`). `spawn` et `exec` contrôlent ce plafond avant le chargement de l'ELF et retournent `OS_TASK_CHILD_LIMIT` (`-66`) sans allocation lorsque la capacité locale est atteinte. Un enfant retiré ou réattribué libère naturellement une place ; ce plafond ne mesure ni mémoire, ni CPU, ni IPC et reste volatile, fixe et non atomique.

Le quarante-cinquième incrément ajoute `SYS_TASK_SET_NAME` (54), `SYS_TASK_CAPACITY` (55), `task-name <pid> <nom>` et `task-capacity`. Une tâche ou son parent direct peut définir un nom ASCII imprimable de 1 à 31 caractères ; un nom invalide retourne `OS_TASK_BAD_NAME` (`-67`). La file globale est bornée à seize tâches actives, noyau inclus : `spawn` et `exec` renvoient `OS_TASK_GLOBAL_LIMIT` (`-68`) avant tout chargement ou allocation lorsqu'elle est pleine, et l'instantané public expose actif, capacité et places libres. Enfin, `SYS_EXIT` et un `kill` autorisé émettent vers le parent direct un événement IPC noyau best-effort (`OS_IPC_TASK_EVENT`) avec PID enfant et raison `exited` ou `killed`, lisible avec `ipc-recv`. Aucun nom, compteur ou événement ne constitue une identité, une réservation, une capacité ou un statut de sortie durable.

Le quarante-sixième incrément ajoute `SYS_TASK_CHILD_RESULT` (56), `child-result <pid>` et `OS_TASK_NO_CHILD_RESULT` (`-69`). Le parent direct conserve le seul dernier départ enfant : PID, code `EBX` de `SYS_EXIT` ou sentinelle `OS_TASK_EXIT_KILLED` (`-128`), raison et tick local. Le résultat est enregistré avant réveil du waiter et réattribution ; il subsiste même si l'événement IPC best-effort est perdu, mais il est écrasé par le départ direct suivant. Cette consultation ne crée ni zombie, ni historique, ni collecte, ni attente rétrospective complète.

Le quarante-septième incrément ajoute `SYS_TASK_CHILD_RESULT_LIST` (57), `child-results` et une fenêtre circulaire de quatre résultats par parent (`OS_TASK_EXIT_HISTORY_CAPACITY`). Chaque départ direct alimente l'historique local avant l'événement IPC best-effort ; l'instantané restitue les entrées retenues du plus ancien au plus récent, et le cinquième résultat écrase l'entrée la plus ancienne. Cette fenêtre est volatile, non atomique, non persistante et ne crée aucun zombie, droit d'ancêtre, historique exhaustif, curseur ou attente rétrospective générale.

Le quarante-huitième incrément ajoute `SYS_TASK_CHILD_RESULT_ACK` (58), `SYS_TASK_CHILD_RESULT_OBSERVE` (59), `wait-result <pid>`, `child-results-clear` et `child-results-observe <génération>`. L'acquittement efface le dernier résultat et l'historique local, puis fait progresser sa génération. Une sortie enfant fait de même ; l'observation n'accepte que la génération attendue et retourne `OS_TASK_HISTORY_STALE` (`-70`) avec la génération courante si elle a changé. Ces opérations restent locales, volatiles et non atomiques : elles ne réservent rien, ne créent ni zombie, ni attente historique générale, ni journal persistant.

Le quarante-neuvième incrément ajoute `SYS_TASK_CHILD_RESULT_FIND` (60), `SYS_TASK_CHILD_RESULT_FORGET` (61), `child-result-any <pid>` et `child-results-forget <pid>`. Le parent cherche un PID dans sa fenêtre locale, ou retire sélectivement cette entrée en compactant l'ordre chronologique et en recalculant le dernier résultat retenu. `wait-result` réutilise cette recherche lorsque l'enfant a déjà quitté la file. Le retrait invalide l'ancienne génération ; recherche, compactage et repli restent locaux, volatils, non atomiques et limités aux quatre entrées retenues.

Le cinquantième incrément ajoute `SYS_TASK_SUSPEND` (62), `SYS_TASK_RESUME` (63), `task-suspend <pid>`, `task-resume <pid>` et l'état public `OS_TASK_SUSPENDED` (`3`, affiché `P`). Le parent direct peut faire passer un enfant utilisateur prêt de `READY` à `SUSPENDED`, puis de `SUSPENDED` à `READY`. Toute transition incompatible retourne `OS_TASK_BAD_STATE` (`-71`). Une tâche suspendue reste vivante, visible et comptée dans les capacités ; elle n'émet aucun événement, ne produit aucun résultat enfant et n'est ni terminée ni réattribuée.

Le cinquante-et-unième incrément ajoute `SYS_TASK_KILL_CHILDREN` (64) et `kill-children`. Le parent courant capture l'instantané de ses enfants directs, y compris les enfants suspendus, et retourne leur nombre après avoir appliqué à chacun le chemin existant de sortie `killed`, notification best-effort, réveil de waiter et réattribution. Les enfants devenus directs pendant cet appel ne sont pas ajoutés à l'instantané ; aucun groupe, signal, cascade récursive, transaction ou persistance n'est introduit.

Le cinquante-deuxième incrément ajoute `SYS_TASK_CHILDREN` (65), `SYS_TASK_WAIT_ANY` (66), `os_task_children_t`, `children`, `wait-any-result` et `OS_TASK_NO_DIRECT_CHILD` (`-72`). `children` restitue un instantané local, non atomique et borné à quatre enfants directs actifs avec PID, PPID, état, type et nom. `SYS_TASK_WAIT_ANY` inscrit le parent comme waiter de ses enfants directs présents et le réveille au premier départ ; la commande Ring 3 lit ensuite le dernier résultat enfant existant. Cette attente prospective reste limitée à la filiation directe, ne crée ni zombie, ni collecte exhaustive, ni réservation d'enfant ou d'instantané, et les résultats demeurent soumis à la fenêtre locale de quatre sorties.

Le cinquante-troisième incrément ajoute `SYS_TASK_CHILD_EXIT_COUNT` (67), `os_task_child_exit_count_t` et `child-exit-count`. Chaque parent compte de façon cumulative ses départs d'enfants directs depuis sa création, sur sortie normale comme sur terminaison autorisée, y compris `kill-children`. Le syscall écrit la valeur non signée de 32 bits dans une structure fournie par l'appelant et retourne séparément son statut, afin qu'une grande valeur ne soit pas confondue avec une erreur. Les acquittements, oublis et rotations de l'historique ne modifient pas ce compteur ; il reste local, volatile, non atomique et sujet au débordement 32 bits.

Le cinquante-quatrième incrément ajoute `SYS_TASK_DELEGATE_CHILD` (68), `task-delegate <pid_enfant> <pid_superviseur>` et `OS_TASK_BAD_DELEGATE` (`-73`). Le parent direct peut remplacer le `parent_pid` d'un enfant par une tâche utilisateur vivante disposant d'une capacité locale. Le noyau refuse l'auto-délégation, les cibles noyau ou terminées, les descendants susceptibles de créer un cycle et les enfants déjà associés à un waiter. Après succès, l'ancien parent perd son autorité et une terminaison future est rapportée au nouveau parent ; aucun historique, compteur, événement, capability, consentement, persistance ou délégation récursive n'est transféré.

Le cinquante-cinquième incrément ajoute `SYS_TASK_SUPERVISION_EVENTS` (69), `task-events` et une fenêtre circulaire de quatre transitions locales de supervision. Le parent courant peut observer les sorties, suspensions, reprises et délégations entrantes ou sortantes de ses enfants via une séquence et une génération locales. Le journal reste volatile, non atomique, non persistant et écrase l'entrée la plus ancienne ; il ne trace ni les refus, ni tous les appels système, ni les opérations VFS/IPC, et ne constitue ni audit de sécurité, ni capability, ni identité vérifiée.

Le cinquante-sixième incrément ajoute `SYS_TASK_SUPERVISION_EVENTS_ACK` (70), `SYS_TASK_SUPERVISION_EVENTS_OBSERVE` (71), `task-events-clear` et `task-events-observe <génération>`. Une observation renvoie `OS_TASK_HISTORY_STALE` avec la génération courante lorsqu'un instantané a changé ; l'acquittement vide globalement la fenêtre locale et avance cette génération sans modifier les résultats de sortie, compteurs, filiation ou messages IPC. Aucun verrou, lecteur multiple, suppression sélective, persistance, intégrité ou audit sécurisé n'est fourni.

Le cinquante-septième incrément ajoute `SYS_TASK_SUPERVISION_EVENT_FIND` (72), `SYS_TASK_SUPERVISION_EVENT_FORGET` (73), `task-event <séquence>`, `task-events-forget <séquence>` et `OS_TASK_NO_SUPERVISION_EVENT` (`-74`). Le parent courant peut rechercher ou retirer une entrée retenue par sa séquence locale non nulle. L'oubli compacte la fenêtre en préservant l'ordre, avance la génération et ne réutilise jamais la séquence supprimée. Cette sélection reste limitée à la fenêtre volatile de quatre entrées : aucune recherche globale, restauration, réservation, persistance, intégrité ou audit sécurisé n'est fourni.

Le cinquante-huitième incrément ajoute `SYS_TASK_SUPERVISION_SUMMARY` (74), `os_task_supervision_summary_t` et `task-summary`. L'instantané local consolide génération du journal, enfants directs actifs, enfants suspendus, compteur cumulatif de départs et nombre d'événements retenus. Il reste non atomique, volatile et uniquement diagnostique : aucun verrou, réservation, historique, quota, persistance ou audit de sécurité n'est fourni.

Le cinquante-neuvième incrément ajoute `SYS_TASK_SUPERVISION_NOTIFY` (75), `task-events-notify <on|off>`, `OS_IPC_TASK_SUPERVISION_EVENT` et `OS_TASK_BAD_NOTIFY` (`-75`). Un parent utilisateur active ou désactive pour lui-même une souscription volatile ; après une sortie, suspension, reprise ou délégation retenue dans son journal local, le noyau peut déposer best-effort un message IPC synthétique (`sender_pid = 0`) de 24 octets avec séquence, action, PID enfant, PID associé, détail et ticks. La désactivation est immédiate et la notification ne remplace ni le journal, ni l'événement historique de sortie enfant. Cette livraison est locale, non atomique et non garantie : une boîte IPC pleine perd le message sans retarder la transition ; il n'existe ni attente bloquante, accusé de réception, lecteur multiple, souscription distante, persistance, capability, identité vérifiée ni audit de sécurité.

Le soixantième incrément ajoute `SYS_TASK_SUPERVISION_NOTIFY_FILTER` (76), `SYS_TASK_SUPERVISION_NOTIFY_STATUS` (77), `task-events-filter`, `task-events-notify-status`, `os_task_supervision_notify_status_t` et `OS_TASK_BAD_NOTIFY_FILTER` (`-76`). Un masque local sélectionne `exit`, `suspend`, `resume`, `delegate-out` et `delegate-in` pour la seule tentative de livraison IPC détaillée ; le journal, les résultats enfants et l'événement historique de sortie restent complets et indépendants. L'instantané expose le booléen d'activation et le masque conservé lors d'une désactivation. Le filtre est local, volatile et non atomique, accepte le masque vide et ne fournit ni sélection par PID, filtre combiné dans le shell, priorité, quota, réservation, garantie, retransmission, accusé, lecteur multiple, souscription distante, persistance, capability, identité vérifiée ni audit de sécurité.

Le soixante-et-unième incrément ajoute `SYS_TASK_SUPERVISION_WATCH` (78), `SYS_TASK_SUPERVISION_WATCH_STATUS` (79), une watchlist locale de quatre enfants directs, `task-events-watch`, `task-events-unwatch`, `task-events-watch-clear`, `task-events-watch-status`, `os_task_supervision_watch_status_t` et les erreurs `OS_TASK_BAD_WATCH` (`-77`), `OS_TASK_WATCH_FULL` (`-78`) et `OS_TASK_NO_SUPERVISION_WATCH` (`-79`). Lorsque la liste est active, seules les transitions détaillées des PID retenus sont proposées à l'IPC ; journal, résultats enfants et événement de sortie historique ne sont pas filtrés. L'entrée est supprimée après l'émission potentielle à la sortie ou au départ en délégation, et n'est jamais transférée au nouveau superviseur. La liste reste locale, volatile, non atomique et bornée, sans filtre combiné par PID dans le shell, priorité, durée, quota, réservation, garantie, retransmission, accusé, lecteur multiple, souscription distante, persistance, capability, identité vérifiée ni audit de sécurité.

Le soixante-deuxième incrément ajoute `SYS_TASK_SUPERVISION_DELIVERY_STATS` (80), `SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK` (81), `os_task_supervision_delivery_stats_t`, `task-events-notify-stats` et `task-events-notify-stats-clear`. Le parent courant obtient les compteurs locaux de tentatives détaillées après souscription, filtre d'action et watchlist, de livraisons acceptées par l'endpoint et de pertes à saturation ; l'acquittement remet ces trois valeurs à zéro sans modifier la configuration, les événements ou les messages retenus. Cette télémétrie est locale, volatile, non atomique et sur 32 bits, sans granularité par PID/action/séquence, persistance, seuil, quota, alerte, retransmission, accusé, garantie, capability, identité vérifiée ni audit de sécurité.

Le soixante-troisième incrément ajoute `SYS_TASK_SUPERVISION_EVENT_REPLAY` (82) et `task-event-replay <séquence>`. Le parent courant peut tenter de rediffuser une copie détaillée d'un événement encore retenu dans son journal local, sans repasser par la souscription, le filtre d'action ou la watchlist qui gouvernaient la première tentative. Une séquence absente retourne `OS_TASK_NO_SUPERVISION_EVENT`, un endpoint saturé retourne `OS_IPC_FULL` et la tentative est comptabilisée dans la télémétrie de livraison. La rediffusion ne modifie ni journal, ni génération, ni filiation, ni résultats ; elle reste locale, volatile, non atomique et limitée à quatre entrées, sans persistance, restauration, attente, retransmission automatique, accusé, garantie, capability, identité vérifiée ni audit de sécurité.

Le soixante-quatrième incrément ajoute `SYS_TASK_SUPERVISION_PRIORITY` (83), `SYS_TASK_SUPERVISION_PRIORITY_STATUS` (84), `os_task_supervision_priority_status_t`, `task-priority-child` et `task-priority-child-status`. Un parent peut sélectionner un enfant direct prioritaire ; lorsque la souscription est active, ses transitions détaillées contournent le filtre d'action et la watchlist, mais restent best-effort et comptabilisées. Le choix est effacé à la sortie ou à la délégation de l'enfant. Cette priorité est locale, volatile, non atomique, limitée à un enfant et ne modifie ni l'ordonnancement CPU, ni l'ordre IPC, ni la capacité du journal ; elle ne fournit ni niveaux multiples, héritage, réservation, garantie, persistance, capability, identité vérifiée ni audit de sécurité.

Le soixante-cinquième incrément ajoute `SYS_TASK_SUPERVISION_NOTIFY_BUDGET` (85), `SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS` (86), `os_task_supervision_notify_budget_status_t`, `task-events-budget <n|off>` et `task-events-budget-status`. Un parent configure une limite locale de tentatives de notifications détaillées ; `0` signifie illimité et toute reconfiguration remet l'usage à zéro. Après souscription, filtre, watchlist ou priorité, une tentative admise incrémente l'usage avant le dépôt IPC best-effort, y compris si l'endpoint est saturé. Une limite atteinte bloque uniquement une nouvelle tentative détaillée : journal, résultats enfants, compteurs et rediffusion explicite restent disponibles. Le budget est local, volatile, non atomique et sur 32 bits ; il ne fournit ni réservation FIFO, garantie, fenêtre glissante, règle par PID/action, persistance, capability, identité vérifiée ni audit de sécurité.

### IA locale, GGUF et BPE

Le chemin `ai <texte>` appelle `SYS_GPT2_GENERATE` avec le profil local GPT-2. Le chargeur utilise le checkpoint `llm.c v3`, le tokenizer binaire, des activations CPU freestanding, le cache clé/valeur par couche et position, SSE2 et un échantillonnage top-k. Le contexte reste de 64 jetons. La génération FP32 produit jusqu'à 24 jetons nouveaux et s'arrête dès 80 caractères et une fin de phrase. Une mesure plus ancienne, de l'ordre de 7 à 9 secondes, concernait une courte génération de quatre jetons. L'objectif inférieur à une seconde n'est **pas atteint**.

Inventaire point 4 (3 octobre 2026) : si `aiworker` detient `ai-engine`, cet appel est relaye au worker Ring 3, qui execute le meme code FP32/SSE2 (`kernel/llm/gpt2_generate.c` compile aussi en Ring 3) et renvoie texte et jetons ; le noyau n'infere plus (`kernel_infer_while_live` 0). `make qemu-ai-worker` montre, sur une fixture synthetique `llm.c v3` (le CI n'a pas les poids 124M), les memes jetons `0 1 2 14 3 4 | 3 0 1 7 6 4` sur les chemins worker, noyau, repli apres mort du worker et apres une tentative `airogue`. Les pas GGUF 109-110 tournent aussi dans `aiworker` quand `GGUF_READY` est pose. Detail : [ai_worker_ring3.md](ai_worker_ring3.md), [ai_worker_gguf.md](ai_worker_gguf.md).

AOS-020 ajoute une sonde freestanding **GGUF v3** bornée. Elle valide magic, version, métadonnées, tenseurs et alignement ; elle a été exercée contre un checkpoint GPT-2 réel contenant des tenseurs F32, Q3_K, Q4_K et Q6_K. Les primitives FP16 -> FP32, Q8_0 x FP32 et les produits Q3_K/Q4_K/Q6_K sont présentes, ainsi qu'un index, un mapping de couches et des briques de forward caller-owned. `ai-model use gpt2.gguf` sélectionne ce profil pour `ai <texte>` : le shell assemble une phrase à partir des pas 109/110. La phrase mesurée est dans [ai_worker_gguf.md](ai_worker_gguf.md).

Le tokenizer BPE gère maintenant un décodage UTF-8 validé et une classe de lettres couvrant notamment Latin étendu, Grec, Arabe, Hébreu, Devanagari, CJK et Hangul. Les emoji et symboles restent des séparateurs BPE. Cette couverture est volontairement pratique et bornée ; elle n'implémente pas l'intégralité de `\p{L}` Unicode.

### Réseau caller-owned - AOS-113 à AOS-154

Le noyau n'est plus un stub réseau vide. Au boot, `ne2k_boot_probe()` sonde le port ISA `0x300`, prépare le contrôleur, configure les anneaux RX/TX et attache IRQ3. Sans carte, le boot continue et `SYS_NET_STATUS` (89) retourne 0. Avec `-device ne2k_isa`, le bitmask vaut 3 (NIC détectée et anneaux configurés). `net-status` et `net-status json` publient `nic=detected` / `ethernet=configured` dans ce cas.

Les couches au-dessus existent comme **primitives caller-owned** (buffers fournis par l'appelant, pas d'allocation noyau) et sont couvertes par des tests Unity. Elles ne sont **pas** un daemon DHCP, un socket TCP utilisateur ni un client HTTPS.

| Lots | Livraison vérifiée | Toujours hors périmètre |
|---|---|---|
| AOS-113, AOS-132 | `net-status` / `net-status json`, `SYS_NET_STATUS`, smoke `make qemu-ne2k-status` | Publication d'un bail, d'une route ou d'un socket |
| AOS-116 | Accès PCI borné (`0xCF8`/`0xCFC`) | Le boot NE2000 reste ISA `0x300`, pas une NIC PCI |
| AOS-117-AOS-136 | Sonde, PROM MAC, anneaux, TX PIO, RX poll, IRQ3 | DMA distant générique, multi-NIC |
| AOS-114, AOS-137-AOS-143 | Codec Ethernet/ARP, réponse ARP, cache statique, résolution active | Table ARP système unique, timeout NDP/IPv6 |
| AOS-121-AOS-123, AOS-140-AOS-146 | Codec IPv4/UDP, DHCP Discover/Offer/Request/ACK, DNS A sur UDP | Client DHCP automatique au boot, résolution de nom pour le shell |
| AOS-124-AOS-125, AOS-147-AOS-154 | SYN, SYN-ACK, ACK, payload, progression de séquence, métadonnées de retransmission bornée | Handshake complet, timer/RTO, congestion, API socket |
| AOS-126-AOS-128 | SHA-256, HMAC-SHA-256, framing TLS 1.2 record | Handshake TLS, certificats X.509, HTTP, OpenAI |

Le shell continue d'afficher ARP/IPv4/DHCP/DNS/TCP/TLS comme **absents** : aucun de ces chemins n'est raccordé à une configuration live ni à une commande utilisateur. `openai` reste `blocked`. Les clés API restent hors image, initrd, overlay et logs série.

QEMU peut relier `ne2k_isa` au backend utilisateur [1] :

```bash
qemu-system-i386 -kernel build/mohhdy.bin -initrd my_initrd.tar -m 1024M \
  -netdev user,id=n0 -device ne2k_isa,netdev=n0
```

Cette ligne est le contrat de `make qemu-ne2k-status`. Elle ne démarre pas DHCP ni TCP dans le guest.

### Fournisseur OpenAI - AOS-025

La commande `ai-provider openai` permet d'activer le fournisseur OpenAI. Le noyau prend en charge l'ensemble du cycle réseau live : acquisition DHCP, résolution DNS A, ouverture TCP, handshake TLS 1.2 authentifié avec validation X.509, requêtes HTTP POST JSON et flux SSE. La fourniture d'un Bearer Token valide s'effectue via `ai-credential <token>` sans persistance dans l'image de boot ni dans Git.

| Jalons AOS | Livraison réellement vérifiée |
|---|---|
| AOS-020 | Sonde GGUF v3, kernels Q3_K/Q4_K/Q6_K, index et mapping ; la génération quantifiée est désormais préparée puis exécutée par le runtime GGUF avec cache KV et top-k caller-owned. |
| AOS-021 | BPE UTF-8 avec catégories de lettres Unicode ciblées |
| AOS-022 | Contrat QEMU versionné : boot, runtime IA, overlay, append et retour au shell |
| AOS-023 | Snapshot ATA overlay V2 avec restauration V1/V2 |
| AOS-024 | Préemption IRQ0 Ring 3, quantum 20 ticks, contrat `spawn spin` puis shell réactif |
| AOS-025 | Stub OpenAI honnête ; `net-status` distingue désormais NIC absente ou NE2000 détectée |
| AOS-026 | Volumes FAT16/FAT32 sur IDE : LFN a la racine et dans les sous-repertoires multi-niveaux, `mkdir`, lecture, statut, pagination et mutations sans ecrasement. Hors perimetre : ecrasement, renommage inter-repertoire, remplacement atomique |
| AOS-113...154 | Primitives reseau caller-owned et pilote NE2000 ISA ; les lots suivants ajoutent sockets 99-108 et `ai-acquire` sur pair local, pas OpenAI public |

## Shell et ABI observables

Le shell Ring 3 affiche une banniere et des libelles ASCII (`[MOHHDY]`, `[IA]`, `[shell]`, `[perf]`) sans emoji ni filets Unicode. Il propose `ls`, `cat`, `mkdir`, `rmdir`, `rm`, `cp`, `mv`, `write`, `append`, `touch`, `stat`, `test`, `grep`, `wc`, `sort`, `head`, `tail`, `ps`, `task-metrics`, `task-priority`, `task-name`, `task-capacity`, `task-suspend`, `task-resume`, `kill-children`, `children`, `wait-any-result`, `child-exit-count`, `task-delegate`, `task-events`, `task-events-observe`, `task-events-clear`, `task-event`, `task-events-forget`, `task-summary`, `task-events-notify`, `task-events-filter`, `task-events-notify-status`, `task-events-watch`, `task-events-unwatch`, `task-events-watch-clear`, `task-events-watch-status`, `task-events-notify-stats`, `task-events-notify-stats-clear`, `task-event-replay`, `task-priority-child`, `task-priority-child-status`, `task-events-budget`, `task-events-budget-status`, `fat16-list`, `fat16-cat`, `child-result`, `child-result-any`, `child-results`, `child-results-clear`, `child-results-observe`, `child-results-forget`, `wait`, `wait-result`, `jobs`, `top`, `spawn`, `yield`, `ipc-send`, `ipc-recv`, `service-publish`, `service-grant`, `service-find`, `service-status`, `service-watch`, `service-event-pull`, `cap-token`, `id-key`, `spill-drops`, `right-token`, `vfs-backend-probe`, `vfs-backend-write-probe`, `vfs-backend-remove-probe`, `vfs-backend-rename-probe`, `vfs-grant`, `vfs-backend-grant`, `vfs-backend-grant-read`, `vfs-backend-grant-mutate`, `vfs-backend-revoke`, `vfs-backend-status`, `vfs-backend-scope`, `vfs-backend-list`, `vfs-backend-observe`, `vfs-read`, `vfs-stat`, `vfs-list`, `vfs-list-page`, `vfs-list-observe`, `vfs-mkdir`, `vfs-rmdir`, `vfs-stats`, `vfs-mount-add`, `vfs-mount-remove`, `vfs-write`, `vfs-remove`, `vfs-rename`, `kill`, `exec`, `mem`, `uptime`, `history`, `alias`, `env`, `ai`, `ai-continue`, `ai-provider`, `ai-model`, `ai-runtime`, `ai-acquire`, `ai-tls-poll`, `ai-credential`, `ai-request` et `net-status`.
 Les opérations de fichiers modifiables passent par l'overlay noyau ou, via le médiateur VFS, les volumes FAT16/FAT32 ; l'initrd demeure en lecture seule. Les programmes empaquetes incluent `shell`, `idle`, `spin`, `ipcserver`, `vfsserver`, `serviceclaim`, `vfsclaim`, `vfscapclaim`, `vfsreadclaim`, `vfsmutateclaim`, `vfsreleaseclaim`, `waitchild`, `ok`, `fake_ai`, `ai_assistant`, `vfsvirtual`, `vfsflight` et `user_program`.

L'ABI courante va de 0 a 155 (`MAX_SYSCALLS = 156` dans `include/os_syscalls.h`). Le detail 0-127 ci-dessous est le socle, dont `SYS_VGA_BLIT` 127. Au-dela : pair reseau 128-130, ATA 131-136, relais reseau 137-138, TCP sur le fil 139-143, `SYS_SOCKET_CONNECT` 144, `SYS_ATA_FS` 145, `SYS_NET_NIC` 146, `SYS_NET_RELAY_BULK` 147, jeton de capability 148, pull d'evenement 149, journal de montages 150, cle d'identite 151, pertes du deversoir 152 (service et supervision), droit de nom 153, reception IPC bloquante 154 (`SYS_IPC_RECV_WAIT`), worker GPT-2 Ring 3 155 (`SYS_AI_ENGINE` : STATUS, MAP, FETCH, REPLY, LOG). Les numeros 0-89 incluent notamment `SYS_APPEND`, `SYS_GPT2_GENERATE`, `SYS_IPC_SEND`, `SYS_IPC_RECV`, `SYS_SERVICE_REGISTER`, `SYS_SERVICE_LOOKUP`, `SYS_SERVICE_UNREGISTER`, `SYS_SERVICE_GRANT`, `SYS_VFS_BACKEND_READ`, `SYS_SERVICE_NOTIFY`, `SYS_VFS_BACKEND_WRITE`, `SYS_VFS_INITRD_READ`, `SYS_VFS_OVERLAY_READ`, `SYS_VFS_OVERLAY_UNLINK`, `SYS_VFS_OVERLAY_RENAME`, `SYS_SERVICE_STATUS`, `SYS_VFS_INITRD_STAT`, `SYS_VFS_OVERLAY_STAT`, `SYS_VFS_INITRD_LISTDIR`, `SYS_VFS_OVERLAY_LISTDIR`, `SYS_VFS_OVERLAY_MKDIR`, `SYS_VFS_OVERLAY_RMDIR`, `SYS_SERVICE_BACKEND_GRANT`, `SYS_SERVICE_BACKEND_REVOKE`, `SYS_SERVICE_BACKEND_GRANT_SCOPED`, `SYS_SERVICE_BACKEND_STATUS`, `SYS_SERVICE_BACKEND_LIST`, `SYS_SERVICE_BACKEND_OBSERVE`, `SYS_TASK_METRICS`, `SYS_TASK_SET_PRIORITY`, `SYS_TASK_WAIT`, `SYS_TASK_SET_NAME`, `SYS_TASK_CAPACITY`, `SYS_TASK_CHILD_RESULT`, `SYS_TASK_CHILD_RESULT_LIST`, `SYS_TASK_CHILD_RESULT_ACK`, `SYS_TASK_CHILD_RESULT_OBSERVE`, `SYS_TASK_CHILD_RESULT_FIND`, `SYS_TASK_CHILD_RESULT_FORGET`, `SYS_TASK_SUSPEND`, `SYS_TASK_RESUME`, `SYS_TASK_KILL_CHILDREN`, `SYS_TASK_CHILDREN`, `SYS_TASK_WAIT_ANY`, `SYS_TASK_CHILD_EXIT_COUNT`, `SYS_TASK_DELEGATE_CHILD`, `SYS_TASK_SUPERVISION_EVENTS`, `SYS_TASK_SUPERVISION_EVENTS_ACK`, `SYS_TASK_SUPERVISION_EVENTS_OBSERVE`, `SYS_TASK_SUPERVISION_EVENT_FIND`, `SYS_TASK_SUPERVISION_EVENT_FORGET`, `SYS_TASK_SUPERVISION_SUMMARY`, `SYS_TASK_SUPERVISION_NOTIFY`, `SYS_TASK_SUPERVISION_NOTIFY_FILTER`, `SYS_TASK_SUPERVISION_NOTIFY_STATUS`, `SYS_TASK_SUPERVISION_WATCH`, `SYS_TASK_SUPERVISION_WATCH_STATUS`, `SYS_TASK_SUPERVISION_DELIVERY_STATS`, `SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK`, `SYS_TASK_SUPERVISION_EVENT_REPLAY`, `SYS_TASK_SUPERVISION_PRIORITY`, `SYS_TASK_SUPERVISION_PRIORITY_STATUS`, `SYS_TASK_SUPERVISION_NOTIFY_BUDGET`, `SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS`, `SYS_FAT16_READ`, `SYS_FAT16_LIST` et `SYS_NET_STATUS`.
 Les numéros 90-98 couvrent la session LLM noyau ; 99-108 les sockets TCP utilisateur ; 109-110 la génération GGUF et `ai-continue` ; 111-114 la lecture et pagination FAT32/FAT16 ; 115 la libération d'une capacité backend ; 116-118 la création, suppression et renommage FAT16 ; 119-121 les mêmes opérations FAT32 ; 122-123 le listage FAT16/FAT32 par chemin avec départ de page en ESI ; 124 l'octroi droit-source ; 125 le statut borné droit-source ; 126 l'octroi droit-source-préfixe relatif en EDI ; 127 `SYS_VGA_BLIT` (bureau 80x25, EBX=0 pour quitter).
 Les structures IPC ajoutent un `request_id` 32 bits opaque, copie par le noyau sans changer le plafond de charge utile. Les événements de service et de sortie d'enfant utilisent ce même transport, avec un émetteur noyau (`sender_pid = 0`) et `request_id = 0`. Le parent retient en outre un unique dernier résultat et une fenêtre circulaire de quatre résultats d'enfants, indépendants de la livraison best-effort de l'événement et observables par génération locale. Le profil local ne nécessite aucun secret ni aucune dépendance réseau à l'exécution.

## Vérification reproductible

```bash
sudo apt-get install -y build-essential gcc-multilib nasm qemu-system-i386 grub-pc-bin xorriso
make clean && make all       # noyau, initrd et disque overlay
make test-all                # 662/662 tests C, rejeu du 3 octobre 2026
make integration-qemu        # contrats AOS-022/AOS-024/AOS-025, NE2000, IPC, VFS avec montages dynamiques, révocation, transfert et notifications Foundation
make iso                     # ISO BIOS/GRUB bootable
make run                     # console curses
make run-gui                 # fenetre QEMU GTK, tapez gui (VBE 1024x768)
```

La suite C execute **648** tests (3 octobre 2026) : PMM, syscall, tâches, overlay, FAT16/FAT32, pagination de sous-répertoire, protocole worker `mkdir`/`rmdir`, ACL backend droit-source-préfixe, tokenizer, GGUF, quantification, IPC, VFS, services, shell, RAMFS, console, PCI, réseau, TLS et robustesse GGUF. `make integration-qemu` démarre sept machines QEMU séparées : le contrat cœur AOS-022, le smoke NE2000 (`make qemu-ne2k-status` avec `-device ne2k_isa`), le contrat IRQ0 AOS-024, le smoke AOS-025 (profil OpenAI bloqué), le contrat Foundation IPC, le contrat VFS et le contrat de transfert de service. Le contrat cœur utilise une image overlay de test isolée. Les cadences du smoke cœur, extras, VFS, service et persistance sont respectivement 0,65 s, 0,80 s, 1,10 s, 0,55 s et 0,65 s pour limiter les doubles frappes PS/2 sous QEMU TCG ; les contrats VFS, service et persistance relancent au plus trois commandes longues jusqu'à leur marqueur fonctionnel, sans assouplir les assertions.

Le budget du scénario QEMU `spawn` est de 240 secondes afin de couvrir les contrats complets de supervision locale, dont l'observation générationnelle, l'acquittement et la configuration puis la consommation d'un budget de notifications, sans réduire les assertions fonctionnelles.

Le build a également produit une ISO GRUB BIOS avec l'initrd GPT-2 local. Avec les actifs disponibles dans l'environnement de vérification, l'ISO est d'environ 481 Mio ; les modèles restent exclus du versionnement.

## Absences importantes

Constat du 3 octobre 2026. Ne plus lister comme absents : les LFN FAT16/FAT32 (racine et sous-repertoires), l'ecriture VFS FAT16 et FAT32, le bureau VBE (`gui`), le pilote ATA Ring 3, la pile NE2000 Ring 3 quand `networker` detient la carte, l'identite de tache (sequence, generation et cle, visible pour soi et l'enfant direct), le droit de nom lu par `right-token`, les deux compteurs de pertes du deversoir et la table de montages restauree en RAM au reenregistrement d'un service. Restent absents : bail DHCP public, TLS vers un hote Internet, client OpenAI effectif, UEFI, multiprocesseur, microkernel termine, IPC bloquant, navigateur Chromium, LLM de production, au-dela des lots des phases 4 a 8 (voir `docs/promptmessage.md`, `docs/p2p.md`, `docs/platform.md`, `docs/collab.md`, `docs/production.md`). Un systeme a inodes (ext2) n'est pas prevu. `net-status` ne pretend pas a Internet public. Le registre nomme n'est pas un controle d'acces general. L'identite livree est sequence, generation et cle en RAM, visible pour la tache et son parent direct, y compris apres la mort de l'ancien parent. Le journal local de supervision en garde une copie au moment de l'evenement. Pas un certificat, pas l'avis IPC de 24 octets. La table de montages survit au reenregistrement dans le meme boot, pas a un reboot ; l'overlay AIOV, lui, est deja persistant. La boite aux lettres IPC, sa capacite, le mediateur VFS et les notifications restent bornes. La capacité IPC est globale au PID propriétaire, sans priorité, quota, délai ou file par nom ; les notifications noyau restent best-effort. L'état de capacité est un instantané non atomique sans réservation, souscription ou métrique historique. L'écriture VFS est limitée à 44 octets par requête ; la télémétrie des tâches repose sur des compteurs 32 bits volatils et non atomiques, sans mesure de mémoire privée, de quota, de temps bloqué ni séparation noyau/utilisateur ; la priorité CPU est un entier local de 1 à 3, contrôlé seulement par la cible ou son parent direct, sans temps réel, vieillissement, prévention de famine, réservation ni garantie de latence ; la filiation est volatile, directe et sans identité vérifiée, capacité, ACL ou délégation ; la création est bornée à quatre enfants directs par parent et à seize tâches globales, sans quota de mémoire, CPU ou IPC ; l'instantané global ne réserve aucune place ; les noms de tâches ne sont ni identités ni capabilities ; `kill`, `wait`, `task-suspend` et `task-resume` restent limités à l'enfant direct ; la suspension ne couvre que l'état prêt et ne fournit ni signal, ni gel atomique, ni suspension récursive ; `kill-children` traite un instantané direct non récursif et ne crée ni groupe ni transaction ; `children` ne fournit qu'un instantané local borné qui ne réserve aucune entrée, tandis que `wait-any-result` attend prospectivement un départ direct sans créer de zombie, collecte exhaustive ou sélection parmi les résultats ; `child-exit-count` reste un cumul local volatile de 32 bits, sans détail par PID, persistance, horodatage ou garantie contre le débordement ; `task-delegate` ne transfère qu'une filiation directe locale sans consentement, capability, historique, compteur, événement, persistance ni délégation récursive, et refuse tout cycle ou enfant déjà attendu ; `task-events` ne presente qu'une fenetre volatile non atomique de quatre transitions locales, qui recopie au moment de l'evenement la sequence, la generation et la cle de l'enfant, sans integrite, persistance, filtre, recherche ou valeur d'audit de securite ; `task-events-observe` et `task-events-clear` n'ajoutent qu'une génération locale et un acquittement global, sans verrou, réservation, lecteur multiple ni restauration ; `task-event` et `task-events-forget` ne consultent ou retirent qu'une copie locale retenue par séquence, sans recherche globale, restauration, transaction, persistance ni audit sécurisé ; `task-summary` ne regroupe que des compteurs locaux non atomiques, sans verrou, réserve, historique, quota ni action de contrôle ; `task-events-notify`, sa politique de filtre, sa watchlist et sa télémétrie n'ajoutent qu'une souscription locale volatile et opt-in, dont la livraison IPC best-effort peut être perdue à saturation ; le filtre vise les actions et la watchlist uniquement les PID détaillés, sans toucher journal, résultats ou événement de sortie historique ; les compteurs ne sont ni atomiques ni associés à un PID, une action ou une séquence ; la rediffusion reste limitée à une séquence retenue et ne restaure rien après écrasement ou oubli ; aucun ne fournit sélection combinée par PID dans le shell, attente, accusé, priorité, durée, quota, lecteur multiple, héritage de délégation, persistance, capability, identité vérifiée ni audit ; les enfants directs sont réattribués au parent du sortant, mais sans adoption active, arbre récursif, groupe de processus, signal, zombie, statut de sortie, attente rétrospective ni terminaison en cascade ; les métadonnées VFS sont des instantanés non atomiques ; le listage VFS couvre la racine ou un sous-répertoire d'un montage, retourne une seule page de quatre noms au plus sans curseur, ordre contractuel ni atomisation ; et les mutations VFS n'apportent ni transaction ni verrouillage ; les transactions worker sont bornées au noyau par un préfixe relatif minimal, mais l'ABI directe ne fournit pas d'autorisation publique par répertoire ni d'identité client vérifiée ; `vfs-rmdir` ne traite que les répertoires overlay vides et ne remplace pas `vfs-remove` pour les fichiers. Les aliases sont locaux au serveur, non persistants, non transférés et dépourvus de capabilities. Les statistiques ne sont ni persistantes, ni atomiques, ni horodatées et peuvent déborder. Les événements de service et de sortie enfant ne sont ni persistants, ni accusés, ni garantis lorsque la boîte IPC est pleine ; ils ne portent pas de statut de sortie ni d'historique. Le parent retient un dernier résultat et une fenêtre locale circulaire de quatre résultats enfants, volatils et non atomiques ; les entrées les plus anciennes sont écrasées, l'acquittement peut être global ou sélectif, et l'observation générationnelle ne réserve rien. Une recherche ou un repli `wait-result` ne couvrent qu'une entrée encore retenue. Ce mécanisme ne fournit ni zombie, ni `waitpid`, ni historique exhaustif, ni collecte, ni statut d'exception fiable. Les rapports historiques conservés dans `docs/` sont des éléments de chronologie et non la description de l'état courant.

## Références

[1] [QEMU, *Network emulation*](https://www.qemu.org/docs/master/system/devices/net.html)


Le lot 80 ajoute une preuve différentielle de sélection disque dans la fixture GGUF/FAT16. Après alignement dynamique sur `general.alignment`, le premier super-bloc Q4_K commence par `0x11` et le second par `0x77`; les lectures de tenseur, de bloc 0 et de bloc 1 vérifient ces valeurs distinctes. La capacité insuffisante et les bornes de ligne restent rejetées. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. Cette preuve porte sur l'offset physique FAT16 et ne constitue toujours pas un forward GPT-2 complet.


Le lot 81 étend le mapping GGUF aux rôles répétés des blocs GPT-2 via `gpt2_gguf_map_layer_role`. L'API construit dans un buffer caller-owned les noms `blk.<layer>.attn_norm`, `attn_qkv`, `attn_output`, `ffn_norm`, `ffn_up` et `ffn_down`, avec poids et biais lorsque la convention est définie, puis résout le tenseur dans l'index sans allocation dynamique. La fixture vérifie deux noms de couche, une couche absente et un buffer trop petit. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. Ce mapping ne branche pas encore les matrices à un forward GPT-2 réel.


Le lot 82 ajoute `gpt2_gguf_layer_t` et `gpt2_gguf_map_layer`, qui regroupent les dix descripteurs d'un bloc GPT-2 dans une structure caller-owned et renseignent le masque `present_mask = 0x3FF`. Le buffer de nom est réutilisé séquentiellement, aucune allocation noyau n'est introduite, et une couche absente est rejetée. La fixture contient les quinze tenseurs globaux et de couche; `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. Le descripteur ne réalise pas encore le forward GPT-2 ni la validation sémantique des dimensions par matrice.


Le lot 83 ajoute `gpt2_gguf_forward_context_t` et `gpt2_gguf_forward_context_init`. Le contexte conserve une référence vers le modèle GGUF, le descripteur complet d'une couche, un scratch caller-owned, `channels` et `position`; il refuse les pointeurs, capacités ou dimensions invalides sans allocation dynamique. Les tests FAT16 vérifient les gardes de contexte et la suite reste à **265 tests réussis, 0 échec et 0 test ignoré**. Aucune attention, normalisation, MLP ou génération autoregressive n'est encore exécutée par ce contexte.


Le lot 84 ajoute `gpt2_gguf_layer_get`, un accès borné aux dix rôles déjà résolus dans `gpt2_gguf_layer_t`. Les rôles globaux, rôles hors intervalle et bits absents du masque sont refusés; le descripteur retourné reste caller-owned. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. > **Note historique réconciliée.** La préparation de génération actuelle (`gpt2_gguf_generation_prepare`) valide désormais les dimensions globales `C/V/T`, les rangs, axes et types des tenseurs, puis appelle `gpt2_gguf_validate_gpt2_layer_storage` sur chaque couche. Le chemin `gpt2_gguf_generation_token_fat16` branche cette préparation aux kernels quantifiés caller-owned.


Le lot 85 ajoute `gpt2_gguf_validate_layer`, qui exige les dix rôles présents, des canaux multiples de `GPT2_QK_K`, des rangs 1/2 non vides et des tailles de données valides; les tenseurs Q3_K/Q4_K/Q6_K doivent avoir une première dimension alignée sur `GPT2_QK_K`. La fonction est caller-owned, sans lecture de poids ni allocation. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. Les relations d'axes complètes QKV/MLP en matrices 2D restent le prochain incrément.


Le lot 86 ajoute `gpt2_gguf_validate_gpt2_layer`, qui applique les formes du forward legacy: biais et normalisations `[C]`, QKV `[C,3C]`, projection attention `[C,C]`, expansion MLP `[C,4C]` et projection MLP `[4C,C]`. Une fixture synthétique couvre les formes 2D et rejette un axe QKV incorrect. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. La correspondance entre formes et tailles de données quantifiées FAT16 reste à vérifier au prochain incrément.


Le lot 87 ajoute `gpt2_gguf_validate_tensor_size`, qui recalcule et compare `byte_size` pour F32/F16 et les super-blocs Q3_K (110 octets), Q4_K (144 octets) et Q6_K (210 octets) sur 256 valeurs. Les produits d'axes, les dépassements et les formes non alignées sont rejetés sans allocation ni lecture de blob. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 88 ajoute `gpt2_gguf_validate_gpt2_layer_storage`, qui compose la validation des axes GPT-2 et la cohérence de `byte_size` des dix tenseurs. `gpt2_gguf_forward_context_init` appelle cette barrière après le mapping de couche; les matrices et vecteurs incohérents sont donc rejetés avant lecture FAT16. L'implémentation reste sans allocation et sans division 64 bits runtime. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 89 ajoute `gpt2_gguf_read_quant_row_fat16`, qui lit une ligne Q3_K, Q4_K ou Q6_K depuis FAT16 dans un buffer caller-owned, avec contrôle de largeur, index, capacité, offsets et lecture partielle. Les marqueurs physiques Q4_K `0x11` et `0x77` sont vérifiés ligne par ligne; Q3_K, Q6_K et le buffer insuffisant sont également couverts. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 90 ajoute `gpt2_gguf_dot_quant_row_buffer`, qui calcule une ligne quantifiée déjà lue sans nouvel accès FAT16, allocation ni cache implicite. La primitive accumule les super-blocs Q3_K, Q4_K ou Q6_K avec les kernels existants et contrôle largeur, capacité et type. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 91 ajoute `gpt2_gguf_project_qkv_row_fat16`, qui valide une matrice QKV `[C,3C]`, lit une sortie quantifiée à la fois depuis FAT16 et la calcule avec un buffer caller-owned. Les bornes `output_index < 3C` et la forme incompatible sont rejetées; aucune matrice complète ni allocation noyau n'est utilisée. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 92 ajoute `gpt2_gguf_project_qkv_fat16`, qui accumule les `3C` projections d'une matrice `[C,3C]` en séparant query, key et value dans trois buffers caller-owned. Un unique buffer de ligne quantifiée est réutilisé et les capacités de sortie sont contrôlées avant lecture. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 93 ajoute `gpt2_gguf_kv_cache_t` et ses opérations d'initialisation, écriture et lecture. Le cache caller-owned stocke `[couche][position][K puis V]`, contrôle la capacité totale et suit la plus grande position écrite via `count`, sans allocation noyau. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 93 ajoute `gpt2_gguf_kv_cache_t` et ses opérations d'initialisation, écriture et lecture. Le cache caller-owned stocke `[couche][position][K puis V]`, contrôle la capacité totale et suit la plus grande position écrite via `count`, sans allocation noyau. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**. Le build i386 et la relance propre du smoke QEMU valident core, extras, persist, spawn et exec.


Le lot 94 ajoute `gpt2_gguf_kv_cache_copy_history`, qui copie un intervalle de positions K/V historiques d'une couche vers des buffers caller-owned compacts. L'itération respecte `cache->count`, isole les couches et rejette les capacités ou intervalles hors bornes. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.


Le lot 95 ajoute `gpt2_gguf_kv_cache_query_scores`, qui calcule les produits scalaires query-key sur un historique KV sans allocation. Les scores restent bruts, sans softmax ni mise à l'échelle, et les capacités du scratch et du tableau de sortie sont contrôlées. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 96 ajoute `gpt2_gguf_kv_cache_accumulate_values`, qui calcule le vecteur de sortie par accumulation des values historiques pondérées. La primitive lit le cache sans le modifier, n'alloue aucun buffer, contrôle les capacités caller-owned et laisse le softmax ainsi que la mise à l'échelle aux incréments suivants. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 97 ajoute `gpt2_gguf_attention_scale_scores` et `gpt2_gguf_attention_softmax`. Les scores query-key sont mis à l'échelle par l'inverse de la racine carrée de la dimension de tête, puis convertis en probabilités par un softmax stable max-subtract, avec approximation exponentielle freestanding, sans allocation et avec buffers caller-owned. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 98 ajoute `gpt2_gguf_kv_cache_attention_head`, qui compose query-key, scaling, softmax et accumulation des values pour une tête d'attention du cache KV. La tranche de tête est sélectionnée sans toucher aux autres canaux, les buffers restent caller-owned et le cache est lu seulement. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 99 ajoute `gpt2_gguf_attention_concat_heads`, qui rassemble les sorties contiguës de toutes les têtes dans le vecteur d'attention sans allocation. L'ordre des canaux est conservé, les multiplications sont bornées et la capacité de destination est contrôlée. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 100 ajoute `gpt2_gguf_project_matrix_fat16`, une projection quantifiée générique de tenseurs GGUF 2D `[input_channels, output_channels]`. Chaque ligne est lue dans le scratch caller-owned puis projetée vers la sortie caller-owned, sans allocation dynamique et sans imposer la forme QKV. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 101 ajoute `gpt2_gguf_kv_cache_attention_multi_head`, qui exécute l'attention complète sur toutes les têtes, réutilise les scratchs de clé et de scores, stocke les sorties intermédiaires caller-owned puis les concatène. Les têtes incompatibles et les capacités insuffisantes sont rejetées sans allocation. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 102 ajoute `gpt2_gguf_add_residual`, qui ajoute in-place la sortie d'attention au résiduel caller-owned. Les pointeurs nuls et capacités insuffisantes sont contrôlés, sans allocation dynamique. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 103 ajoute `gpt2_gguf_layernorm` et `gpt2_gguf_gelu`. LayerNorm calcule moyenne, variance, epsilon et affine gamma/beta dans des buffers caller-owned; GELU utilise une approximation sigmoïde freestanding bornée. Aucune allocation ni dépendance libm n'est introduite. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 104 ajoute `gpt2_gguf_mlp_forward_fat16`, qui compose les projections quantifiées `ffn_up` et `ffn_down`, les biais optionnels et GELU dans des buffers caller-owned. Les formes `[C,4C]` et `[4C,C]`, les capacités et les erreurs FAT16 sont contrôlées sans allocation. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 105 ajoute `gpt2_gguf_mlp_forward_add_residual_fat16`, qui exécute le MLP puis écrit sa sortie dans le résiduel caller-owned. La capacité du résiduel est vérifiée avant lecture et les erreurs du MLP sont propagées. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 106 ajoute `gpt2_gguf_block_mlp_forward_fat16`, qui compose LayerNorm pré-MLP, projections `ffn_up`/`ffn_down`, GELU et résiduel. Tous les buffers restent caller-owned; capacités, epsilon, pointeurs et formes sont contrôlés sans allocation. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 107 ajoute `gpt2_gguf_attention_output_add_residual_fat16`, qui projette la concaténation multi-têtes par `attn_output.weight`, ajoute le biais optionnel puis écrit dans le résiduel. Les formes carrées, capacités et scratchs caller-owned sont contrôlés sans allocation. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 108 ajoute `gpt2_gguf_block_attention_forward_fat16`, qui compose LayerNorm pré-attention, attention multi-têtes KV, projection de sortie, biais et résiduel. L'ensemble reste caller-owned, sans allocation dynamique, avec contrôles des pointeurs et capacités. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 109 ajoute `gpt2_gguf_kv_cache_reset`, qui réarme le cache KV en O(1) en remettant son compteur à zéro sans effacer le stockage caller-owned. La primitive permet de réutiliser le cache entre prompts tout en conservant ses contrats de pointeurs. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 110 ajoute `gpt2_gguf_runtime_prepare`, qui matérialise une table caller-owned de rôles GPT-2 GGUF pour toutes les couches et valide leurs formes et tailles physiques avant exécution. La structure runtime publiée indique explicitement l'état `ready`; aucune allocation n'est effectuée. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 111 ajoute `gpt2_gguf_runtime_get_layer`, qui fournit une vue bornée par valeur d'une couche déjà préparée sans rescanner le blob GGUF. Les runtimes non prêts, pointeurs nuls et indices hors limites sont rejetés sans allocation. `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.
Le lot 112 étend `benchmark_gpt2_kv_latency.py` pour mesurer séparément `COLD_LATENCY_SECONDS` et `WARM_LATENCY_SECONDS`, avec `KV_REUSE=1` après deux prompts successifs. La mesure reste explicite sur les limites de QEMU TCG et ne modifie pas le contrat du moteur local. La syntaxe Python et `git diff --check` sont validés.


Le lot 11 ajoute la validation dynamique CSP des origines de ressources (`webrt_csp_validate_display_list_resource`) dans `userspace/webrt.c` et `userspace/osui_runtime.c` pour le Web Runtime (US-039 / US-031), l'estimation RTT EWMA et l'invalidation automatique du cache P2P lors de la déconnexion d'un pair dans `userspace/p2p.c` (US-063 / US-070), et l'intégration du profil d'énergie dans la boucle d'ordonnancement OS-UI (`plat_power_schedule_tick` et `osui_stage_tick`) dans `userspace/platform.c` et `userspace/osui_runtime.c` (US-081). `make test-all` valide l'ensemble de la suite.


Le lot 12 fait évoluer le conteneur d'onglets du Navigateur-OS avec la persistance de session isolée par site/origine (`extract_origin_from_url`, `origin_ok`) et la persistance chiffrée sur disque du stockage local par onglet (`browser-storage-set`, `browser-storage-get`, `browser-fs-save`, `browser-fs-load`) dans `userspace/osui_runtime.c` (US-031 / US-038). Il intègre l'inspection VFS et la vue du système de fichiers sandboxé directement dans les panneaux natifs OS-UI `/browser` et `/fs` (`osui_gui_browser_inspect`, `osui_gui_vfs_inspect`) dans `userspace/osui_gui.c` et `userspace/osui_gui.h` (US-045). Enfin, il valide le protocole de benchmark de latence KVM avec la réutilisation du cache KV multi-tours (`gpt2_gguf_infer_preload_kv_cache`, `gpt2_gguf_cache_matches`, `gpt2_gguf_infer_kv_cache_count`) dans `kernel/llm/gpt2_gguf_infer.c` et `tests/scripts/benchmark_qemu_gguf_kvm_latency.py` (Garde 3). `make test-all` est vert (912 tests réussis), `make gguf-kvm-benchmark-check`, `make strict-image-check`, `make secrets-check`, et `make tree-clean-check` sont validés.

Le lot 16 enrichit le journal de sécurité audit (`osui-audit`) pour la détection systématique des violations CSP, des refus d'origine (`origin_denied`), et des accès MCP non autorisés dans `userspace/osui_runtime.c` et `userspace/webrt.c` (US-039 / US-081) avec persistance overlay VFS sur `browser-fs-save` et `browser-fs-load`. Il intègre l'impact des anomalies de trafic par pair dans le coût de lien EWMA (`p2p_calc_link_cost`) et la régulation adaptative AIMD (`p2p_qos_note_loss`) dans `userspace/p2p.c` (US-068 / US-075). Il inclut l'empreinte FNV-1a de la feuille de route (`checksum_fnv`) dans le rapport `prod_support_bundle_text` et consolide le modèle de prédiction du risque de maintenance `prod_maint_model_risk` dans `userspace/prod.c` (US-116 / US-120).

Le lot 18 consolide le moteur Web Runtime avec une table étendue `WEBRT_VARS` à 32 variables CSS et une évaluation dynamique cascade/fallback (`var()`) renforcée dans `userspace/webrt.c` et `userspace/osui_runtime.c` (US-041). Il étend le journal d'audit de sécurité (`osui-audit`) pour traquer les opérations du stockage local par onglet et de la sandbox VFS (US-081). Côté P2P, il renforce la détection fine des anomalies de trafic par pair et la régulation adaptative AIMD (`p2p_qos_note_loss`), ainsi que l'invalidation automatique du cache P2P lors des déconnexions/reconnexions (US-070 / US-075). Enfin, il enrichit la base de signatures de sécurité `prod-secdb` (`MSA-2026-0006`) et valide le contrôle FNV-1a de la feuille de route `prod-roadmap` et du `prod-support-bundle` dans `userspace/prod.c` (US-112 / US-120). `make test-all` passe à 917 tests verts.

Le lot 19 perfectionne la gestion dynamique du cycle de vie des onglets (`browser-tab-rename`, `browser-tab-pin`, `browser-tab-duplicate`, `browser-tab-next`, `browser-tab-prev`) et l'isolation du stockage local par site dans `userspace/osui_runtime.c` (US-038 / US-045). Il renforce la traçabilité des opérations de navigation, d'inspection DOM/VFS (`/browser` et `/fs`), et de la persistance chiffrée (`ENC_V1:`) dans le journal d'audit `osui-audit`. Côté P2P, il optimise le calcul du coût de lien EWMA (`p2p_calc_link_cost`) intégrant RTT, taux de perte de paquets et pénalités d'anomalies de trafic, ainsi que la détection des chemins de routage (`p2p_route_find`), la réinvalidation automatique du cache P2P et l'anti-entropie Raft dans `userspace/p2p.c` (US-068 / US-073). Enfin, il affine la prédiction du risque de maintenance logistique Q12 (`prod_maint_model_risk`), la génération et la vérification d'intégrité FNV-1a des archives de diagnostic (`prod-support-bundle`) et l'évaluation dynamique de la feuille de route (`prod-roadmap`) dans `userspace/prod.c` et `userspace/shell_prod.c` (US-115 / US-116). `make test-all` passe à 918 tests verts.

Le lot 20 perfectionne la gestion des requêtes médias responsive (`@media aspect-ratio`, `min-aspect-ratio`, `max-aspect-ratio`) et la substitution dynamique récursive des variables CSS (`webrt_css_resolve_vars`) dans la table `WEBRT_VARS` à 32 slots dans `userspace/webrt.c` (US-040 / US-041). Il étend la traçabilité du journal de sécurité `osui-audit` pour les refus CSP (`csp_blocked`), l'authentification et les invocations MCP (`mcp_auth`, `mcp_credentials`, `mcp_invoke`, `mcp_invoice`), et le stockage local isolé par origines Same-Origin (`storage_set_ok`, `storage_get_ok`, `storage_origin_denied`) dans `userspace/osui_runtime.c`. Côté P2P mesh, il optimise la réinvalidation automatique du cache (`p2p_cache_on_peer_down`, `p2p_cache_invalidate_v`), le contrôle de l'anti-entropie du journal Raft lors des partitions/failovers et la régulation AIMD du débit sous charge/perte (`p2p_qos_note_loss`) dans `userspace/p2p.c` (US-070 / US-076). Enfin, il consolide la base de signatures de sécurité `prod-secdb` (`MSA-2026-0001` à `MSA-2026-0006`) avec audits W^X ELF32 (`prod_elf_audit`) et le pipeline CI/CD local (`prod-cicd` / `prod-pipeline`) pour les conteneurs PromptMessage et l'évaluation de la feuille de route `prod-roadmap` dans `userspace/prod.c` et `userspace/shell_prod.c` (US-108 / US-112). `make test-all` passe à 921 tests verts.

Le lot 21 perfectionne l'execution sandboxee des PromptPrograms (.pm / PMC1) dans userspace/osui_runtime.c, userspace/promptmessage.c, userspace/promptmessage.h et userspace/csig.c, incluant la verification des signatures Schnorr (.sig / pmsig1) via csig_verify, le controle de la chaine de confiance CA local (/etc/pm_ca.pub) et des revocations (CRL / pm_crl_has) avec tracabilite dans osui-audit (US-038 / US-045). Cote P2P mesh, il optimise le routage multi-sauts (p2p_route_find) avec detection et evitement des boucles a 2 nœuds, mise a jour dynamique du cout de lien EWMA, appels gRPC-Wire types avec controle de deadline (p2p_grpc_call_deadline) et traversement de NAT / STUN RFC 5389 avec repli automatique (p2p_nat_punch, p2p_stun_probe) dans userspace/p2p.c, userspace/p2p.h, userspace/grpcwire.c et userspace/grpcwire.h (US-069 / US-074). Enfin, il consolide la persistance chiffree (ENC_V1:) des 5 overlays VFS (node, prod, collab, p2pkv, web) et leur synchronisation (browser-fs-sync, browser-fs-save, browser-fs-load), la tracabilite des inspections VFS (/browser, /fs) et la verification FNV-1a d'integrite des bundles de diagnostic (prod-support-bundle) dans userspace/osui_runtime.c, userspace/shell_persist.c, userspace/prod.c et userspace/prod.h (US-081 / US-115).

Le lot 22 consolide l'execution sandboxee des composants OS-UI et PromptMessage (.pm / PMC1) dans cmd_browser_pm_run avec verification des certificats CA (/etc/pm_ca.pub), signatures Schnorr (csig_verify) et revocations CRL (pm_crl_has), et etend la tracabilite d'audit de securite (osui-audit) pour la rotation de cles CA et la gestion des sessions utilisateur dans userspace/osui_runtime.c (US-041 / US-060). Cote P2P et economie collaborative, il perfectionne la synchronisation d'etat (p2p_sync), le consensus Raft (p2p_raft_elect, p2p_raft_compact), la resolution LWW et le scellement AES-128-GCM des cles de signature (US-075 / US-082). Cote production, il affine la prediction du risque de maintenance (prod_maint_model_risk), l'autoscaling de charge (prod_autoscale_workload_eval) et la generation des bundles de diagnostic (prod_support_bundle_text) avec empreinte FNV-1a verifiee dans userspace/prod.c (US-112 / US-116).

Le lot 23 perfectionne le filtrage dynamique CSP (webrt_csp_enforce, webrt_csp_validate_display_list_resource) et l'isolation des origines pour les ressources d'affichage, scripts, styles et medias, ainsi que la tracabilite des refus CSP, acces d'origines non autorisees et autorisations MCP dans le journal d'audit de securite osui-audit dans userspace/webrt.c et userspace/osui_runtime.c (US-039 / US-041). Cote P2P mesh, il affine l'anti-entropie du journal Raft lors des reconnexions apres partitions et la gestion des instantanes (p2p_raft_compact, raft_install_snapshot), le calcul du cout de lien EWMA (p2p_calc_link_cost), le controle de debit AIMD (p2p_qos_note_loss) et l'invalidation automatique du cache P2P (p2p_cache_on_peer_down) dans userspace/p2p.c (US-070 / US-076). Enfin, il consolide l'autoscaling dynamique sous scenarios de charge extreme (prod_autoscale_workload_eval), le moniteur de performance global (prod_perf_monitor_eval), la verification FNV-1a de la feuille de route (prod_roadmap_eval), et l'inclusion du score de sante global dans les bundles de support diagnostic (prod-support-bundle, prod_support_bundle_text) dans userspace/prod.c (US-110 / US-120). make test-all est valide avec 932 tests reussis.

Le lot 29 realise l'extension et la consolidation industrielle Ring 3 Freestanding C (~5000 lignes de code) couvrant les sous-systemes majeurs. En infrastructure VFS (US-002 / US-034), il developpe la table circulaire de journaux de mutations VFS (`vfs_mutation_log_t`, `vfs_mutation_log_init`, `vfs_mutation_log_append`, `vfs_mutation_roll_forward`) garantissant le roll-forward automatique post-crash, l'isolation transactionnelle multi-fichiers avec verrouillage hierarchique de repertoires (`vfs_dir_lock_tree`, `vfs_dir_unlock_tree`), et la verification d'integrite de la table de noeuds avec detection de cycles de pointeurs (`vfs_integrity_check`). En Web Runtime (US-032 / US-043), il developpe l'isolation Shadow Roots (`open`/`closed`) pour Custom Elements (`webrt_shadow_root_attach`, `webrt_shadow_root_get`, `webrt_shadow_root_add_node`), la propagation d'evenements DOM avec phases capture, target et bubbling (`webrt_dom_dispatch_event_path`), et la boucle d'evenements asynchrone avec gestion microtasks (Promises) / macrotasks (Timers/Events) dans `webrt_event_loop_tick`. En reseau P2P Zero-Trust Mesh (US-064 / US-075), il developpe la rotation dynamique automatique des clefs de session AES-128-GCM (`p2p_rekey_check_and_rotate`), la verification Zero-Trust avec revocation instantanee sur tentative de rejeu ou anomalie de trafic (`p2p_zero_trust_verify_pkt`, `p2p_peer_revoke`), et l'extension du journal Raft avec consensus Two-Phase Commit (`p2p_raft_2pc_prepare`, `p2p_raft_2pc_commit`, `p2p_raft_2pc_abort`). En supervision Ring 3 & CI/CD (US-108 / US-118), il developpe le moteur de pipeline CI/CD autonome (`prod_cicd_run_pipeline`), le detecteur de regressions de performance comparant aux metriques historiques (`prod_perf_regression_eval`), et le systeme d'alerte adapte base sur la derivee seconde des consommations CPU/memoire (`prod_adaptive_alert_eval`). `make test-all` est valide avec 951 tests reussis.
Le lot 28 etend et consolide les briques majeures d'infrastructure systeme C Ring 3 / Freestanding. Il livre un gestionnaire de page cache virtuel LRU/Clock pour les blocs VFS (32 Mo max), un mecanisme de dirty-writeback transactionnel (vfs_cache_flush, vfs_cache_sync) et un scheduler d'E/S par priorites avec coalescing de secteurs dans userspace/ramfs.c et userspace/ramfs.h (US-001 / US-033). Il implemente le calcul complet du CSS Box Model, le decoupage multi-lignes de texte (line wrapping), l'alignement texte, les composants de formulaires HTML interactifs (<input>, <button>, <textarea>) et un moteur de rendu 2D vectoriel leger traduisant l'arbre DOM en commandes VBE Framebuffer dans userspace/webrt.c et userspace/webrt.h (US-031 / US-040 / US-042). Cote reseau P2P WAN mesh, il integre un protocole de gossip epidemique (p2p_gossip_tick, p2p_gossip_broadcast), une table d'etats de traverser NAT (STUN keepalive table) et un multiplexage multi-chemins avec equilibrage EWMA RTT dans userspace/p2p.c et userspace/p2p.h (US-061 / US-063 / US-074). Enfin, cote production, il livre un profiler de consommation CPU/memoire avec detection de fuites/boucles, un runtime d'auto-guerison autonome (prod_auto_heal_tick) et un moteur d'agregation de telemetrie avec calcul de percentiles de latence (p50, p95, p99) dans userspace/prod.c et userspace/prod.h (US-107 / US-110 / US-115). make test-all est valide avec 951 tests reussis.

Le lot 37 realise l'extension systeme industrielle avancee et haute disponibilite systeme (~5000 lignes de code C Ring 3 / Freestanding). En stockage VFS (US-009 / US-042), il developpe le dechargement dynamique NVMe Queue Pair (vfs_nvme_qp_offload_submit, vfs_nvme_qp_poll_completion) et un moteur transactionnel Erasure Coding Reed-Solomon K=4 + M=2 sur GF(2^8) avec encodage, reconstruction et validation de transactions (vfs_erasure_encode, vfs_erasure_reconstruct, vfs_erasure_commit_tx) dans userspace/ramfs.c et userspace/ramfs.h. En Web Runtime (US-057 / US-059), il developpe le moteur de synchronisation en arriere-plan Service Workers (webrt_bg_sync_register, webrt_bg_sync_process) et l'acceleration SIMD WASM 128-bit pour la multiplication matricielle 4x4 et la contraction tensorielle 3D en virgule fixe 16.16 (webrt_wasm_simd_mat4_mul, webrt_wasm_simd_tensor3d_contract) dans userspace/webrt.c et userspace/webrt.h. En reseau P2P Mesh (US-072 / US-084), il livre l'election de leader de groupe Raft WAN resiliente avec bails dynamiques (p2p_wan_raft_leader_elect, p2p_wan_raft_check_leader_lease) et un moteur de verification par lot de preuves inner-product Zero-Knowledge Bulletproofs (p2p_zk_bulletproof_batch_add, p2p_zk_bulletproof_batch_verify) dans userspace/p2p.c et userspace/p2p.h. Enfin, en daemon de production (US-120 / US-129), il integre un moteur d'equilibrage de charge adaptatif en temps reel (prod_adaptive_lb_select_node, prod_adaptive_lb_rebalance) et un moteur d'auto-guerison IA en temps reel avec analyse de telemetrie et remediation automatique (prod_ai_self_heal_eval, prod_ai_self_heal_execute) dans userspace/prod.c et userspace/prod.h.

Le lot 38 acheve l'extension systeme industrielle avancee et la haute disponibilite systeme (~5000 lignes de code C Ring 3 / Freestanding). En stockage VFS (US-010 / US-043), il livre le moteur DMA Scatter-Gather PRD/SGL pour NVMe sans allocation dynamique (vfs_nvme_sg_init, vfs_nvme_sg_add_descriptor, vfs_nvme_sg_execute) et la synchronisation distribuee inter-noeuds des bandes Erasure Coding Reed-Solomon (K=4, M=2) avec reconstruction a chaud (vfs_erasure_dist_sync_init, vfs_erasure_dist_sync_node, vfs_erasure_dist_hot_reconstruct) dans userspace/ramfs.c et userspace/ramfs.h. En Web Runtime (US-058 / US-060), il livre la synchronisation futex atomique multi-workers sur SharedArrayBuffer sans mutex OS hote (webrt_sab_futex_init, webrt_sab_futex_wait, webrt_sab_futex_wake) et l'acceleration SIMD WASM 128-bit pour convolutions tensorielles 2D et multiplications matricielles 8x8 en virgule fixe 16.16 (webrt_wasm_simd_mat8_mul, webrt_wasm_simd_tensor_conv2d) dans userspace/webrt.c et userspace/webrt.h. En reseau P2P Mesh (US-073 / US-085), il livre le sharding de consensus Multi-Raft geo-distribue avec basculement automatique de leader et reequilibrage de bails (p2p_multi_raft_wan_shard_init, p2p_multi_raft_wan_shard_failover, p2p_multi_raft_wan_shard_rebalance_leases) et la verification par lot de preuves de plage Zero-Knowledge Bulletproofs (p2p_zk_range_batch_init, p2p_zk_range_proof_create, p2p_zk_range_batch_add, p2p_zk_range_batch_verify) dans userspace/p2p.c et userspace/p2p.h. Enfin, en daemon de production (US-121 / US-130), il integre le moteur de prediction des pannes d'infrastructure materielle/reseau base sur l'analyse de series temporelles (prod_infra_fail_pred_init, prod_infra_fail_pred_record, prod_infra_fail_pred_eval) et le moteur d'auto-remediation autonome dirige par l'IA (prod_ai_remed_engine_init, prod_ai_remed_register_plan, prod_ai_remed_eval_and_execute) dans userspace/prod.c et userspace/prod.h.

Le lot 39 livre l'extension systeme industrielle avancee et la haute disponibilite systeme (~5000 lignes de code C Ring 3 / Freestanding). En stockage VFS (US-011 / US-044), il integre la protection d'integrite de donnees NVMe bout en bout DIF/DIX T10-PI 8-octets sans allocation dynamique (vfs_nvme_dif_crc16, vfs_nvme_dif_generate, vfs_nvme_dif_verify, vfs_nvme_dix_format_block, vfs_nvme_dix_strip_and_verify) et le collecteur de memoire morte distribue pour MVCC avec nettoyage asynchrone des versions d'inodes et de blocs VFS (vfs_mvcc_gc_init, vfs_mvcc_gc_mark_obsolete, vfs_mvcc_gc_collect_async, vfs_mvcc_gc_stat) dans userspace/ramfs.c et userspace/ramfs.h. En Web Runtime (US-059 / US-061), il livre l'acceleration SIMD WASM 256-bit / AVX2 simulee pour operations tensorielles (webrt_simd256_load, webrt_wasm_simd256_dot, webrt_wasm_simd256_fma, webrt_wasm_simd256_relu, webrt_wasm_simd256_gelu) et la machine d'etat d'execution de pipelines de calcul WebGPU Compute Pass (webrt_webgpu_compute_pass_init, webrt_webgpu_bind_group_set, webrt_webgpu_dispatch_workgroups, webrt_webgpu_compute_pass_finish) dans userspace/webrt.c et userspace/webrt.h. En reseau P2P Mesh (US-074 / US-086), il livre le moteur de replication asynchrone de consensus Cross-WAN avec fenetres glissantes et compression de deltas Raft (p2p_wan_async_repl_init, p2p_wan_async_repl_push, p2p_wan_async_repl_ack, p2p_wan_async_repl_compress_delta, p2p_wan_async_repl_decompress_delta) et le moteur de verification de preuves ZK-PLONK pour circuits arithmetiques complexes et transactions confidentielles (p2p_zk_plonk_verifier_init, p2p_zk_plonk_proof_create, p2p_zk_plonk_verify) dans userspace/p2p.c et userspace/p2p.h. Enfin, en daemon de production (US-122 / US-131), il integre le moteur de prediction et d'isolation preventive d'incidents multi-regions base sur le clustering d'anomalies (prod_multi_region_isolator_init, prod_multi_region_register, prod_multi_region_record_telemetry, prod_multi_region_cluster_and_isolate) et le moteur d'optimisation autonome auto-pilote Self-Driving ajustant la repartition des processus, l'ordonnancement de charge et la memoire cache (prod_self_driving_init, prod_self_driving_register_proc, prod_self_driving_optimize, prod_self_driving_get_proc_assignment) dans userspace/prod.c et userspace/prod.h.

Le lot 40 realise l'extension systeme industrielle avancee et la haute disponibilite systeme (~5000 lignes de code C Ring 3 / Freestanding). En stockage VFS (US-012 / US-045), il livre le moteur de gestion des zones ZBD NVMe (vfs_nvme_zbd_init, vfs_nvme_zbd_zone_alloc, vfs_nvme_zbd_zone_reset, vfs_nvme_zbd_zone_append) et le reequilibreur de metadonnees distribuees multi-maitres avec basculement automatique et resolution par arbre d'horloges vectorielles (vfs_dist_metadata_init, vfs_dist_metadata_register_node, vfs_dist_metadata_node_failover, vfs_dist_metadata_rebalance, vfs_dist_metadata_resolve_vector_conflict) dans userspace/ramfs.c et userspace/ramfs.h. En Web Runtime (US-062 / US-087), il livre l'acceleration SIMD WASM 512-bit / AVX-512 simulee pour operations tensorielles (webrt_simd512_load, webrt_wasm_simd512_dot, webrt_wasm_simd512_fma, webrt_wasm_simd512_silu, webrt_wasm_simd512_softmax) et l'encodeur parallele de commandes WebGPU Compute (webrt_webgpu_parallel_encoder_init, webrt_webgpu_parallel_create_command_buffer, webrt_webgpu_command_buffer_encode_dispatch, webrt_webgpu_parallel_encoder_finish) dans userspace/webrt.c et userspace/webrt.h. En reseau P2P Mesh (US-075 / US-087), il livre le sharding de consensus Raft WAN geo-distribue avec reequilibrage de bails et compression d'etat par arbre Merkle-Patricia Trie (p2p_wan_geo_sharding_init, p2p_wan_geo_sharding_add_shard, p2p_wan_geo_sharding_rebalance_leases, p2p_patricia_trie_init, p2p_patricia_trie_insert, p2p_patricia_trie_compress) et le moteur de verification de preuves ZK-STARK FRI pour validation post-quantique de transactions P2P (p2p_zk_stark_verifier_init, p2p_zk_stark_proof_create, p2p_zk_stark_verify_fri) dans userspace/p2p.c et userspace/p2p.h. Enfin, en daemon de production (US-123 / US-132), il integre le moteur d'auto-guerison multi-clusters et d'isolation de pannes par telemetrie (prod_multi_cluster_auto_heal_init, prod_multi_cluster_register, prod_multi_cluster_record_telemetry, prod_multi_cluster_eval_and_heal) et le moteur d'allocation autonome de ressources Self-Driving Resource Scheduler reajustant en temps reel coeurs CPU, bande passante memoire et cache (prod_self_driving_resource_engine_init, prod_self_driving_resource_register, prod_self_driving_resource_update_load, prod_self_driving_resource_rebalance, prod_self_driving_resource_get_quota) dans userspace/prod.c et userspace/prod.h.

Le lot 41 etend l'infrastructure systeme industrielle avancee et la haute disponibilite systeme (~5000 lignes de code C Ring 3 / Freestanding). En stockage VFS (US-013 / US-046), il livre la gestion du buffer memoire hote partage NVMe HMB avec ring buffer en acces direct sans malloc dynamique (vfs_nvme_hmb_init, vfs_nvme_hmb_register_page, vfs_nvme_hmb_ring_write, vfs_nvme_hmb_ring_read) et le reequilibreur de zones dynamiques multi-regions VFS avec basculement automatique par bandes d'Erasure Coding distribuees (vfs_multi_region_rebalancer_init, vfs_multi_region_add_zone, vfs_multi_region_failover, vfs_multi_region_rebalance) dans userspace/ramfs.c et userspace/ramfs.h. En Web Runtime (US-063 / US-088), il livre le noyau d'attention MHA (Multi-Head Attention) vectorise WASM SIMD pour la projection QKV et Softmax rapide en virgule fixe 16.16 (webrt_mha_init, webrt_mha_qkv_proj, webrt_mha_softmax_fast_q16, webrt_mha_compute_attention) et le soumissionnaire multi-files de commandes WebGPU Compute (webrt_webgpu_multi_queue_init, webrt_webgpu_multi_queue_submit, webrt_webgpu_multi_queue_process) dans userspace/webrt.c et userspace/webrt.h. En reseau P2P Mesh (US-076 / US-088), il livre le maillage de consensus geo-replique Cross-WAN avec reequilibrage dynamique de topologie et agregation d'etat par Merkle Tree (p2p_cross_wan_mesh_init, p2p_cross_wan_mesh_add_node, p2p_cross_wan_mesh_rebalance, p2p_cross_wan_mesh_merkle_aggregate) et le moteur d'agregation de preuves Zero-Knowledge (STARK/PLONK Batch Verification) pour la validation post-quantique multi-transactions P2P (p2p_zk_aggregator_init, p2p_zk_aggregator_add_proof, p2p_zk_aggregator_verify_batch) dans userspace/p2p.c et userspace/p2p.h. Enfin, en daemon de production (US-124 / US-133), il integre le moteur de resilience autonome multi-regions contre l'injection de fautes chaotiques et l'isolement d'incidents (prod_chaos_resilience_init, prod_chaos_resilience_inject, prod_chaos_resilience_isolate_and_heal) et l'ordonnanceur autonome de coeurs processeur avec affinite dynamique, migration de threads et ajustement thermique/puissance (prod_core_scheduler_init, prod_core_scheduler_bind_thread, prod_core_scheduler_migrate_thread, prod_core_scheduler_scale_power_thermal) dans userspace/prod.c et userspace/prod.h.

## Regles de realisation du projet

Les regles suivantes s'appliquent a toute la suite du projet :

- Toutes les User Stories restent dans le perimetre du projet. Aucune fonctionnalite n'est annulee.
- Il n'existe pas de produit parallele, de second projet ou d'implementation de demonstration separee.
- Toute fonctionnalite doit etre integree au meme systeme d'exploitation MOHHDY et fonctionner dans QEMU.
- Le code partage du depot est la source unique. Les composants noyau, Ring 3, OS-UI, IA, reseau, stockage et Web Runtime doivent s'integrer au meme guest.
- Le portail web, s'il est realise, sera une fonction du Web Runtime de MOHHDY dans QEMU. Il ne remplacera pas le systeme d'exploitation et ne constituera pas une application independante.
- Une User Story n'est consideree comme livree qu'apres son implementation dans le guest, un test reproductible dans QEMU, un test de regression et la mise a jour de cet etat reel.
- Les limitations temporaires d'une implementation ne changent pas le perimetre. Elles doivent etre corrigees dans les increments suivants.

La feuille de route doit donc conduire progressivement du noyau actuel vers l'IA complete, l'OS-UI, PromptMessage, le Web Runtime, le reseau P2P, le multi-plateforme, les fonctions collaboratives et la production, sans retirer de phase.

## GUI terminal window and usability audit (PR #196)

Root causes and fixes:
- Hover erased the terminal: the terminal was painted only in the LFB while
  the soft cursor restored pixels from g_back (desktop composition). The
  terminal is now composed into g_back, then dirty rows are blitted.
  Test: tests/scripts/test_qemu_gui_term_window.py (hover pixels).
- No window chrome: the terminal window now has close, minimize, maximize
  buttons (mouse) and a draggable title bar. Close/minimize return to the
  desktop; the shell state is kept (same vga_console). Keyboard: `gui`
  returns to the desktop, `console` to text mode; Shell icon or /shell
  reopens the window. Close during a running command applies at the next
  prompt. Tests: QEMU term window test + test_gfx_term unit hit test.
- Close key was dropped by keyboard_poll_char (0x11 filtered): fixed.
- Audit: Scene IA showed through open panes; floating chat covered the
  Statut/Fichiers icons and the dock; chat text overflowed. Root cause:
  drawing and hit testing duplicated the layout. Drawing now uses
  gfx_desktop_get_layout. Test: test_pane_layout_has_no_overlaps,
  test_desktop_icons_click.

Remaining (not fixed): chat window has no close button; pane traffic
lights all close the pane (no min/max); pane content is sparse; menu bar
has no active state; terminal grid fixed at 80x25 (no VBE resize while
shown); no history recall or Tab completion in the console.

## Desktop apps: file explorer and text editor (PR #198, UX3)

What exists, tested by unit tests and `make qemu-gui-apps` (CI, collab job):

- One file library, `userspace/fsops.c`: the shell commands mkdir, rmdir,
  rm, cp, mv, touch and the desktop apps call the same functions (the old
  256-byte cp limit is gone). Read-only initrd paths report
  "protege (initrd)"; errors are shown in the app status line.
- File explorer (`/files`, Fichiers dock icon / menu / desktop icon):
  breadcrumbs, browse, open, new file, new folder, rename, delete with
  confirmation, copy, move, info, keyboard navigation, toolbar clicks,
  double click opens. The old FS pane stays on `/fs`.
- Text editor (`/edit FILE`, or Enter / double click in the explorer):
  plain-text mode of the pm-ide core (`userspace/osui_ide.c`): open, save,
  save as, selection, cut/copy/paste, undo/redo, find, line numbers, dirty
  marker, asks once before closing unsaved text. The QEMU test saves a
  file in the editor and reads the same bytes with `cat` in the terminal.
- Windows: one info pane, the chat, the explorer and the editor, with a
  kernel z-order, click-to-front and keyboard focus. Not arbitrary windows.
- Dock: 8 drawn pictograms with a name tooltip on hover.
- Status strip and chat: user-facing engine name ("GPT-2 local", "echo
  (sans modele)"); chat messages wrap; the floating chat no longer covers
  the Scene IA card.
- P2P / Collab / Prod / Support windows show the captured output of a real
  shell command run when the window opens (p2p-stats, collab-balances,
  prod-metrics, prod-diag). Browser-OS lists the local /web pages; showing
  one still goes through `web-render /web/<page>`.
- Admin: the vfs server is shown as "a la demande" (it is transient,
  started by the FAT commands), not as absent.
- Terminal: mouse drag selects and copies, right click pastes (newlines
  become spaces, so a paste never runs a command).
- `make run-gpt2` (GTK): a COM2 socket and `scripts/qemu_gui_fit.py --watch`
  make the desktop follow a window resize (needs xdotool and xwininfo).

Limits: the editor holds 96 lines of 72 columns, the app grid is a fixed
22 rows, two app slots, clipboard of 512 bytes, the Browser pane does not
embed the page itself.

## Desktop: terminal features as windows (PR #199, UX4)

What exists, tested by unit tests and `make qemu-gui-tools` (CI, p2p-mesh
job) plus `make qemu-gui-apps`:

- Generic app windows: 6 slots, any app kind in any slot, one window per
  app kind, cascaded placement, click-to-front z-order, focus events carry
  the slot index. A seventh app is refused ("no_slot"). Windows are opaque
  (nothing behind bleeds through).
- Tool apps (`userspace/osui_tools.c`) reuse the terminal: their content is
  the captured output of the real shell commands (`shell_app_run`, an allow
  list of non-interactive commands: ps, mem, kill, service-status, date,
  uptime, whoami, p2p-*, pm-run, pm-check, ai-model, hal-info,
  screen-adapt; no `;|&<>$` and no interactive command).
  - Task manager (`/tasks`): ps + mem, select a row, Tuer asks for a
    confirmation then runs `kill PID`; the rights check is the kernel's
    (protected PIDs and the shell itself are refused, the window shows the
    answer). Services view: service-status of the registry services, read
    only. Restarting a driver from the window is not implemented.
  - Viewer (`/view FILE`, explorer key `v`, Enter / double click on a
    .pgm/.ppm/.pnm): text up to 8 KiB with scrolling; PGM/PPM (P2, P3,
    P5, P6) as an ASCII luminance preview (10 levels), not pixels. Other
    image formats (PNG, JPEG, BMP) are not decoded.
  - Settings (`/settings`): RTC date (`date`), uptime, VBE framebuffer size
    (new `SYS_VGA_BLIT` command 4), user. Read only: there is one theme and
    one keyboard layout (US QWERTY), the window says so.
  - P2P (`/peers`): p2p-peers / p2p-stats, Up (NAME IP from the input line),
    Down, Envoyer (PEER TEXT); the window does not start a node by itself.
  - PromptMessage (`/pmrun FILE`, editor `ESC r` on a saved file): output
    of `pm-run`, Relancer, Verifier (`pm-check`), Editer.
  - GPT-2 chat (`/chat`): the desktop loop runs one GPT-2 step per idle tick
    (GGUF: generate then continue), so pieces appear as they come; Annuler
    (or Esc) stops between steps, Suite continues, Modele switches between
    gpt2.gguf and gpt2_124M.bin through `ai-model use`. The FP32 checkpoint
    answers in one step (no streaming). A step itself is not interruptible
    from the window (the kernel Esc cancel still works).
- Shared clipboard: one 4 KiB kernel buffer (`kernel/clipboard.c`,
  `SYS_CLIPBOARD`) used by the VBE terminal selection / right-click paste,
  the editor (copy pushes, paste pulls) and the explorer (`y` copies the
  selected path). Terminal paste still turns newlines into spaces.
- Browser: `/browse` (or `web FILE`) renders a local page with the guest
  web engine; a click on a link is hit-tested in Ring 3 on the engine's
  layout boxes (`we_link_at`) and loads the target page; `<` `>` buttons in
  the title bar walk an 8-entry history. Only local paths: schemes
  (http:, javascript:), queries and fragments are refused; the sanitizer
  already drops javascript: links. The Browser-OS info pane still lists
  /web.
- Keyboard: the Ring 3 PS/2 driver re-reads Shift/Caps from the kernel at
  each wake; before, a stall (GPT-2 load) let the kernel's Ring 0 drain
  change Shift without the driver knowing, which garbled the case.
- The shell context (46 KiB) moved off the 64 KiB user stack: a GUI window
  running `pm-run` from inside `gui` overflowed it.

Limits: the editor still holds 96 lines of 72 columns with the fixed
22x78 app grid (no dynamic size); tool windows cannot be resized beyond
the grid; one window per app kind.

## Desktop: GUI parity with the terminal (PR #200, UX5)

What exists, tested by unit tests and `make qemu-gui-ux5` plus
`make qemu-gui-parity` (CI job gui-parity, under 15 minutes):

- User heap: `SYS_HEAP_GROW` (162) maps pages at 0x60000000 (64 MiB max,
  grow only, per task `heap_size`); `userspace/umalloc.c` is a first-fit
  malloc/free/realloc over it. The shell uses it for the editor.
- Editor: lines live on the heap (no 96-line cap; a 400-line 28 KB file
  opens in full), the grid follows the window: the kernel sends
  `app K size R C` after a resize or maximize (up to 48x128 cells). Limits:
  a line is still 72 columns (horizontal scroll); undo is disabled above
  8 KiB ("annuler indisponible"); saving goes through the writable overlay
  store whose node holds 384 bytes, so a large Save As is refused with the
  store error (QEMU test checks it), never cut silently. Only the editor
  follows the window size; the other apps keep the 22x77 grid.
- Viewer: PGM/PPM (P2, P3, P5, P6) are decoded in Ring 3, scaled down to
  fit 320x240 and drawn as real pixels by the compositor (`SYS_VGA_BLIT`
  command 5, integer upscale up to x8); closing clears the image. PNG,
  JPEG and BMP are not decoded.
- Rights model of the tool windows: the fixed allowlist is replaced by the
  shell registry: any registry command may run from a window, except shell
  metacharacters and terminal-only commands (interactive, session, desktop
  nesting). The command runs in the shell task, so the kernel checks the
  rights exactly as for a typed command. See `docs/GUI_PARITY.md`.
- Panels per command family (`userspace/osui_parity.c`): /net, /disk,
  /services, /logs, /ledger, /ci, /security, /search, /devtools, /notes
  and the generic /cmd. 58 entries, each a real shell command line;
  state-changing entries need a second Enter. `scripts/check_gui_parity.py`
  (in test-all) fails if a panel names a command missing from the registry.
- `grep -r MOTIF [REP]` in the terminal (shared `fsops_grep`, also used by
  /search): recursive, depth 6, 32 entries per directory, first 8 KiB of
  each file, 64 hits max; the summary line counts the cut files.

GUI parity track (status per family, details in `docs/GUI_PARITY.md`,
counted by `prod-roadmap` as phase 9 of `/roadmap.txt`):

| Family | Status |
|---|---|
| files, search, generic command window, tools rights model | done |
| services/drivers, storage, logs, collab ledger, production/CI, security/CA (local Schnorr CA), web devtools, notes/archives (Lot 46: every panel entry in `qemu-gui-panels`) | done |
| editor (saves 30 KiB), viewer (no JPEG), processes, network (no interface configuration command), p2p, PromptMessage, AI, system settings | partial |
| sessions, mcp, media (simule), pwa, devsync, os-*, admin-*, api-* panels; window-sized grid for every app (Lot 45); chat /help (Lot 45) | done |
| large persistent saves (30 KiB, 64 KiB needs a new disk layout) | partial |

Lot 44 (PR #201, reviewed, details and per item status in
`docs/lot44_plan.md`): overlay files up to 30 KiB (chained 384-byte blocks,
dense v3 image inside the 64-sector AIOV area, refused cleanly beyond it,
kernel relay in 4 KiB chunks through the 8 KiB atadriver buffer); panels
/vfs /sessions /mcp /media /grpc /pwa /devsync /sysadmin /api (registry
commands, media simule); `driver-restart`; ledger forms in the shell
grammar; `grep -r MOTIF [REP] [PAGE]` with 32 KiB per file. Not done in
Lot 44: US-G15, editor lines over 72 columns, 64 KiB files, BMP/PNG,
clipboard over 4 KiB, capture of spawned programs, Raft partition tests,
HTTPS speedup, TCP ports.

Lot 45 (PR on `cursor/lot45`, per item status in `docs/lot45_plan.md`):
the reviewed Lot 44 fixes are restored (the late commit 0f82add of #201 had
reverted them, including the 241 KB overlay image that could write over
FAT16); `/help` in the chat opens the Aide window (slash programs from the
dispatch table, terminal help); every tool window and the file explorer
follow their window grid (US-G15); editor lines up to 256 columns with
horizontal scroll; shared `imgdec` (BMP 24/32, PNG depth 8 non-interlaced,
bounded inflate) in the viewer (zoom, fit, pan) and `img-info`; clipboard
64 KiB; `grep -r` and /search read whole files up to 1 MiB through the
shell heap; QEMU proofs of the ledger offer form, /grpc, /devsync and the
supervised respawn of pcidriver. Not done in Lot 45: JPEG, output of
spawned programs, US-001, Raft partition, TCP ports, overlay files over
30 KiB.

Lot 46 PR A (branch `lot46-a-gui-parity`, per item status in
`docs/lot46_plan.md`): the output of programs launched from a window
(spawn, exec) is captured by the kernel (`SYS_OUT_CAPTURE`, 16 KiB and 16
tasks per run, descendants of the shell only, rights unchanged) and shown
in the window with a note for the dropped bytes (`test_out_capture`,
`qemu-gui-parity` with `outflood`, 19825 bytes: 16384 kept); output
already shown in a tool window rewraps at resize and the explorer name
column follows the window width (`test_osui_tools`, `test_osui_files`,
`qemu-gui-ux5`); driver-restart of atadriver (supervised respawn, new pid,
write/cat and ata-status served) and networker (new pid, netrelay TCP
loopback through the new relay) in `qemu-gui-parity`; every entry of /net
/disk /services /logs /ci /security /devtools /notes runs in the new
`qemu-gui-panels` (64 runs, CI job gui-panels); deploy-verify in /ci takes
its manifest (it always answered "manifest not found"). Phase 9 of
`prod-roadmap`: done 14 partial 4 (77%). Still partial: US-G02, US-G03,
US-G04, US-G16. Not done in PR A, planned in PR B-D: 64 KiB overlay files,
JPEG and interlaced PNG, US-001, TCP ports, interface configuration, Raft
partition. A program that prints after the bounded wait of 50 ticks prints
on the console, not in the window.

Lot 46 PR B-D (same branch, before the merge):
- Review fixes of PR A: a driver relaunched from a window (networker after
  driver-restart) registers its service and leaves the capture, so the
  window no longer says "1 programme encore actif"; the capture is
  released at once when its owner shell exits, is killed or faults
  (`out_cap_task_gone` in `task_report_parent_exit`). Proof:
  `test_out_capture`, `qemu-gui-parity`. `outflood` stays in the initrd
  next to the other test probes (ok, devrogue, atarogue, pcirogue): the
  build has one initrd, a test-only image is not done.
- US-G16 / US-G02 done. Overlay image v4: records over LBA 0-63 and a
  128-sector extension after the FAT16 volume and after the journals
  (4222-4224): ext_lba = max(4225, 64 + FAT16 total sectors), enabled by
  atadriver only when IDENTIFY words 60-61 report ext_lba + 128 sectors;
  the LBA is in the v4 header and checked >= 4225 and >= the end of the
  FAT16 volume before any read. Store 96 KiB, files 64 KiB. Without the
  extension the store writes v3 as before (32 KiB, 30 KiB per file). v1,
  v2 and v3 images load; a v3 disk is rewritten as v4 on the next save.
  The kernel job queue, the mirror, the boot install and the Ring 0
  fallback follow the header. Test disk: 4353 sectors (was 4225).
  Proof: `test_overlay` (v4 64 KiB roundtrip, LBA map never in 64-4224,
  half image refused, v3 to v4), `test_ata_job` (two areas, chunks never
  cross 63/64), `make qemu-gui-bigsave` (CI job gui-panels): editor Save
  As 65536 bytes, host parse of the disk: byte exact, LBA 64-4221 byte
  identical to the fresh disk; cold reboot: stat 65536, last line found;
  Save As of 65537 bytes refused, no file. `make qemu-ata-driver` and
  `make qemu-persistence` pass with the extension on (disk 8192 sectors).
- US-001: nothing left to move for this item. ATA IDENTIFY and the FAT
  PIO fallback are absent from both images since Lot 2
  (`make strict-image-check`); supervised restart after a crash (kill and
  mid-job #GP, respawn, reboot) re-run with the v4 layout:
  `make qemu-ata-driver` PASS. Still Ring 0 by design: PIC, PIT, IRQ
  stubs, early boot console, panic path, early boot probes.
- US-064 (later in the branch): `qemu-p2p` 2b adds an isolated pre-vote
  (gamma cut from both peers campaigns after the lease, its term stays,
  the leader keeps its term after the heal). These runs found three
  bugs, fixed in `userspace/p2p.c` with unit tests that fail without the
  fix: follower commit = min(leaderCommit, matched index) (the old leader
  applied its own uncommitted entry after the heal); pre-candidates grant
  each other; leader resends paced per peer (6697 appends in one cut).
  2c now accepts beta or gamma as the new leader.
- TCP ports: `qemu-osui-web` serves GET /status on port 9137 (custom port).
- Editor: a refused Save As restores the previous name.
- US-G04: not done, the shell has no interface configuration command.
- US-064: `make qemu-p2p` scenario 2c: the leader is cut from both peers
  (hub wire cut), its own put is not committed, after the 10 s lease beta
  is elected at a higher term (3 in this run) and commits, after the heal
  alpha is follower at that term and applies the majority entry; the
  isolated entry is applied on no guest. Pre-vote, compaction and
  InstallSnapshot were already covered by scenario 2b.
- TCP ports: no code path that ignores the requested port was found.
  `web-serve` listens on the requested port (8080 and 8443 in
  `make qemu-osui-web`), `ai-acquire` and `ai-peer-listen` take a port.
  No new test in this lot.
- Not done: JPEG and interlaced / 16-bit PNG (US-G03), interface
  configuration command and its /net entry (US-G04), a test-only initrd.
  Phase 9 of `prod-roadmap`: done 16 partial 2 (88%).

Next steps: 64 KiB files (needs a new
disk layout, the overlay area is 32 KiB);
panels for vfs, task, session, mcp, media and grpc (US-G17); forms for the
collab ledger (offers, votes, escrow) and driver restart in /services.

Lot 47 (`docs/lot47_plan.md`, PR #204):
- US-G04 done. The shell command `net-if` shows the interface and sets
  it: `static IP MASK GW` or `dhcp`. It goes through `SYS_PEER_DATA` op
  14 to the Ring 3 networker, which owns the single lease.
  - Changes are allowed to the root shell task only, checked by the
    kernel before the relay.
  - Bad addresses are refused: broadcast, network, loopback, multicast,
    a non-contiguous mask, a gateway off the subnet.
  - The return to DHCP is a DHCP-only exchange; the DHCP renewal is off
    while the static mode is on.
  - /net entries 6 and 7 reuse the command; a change is armed by a first
    Enter.
  - QEMU, `make qemu-osui-web`: TCP answers 200 on the static address
    10.32.0.77 and again on 10.32.0.15 after `net-if dhcp`. The
    test-only program `ifcrogue` (not the root shell) is refused both
    changes.
  - Not tested: a TCP connection kept open across an address change.
- US-G03 done. The shared imgdec decodes JPEG baseline (SOF0/SOF1, gray
  or YCbCr, sampling 1..2, restart markers) and PNG of every bit depth
  (1/2/4/8/16) and Adam7. `img-info` and the viewer use it.
  - Progressive, arithmetic, 12-bit and CMYK JPEG are refused with a
    reason.
  - 16-bit samples keep their high byte.
  - Proof: `test_imgdec` (reference images, lossy tolerance 8 levels at
    4:4:4) and `make qemu-gui-parity`.
- Test-only initrd. QEMU tests boot `my_initrd_test.tar`. The production
  `my_initrd.tar` (run, iso, run-gpt2) no longer carries the 28 proof
  programs (outflood, rogues, claims, netrelay, ...). `make
  strict-image-check` fails if one of them is in the production image.
- US-120 stays partial. `/roadmap.txt` is still hand-kept, but `make
  roadmap-check` (CI) checks phase 9 against the US master table and the
  prod-roadmap counts of the QEMU test against the file.
- Phase 9 of `prod-roadmap`: done 18 partial 0 (100%).

## Lot 48 and lot 49 (status)

- Lot 48 (PR #205, merged): net-if address change ends old-address TCP
  connections with FIN and the LLM TLS session with close_notify + FIN;
  TLS handshake guest time 1.98 s -> 0.11 s (X25519 field arithmetic,
  Montgomery modexp, crypto unchanged; QEMU timing guard); phase 5
  `make qemu-p2p-phase5`: US-069, US-070, US-075 done, US-068/073 partial.
- Lot 49 (branch cursor/lot49): US-120 done, `roadmap-check` compares the
  phase 6-8 statuses with the new US master table. See docs/lot49_plan.md.
