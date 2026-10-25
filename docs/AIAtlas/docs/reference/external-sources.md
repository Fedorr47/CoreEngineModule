# Внешние источники теории

Внешние работы использованы **только для теории и сравнения**; поведение CoreEngine подтверждается его исходниками
(реестр источников). Номера страниц не указываются там, где они не проверялись; работы цитируются пересказом.
Онлайн-доступность адресов при подготовке атласа не проверялась [[NV]].

| Работа | Где используется | Что взято | Чего НЕ следует из работы |
|---|---|---|---|
| R. E. Fikes, N. J. Nilsson. *STRIPS: A New Approach to the Application of Theorem Proving to Problem Solving.* Artificial Intelligence 2 (3–4), 1971. | [02](../chapters/02-goap-strips.md) | представление действий через pre/add/del, допущение STRIPS | числовые факты, стоимости и continuation CoreEngine |
| J. Orkin. *Three States and a Plan: The A.I. of F.E.A.R.* Game Developers Conference, 2006. | [02](../chapters/02-goap-strips.md) | GOAP: цели, стоимости, планирование в играх; регрессивный поиск A* в F.E.A.R. | что `FindAIPlan` — A* (он uniform-cost, прогрессивный) |
| M. Fox, D. Long. *PDDL2.1: An Extension to PDDL for Expressing Temporal Planning Domains.* JAIR 20, 2003. | [04](../chapters/04-goals-actions.md) | различие at start / over all / at end условий | реализация длительных действий в CoreEngine |
| E. W. Dijkstra. *A Note on Two Problems in Connexion with Graphs.* Numerische Mathematik 1, 1959. | [05](../chapters/05-planner.md), [11](../chapters/11-spatial-routes.md) | кратчайшие пути при неотрицательных весах | — |
| P. E. Hart, N. J. Nilsson, B. Raphael. *A Formal Basis for the Heuristic Determination of Minimum Cost Paths.* IEEE Trans. SSC 4(2), 1968. | [05](../chapters/05-planner.md) | A* и допустимые эвристики (для сравнения) | наличие эвристики в CoreEngine |
| A. B. Kahn. *Topological Sorting of Large Networks.* Communications of the ACM 5(11), 1962. | [07](../chapters/07-observations-events.md) | топологическая сортировка | устойчивый выбор «первого готового» — свойство кода CoreEngine |
| E. Gat. *On Three-Layer Architectures.* In: Artificial Intelligence and Mobile Robots, AAAI Press, 1998. | [01](../chapters/01-architecture.md) | делиберативный слой / исполнитель / реактивный слой | — |
| C. W. Reynolds. *Steering Behaviors for Autonomous Characters.* Game Developers Conference, 1999. | [13](../chapters/13-steering.md), [15](../chapters/15-obstacle-avoidance.md) | слои action selection / steering / locomotion, seek, flee, arrival, obstacle avoidance | что magnitude CoreEngine — сила; формулы CoreEngine проще |
| M. Mononen. *Recast & Detour* (RecastNavigation v1.6.0, репозиторий и документация `recastnavigation`). | [11](../chapters/11-spatial-routes.md) | вокселизация, эрозия, `findNearestPoly`, `findPath`, `findStraightPath`, частичные результаты | политика CoreEngine (частичный коридор → NoPath) — из его кода |
| Jolt Physics 5.6 (документация `JoltPhysics`). | [15](../chapters/15-obstacle-avoidance.md), [16](../chapters/16-traversal-physics.md) | sweep сферы, лучи, CharacterVirtual, гравитация по умолчанию | — |
| G. Fowler, L. C. Noll, K.-P. Vo. *FNV hash* (описание алгоритма Fowler–Noll–Vo). | [03](../chapters/03-state-model.md) | константы FNV-1a | — |
| H. Minkowski (сумма множеств); стандартная вычислительная геометрия. | [11](../chapters/11-spatial-routes.md), [15](../chapters/15-obstacle-avoidance.md) | эрозия/раздувание на радиус | — |

Исторический источник проекта: `docs/_reference/ai-atlas/CoreEngine_AI_Documentation_RU.pdf` (редакция
08.09.2026, снимок `55de4ab`) — [[HIST]]; сверка — `audit/reference-delta.md`.
