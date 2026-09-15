# AOS-174 — Accumulateur de fragments TLS caller-owned

AOS-174 ajoute `net_tls_record_accumulator_t`, une structure d’état fournie et possédée par l’appelant. Elle référence un buffer d’assemblage externe, sa capacité et sa longueur courante. Chaque fragment TCP est copié dans ce buffer uniquement après contrôle de capacité.

La primitive retourne `1` lorsque le record TLS est incomplet et `0` lorsque le record annoncé est complet et parsé. Elle refuse les erreurs de framing et les dépassements, sans allocation, pointeur global ou buffer interne. La vue TLS publiée pointe vers le buffer de l’appelant et reste valide jusqu’à sa prochaine modification.

| Élément | Statut |
|---|---|
| État d’accumulation caller-owned | Implémenté. |
| Fragment incomplet non publié | Validé. |
| Record complet publié après second fragment | Validé. |
| Dépassement de capacité | Rejeté. |
| Allocation dynamique | Absente. |
| Cryptographie TLS et handshake complet | Non implémentés. |
