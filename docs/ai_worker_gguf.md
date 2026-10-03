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
`gguf_prompt_tokens`, `gguf_token_count`). `aborted`, `rogue_refused` et
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

## Budget memoire

- `aiworker` : bss 12578400 octets (12,0 Mio ; 5619904 avant cette tranche),
  a cause des tampons statiques du runtime GGUF : cache KV GGUF 4,5 Mio
  (`gguf_kv_storage`, dimensionne pour GPT-2 124M) et tampon d'en-tete 2 Mio
  (`gguf_header`). Ce cout existe meme sans disque GGUF.
- Copie GGUF du worker : taille du fichier (4024704 octets pour la fixture,
  environ 93 Mio pour le Q3_K_M reel), en plus du tampon resident noyau de
  100 Mio qui reste necessaire au repli et a la lecture bulk.

## Limites et suite

- Prouve sur la fixture synthetique seulement : le GGUF reel (Q3_K_M, 124M)
  n'a pas ete passe par le worker dans cette tranche.
- La copie residente du noyau reste (double occupation memoire avec un worker
  GGUF vivant).
- Tokenisation, decodage et session restent dans le noyau ; seul le pas
  d'echantillonnage passe en Ring 3.
- Un worker suspendu n'est detecte que par le delai de 300 s ou par sa mort.
- Suite possible : charger les poids par `atadriver`/VFS puis liberer le
  tampon resident noyau quand un worker GGUF est pret, et deplacer la session
  et la tokenisation dans le worker.
