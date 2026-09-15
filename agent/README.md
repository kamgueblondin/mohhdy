# Runtime agent MOHHDY (scaffold ASSIST-050)

Arborescence parallèle au prototype i386. Ce n'est **pas** le noyau Multiboot,
**pas** un chat IA, **pas** des sessions persistantes.

Guide : [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md).
Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

```text
python3 agent/server.py
# ou
make agent-smoke
make agent-docker
docker run --rm -p 8080:8080 mohhdy-agent
```
