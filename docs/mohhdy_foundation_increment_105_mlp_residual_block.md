# MOHHDY Foundation — Incrément 105 : MLP et résiduel de bloc

**État :** implémenté et testé.

## Objectif

Le lot 105 ajoute `gpt2_gguf_mlp_forward_add_residual_fat16`. Cette primitive compose le forward MLP quantifié du lot 104 puis écrit sa sortie directement dans le résiduel caller-owned. Elle matérialise le second sous-bloc du Transformer sans introduire de buffer de sortie interne supplémentaire.

## Contrat

Les matrices suivent les formes GPT-2 `[C,4C]` et `[4C,C]`; `hidden` sert de scratch intermédiaire et `residual` reçoit la sortie finale additionnée. Les biais restent optionnels. La capacité du résiduel est vérifiée avant toute lecture quantifiée, et les erreurs du forward MLP sont propagées.

## Validation

Les tests vérifient le rejet d’un résiduel trop court et d’un pointeur nul sur la fixture GGUF existante. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le chemin MLP peut désormais être placé après LayerNorm et connecté au résiduel d’un bloc complet. La prochaine étape est l’assemblage d’un contexte de bloc combinant attention, projection de sortie, résiduel, LayerNorm et MLP.
