
## Regles obligatoires du backlog

Ces regles completent toutes les phases et toutes les User Stories du document :

1. Toutes les User Stories US-001 a US-120 restent a implementer. Aucune User Story et aucune fonctionnalite demandee n'est annulee.
2. Toutes les implementations doivent fonctionner dans le meme systeme d'exploitation MOHHDY lance dans QEMU.
3. Il n'y aura pas d'application parallele, de prototype detache, de second code source ou de service qui remplace le guest MOHHDY.
4. Le noyau, Ring 3, l'OS-UI, l'IA locale, l'IA reseau, le stockage, le Web Runtime, PromptMessage, le reseau P2P, le multi-plateforme, la collaboration et la production doivent etre integres progressivement dans le meme code et le meme systeme.
5. Une fonctionnalite est livree uniquement lorsqu'elle est implementee dans le guest, utilisable depuis MOHHDY dans QEMU et couverte par un test reproductible.
6. Une limite technique ou une etape non terminee est un travail restant. Elle ne constitue jamais une annulation du besoin.
7. Le Web Runtime et toute interface web future sont des composants de MOHHDY executes dans QEMU. Ils ne sont pas un produit independant.

La feuille de route operationnelle doit etre l'ordre de construction de l'ensemble des phases, et non une selection qui reduit le perimetre historique. Les priorites indiquent l'ordre de travail, mais elles ne suppriment aucune User Story.
