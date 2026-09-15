# AOS-969 à AOS-976 — politique explicite fournisseur/modèle

Le chemin LLM expose `net_llm_model_policy_t`, une structure caller-owned contenant le fournisseur, un pointeur vers le nom du modèle, sa longueur et un indicateur d’autorisation de rotation. `net_llm_model_policy_validate` rejette les pointeurs nuls, les longueurs nulles ou supérieures à `NET_LLM_MODEL_NAME_MAX`, les fournisseurs inconnus, les indicateurs hors domaine et les caractères non imprimables.

La fonction ne copie pas le nom du modèle et ne déclenche aucune connexion, rotation ou émission réseau. Elle constitue un garde déterministe à placer avant les builders JSON et avant une éventuelle bascule de fournisseur. Les secrets et bearers restent en dehors de cette politique.

Cette séparation rend explicite la décision de modèle et permet au caller d’interdire la rotation lorsqu’une session doit rester liée à un fournisseur donné. Le test couvre une politique valide, une longueur nulle et un fournisseur inconnu. Validation locale : **413/413 tests verts**.

Auteur : **Manus AI**
