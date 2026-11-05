# Resolution Toolbar (Toolbar de Resolução) para OBS Studio

Plugin nativo do OBS Studio: uma **toolbar dentro da interface do OBS** para trocar
resolução sem abrir a janela de configurações de vídeo.

- **Sem WebSocket, sem porta, sem senha** — o plugin fala direto com o `libobs`.
- **Instalar = copiar uma pasta** para `obs-plugins` (ou rodar o instalador `.exe`).
- Distribuição via **GitHub Releases**, com o build automático do GitHub Actions.

## O que ele faz

| Controle | Função |
|---|---|
| Dropdown `Resolução…` | Aplica presets prontos: **1080p H/V** e **720p H/V** (muda Canvas **e** Saída, igual a config manual de vídeo) |
| Botão `⇄` | Alterna **horizontal ↔ vertical** em 1 clique, mantendo a qualidade atual (funciona com qualquer resolução, tipo 1440p) |
| Visor `1920×1080` | Mostra a resolução de saída atual (tooltip mostra o Canvas também) |
| Timer interno | Re-sincroniza o visor a cada 2s se você mudar a resolução por fora |

Comportamentos de segurança:

- Com **gravação ou transmissão ativa**, o plugin avisa que não é possível mudar
  (o OBS 31 bloqueia `obs_reset_video` enquanto há saída rodando).
- Mensagens de erro detalhadas se o OBS recusar a mudança (parâmetros inválidos, etc).
- Interface em **pt-BR** e **en-US** (usa o idioma do OBS).

## Instalar (usuário final)

Baixe o último release em **GitHub Releases** e:

- **Windows**: rode o instalador `.exe`, **ou** extraia o `.zip` e copie a pasta
  `resolution-toolbar` para `%APPDATA%\obs-studio\obs-plugins\` (ou abra o OBS e
  use *Add-Ons → Plugin/Aplicativo* se preferir arrastar).
- **macOS**: rode o `.pkg` ou extraia o `.tar.xz` em `~/Library/Application Support/obs-studio/plugins/`.
- **Linux**: extraia o `.tar.xz` para `~/.config/obs-studio/plugins/`.

Reinicie o OBS. A toolbar aparece em **View → Docks → Resolução** (dock flutuante, arraste
pra onde quiser).

## Compilar / distribuir (você)

Este projeto é baseado no [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
oficial e já vem com o pipeline pronto:

1. **Push do código**:
   ```bash
   git init
   git add .
   git commit -m "Toolbar de Resolução para OBS Studio"
   git remote add origin https://github.com/victoralexbr/meu-plugin-obs.git
   git push -u origin main
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
