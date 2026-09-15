# Audit d'intégration LLM - MOHHDY

> **Lecture (août 2026).** Ce texte a commencé comme un audit d'un simulateur `fake_ai`. L'état courant est [ETAT_REEL.md](ETAT_REEL.md) : GPT-2 124M local, sonde GGUF, stub OpenAI. `fake_ai` n'est plus le moteur de `ai <texte>`.

## État observé du projet (mis à jour)

- MOHHDY est un hobby OS i386 32-bit freestanding (`-m32 -ffreestanding -nostdlib`), lancé dans QEMU. Ce n'est pas une distribution Linux.
- La commande `ai <texte>` appelle `SYS_GPT2_GENERATE` lorsque les poids sont dans l'initrd ; sinon le shell peut retomber sur un binaire historique.
- Aucune pile réseau, pilote Ethernet, flux TCP ou client HTTP n'existe dans le noyau. `net-status` et le profil OpenAI le disent explicitement.
- Les poids GPT-2, s'ils sont fournis au build, portent l'initrd à plusieurs centaines de Mio.

## Contraintes des fournisseurs externes

- Ollama est un programme d'un **autre** système (service hôte). Il n'est pas exécutable par le noyau freestanding et **n'est pas une voie d'intégration**.
- Une API OpenAI exige un transport HTTPS et une clé hors de l'image. Ce transport n'existe pas encore ([aos025_network_stub.md](aos025_network_stub.md)).

## Conclusion

L'objectif « inférence locale sur une machine sans OS préinstallé » se tient uniquement par un **moteur porté dans MOHHDY** (GPT-2 aujourd'hui, GGUF quantifié ensuite). Embarquer MOHHDY dans une distribution Linux pour y lancer Ollama est **rejeté** : cela ferait de MOHHDY un invité d'un autre OS, pas un hobby OS autonome.

Un modèle trop grand pour l'initrd se lira plus tard depuis un **volume FAT** sur disque IDE, pas depuis un système à inodes. Conception : [aos_fat_volume.md](aos_fat_volume.md).

## Sources

- https://docs.ollama.com/linux (documentation d'un service hôte, hors périmètre)
- https://docs.ollama.com/api/openai-compatibility
