# OS-UI : bureau graphique QEMU + cerveau guest C

Le chrome produit est le **framebuffer VBE** dans la fenetre QEMU
(`kernel/gfx_desktop.c`, VBE suit la fenetre QEMU). Le **cerveau** reste
`userspace/osui_runtime.c`. `gui` / `graphics` / `desktop` entre ce mode ;
`console` / ESC revient au texte.

Ce n'est **pas** une fenetre HTML hote, **pas** le sidecar `agent/`
(retire, OS-UI-3). `python_facade=false`. `display_host=false`.
`guest_html_stage=false`. `llm=stub_echo`. `us031_complete=false`.
`chrome=qemu_fb`. `display_surface=vbe_lfb`.

```text
make all
make run-gui          # QEMU GTK ; tapez gui apres MOHHDY>
make qemu-osui-gui    # contrat nographic + screendump, hors integration-qemu
make osui-smoke       # registre + facade absente
```

Docker reste nographic par defaut.

Guides : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md),
[../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md).
