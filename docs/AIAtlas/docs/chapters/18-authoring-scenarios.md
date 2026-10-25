# 18. Авторинг и реальные сценарии: от минимального поведения до полного пути данных

!!! abstract "Коротко"
    Новое поведение — это согласованный набор: факты и действия (`.goap.json`), наблюдения и исполнители
    (`.behavior.json`), роли уровня (`.bindings.json`), при необходимости граф маршрутов (`.routes.json`), запись
    каталога и development-сценарий для воспроизводимой проверки. Ключевые правила: каждый эффект должен быть
    **наблюдаем**, каждый факт — иметь одного writer, радиусы исполнения и наблюдения — быть согласованы.
    Глава проводит Marker Visit от данных до результата и сводит все текущие сценарии. Статус: [[SV]].

## 1. Задача и интуиция

Предыдущие главы объясняли механизмы по отдельности. Автору поведения нужен **порядок работы** и проверочный
список: что объявить, что с чем должно совпадать, как убедиться, что агент действительно достигает цели, а не
«почти». Development-сценарии — способ проверять это через production API на специально построенных уровнях.

## 2. Теоретическая выкладка: правила корректного поведения

### 2.1. Наблюдаемость эффектов

Для каждого действия $a$ и каждого эффекта $(f, v) \in E_a$ должен существовать механизм наблюдения, который после
успеха $a$ в мире установит $f = v$. Иначе после успешного шага решение увидит невыполненную цель и будет
перепланировать (глава [08](08-decision-runtime.md)). Формально нужен «мост» $\mathcal O_f$: если runtime $a$
завершился успешно, то на ближайшем наблюдении $\mathcal O_f(\text{мир}) = v$.

### 2.2. Согласованность радиусов

Исполнение и наблюдение часто описывают «одно и то же место» разными числами. Для `move_to` к роли $T$ и
`within_distance` к той же роли:

$$
\underbrace{(r_{\text{move}} + 0.001)^2 + \Delta y^2}_{\text{худшая точка остановки}} \;\le\; R_{\text{obs}}^2 ,
$$

где $r_{\text{move}}$ — acceptance (горизонтальный), $\Delta y$ — разница высот основания агента и узла роли,
$R_{\text{obs}}$ — радиус наблюдения (3D). Для Access Key: `move_to` к монете — 0.35, `nearest_location` — 0.6,
монета на 0.35 выше пола: $0.351^2 + 0.35^2 \approx 0.246 \le 0.36$ ✓. Для Marker Visit: $r = R = 0.8$,
$\Delta y = 0.2$: $0.801^2 + 0.04 \approx 0.682 > 0.64$ ✗ (глава [13](13-steering.md) §6) [[INF]].

### 2.3. Минимальность фактов и конечность пространства

Каждый факт удваивает потенциальное число булевых состояний; каждый неограниченный счётчик делает пространство
бесконечным. Факты должны покрывать ровно то, от чего зависит применимость действий и цель (марковость, глава
[03](03-state-model.md)), а ресурсы — иметь конечные источники.

### 2.4. Сценарий как эксперимент

Сценарий — воспроизводимый эксперимент: **setup** создаёт нужные сущности и сохраняет baseline, **start**
приводит мир к baseline и вызывает production API, **update** вносит контролируемые вмешательства, **stop**
прекращает работу, **reset** восстанавливает всё изменённое (трансформы, видимость, pickup, InteractionPoint,
брони, links). Состояние сценария живёт в App; runtime Gameplay не должен содержать полей вида «Maze уже
запущен». Сценарий не заменяет тест: он упражняет настоящий путь кода, но результат оценивается человеком
(глава [19](19-diagnostics.md)).

## 3. Визуальное объяснение

@@diagram d18-authoring-steps | Порядок добавления поведения; нижняя ветвь — когда нужен новый тип компонента (C++).

@@diagram d18-marker-e2e | Сквозной путь Marker Visit: от операций сценария через фабрику, первый update, старт шага и движение до подтверждения цели наблюдением.

@@diagram d18-scenario-lifecycle | Жизненный цикл development-сценария.

## 4. Как это реализовано в CoreEngine

### 4.1. Минимальное поведение: Marker Visit

Определение ([[AS-MARKER-GOAP]]):

@@code AS-MARKER-GOAP 44

Behavior ([[AS-MARKER-BEHAVIOR]]) и роли ([[AS-MARKER-BINDINGS]]):

@@code AS-MARKER-BEHAVIOR 32

Отдельного C++-класса для поведения нет: используются зарегистрированные `target_available`, `within_distance` и
`move_to`.

### 4.2. Запуск из сценария

@@code SC-START-DECISION 6

Операции сценария вызывают тот же `GameplayRuntime::StartAIDecision`, что и игровой код ([[GR-START-DECISION]]).

### 4.3. Сценарии текущего снимка

| Сценарий | Вход | Что упражняет | Уровень |
|---|---|---|---|
| `ai-goap-marker-visit` | `startAIDecision marker_visit` | минимальная композиция, Arrival с бегом | `ai_goap_marker_visit_development` |
| `ai-goap-target-recovery` | `target_recovery` + `temporarilyRemoveTargetAtMidpoint` (0.5, 5 с) | continuation, `Failed` живого решения, восстановление ([[AS-SC-RECOVERY]]) | `ai_goap_target_recovery_development` |
| `ai-goap-access-key` | `access_key` | route costs, сбор, покупка, реакция | `ai_goap_access_key_development` |
| `ai-goap-access-key-replanning` | `access_key` + сбор coinA «чужими руками» после coinC | same-goal replanning ([[AS-SC-REPLAN]]) | `..._replanning_development` |
| `ai-goap-access-key-reservation` | два агента, `access_key_reserved` | брони, конкуренция ([[AS-SC-RESERVATION]]) | `..._reservation_development` |
| `ai-goap-access-key-jump` | `access_key_jump` + jump link 9470001 | traversal в GOAP-маршруте ([[AS-SC-JUMP]]) | `..._jump_development` |
| `core.ai_steering_playground` | `startFollowTarget`, `startFleeTarget` (5/7), `startMoveTo` | Seek, гистерезис, look-ahead, avoidance ([[AS-SC-PLAYGROUND]]) | `ai_steering_playground` |
| `core.navigation_pathfinding_maze` | `startNavigationPath` | NavMesh-путь, прямой FollowRoute ([[AS-SC-MAZE]]) | `navigation_pathfinding_maze` |

В каталоге решений 5 записей; сценарии Steering Playground и Maze — не решения, а прямые запуски действий. ID
сценария принадлежит App, ID решения — фабрике AI.

## 5. Разобранный пример: новый вариант «дойти до маркера и вернуться»

Учебный пример авторинга **только данными** [[PROP]] (в репозитории его нет; не добавлять в runtime в рамках
документации):

1. **Факты**: `markerPresent`, `markerReached` (latch — «побывал»), `atHome`.
2. **Цель**: `returnedAfterVisit` = `{markerReached: true, atHome: true}`.
3. **Действия**: `move_to/goMarker` (pre `markerPresent`, eff `markerReached`, cont `markerPresent`);
   `move_to/goHome` (pre `markerReached`, eff `atHome`).
4. **Наблюдения**: `target_available` → `markerPresent`; `within_distance` (marker, latch) → `markerReached`;
   `within_distance` (home, `requireEntity: false`) → `atHome`.
5. **Capabilities**: два контекста `move_to` с ролями `home → marker` и `marker → home`; радиусы выбрать по §2.2:
   при подъёме маркера на 0.2 и `R_obs = 0.8` взять acceptance 0.7 ($0.701^2 + 0.04 = 0.531 \le 0.64$).
6. **Bindings**: роли `home`, `marker` на узлы уровня; запись каталога; сценарий с baseline/reset.

Почему latch для `markerReached`: без него после ухода от маркера факт стал бы ложным, и остаток плана
(`goHome` требует `markerReached`) провалился бы на первом же кадре обратного пути.

## 6. Типичные ошибки, границы и проверка

- **Ненаблюдаемый эффект** → вечное перепланирование после успешного шага.
- **Несогласованные радиусы** → см. §2.2; проверяйте неравенство до запуска.
- **Факт без writer или два writer** → ошибка сборки.
- **Ветка по имени сценария в runtime** → запрещено архитектурой: сценарная специфика живёт в ассетах и App.
- **Сценарий без reset** → следующий запуск начинается с изменённого мира.
- **Реальное время в сценарии** → `temporarilyRemoveTargetAtMidpoint` использует `steady_clock`; на паузе или
  при медленном кадре длительность в кадрах различается ([[SC-REMOVE-TARGET]]).

Проверки: `TestGameplayAIAssetComposition.cpp` (14), `TestGameplayAIDecision.cpp` (5) — наличие [[SV]]; ручные
проверки сценариев — глава [19](19-diagnostics.md).

Далее: [19. Диагностика и проверка](19-diagnostics.md).
