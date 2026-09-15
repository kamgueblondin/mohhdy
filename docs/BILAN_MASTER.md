# Bilan de la derniere version master

Date du bilan : 13 septembre 2026
Depot source : `github.com/kamgueblondin/ai-os`
Branche analysee : `origin/master`
Commit : `9078d5f` (`Add deep nested subdirectory test for FAT16 (#526)`)
Date du commit : 12 septembre 2026

Ce document decrit l'etat reel de la derniere version publiee sur
`master` avant le rebrand produit vers MOHHDY. Il ne remplace pas
`docs/ETAT_REEL.md` : il en fait la synthese a cette date.

## 1. Identite du projet au moment du bilan

Hobby OS i386 Multiboot, Ring 3, QEMU. Le depot s'appelait encore
`ai-os`. Les documents de fondation parlaient aussi de `mohhos`.
La version produit affichee dans le Makefile est **v7.0**.

Le noyau, le shell, le VFS, FAT16/FAT32, le reseau local et le runtime
IA local (GPT-2 124M optionnel, GGUF TinyLlama documente) sont presents
et testes. Rien dans ce snapshot n'est un acces Internet public, un
DHCP, ni un TLS authentifie vers un hote reel.

## 2. Preuves automatisees

`docs/ETAT_REEL.md` (constat du 27 aout 2026, toujours la page de
verite) et le tableau du README annoncent **522 / 522** tests verts
(`make test-all`).

Le dernier commit master ajoute un test FAT16 a trois niveaux de
sous-repertoires (`sub1/sub2/sub3/FILE.TXT`). C'est une preuve
supplementaire du parcours FAT multi-niveaux, pas un nouveau sous-systeme.

Rejeu local le 13 septembre 2026, apres le rebrand, sur ce meme
snapshot :

- `make all` : `build/mohhdy.bin` (606K) + initrd
- `make test-all` : **523 / 523** en 23 s. Le compteur documente
  522 etait donc deja en retard d'au moins le test profond #526.
  Le chiffre vivant a cette date est 523.
- `make qemu-smoke` : vert au second essai (core, extras, persist,
  spawn, syscalls, exec). Premier essai : residu PS/2 `hi.txtt`
  dans le sous-scenario syscall, flake clavier deja connu,
  independant du nom produit.

Le README contient encore une phrase plus ancienne qui parle de
"506 tests" plus bas dans le texte. C'est un residu de documentation,
pas un second compteur vivant.

## 3. Ce qui a ete ajoute sur master depuis le lot TLS d'aout 2026

Les merges recents, du plus recent au plus ancien :

- #526 test FAT16 profond (`sub1/sub2/sub3`)
- #525 Release v7.0 : GPT-2 124M local, validation QEMU, initrd
- #524 documentation reseau / TLS / HTTP / SSE / OpenAI local
- #523 verification de build, correction capture GUI
- #522 / #521 ICMP IPv4
- #520 / #519 verification d'execution
- #518 FAT16 / FAT32 : sous-repertoires multi-niveaux
- #517 a #498 : portee VFS, prefixe, worker, alias, NE2000 multi-pairs

Lots TLS / HTTP locaux deja sur master (aout 2026) :

- `make qemu-ne2k-tls-http` : HTTP clair puis TLS 1.2 / GET `/v1/models`
- `make qemu-ne2k-tls-sse` : SSE local `data: ping` / `data: done`
- `make qemu-ne2k-tls-next` : reutilisation de session + `ai-next`
- ICMP IPv4 local
- NE2000 multi-pairs

Ces cibles ne sont pas dans `qemu-smoke`. Elles restent du reseau
local QEMU (paire, HTTP, TLS d'exemple). Le SAN de test est
`example.com`.

## 4. Capacites reelles (pas marketing)

Fonctionne dans QEMU, avec preuves automatisees ou captures deja
faites sur ce depot :

- Boot Multiboot, clavier PS/2, VGA 80x25, scroll et curseur
- Shell Ring 3, builtins, overlay persistante (`AIOV`, format inchangé)
- VFS : `vfsvirtual`, `vfsserver`, prefixe refuse hors montage
- FAT16 / FAT32 : lecture, ecriture, mkdir, sous-repertoires imbriques
- Reseau local : NE2000, ARP, IPv4, ICMP, TCP, HTTP, TLS 1.2 d'exemple,
  SSE, session suivante
- IA locale : GPT-2 124M si les poids sont dans l'initrd ; GGUF
  TinyLlama documente (premiere generation lente, continue plus rapide)
- GUI : captures VGA possibles ; le paquet `qemu-system-gui` est
  necessaire pour le affichage GTK

Ne pas affirmer :

- un acces Internet public
- un DHCP
- un TLS authentifie vers un vrai hote
- un ChatGPT live ; `ai hello` en local produit un echantillon GPT-2
  court (`a the to.` dans les captures de validation), pas une phrase
  de chat

Les poids GPT-2 / GGUF ne sont pas dans Git. Leur absence n'est pas
une regression produit.

## 5. Dettes et points d'attention

- Divergence mineure README : 522 dans le tableau, 506 dans un
  paragraphe plus bas.
- `docs/ETAT_REEL.md` date son constat au 27 aout 2026 ; master a
  continue (#518 a #526) sans reouvrir cette page.
- Identite double `AI-OS` / `mohhos` dans le code, les docs, les
  labels FAT, les cibles de build (`build/ai_os.bin`, `build/ai_os.iso`)
  et l'URL `github.com/kamgueblondin/ai-os`.
- Numeros de tickets historiques `AOS-NNNN` : schema de suivi, pas le
  nom produit. Ils restent tels quels pour ne pas casser la tracabilite.
- Format overlay `AIOV` : magique on-disk, independant du nom produit.
- Commandes shell `ai`, `ai-runtime`, `ai-model`, `ai-continue`,
  `ai-next` : ce sont les commandes IA, pas le nom de l'OS.

## 6. Decision prise le 13 septembre 2026

Le produit s'appelle desormais **MOHHDY**. Toutes les references
produit `AI-OS` / `ai-os` / `mohhos` dans le code et la documentation
sont alignees sur ce nom. Le depot GitHub canonique est
`kamgueblondin/mohhdy`.

Concretement, dans cette livraison :

- images : `build/mohhdy.bin`, `build/mohhdy.iso`, menu GRUB `MOHHDY`
- prompt shell : `MOHHDY>`
- gardes d'inclusion et variables d'environnement : `MOHHDY_*`
- labels FAT OEM / volume : `MOHHDY` / `MOHHDY F16` / `MOHHDY F32` / `MOHHDY GGUF`
- fichiers `docs/mohhdy_foundation_increment_*.md`, `US/mohhdy_*.md`

Inchanges volontairement :

- tickets historiques `AOS-NNNN` et fichiers `docs/aos*.md`
- magique overlay `AIOV`
- commandes IA `ai`, `ai-runtime`, `ai-model`, `ai-continue`, `ai-next`
- le mot **OpenAI** (fournisseur / stub, pas le nom de l'OS)

## 7. Publication

La branche `cursor/rebrand-mohhdy-6f7a` est poussee sur
`kamgueblondin/ai-os` (PR #527). Le depot `kamgueblondin/mohhdy`
existe ; le push depuis cet agent a ete refuse (`cursor[bot]` n'a
pas le droit d'ecriture sur ce depot).
