# Inventaire point 4, tranche GGUF : pas 109-110 en Ring 3 (`aiworker`)

Date : 3 octobre 2026. Texte ASCII. Suite de [ai_worker_ring3.md](ai_worker_ring3.md)
(FP32 en Ring 3, PR #94) : cette tranche est empilee sur #94 et se fusionne
apres elle.

## Ce qui a bouge

- Les noyaux K-quant (`kernel/llm/gpt2_quant.c` : Q3_K, Q4_K, Q6_K) et le
  runtime GGUF (`gpt2_gguf.c`, `gpt2_gguf_loader.c`, `gpt2_gguf_infer.c`) sont
  compiles aussi dans `userspace/aiworker` (`-DMOHHDY_RING3 -msse2`). Le meme
  code reste dans le noyau pour le repli.
- Chaque pas d'echantillonnage de la session GGUF (`SYS_GPT2_GGUF_GENERATE`
  109 puis `SYS_GPT2_GGUF_CONTINUE` 110 : passe avant quantifiee + top-k) est
  relaye au worker des qu'il a declare `GGUF_READY`. Le job (type
  `OS_AI_JOB_GGUF_STEP`) porte tout l'etat de la session : jetons (64 au plus),
  nombre de jetons generes, etat du generateur aleatoire. La reponse rend le
  jeton suivant et le nouvel etat du generateur ; le noyau l'ajoute a la
  session. Un worker relance, ou le noyau, peut donc reprendre a n'importe quel
  pas, sans rejeu.
- Sans worker GGUF pret, le pas tourne en Ring 0 comme avant (`path kernel`).
  Si le worker meurt ou reste bloque 300 s pendant un pas, le pas est abandonne
  (`[AI] relay aborted ...`) et refait en Ring 0 (`path fallback`).
- Restent Ring 0 : la session elle-meme (jetons, graine), la tokenisation du
  prompt et le decodage du texte, le montage FAT16 et le chargement resident de
  `GPT2.GGU` au boot (tampon noyau de 100 Mio, inchange), le repli.
  (Etat de la premiere tranche ; la session est passee en Ring 3 ensuite, voir
  la section suivante.)

## Session et tokenizer GGUF en Ring 3 (`OS_AI_JOB_GGUF_SESSION`)

Le code de session est sorti de `syscall.c` dans
`kernel/llm/gpt2_gguf_session.{c,h}` (64 jetons au plus, graine, nombre de
jetons du prompt, actif, id). Il est compile dans le noyau et dans
`aiworker` : memes regles des deux cotes (fin sur EOT ou a 64 jetons, une
erreur de pas garde la session, un jeton non decodable rend -4).

Chemin worker (des que le worker a declare `GGUF_READY`) :

1. 109 : le noyau normalise le prompt (`gpt2_generate_normalize`, Ring 0),
   prend un nouvel id de session et envoie un job `GGUF_SESSION` / `START`
   avec le texte normalise. Le worker tokenise le prompt avec son propre
   tokenizer (la copie `OS_AI_ENGINE_MAP` du tokenizer de l'initrd), seme le
   generateur, fait le premier pas d'echantillonnage, decode le jeton et rend
   texte + jetons + graine + etat actif.
2. 110 : job `GGUF_SESSION` / `STEP` avec l'id et le miroir noyau. Le worker
   utilise sa session si l'id et le nombre de jetons correspondent ; sinon
   (worker relance entre deux pas) il adopte le miroir et le signale
   (`resumed`). Il rend le texte decode et le nouvel etat.
3. Le noyau valide la reponse (texte borne par `max`, au plus 64 jetons,
   jetons du prompt <= jetons, actif 0/1, `result` = longueur du texte) et la
   garde comme miroir de la session. Le texte du worker est copie tel quel
   vers l'appelant.

Le tokenizer et le decodage ne tournent donc plus en Ring 0 sur ce chemin :
`aiworker gguf session start|step <id> job <j> rc <r> tokens <n> [resumed]`
dans le journal du worker, et aucun pas `OS_AI_JOB_GGUF_STEP` n'est plus
emis par le noyau (le worker sait encore y repondre).

Restent en Ring 0, honnetement :

- la normalisation du prompt (`gpt2_generate_normalize`, avant l'envoi) ;
- le miroir de la session (jetons, graine, nombre de jetons du prompt, actif),
  qui sert a reprendre la session si le worker change ;
- le montage FAT16 au boot (aucune tache n'existe encore) et, a ce moment,
  le remplissage d'un instantane PMM a la taille du fichier ;
- la detection d'un worker suspendu (seulement le delai de 300 s) ;
- le repli FP32 de `SYS_GPT2_GENERATE`.

Le defaut `GGUF_RING0_FALLBACK=0` ne fait plus tourner le tokenizer ni la
session GGUF en Ring 0. Voir la section suivante. `GGUF_RING0_FALLBACK=1`
restaure ce chemin (`path kernel` / `path fallback`).

## Poids : lecture bulk reservee au worker

Le worker ne recoit pas de projection des pages noyau. Il remplit sa propre
copie :

1. `OS_AI_ENGINE_GGUF_OPEN` (worker seul) : le noyau mappe a
   `0xA1000000` des pages neuves, mises a zero, possedees par le worker (RW,
   128 Mio au plus), de la taille du `GPT2.GGU` resident. Elles sont rendues a
   la mort du worker.
2. `OS_AI_ENGINE_GGUF_READ(offset, longueur)` : morceaux de 1 Mio au plus,
   strictement sequentiels. Le noyau lit par `fat16_read_file_range_disk()`
   (secteurs du volume, `atadriver` des que ce pilote est vivant), meme si
   l'instantane boot existe encore, vers fenetre + offset. La destination
   est fixee par le noyau, pas par l'appelant.
3. Le worker branche son runtime sur cette memoire par une petite cale FAT16
   (`userspace/ai_fat16_shim.c`), initialise `gpt2_gguf_infer`, puis
   `OS_AI_ENGINE_GGUF_READY`, accepte seulement si toute la taille a ete lue
   (`[AI] ai-engine GGUF ready in Ring 3`). Ce syscall libere ensuite
   l'instantane noyau (`[AI] GGUF resident released pages N`).

Une tache autre que le worker recoit -144 sur ces trois operations (verifie
par `airogue`). La taille enregistree au boot reste apres la liberation, pour
qu'un worker suivant puisse rouvrir la fenetre et relire le disque.

## Compteurs (`OS_AI_ENGINE_STATUS`)

`gguf_worker_pid`, `gguf_bytes_loaded`, `gguf_forwarded`, `gguf_completed`,
`gguf_kernel`, `gguf_kernel_while_live` (pas faits en Ring 0 alors qu'un
worker GGUF etait pret : doit rester 0), `gguf_fallbacks`, `gguf_last_path`,
`gguf_last_result` et l'instantane de la session (`gguf_tokens`,
`gguf_prompt_tokens`, `gguf_token_count`). Session : `gguf_session_worker`
(jobs de session servis par le worker), `gguf_session_resumed` (pas ou le
worker a adopte le miroir noyau), `gguf_session_kernel` (sessions demarrees en
Ring 0). `ggufclient` les affiche (`session worker N resumed N ring0 N`). `aborted`, `rogue_refused` et
`pending` sont partages avec le chemin FP32 (un seul slot de relais).

## Preuve QEMU : `make qemu-ai-gguf`

La table ci-dessous decrit la pointe d'avant le repli strict (chemins `kernel`
et `fallback`, instantane de 100 Mio). La preuve courante est la section
"Instantane libere et repli strict".

Le CI n'a pas de poids GPT-2 GGUF. `tests/scripts/ai_gguf_fixture.py` ecrit un
GGUF v3 synthetique (architecture `gpt2`, graine 20261004, C=768, 1 couche,
vocabulaire 16, 32 positions ; plongements, normes et biais F32 ; `attn_qkv`
et `ffn_down` Q4_K, `attn_output` et `output` Q6_K, `ffn_up` Q3_K ; 4024704
octets) et le met en `GPT2.GGU` sur une image FAT16 IDE. L'initrd recoit la
fixture `llm.c` de #94 (tokenizer de 16 pieces). Un boot `-m 1024M`, environ
215 s en local :

| Etape | Resultat |
|---|---|
| Boot | `aiworker gguf ready bytes 4024704 at 0xa1000000 chunks 4`, `[AI] ai-engine GGUF ready in Ring 3` |
| `ggufclient` (109 puis 110 x 7) | 8 pas `path worker`, jetons `0 1 2 14 3 4 \| 1 14 6 14 0 5 13 7` |
| Worker tue | 8 pas `path kernel`, memes jetons |
| Nouveau worker | recharge sa copie, 8 pas `path worker`, memes jetons |
| Worker bloque (`task-priority`), pas `pending 1`, puis tue | premier pas `path fallback`, suite `path kernel`, memes jetons |
| `airogue` contre un worker GGUF vivant | GGUF_OPEN / READ / READY -144 ; puis 8 pas `path worker`, memes jetons |
| `ggufpause` : pas 0 sur le worker, worker tue et remplace avant le pas 1 | le nouveau worker adopte le miroir (`resumed` sur le premier `session step`), 7 pas `path worker`, memes jetons |
| Meme worker, FP32 | `aiclient` `path worker`, jetons `0 1 2 14 3 4 \| 3 0 1 7 6 4` |
| Compteurs finaux | gguf fwd 33, done 32, kernel 16, live 0, fallback 1, aborted 1, rogue 1, pending 0 ; session worker 32, resumed 1, ring0 2 |
| Memoire | empreinte du worker 4106 pages dont 983 pour la copie GGUF ; pages libres avant/apres sa vie 231994/231994 |

Depuis la tranche session, chaque execution worker est aussi verifiee sur le
journal du worker : une ligne `aiworker gguf session start <id>` puis une
ligne `session step <id>` par pas suivant, meme id, nombre de jetons final egal
a celui de l'instantane noyau, aucune ligne `aiworker gguf step` (ancien job
sans etat). Les chemins noyau et repli (`ring0` 1 puis 2) donnent les memes
jetons que le worker : le tokenizer Ring 3 et le tokenizer Ring 0 lisent le
meme fichier de l'initrd et executent le meme code. Environ 280 s en local
(196 s avant la scene `ggufpause`). En CI le contrat a son propre job (`ai-gguf`, environ 6 min sur un
runner) : place dans le job vfs-service, il portait ce job a 15 min 26 s.

L'egalite des jetons vient du meme code K-quant, des memes octets, du meme
code de session et de tokenizer et du meme etat de generateur. Le miroir
renvoye par le worker n'est pas une porte d'entree pour le Ring 0 : le
noyau borne la reponse et le pas Ring 0 refuse deja tout jeton hors
vocabulaire (`gpt2_gguf_infer`).

## Verification manuelle sur le GGUF reel (hors CI)

`gpt2-Q3_K_M.gguf` de `tensorblock/gpt2-GGUF` (97668800 octets, non commite),
image FAT16 par `scripts/make_gguf_fat16_image.py`, vrai tokenizer GPT-2 dans
l'initrd, sans checkpoint FP32, `-m 1024M`, TCG. Le worker charge sa copie
(`aiworker gguf ready bytes 97668800 at 0xa1000000 chunks 94`, environ 19 s
apres le lancement de QEMU). Dans le shell, `ai-model use gpt2.gguf`, puis
`ai abc de` et trois `ai-continue` :

| Pas | Worker | Noyau (worker tue) | Texte |
|---|---|---|---|
| `ai abc de` (109) | 5,4 s | 22,6 s | `maxwell` |
| `ai-continue` (110) | 2,6 s | 12,1 s | `DeliveryDate` |
| `ai-continue` | 2,6 s | 11,6 s | `Nitrome` |
| `ai-continue` | 2,7 s | 12,3 s | suite d'octets non ASCII, identique |

Textes egaux sur les deux chemins ; jetons du worker 29047 39749 42066 14827.

Cause de l'ecart (memes options -O3 -msse2 des deux cotes) : le chemin noyau
lisait les grandes matrices (`ffn_up`, `ffn_down`, projections d'attention,
`output`) par `fat16_open_file` / `fat16_file_seek` / `fat16_file_read`, qui
ignoraient la copie residente : relecture de l'entree racine, parcours de la
chaine FAT depuis le premier cluster a chaque seek, puis secteurs relus sur le
disque IDE par ATA PIO a chaque jeton. Seul `fat16_read_file_range` servait
la copie residente. Corrige ensuite : un fichier ouvert dont le nom est celui
de l'instantane resident est servi depuis la RAM (open, seek, read), le
chemin disque reste pour les autres fichiers ; l'instantane est abandonne
sur remontage, unlink ou rename du volume. Mesure sous TCG, meme protocole,
avant / apres la correction :

| Pas | Noyau avant | Noyau apres | Worker | Texte |
|---|---|---|---|---|
| `ai abc de` (109) | 22,9 s | 5,0 s | 5,5 s | `maxwell` |
| `ai-continue` (110) | 10,9 s | 2,4 s | 2,7 s | `DeliveryDate` |
| `ai-continue` | 10,7 s | 2,4 s | 2,6 s | `Nitrome` |
| `ai-continue` | 11,3 s | 2,4 s | 2,8 s | identique |

Sur la fixture synthetique : noyau 3,5 / 0,6 / 0,6 / 0,6 s avant, 1,0 / 0,2 /
0,2 / 0,2 s apres (worker 1,0 / 0,2 / 0,2 / 0,2 s), memes textes. Une mesure
par cas.

Rejeu apres la tranche session (meme image, meme protocole, une mesure par
cas) : le worker tokenise lui-meme le prompt avec le vrai tokenizer GPT-2
(`aiworker gguf session start 1 job 1 rc 7 tokens 3`, 2 jetons de prompt + 1
genere), puis `session step 1` pour chaque `ai-continue` (tokens 4, 5, 6).
Worker 5,2 / 2,7 / 2,6 / 2,6 s, noyau (worker tue) 5,1 / 2,6 / 2,7 / 2,6 s,
textes egaux (`maxwell`, `DeliveryDate`, `Nitrome`, meme suite d'octets).

## Phrase sur le GGUF reel (3 octobre 2026)

Les tableaux ci-dessus restent la mesure historique : un jeton par appel,
textes `maxwell` / `DeliveryDate` / `Nitrome`. Ce n'etait pas une preuve de
prose. Le decodeur Q3_K reutilisait les echelles 0..7 sur la seconde moitie
de chaque super-bloc, le decodeur Q6_K appliquait une echelle a 32 valeurs
au lieu de 16, et l'attention multi-tetes relisait toujours la requete de la
tete 0. L'embedding, le MLP (`ffn_up` Q3_K) et `output.weight` (Q6_K) etaient
donc du bruit.

Apres l'alignement sur le dequant GGML et la tranche de requete par tete,
`make qemu-gpt2-sentences` (TCG, `-m 1024M`, meme fichier 97668800 octets,
hors CI) envoie `the capital of france is` une fois le worker `GGUF ready`.
Le shell enchaine les pas 109/110 (temperature 0.2, ban du jeton precedent,
penalite 1.2, arret apres 80 caracteres et une fin de phrase, ou 24 pieces).
Une mesure :

| Prompt | Reponse | Duree | Accel |
|---|---|---|---|
| `the capital of france is` | `the city of the city of the capital, which is called "the city of france" and is known as "` | 67,4 s | tcg |

Le prompt mesure reste en minuscules. Le verrouillage en capitales venait du
pilote : le relachement Maj gauche est le scancode `0xAA`, filtre alors comme
code de controle, et Verr Maj n'inversait plus les lettres quand Maj etait
enfoncee. Les lettres suivent maintenant Maj XOR Verr Maj, dans les deux
sens. Les 43,916 s / 20,224 s ne sont pas rejoues.
`sub_second_claim_allowed` reste faux.

## Budget memoire

- `aiworker` : bss 12578400 octets (12,0 Mio ; 5619904 avant cette tranche),
  a cause des tampons statiques du runtime GGUF : cache KV GGUF 4,5 Mio
  (`gguf_kv_storage`, dimensionne pour GPT-2 124M) et tampon d'en-tete 2 Mio
  (`gguf_header`). Ce cout existe meme sans disque GGUF.
- Copie GGUF du worker : taille du fichier (4024704 octets pour la fixture,
  environ 93 Mio pour le Q3_K_M reel). L'instantane noyau n'est plus un BSS
  de 100 Mio : il est alloue en pages PMM a la taille du fichier au boot, puis
  rendu au PMM des `GGUF_READY`. Un worker vivant ne le double plus.

## Instantane libere et repli strict (3 octobre 2026)

- Au boot, `gpt2_gguf_infer_init_fat16` ouvre `GPT2.GGU`, alloue
  `ceil(taille / 4096)` pages avec `pmm_alloc_pages` et les remplit
  (`fat16_load_resident`) pendant qu'aucune tache ne tourne encore (PIO Ring 0).
- `OS_AI_ENGINE_GGUF_READ` ignore cet instantane
  (`fat16_read_file_range_disk`). Avec `atadriver` vivant, ce sont ses
  lectures de secteurs.
- `OS_AI_ENGINE_GGUF_READY` appelle `gpt2_gguf_infer_release_resident` :
  `fat16_resident_drop` puis `pmm_free_pages`. La taille du fichier reste.
  Un second worker relit le disque sans cet instantane.
- Defaut `GGUF_RING0_FALLBACK=0` (`-DMOHHDY_GGUF_NO_RING0_FALLBACK`, tampon
  `build/.gguf-ring0-fallback-0`). Sans worker GGUF pret, ou si le relais est
  abandonne (worker mort ou bloque 300 s), 109 et 110 rendent
  `OS_AI_GGUF_NO_WORKER` (-148). `ggufclient` imprime `path none`. Les
  compteurs `gguf_kernel`, `gguf_fallbacks` et `gguf_session_kernel` ne
  bougent pas. Le journal garde le prefixe
  `[AI] relay aborted: ai-engine lost or stalled` et, pour un job GGUF,
  termine par `Ring 0 refused`.
- `GGUF_RING0_FALLBACK=1` recompile `syscall.c` sans ce define et rend
  l'ancien chemin noyau. Le repli FP32 de `SYS_GPT2_GENERATE` ne depend pas
  de ce drapeau.

Preuve locale, binaires deja construits :
`python3 tests/integration/test_qemu_ai_gguf.py`, exit 0, 255 s
(3 octobre 2026). `make qemu-ai-gguf` du meme arbre, en comptant la
reconstruction userspace due au changement d'en-tete, a dure 361 s et a
aussi termine par exit 0. Fixture 4024704 octets, 983 pages liberees.

| Etape | Resultat |
|---|---|
| Boot | `[AI] build GGUF_RING0_FALLBACK=0`, `[AI] GGUF resident released pages 983`, `aiworker gguf ready bytes 4024704` |
| `ggufclient` | 8 pas `path worker`, jetons `0 1 2 14 3 4 \| 1 14 6 14 0 5 13 7` |
| Worker tue | un pas `rc -148 path none`. `kernel` 0, `ring0` 0 |
| Nouveau worker | recharge depuis le disque, 8 pas `path worker`, memes jetons |
| Pas en vol, worker tue | `rc -148 path none`, `aborted` 1, `fallback` 0, `kernel` 0, `ring0` 0 |
| `airogue` | GGUF_OPEN / READ / READY -144, puis 8 pas `path worker` |
| `ggufpause` | reprise `resumed`, memes jetons, chemin worker |
| FP32 | `aiclient` jetons `0 1 2 14 3 4 \| 3 0 1 7 6 4` |
| Compteurs finaux | fwd 33, done 32, kernel 0, live 0, fallback 0, aborted 1, rogue 1, pending 0 ; session worker 32, resumed 1, ring0 0 |
| Memoire | empreinte worker 4106 pages dont 983 pour la copie ; libres avant/apres 257599/257599 |

`make test-all` : 675/675 (674 avant `test_disk_range_ignores_resident_bytes`).
Registre guest : 206 noms. Pas de rejeu KVM. `sub_second_claim_allowed` reste
faux. Les 43,916 s / 20,224 s ne sont pas une mesure de cette pointe. Pas de
CI distante de ce commit.

## Limites et suite

- Le contrat local prouve le mecanisme sur la fixture synthetique seulement.
  La phrase reelle Q3_K_M est la section "Phrase sur le GGUF reel" : une
  mesure TCG, pas un nouveau tour de latence. La verification manuelle
  historique (4 pas, `maxwell`) reste dans son tableau.
- La normalisation du prompt et le miroir de session restent Ring 0. Le
  repli FP32 reste. Le montage ATA au boot, le repli disque sans `atadriver`,
  la sonde NIC et l'IRQ NIC restent Ring 0.
- Un worker suspendu n'est detecte que par le delai de 300 s ou par sa mort.
- Hors file : US-001, phases 4 a 8, TensorFlow Lite, facturation, OpenAI
  public, ecrasement FAT, renommage inter-repertoire, remplacement atomique,
  un nouveau tour de latence GGUF.
