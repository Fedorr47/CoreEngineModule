---
name: coreengine-interactive-atlas
description: Создание и обновление локального HTML-атласа CoreEngine (docs/AIAtlas) — MkDocs Material, офлайн-поиск, схемы Mermaid и вычисленные SVG, KaTeX при сборке, viewer с zoom/pan/fit/reset/expand, адаптивная раскладка, reference-страницы и учебные демо. Применять при добавлении главы, схемы, карточки сущности или демонстрации, при изменении сборки, CSS/JS атласа, и когда страница должна открываться из file:// без сети.
---

# CoreEngine: интерактивный атлас

Атлас — статический сайт `docs/AIAtlas/site/index.html`, собираемый из Markdown. Toolchain изолирован в
`docs/AIAtlas/.toolchain/` (в .gitignore) и не трогает сборку движка.

## Команды

```text
python -I -B docs/AIAtlas/tools/build_site.py          полная strict-сборка + проверки
python -I -B docs/AIAtlas/tools/build_site.py --force  + перерисовать все схемы
node docs/AIAtlas/tools/qa_browser.js                   браузерная проверка из file://, сеть заблокирована
python -I -B docs/AIAtlas/tools/check_drift.py          дрейф исходников с последней сборки
```

Подготовка toolchain с нуля — `docs/AIAtlas/README.md` (venv с mkdocs-material, npm: mermaid-cli, katex,
iframe-worker; puppeteer использует системный Chrome, Chromium не скачивается).

## Авторские директивы (обрабатывает `hooks/atlas_hooks.py`)

| Директива | Результат |
|---|---|
| `[[REF-ID]]` | ссылка на запись реестра источников; неизвестный ID ломает сборку |
| `@@code REF-ID N` (отдельная строка) | выдержка N строк от якоря с реальными номерами строк |
| `@@diagram id \| Подпись` (отдельный абзац) | встроенный SVG + подпись с видом, уровнем, источником, кодом |
| `[[cls:Name]]` | ссылка на карточку сущности |
| `[[SV]] [[EV]] [[INF]] [[HIST]] [[PROP]] [[NV]]` | значки статуса |
| `$…$`, `$$…$$` | формулы, рендер KaTeX при сборке (без JS в браузере) |

## Схемы

- Mermaid: `docs/AIAtlas/diagrams/mermaid/<id>.mmd` с заголовком `%% title:`, `%% kind:`, `%% level:`, `%% refs:`
  (+ `%% status: PROPOSED` для предложений — только в главе 20). Стрелки подписывать смыслом: вызов, данные,
  событие, владение, переход. Цепочки длиннее 4 узлов — `flowchart TB`. Отделять compile/load time, создание
  instance, кадр и teardown.
- Геометрия и графики — функции в `tools/gen_geometry.py` (вычисляются моделями `tools/atlas_models.py`); вид сверху
  X →, Z ↓; подписи не должны перекрываться — проверять скриншотом.
- После рендера: скриншот (`qa_browser.js --pages ... --full-page`) и просмотр; при наложениях — правка.

## Офлайн-требования (проверяются автоматически)

- Никаких CDN: шрифты отключены (`font: false`), KaTeX CSS/шрифты локально, iframe-worker shim локально
  (`extra.polyfills`) — иначе offline-плагин Material подставит unpkg и поиск без сети не работает.
- Наш JS не использует `fetch()`/динамический `import()`; данные демо встраиваются как `atlas-demo-data.js`.
- Без JS: текст, таблицы, SVG и формулы читаемы; `<details>` раскрываются кликом.

## Адаптивность

- Текст ограничен мерой `--atlas-reading-width`; таблицы и схемы — на всю ширину колонки.
- Длинные идентификаторы в таблицах получают `<wbr>` на границах camelCase/`::` при сборке.
- Проверять ширины 390, 1280, 1920, 3440 и масштаб 125–200 % (`--zoom 1,1.25,2`); горизонтальный overflow
  страницы — провал.

## Добавление главы

1. Файл `docs/chapters/NN-name.md` по шаблону skill `coreengine-theory-to-code`.
2. Записи реестра для новых якорей (`tools/references.py`), схемы, численные проверки.
3. Строка в `nav` (`mkdocs.yml`) и в таблице глав `docs/index.md`.
4. `build_site.py` → `qa_browser.js` → просмотр скриншотов.

Подробные правила вёрстки и типовые дефекты: [references/atlas-pitfalls.md](references/atlas-pitfalls.md).
