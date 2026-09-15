# AOS-497 à AOS-504 — Horloge RTC UTC dans la politique NE2000/TLS

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** RTC CMOS i386, NE2000, politique temporelle X.509/TLS

## Objectif

Ce macro-lot relie la lecture UTC stable de l’horloge RTC i386 à la variante de production du polling TLS NE2000 qui utilise le premier certificat intermédiaire reçu. L’appelant n’a plus à composer ni fournir une chaîne UTC pour cette façade : l’instant est lu immédiatement avant la délégation à la politique TLS existante.

L’API ajoutée est `ne2k_tls_client_poll_received_chain_rtc`. Elle conserve tous les buffers réseau, workspaces RSA/X25519/PRF et états TCP/TLS appartenant à l’appelant, mais remplace le paramètre `utc_time` par un `rtc_io_t` injectable.

## Chemin d’exécution

| Étape | Action |
|---|---|
| 1 | La façade alloue uniquement un buffer automatique de 16 octets (`RTC_UTC_BUFFER_LENGTH`). |
| 2 | `rtc_read_utc` lit une photographie CMOS stable et produit `YYYYMMDDHHMMSSZ`. |
| 3 | En cas d’échec RTC, la façade retourne immédiatement une erreur, sans déléguer au polling TLS. |
| 4 | En cas de succès, elle appelle `ne2k_tls_client_poll_received_chain` avec l’instant UTC local. |
| 5 | La politique TLS existante vérifie chaîne, hostname et validité X.509 avant d’autoriser le flight X25519. |

> La lecture RTC est effectuée avant toute transition du client TLS dans cette façade. Une horloge indisponible, instable ou mal formée bloque donc l’authentification temporelle sans créer de chemin de repli vers une date arbitraire.

## Contrat mémoire et comportement d’erreur

| Propriété | Garantie |
|---|---|
| Allocation dynamique | Aucune. |
| Buffer UTC | Tableau automatique de 16 octets ; durée de vie limitée à la délégation synchrone. |
| I/O RTC | Injectée par `rtc_io_t` ; les tests ne dépendent pas des ports CMOS réels. |
| Échec RTC | Retour négatif avant appel TLS ; les sorties `consumed` et `flight_records_length` restent inchangées. |
| Échec TLS ultérieur | Les rollbacks transactionnels déjà fournis par `ne2k_tls_client_poll_received_chain` restent inchangés. |
| Compatibilité | Les APIs historiques recevant `utc_time` restent disponibles afin de ne pas casser les intégrations existantes. |

Le test NE2000 lie désormais explicitement `rtc.c` dans le Makefile et dans le runner direct de non-régression. Cette double déclaration élimine une divergence de liaison entre exécution ciblée et exécution complète.

## Tests ajoutés

Le fixture NE2000 configure un faux CMOS BCD 12 heures représentant `20260818000000Z`. Il vérifie que le polling RTC retourne le statut d’attente habituel sur RX vide et remet à zéro `consumed` ainsi que `flight_records_length` via la délégation existante. Il injecte ensuite une seconde BCD invalide ; la façade doit échouer avant le polling et préserver les deux sorties à leurs valeurs sentinelles.

| Vérification | Résultat |
|---|---|
| Test Unity NE2000 ciblé | **8/8** tests réussis. |
| Suite noyau | **32/32** exécutables de test réussis. |
| Suite complète | **368/368** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

La RTC n’est pas une source de temps authentifiée. La batterie, l’année siècle matérielle, la dérive, le fuseau firmware et toute synchronisation NTP sécurisée restent hors périmètre. La nouvelle façade ne remplace pas les APIs UTC caller-owned historiques et ne configure pas les ports CMOS réels : le noyau appelant doit toujours construire l’adaptateur i386 par `rtc_i386_io` lorsque ce chemin est choisi.

## Références

[1] [AOS-401 à AOS-408 — RTC UTC i386](aos401_408_rtc_utc.md)  
[2] [AOS-353 à AOS-360 — politique temporelle dans NE2000/TLS](aos353_360_ne2k_tls_time_policy.md)
