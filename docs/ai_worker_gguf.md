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
  qui sert a reprendre la session si le worker change ou disparait ;
- un tokenizer, un decodeur et le meme code de session pour le repli : sans
  worker GGUF pret, ou si un job est abandonne (worker mort ou bloque 300 s),
  le noyau fait `start` + pas ou le pas lui-meme a partir du miroir (`path
  kernel` / `path fallback`) ;
- le montage FAT16 et la copie residente de 100 Mio de `GPT2.GGU` (servent la
  lecture bulk du worker et le repli) ;
- la detection d'un worker suspendu (seulement le delai de 300 s).

## Poids : lecture bulk reservee au worker

Le worker ne recoit pas de projection des pages noyau. Il remplit sa propre
copie :

1. `OS_AI_ENGINE_GGUF_OPEN` (worker seul) : le noyau mappe a
   `0xA1000000` des pages neuves, mises a zero, possedees par le worker (RW,
   128 Mio au plus), de la taille du `GPT2.GGU` resident. Elles sont rendues a
   la mort du worker.
2. `OS_AI_ENGINE_GGUF_READ(offset, longueur)` : morceaux de 1 Mio au plus,
   strictement sequentiels. Le noyau copie par `fat16_read_file_range()`
   depuis son instantane resident vers fenetre + offset ; la destination est
   fixee par le noyau, pas par l'appelant.
3. Le worker branche son runtime sur cette memoire par une petite cale FAT16
   (`userspace/ai_fat16_shim.c`), initialise `gpt2_gguf_infer`, puis
   `OS_AI_ENGINE_GGUF_READY`, accepte seulement si toute la taille a ete lue
   (`[AI] ai-engine GGUF ready in Ring 3`).

Une tache autre que le worker recoit -144 sur ces trois operations (verifie
par `airogue`). Le chemin `atadriver`/VFS n'est pas utilise : la lecture
passe par la copie residente que le noyau a deja.

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
| Meme worker, FP32 | `aiclient` `path worker`, jetons `0 1 2 14 3 4 \| 3 0 1 7 6 4` |
| Compteurs finaux | gguf fwd 25, done 24, kernel 16, live 0, fallback 1, aborted 1, rogue 1, pending 0 |
| Memoire | empreinte du worker 4106 pages dont 983 pour la copie GGUF ; pages libres avant/apres sa vie 232117/232117 |

L'egalite des jetons vient du meme code K-quant, des memes octets et du meme
etat de generateur transmis a chaque pas.

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

## Budget memoire

- `aiworker` : bss 12578400 octets (12,0 Mio ; 5619904 avant cette tranche),
  a cause des tampons statiques du runtime GGUF : cache KV GGUF 4,5 Mio
  (`gguf_kv_storage`, dimensionne pour GPT-2 124M) et tampon d'en-tete 2 Mio
  (`gguf_header`). Ce cout existe meme sans disque GGUF.
- Copie GGUF du worker : taille du fichier (4024704 octets pour la fixture,
  environ 93 Mio pour le Q3_K_M reel), en plus du tampon resident noyau de
  100 Mio. Ce tampon sert la lecture bulk du worker et, depuis la correction
  ci-dessus, les lectures d'un pas d'inference du repli Ring 0 (plages par
  nom et fichiers ouverts) ; sans lui le repli relirait le disque a chaque
  jeton.

## Limites et suite

- Le CI prouve le mecanisme sur la fixture synthetique seulement ; le GGUF
  reel (Q3_K_M, 124M) n'a ete verifie qu'a la main (4 pas, voir plus haut).
- La copie residente du noyau reste (double occupation memoire avec un worker
  GGUF vivant).
- Tokenisation, decodage et session passent en Ring 3 sur le chemin worker,
  mais le noyau garde la normalisation, un miroir de la session et tout le
  code de repli (voir plus haut).
- Un worker suspendu n'est detecte que par le delai de 300 s ou par sa mort.
- Suite possible : charger les poids par `atadriver`/VFS puis liberer le
  tampon resident noyau quand un worker GGUF est pret ; retirer le repli
  Ring 0 (tokenizer, session) une fois un mode strict prouve, comme pour le
  reseau.
