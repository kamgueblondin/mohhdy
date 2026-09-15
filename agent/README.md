# Runtime agent MOHHDY (ASSIST-050 + sessions)

Arborescence parallèle au prototype i386. Origine HTTP, widget d'embed,
sessions visiteur isolees, console admin. Les reponses sont un **echo
stub local**, pas un LLM de production. Pas d'appel OpenAI. Pas le noyau
Multiboot.

Guides :

- [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md)
- [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md)

Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

```text
python3 agent/server.py
make agent-smoke
make agent-docker
docker run --rm -p 8080:8080 mohhdy-agent
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
```
