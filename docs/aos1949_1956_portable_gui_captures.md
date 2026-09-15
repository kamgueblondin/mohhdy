# AOS-1949…1956 — Captures QEMU GUI portables et validation graphique

## Objet

Le test GUI historique nécessitait une fenêtre QEMU et une intervention manuelle. Le scénario `gui-captures` est désormais reproductible sans privilège : il place ses captures dans `test_logs/gui-captures/` par défaut, avec surcharge possible par `MOHHDY_GUI_SHOT_DIR`.

| Propriété | Comportement livré |
|---|---|
| Répertoire de captures | Local au dépôt par défaut, sans écriture dans `/opt`. |
| Pilotage QEMU | Monitor Unix, injections `sendkey` et captures `screendump`. |
| Saisie robuste | Contrôle de la ligne lue ; jusqu’à trois essais en cas de duplication transitoire de caractères. |
| Preuves | 22 captures PNG : shell, aide, FAT16, overlay, IA/OpenAI et NE2000. |

## Exécution validée

`make gui-captures` a été exécuté avec succès. La session core a produit les captures `01-shell.png` à `19-ai-hello-openai.png`. La session NE2000 a produit `20-ne2k-shell.png` à `22-ne2k-net-status-json.png`.

> Les captures montrent le shell MOHHDY prêt à recevoir des commandes, puis `net-status json` avec une carte NE2000 détectée, TCP socket disponible et TLS authentifié.

Les artefacts de test restent ignorés par Git sous `test_logs/`, afin de ne pas alourdir le dépôt ; le script et le rapport final documentent la commande de reproduction.

## Validation

| Contrôle | Résultat |
|---|---:|
| `make gui-captures` | Réussi, 22 captures PNG |
| Capture shell initiale | Rendu QEMU GTK validé |
| Capture NE2000 finale | NIC détectée et état JSON validé |
| `make -s test-all` | Exécuté dans le gate de publication |

## Référence

[1] [QEMU Monitor Protocol — Human Monitor Commands](https://www.qemu.org/docs/master/system/monitor.html)
