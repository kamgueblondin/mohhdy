# Runtime agent MOHHDY (ASSIST-050 + sessions + droits)

Arborescence parallèle au prototype i386. Origine HTTP, widget d'embed,
sessions visiteur isolees, base autorisee locale, masque de droits,
escalade et handoff humain. Les reponses sont un **stub local** (echo
ou extraits de KB), pas un LLM de production. Pas d'appel OpenAI. Pas
le noyau Multiboot.

Guides :

- [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md)
- [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md)
- [../docs/assist012_droits_handoff.md](../docs/assist012_droits_handoff.md)

Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

```text
python3 agent/server.py
MOHHDY_AGENT_CONFIG=agent/config.example.json python3 agent/server.py
make agent-smoke
make agent-docker
docker run --rm -p 8080:8080 mohhdy-agent
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
docker run --rm -p 8080:8080 \
  -e ADMIN_TOKEN=change-me-at-runtime \
  -e MOHHDY_AGENT_CONFIG=/app/config.example.json \
  mohhdy-agent
```
