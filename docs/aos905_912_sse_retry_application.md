# AOS-905 à AOS-912 — application du hint `retry:` SSE

Le scheduler SSE consomme désormais `retry_delay_ms` lorsque `retry_valid` est actif et que la valeur est strictement positive. Le délai est plafonné à `max_delay` avant délégation au backoff HTTP ; le reset consomme le hint tout en conservant `Last-Event-ID`. Aucun timer global, pointeur utilisateur ou `kmalloc` n’est ajouté.

Le test couvre l’application de 1 500 ms, le retour au backoff exponentiel après reset et le plafonnement d’un hint de 600 000 ms. La CI doit confirmer les 406 tests, le build i386 et les smokes QEMU.

L’intégration dans la boucle matérielle NE2000/TLS, les timers IRQ, le jitter et la persistance inter-session restent le prochain axe.
