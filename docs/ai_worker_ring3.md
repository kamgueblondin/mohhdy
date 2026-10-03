# Inventaire point 4 : inference GPT-2 FP32 en Ring 3 (`aiworker`, service `ai-engine`)

Date : 3 octobre 2026. Texte ASCII.

## Ce qui a bouge

- La generation FP32 `llm.c v3` de `SYS_GPT2_GENERATE` (22) tourne dans la tache
  Ring 3 `aiworker` des que celle-ci detient le service `ai-engine`. Le meme code
  (`kernel/llm/gpt2_generate.c`, `gpt2_infer.c`, `gpt2_sample.c`,
  `gpt2_tokenizer.c`, `gpt2_model.c`) est compile deux fois : dans le noyau
  (repli) et dans `userspace/aiworker` avec `-DMOHHDY_RING3 -msse2
  -mfpmath=sse` (820 instructions xmm dans le binaire : le chemin SSE2 du lot
  #61 s'execute a CPL 3).
- Le noyau ne calcule plus rien quand le worker est vivant : il normalise le
  prompt, le garde dans un slot de relais unique (`kernel/ai_relay.c`), sonne le
  worker par un message IPC d'expediteur 0 (`OS_IPC_AI_ENGINE_REQUEST`, trois
  mots : job, capacite, longueur du prompt ; jamais les octets du prompt) puis
  bloque l'appelant (`TASK_BLOCKED_KERNEL`). Le worker attend sur
  `SYS_IPC_RECV_WAIT` (154, reception bloquante de #93), tire le job par
  `SYS_AI_ENGINE` FETCH (prompt de 128 octets au plus), genere, puis repond par
  REPLY (texte 512 octets au plus, jetons, nombre de jetons du prompt), ce qui
  reveille l'appelant.
- Restent Ring 0 : le chargement au boot du checkpoint et du tokenizer depuis
  l'initrd (pour le repli) et le repli FP32 lui-meme. Le chemin GGUF
  (109-110) a suivi dans la tranche suivante : ses pas d'echantillonnage
  tournent aussi dans `aiworker`, voir [ai_worker_gguf.md](ai_worker_gguf.md).

## Poids : pas de copie

`SYS_AI_ENGINE` MAP (worker seul) projette en lecture seule, dans l'espace du
worker, les pages initrd qui contiennent `models/gpt2_124M.bin` (fenetre
`0x80000000`, 512 Mio au plus) et `models/gpt2_tokenizer.bin` (fenetre
`0xA0000000`, 16 Mio au plus). Ce sont les memes cadres physiques que ceux du
repli noyau. Les entrees de page portent le bit AVL `PAGE_BORROWED` (0x200) :
`vmm_destroy_user_directory` ne libere jamais ces cadres a la mort du worker.
Le worker construit son modele avec `gpt2_model_load_from_buffer` et son
tokenizer depuis la fenetre ; aucune donnee de poids ne transite par l'IPC de
96 octets ni par un tampon intermediaire. Limite assumee : les octets initrd
voisins qui partagent la premiere et la derniere page (moins de 4 Kio de chaque
cote) sont aussi lisibles ; l'initrd entier l'est deja par `SYS_READFILE`.

## Etat FPU/SSE par tache (nouveau)

Avant ce lot, aucun etat x87/SSE n'etait sauve : une tache Ring 3 qui utilise
xmm aurait vu ses registres ecrases par le noyau (lui aussi compile SSE2) ou
par une autre tache. Maintenant :

- `task_t` porte `fx_state[512]` aligne 16, initialise a FCW 0x037F et MXCSR
  0x1F80 (`task_fx_init`).
- Les stubs `irq0`, `irq1`, `irq3`, `isr_schedule`, `isr_syscall` et
  `isr_common_stub` font `fxsave` dans `current_task->fx_state` a l'entree
  depuis Ring 3 et `fxrstor` au retour ; depuis Ring 0 ils sauvent sur la pile
  noyau (zone alignee de 512 octets). `jump_to_task` restaure l'etat de la
  tache cible quand elle reprend en Ring 3.
- CR4.OSFXSR est deja positionne au boot (chemin SSE2 du lot #61).

## Exclusion, compteurs et repli

- `ai-engine` ne peut etre enregistre que par une tache nommee `aiworker`
  (`service_registry.c`) ; `task_set_name` refuse desormais les noms
  `aiworker`, `atadriver` et `ataclient` (ferme le contournement par renommage
  pour les trois services epingles).
- Pendant que le worker est vivant, il ne peut pas emprunter le moteur Ring 0
  (`SYS_GPT2_GENERATE` depuis le worker : -144). FETCH, MAP et LOG sont reserves
  a son PID. Une REPLY d'un autre PID est refusee (-144) et comptee
  `rogue_refused` ; une REPLY pour un job perime ou deja termine est refusee
  (-146) et comptee `stale_refused`.
- Compteurs (`SYS_AI_ENGINE` STATUS, `aistat`) : `forwarded`, `completed`,
  `aborted`, `fallbacks`, `kernel_infer`, `kernel_infer_while_live` (doit
  rester 0), `rogue_refused`, `stale_refused`, `pending`, et le dernier chemin
  (`OS_AI_PATH_KERNEL`/`WORKER`/`KERNEL_FALLBACK`) avec ses jetons.
- Chien de garde IRQ0 (`syscall_ai_relay_watchdog`) : worker tue, remplace ou
  bloque plus de 30000 ticks (300 s) -> le job est abandonne (`aborted`), le
  journal serie imprime `[AI] relay aborted: ai-engine lost or stalled; Ring 0
  fallback` et l'appelant execute le repli FP32 noyau (`fallbacks`). Jamais de
  rejeu vers le worker. Un appelant mort libere le slot.
- Un seul job en vol : un second appelant reprend son syscall plus tard.
- Le noyau lance `bin/aiworker` au boot si le checkpoint et le tokenizer sont
  presents dans l'initrd (`[AI] boot aiworker spawned`). Sans modele, pas de
  worker et le comportement anterieur est inchange.

## ABI

`SYS_AI_ENGINE` = 155, `MAX_SYSCALLS` = 156. Operations : STATUS 1, MAP 2,
FETCH 3, REPLY 4, LOG 5. Erreurs : `OS_AI_ENGINE_REQUIRED` -144,
`OS_AI_ENGINE_BAD_ARGUMENT` -145, `OS_AI_ENGINE_STALE` -146,
`OS_AI_ENGINE_NO_MODEL` -147. Structures dans `include/os_syscalls.h`.

## Preuve QEMU : `make qemu-ai-worker`

Le CI n'embarque pas les poids GPT-2 124M. Le contrat
(`tests/integration/test_qemu_ai_worker.py`) genere donc une fixture synthetique
`llm.c v3` (`tests/scripts/ai_worker_fixture.py` : T=32, V=16, L=2, 2 tetes,
C=32, graine 20261003, 109056 octets ; tokenizer de 16 pieces, 1068 octets),
l'emballe dans l'initrd, demarre `-m 1024M` sans NIC ni disque, puis restaure
l'initrd. Etapes et resultats locaux (environ 80 s) :

| Etape | Resultat |
|---|---|
| Boot | `[AI] boot aiworker spawned`, `aiworker ready ai-engine checkpoint 109056 ... layers 2 channels 32`, auto-test REPLY perimee -146 |
| Chemin worker | `aiclient rc 6 path 2 text [dabhge]`, jetons `0 1 2 14 3 4 \| 3 0 1 7 6 4` |
| Worker tue -> chemin noyau | `path 1`, memes jetons, meme texte |
| Nouveau worker bloque (`task-priority`), job `pending 1`, puis tue | `[AI] relay aborted ...`, `path 3` (repli), memes jetons |
| Worker + `airogue` | enregistrement -144, renommage en `aiworker` refuse, REPLY forgee -144 (comptee rogue), MAP -144, FETCH -144 ; puis chemin worker, memes jetons |
| Compteurs finaux | forwarded 4, completed 3, aborted 1, fallbacks 1, kernel_infer 2, kernel_infer_while_live 0, rogue 1, stale 3, pending 0 |
| Memoire | empreinte du worker 1404 pages ; pages libres avant/apres sa vie : 232240/232240 (les cadres initrd empruntes ne sont pas liberes) |

La generation n'est pas un argmax : c'est l'echantillonnage top-k deterministe
du noyau (graine derivee du prompt normalise, 12 pas au plus, arret sur EOT,
jeton repete ou saut de ligne). L'egalite des jetons entre chemins vient du
meme code, des memes poids et de la meme graine.

## Budget memoire

- `aiworker` : texte environ 25 Kio, bss 5619904 octets (5,36 Mio : cache KV
  4,5 Mio pour 12 couches x 64 positions x 768 canaux x K/V, logits 196 Kio,
  tables du tokenizer 658 Kio). Empreinte mesuree : 1404 pages de 4 Kio.
- Poids 124M : `gpt2_124M.bin` fait 497904640 octets (474,8 Mio) ; il tient dans
  la fenetre de 512 Mio et n'est pas copie. Cout : environ 119 tables de pages
  (environ 476 Kio de cadres noyau) a la projection, rendues a la mort du
  worker. L'invite doit avoir assez de RAM pour l'initrd (`-m 1024M` dans le
  contrat).

## Limites et suite

- Phrase mesuree sur les poids 124M, hors CI : le GELU FP32 est la formule
  OpenAI. Le guest a repondu a `ai my name is` par
  `a little bit different, but I'm not sure if it's because of the fact that I'm from the same country`.
  Detail : [gpt2_baremetal_deployment.md](gpt2_baremetal_deployment.md).
  La fixture synthetique de `make qemu-ai-worker` ne change pas.
- Le CI prouve le mecanisme sur la fixture synthetique, pas sur les poids 124M.
- Texte de reponse borne a 512 octets, prompt normalise a 128 octets (comme le
  chemin noyau).
- Un worker suspendu n'est detecte que par le delai de 300 s ou par sa mort ;
  `task-suspend` ne vise que les enfants READY, d'ou `task-priority` dans le
  contrat pour garder un job en attente.
- GGUF : fait dans la tranche suivante ([ai_worker_gguf.md](ai_worker_gguf.md)) :
  noyaux Q3_K/Q4_K/Q6_K et runtime GGUF compiles pour le worker, poids copies
  par une lecture bulk reservee au worker, pas 109-110 relayes. Le bss du
  worker passe alors a 12578400 octets (empreinte 3071 pages au moins).
