# Constats techniques - GPT-2 bare-metal

GPT-2 est un transformeur causal unidirectionnel : chaque jeton ne peut prêter attention qu'aux jetons précédents. Le premier moteur local devra donc implémenter au minimum les embeddings, les blocs de transformeur, l'attention causale, les couches de projection et la génération auto-régressive.

Le tokenizer GPT-2 est un Byte-Level BPE. Il ne suffit pas de séparer les mots par espaces : l'implémentation bare-metal devra charger le vocabulaire, les règles de fusion et respecter le traitement des espaces employé pendant l'entraînement.

Le noyau MOHHDY actuel reste un binaire i386 freestanding utilisant des adresses et un gestionnaire de pages 32 bits. Il ne peut pas exploiter les 16 Gio de la configuration de référence sans une évolution mémoire 64 bits ou PAE, ni adresser directement les poids d'un modèle GPT-2 complet au-delà de son espace virtuel disponible. Le premier jalon réellement testable doit donc dissocier : (1) un moteur GPT-2 avec de petits poids de test, (2) un chargeur de modèles sur support de démarrage, puis (3) la prise en charge d'un checkpoint GPT-2 compact quantifié.

Les sources consultées décrivent GPT-2 comme un modèle causal et son tokenizer comme un BPE au niveau octet. Le dépôt OpenAI de GPT-2 est archivé et son code est fourni sans mises à jour attendues, ce qui impose de figer localement les formats et outils retenus pour la reproduction.

## Sources

- https://huggingface.co/docs/transformers/en/model_doc/gpt2
- https://github.com/openai/gpt-2
