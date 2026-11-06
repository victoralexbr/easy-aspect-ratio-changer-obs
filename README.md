# Easy Aspect Ratio Changer for OBS (Toolbar de Resolução)

**Se fui útil considere ajudar no https://livepix.gg/victoralexbr com qualquer valor** 💙

Plugin nativo do OBS Studio: uma **toolbar dentro da interface do OBS** para trocar
resolução sem abrir a janela de configurações de vídeo.

- **Sem WebSocket, sem porta, sem senha** — o plugin fala direto com o `libobs`.
- **Instalar = descompactar o `.zip`** dentro da pasta do OBS Studio (sem configuração).
- Distribuição via **GitHub Releases**, com o build automático do GitHub Actions.

## O que ele faz

| Controle | Função |
|---|---|
| Dropdown `Resolução…` | Aplica presets prontos: **1080p H/V** e **720p H/V** (muda Canvas **e** Saída, igual a config manual de vídeo) |
| Botão `⇄` | Alterna **horizontal ↔ vertical** em 1 clique, mantendo a qualidade atual (funciona com qualquer resolução, tipo 1440p) |
| Visor `1920×1080` | Mostra a resolução de saída atual (tooltip mostra o Canvas também) |
| Botão `?` / clique direito | Escolhe o layout da barra: **Automático / Horizontal / Empilhada** + item **Sobre…** (persistido entre sessões) |
| Timer interno | Re-sincroniza o visor a cada 2s se você mudar a resolução por fora |

Comportamentos de segurança:

- Com **gravação ou transmissão ativa**, o plugin avisa que não é possível mudar
  (o OBS 31 bloqueia `obs_reset_video` enquanto há saída rodando).
- Mensagens de erro detalhadas se o OBS recusar a mudança (parâmetros inválidos, etc).
- Interface em **pt-BR** e **en-US** (usa o idioma do OBS).

## Instalar (usuário final)

Baixe o último release em **GitHub Releases** e:

- **Windows**: extraia (descompacte) o `.zip` **direto dentro da pasta de instalação do OBS** —
  normalmente `C:\Program Files\obs-studio\` (aceite a mesclagem de pastas; pede permissão de
  administrador). O próprio zip já vem no formato certo: o conteúdo da pasta `64bit\` vai para
  `obs-plugins\64bit\` e a pasta `data\` vai para `data\obs-plugins\easy-aspect-ratio-changer-obs\`.
  Reinicie o OBS depois. (Nenhum plugin moderno usa a pasta `32bit` — o OBS é 64 bits desde a
  versão 28.)
- **macOS**: rode o `.pkg` ou extraia o `.tar.xz` em `~/Library/Application Support/obs-studio/plugins/`.
- **Linux**: extraia o `.tar.xz` para `~/.config/obs-studio/plugins/`.

Reinicie o OBS. A toolbar aparece em **View → Docks → easy-aspect-ratio** (dock flutuante,
arraste pra onde quiser). Quando a dock estiver estreita e alta (ex.: apoiada na lateral) a
barra fica **empilhada** automaticamente; use o botão `?` ou o clique-direito na barra para
escolher **Automático / Horizontal / Empilhada** a qualquer momento. Ao trocar a resolução o
plugin também **reposiciona o zoom da pré-view** (ajuste automático à janela).

## Compilar / distribuir (você)

Este projeto é baseado no [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
oficial e já vem com o pipeline pronto:

1. **Push do código**:
   ```bash
   git init
   git add .
   git commit -m "Toolbar de Resolução para OBS Studio"
   git remote add origin https://github.com/victoralexbr/easy-aspect-ratio-changer-obs.git
   git push -u origin main --tags
   ```
2. O GitHub Actions roda o workflow **Push → Build Project** e compila para
   **Windows x64, macOS universal e Ubuntu 24.04** — os artefatos ficam na aba *Actions*.
3. Para publicar versões: crie uma **tag** no formato `1.0.0` (sem o `v`) e faça `push`.
   O workflow cria um **release draft** com `.zip`, `.exe` (instalador), `.pkg` e `.deb`,
   mais checksums. Revise e publique.
4. Sem placeholders pendentes: `buildspec.json` e os cabeçalhos de copyright em
   `src/*` já estão preenchidos com autor `victoralexbr`.

### Requisitos de build

Nada local: o Actions baixa o OBS 31.1.1, obs-deps e Qt6 automaticamente conforme
o `buildspec.json`. Para compilar local, veja as instruções do template oficial.

## Desenvolvimento rápido (pra onde mexer)

| Arquivo | O que é |
|---|---|
| `src/plugin-main.cpp` | Ciclo de vida do plugin, registra callback de frontend |
| `src/toolbar.cpp` | A toolbar: presets, toggle ⇄, visor de resolução, avisos |
| `data/locale/pt-BR.ini` / `en-US.ini` | Textos da UI (não precisa recompilar mexer aí) |
| `buildspec.json` | Nome do plugin, versão, dependências (não mexer normalmente) |
| `.github/workflows/*` | CI/CD de build e release |

## Licença

GPL-2.0 (mesma do template oficial do OBS — veja `LICENSE`).

---

Na pasta `docs/` ficam o overview da arquitetura nativa (do início do projeto) e uma
**versão web alternativa** da toolbar (`toolbar-resolucao.html`) que funciona como
Custom Browser Dock **sem compilar nada** (usa obs-websocket embarcado no OBS), caso
você queira testar antes de sair buildando.
