
## Regles de realisation du projet

Les regles suivantes s'appliquent a toute la suite du projet :

- Toutes les User Stories restent dans le perimetre du projet. Aucune fonctionnalite n'est annulee.
- Il n'existe pas de produit parallele, de second projet ou d'implementation de demonstration separee.
- Toute fonctionnalite doit etre integree au meme systeme d'exploitation MOHHDY et fonctionner dans QEMU.
- Le code partage du depot est la source unique. Les composants noyau, Ring 3, OS-UI, IA, reseau, stockage et Web Runtime doivent s'integrer au meme guest.
- Le portail web, s'il est realise, sera une fonction du Web Runtime de MOHHDY dans QEMU. Il ne remplacera pas le systeme d'exploitation et ne constituera pas une application independante.
- Une User Story n'est consideree comme livree qu'apres son implementation dans le guest, un test reproductible dans QEMU, un test de regression et la mise a jour de cet etat reel.
- Les limitations temporaires d'une implementation ne changent pas le perimetre. Elles doivent etre corrigees dans les increments suivants.

La feuille de route doit donc conduire progressivement du noyau actuel vers l'IA complete, l'OS-UI, PromptMessage, le Web Runtime, le reseau P2P, le multi-plateforme, les fonctions collaboratives et la production, sans retirer de phase.
