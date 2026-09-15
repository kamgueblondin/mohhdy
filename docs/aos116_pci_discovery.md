# AOS-116 — Accès PCI borné pour le futur pilote NIC

## Objectif

Le lot AOS-116 ajoute la première primitive matérielle nécessaire au réseau effectif: accès à l’espace de configuration PCI via les ports standard `0xCF8` et `0xCFC`, calcul d’adresses alignées et recherche bornée d’un périphérique par classe et sous-classe.

## Contrat

`pci_config_address` masque les champs bus, slot, fonction et offset aux largeurs PCI attendues. `pci_decode_id` extrait les identifiants vendor/device sans conserver de pointeur externe. `pci_find_class` parcourt au plus 256 bus, 32 slots et 8 fonctions et écrit le résultat dans une structure caller-owned.

La recherche ignore les entrées vendor `0xffff`. Aucun tableau dynamique ni état global n’est créé. La lecture matérielle n’est pas invoquée dans les tests Unity; les tests couvrent le calcul d’adresse et le décodage pur.

## Validation

Le test `tests/unit/kernel/test_pci.c` vérifie l’alignement et le masquage de l’adresse de configuration ainsi que l’extraction des identifiants d’un registre PCI. La validation du groupe est :

```text
make test-all                  272 tests, 272 passés, 0 échec, 0 ignoré
make all                       build i386 et initrd réussis
```

Le lot ne sélectionne encore aucun modèle de NIC et ne programme pas d’anneau matériel. Le prochain jalon devra détecter explicitement un contrôleur supporté, récupérer ses BAR/IRQ et raccorder ses buffers à `net_nic_queue_t`.
