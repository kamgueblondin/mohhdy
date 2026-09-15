# AOS-593 à AOS-600 — Identité Ethernet requise par la session LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** boot NE2000, PROM MAC, préparation ARP/DHCP/DNS, statut LLM shell/QEMU

## Objectif

Ce macro-lot renforce le prérequis le plus bas niveau de l’orchestration LLM réseau : l’identité Ethernet. Avant cette modification, la sonde de boot configurait les anneaux NE2000 mais ne lisait pas la MAC PROM. Les API ARP, DHCP et TCP du pilote exigent pourtant une MAC valide fournie par `ne2k_device_t`.

> Un NE2000 ne peut être déclaré prêt pour une session LLM que si ses anneaux sont configurés **et** si sa MAC PROM est lue et validée.

## Changement de boot

La séquence `ne2k_boot_probe` exécute maintenant, dans cet ordre :

| Étape | Condition de succès |
|---|---|
| Sonde NE2000 | Le contrôleur ISA répond au reset. |
| Préparation et anneaux | Le mode PIO et les anneaux RX/TX sont configurés. |
| Lecture PROM | `ne2k_read_mac` extrait les six octets pairs de la PROM. |
| Validation MAC | L’adresse n’est ni nulle ni multicast. |
| Attachement IRQ | Le service d’interruption NE2000 est établi. |
| Publication | `boot_ne2k_present` devient vrai ; le contexte LLM peut signaler un NE2000 prêt. |

Toute erreur de configuration ou de MAC affiche un diagnostic et empêche la publication de disponibilité. Le statut LLM déjà exposé par `SYS_LLM_SESSION_STATUS` conserve donc une sémantique plus forte : son bit de préparation NE2000 implique une identité Ethernet utilisable, sans en divulguer les octets au shell.

## Intégration shell et QEMU

Dans le scénario QEMU avec `ne2k_isa`, `net-status json` confirme la détection NIC. Le smoke exécute ensuite `ai-runtime` et exige :

```text
Session LLM noyau  : IDLE (NE2000 pret)
```

Ce contrôle montre que la phase de session initiale est publiée sur une carte dotée d’une MAC validée. Dans le scénario sans NIC, le diagnostic reste `IDLE (NE2000 absent)`.

## Garanties

La MAC est conservée uniquement dans `ne2k_device_t`, où elle est déjà requise par les codecs Ethernet/ARP/DHCP. Elle n’est pas ajoutée au syscall LLM, ni imprimée par le shell. Le changement n’introduit ni allocation dynamique, ni buffer persistant additionnel, ni secret fournisseur.

## Tests et validation locale

| Vérification | Résultat |
|---|---|
| Suite complète | **378/378** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur sans NIC | Réussi. |
| Smoke QEMU NE2000 avec MAC PROM | Réussi. |
| `ai-runtime` avec NE2000 | Phase `IDLE (NE2000 pret)` vérifiée. |

## Limites connues

La MAC valide ne constitue pas encore un bail IPv4. DHCP, passerelle, DNS live, sélection de destination, buffers de service LLM noyau, syscalls d’émission/polling, identifiants hors image, validation TLS live, timeout/retry, fermeture TCP/TLS, historique conversationnel, tool calls, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-133 — lecture MAC PROM NE2000](aos133_ne2k_prom_mac.md)  
[2] [AOS-145 — offre DHCP et état de bail caller-owned](aos145_dhcp_offer_lease.md)  
[3] [AOS-585 à AOS-592 — plan de contrôle shell/noyau de session LLM](aos585_592_llm_session_control_plane.md)
