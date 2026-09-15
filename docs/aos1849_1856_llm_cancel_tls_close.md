# AOS-1849…1856 — Annulation LLM avec fermeture TLS ordonnée

## Objet

`kernel_llm_close_internal()` tente désormais une alerte TLS `close_notify` avant le FIN TCP best-effort lorsque le socket LLM est établi et que le handshake TLS est complet. L’implémentation utilise un adaptateur socket qui copie la connexion TCP et le client TLS, publie le suivi d’émission seulement après succès, puis restaure les deux états sur une erreur antérieure au commit.

| Condition du contexte | Action |
|---|---|
| Socket absent ou non établi | Purge locale inchangée |
| Socket établi, TLS incomplet | FIN best-effort puis purge locale |
| Socket établi, TLS complet | `close_notify`, puis FIN best-effort, puis purge locale |
| Échec de l’alerte ou du FIN | Code de fermeture transport en échec, purge locale maintenue |

> La purge locale reste inconditionnelle après une annulation active : elle efface le contexte LLM et les matériaux TLS conformément au contrat existant, même si le matériel réseau ne peut pas transmettre une étape de fermeture.

La validation complète confirme la compilation du noyau et **474/474** tests réussis. Aucun appel d’allocation dynamique n’est ajouté dans `kernel.c`, `ne2k.c` ou les contrats de fermeture TLS.

## Références

[1] [AOS-1841 à AOS-1848 — émission NE2000 de close_notify](aos1841_1848_ne2k_tls_close_notify.md)  
[2] [RFC 5246 — TLS 1.2, §7.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-7.2.1)
