/*
 * CoreEngine AI Atlas — учебные демонстрации (без зависимостей и без fetch; данные — window.ATLAS_DEMO_DATA).
 *
 * 1) GOAP-поиск: пошаговая модель FindAIPlan (uniform-cost, порядок действий (actionId, contextId), tie-break по
 *    порядку вставки, отсечение known <= nextCost, проверка цели при извлечении) на реальных ассетах.
 * 2) Arrival и гистерезис Flee: формулы BuildGameplayArrivalSteering и AIFleeTargetActionRuntime.
 * Это учебные модели (float64), а не запуск движка: нет наблюдений, исполнения, кадров и физики.
 */
(function () {
    'use strict';

    const OPS = {
        '>=': (a, b) => a >= b, '<=': (a, b) => a <= b, '==': (a, b) => a === b,
        '!=': (a, b) => a !== b, '<': (a, b) => a < b, '>': (a, b) => a > b,
    };
    const fmt = (value) => (Math.round(value * 10000) / 10000).toString();
    const esc = (text) => String(text).replace(/[&<>"]/g, (c) => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}[c]));

    // ------------------------------------------------------------------ GOAP
    function compileActions(domain, data) {
        return domain.goap.actions.map((action, index) => Object.assign({}, action, {
            contextId: index,
            actionId: data.semanticIds[action.action],
            cost: Object.prototype.hasOwnProperty.call(domain.routeCosts, action.context)
                ? domain.routeCosts[action.context] : action.cost,
        })).sort((a, b) => (a.actionId - b.actionId) || (a.contextId - b.contextId));
    }

    function failed(state, action) {
        const reasons = [];
        (action.preconditions || []).forEach((c) => {
            if (state[c.fact] !== c.value) {
                reasons.push(`${c.fact} == ${c.value} (сейчас ${state[c.fact]})`);
            }
        });
        (action.numericPreconditions || []).forEach((c) => {
            if (!OPS[c.op](state[c.fact], c.value)) {
                reasons.push(`${c.fact} ${c.op} ${c.value} (сейчас ${state[c.fact]})`);
            }
        });
        return reasons;
    }

    function apply(state, action) {
        const next = Object.assign({}, state);
        (action.effects || []).forEach((e) => { next[e.fact] = e.value; });
        for (const e of action.numericEffects || []) {
            const value = e.op === 'set' ? e.value : next[e.fact] + e.value;
            if (value < -2147483648 || value > 2147483647) {
                return null;
            }
            next[e.fact] = value;
        }
        return next;
    }

    // Генератор событий поиска: позволяет шагать по одному извлечению.
    function* search(domain, actions, start) {
        const facts = domain.goap.facts.map((f) => f.name);
        const key = (s) => facts.map((f) => String(s[f])).join('|');
        const goal = domain.goap.goals[0];
        const satisfied = (s) => goal.facts.every((c) => s[c.fact] === c.value);
        if (satisfied(start)) {
            yield {type: 'done', plan: [], cost: 0, expanded: 0, note: 'цель уже выполнена — пустой план'};
            return;
        }
        const best = new Map([[key(start), 0]]);
        let sequence = 0;
        const queue = [{cost: 0, seq: 0, state: start, steps: []}];
        let expanded = 0;
        while (queue.length) {
            queue.sort((a, b) => (a.cost - b.cost) || (a.seq - b.seq));
            const node = queue.shift();
            if (best.get(key(node.state)) !== node.cost) {
                yield {type: 'stale', node, queue: queue.slice()};
                continue;
            }
            expanded += 1;
            if (satisfied(node.state)) {
                yield {type: 'done', plan: node.steps, cost: node.cost, expanded, node};
                return;
            }
            const rows = [];
            for (const action of actions) {
                const reasons = failed(node.state, action);
                if (reasons.length) {
                    rows.push({context: action.context, verdict: 'отклонено', detail: reasons.join('; ')});
                    continue;
                }
                const next = apply(node.state, action);
                if (!next) {
                    rows.push({context: action.context, verdict: 'отклонено', detail: 'переполнение int32'});
                    continue;
                }
                const nextCost = node.cost + action.cost;
                const known = best.get(key(next));
                if (known !== undefined && known <= nextCost) {
                    rows.push({context: action.context, verdict: 'не лучше', detail: `known ${fmt(known)} ≤ ${fmt(nextCost)}`});
                    continue;
                }
                sequence += 1;
                best.set(key(next), nextCost);
                queue.push({cost: nextCost, seq: sequence, state: next, steps: node.steps.concat(action.context)});
                rows.push({context: action.context, verdict: 'в очередь', detail: `cost ${fmt(nextCost)}, seq ${sequence}`});
            }
            yield {type: 'expand', node, rows, queue: queue.slice().sort((a, b) => (a.cost - b.cost) || (a.seq - b.seq)), expanded};
        }
        yield {type: 'done', plan: null, expanded, note: 'очередь пуста — FindAIPlan вернул бы nullopt'};
    }

    function stateText(state, goap) {
        const bools = goap.facts.filter((f) => f.type === 'bool' && state[f.name]).map((f) => f.name);
        const ints = goap.facts.filter((f) => f.type === 'int').map((f) => `${f.name} = ${state[f.name]}`);
        return (bools.length ? bools.join(', ') : '(все bool = false)') + (ints.length ? '; ' + ints.join(', ') : '');
    }

    function setupGoap(root) {
        const data = window.ATLAS_DEMO_DATA;
        if (!data) {
            root.textContent = 'Данные демонстрации не загружены.';
            return;
        }
        const domainSelect = root.querySelector('[data-demo-domain]');
        const presetSelect = root.querySelector('[data-demo-preset]');
        const factsBox = root.querySelector('[data-demo-facts]');
        const out = root.querySelector('[data-demo-output]');
        const summary = root.querySelector('[data-demo-summary]');
        const costs = root.querySelector('[data-demo-costs]');
        let iterator = null;
        let finished = false;

        Object.entries(data.domains).forEach(([id, domain]) => {
            domainSelect.insertAdjacentHTML('beforeend', `<option value="${id}">${esc(domain.title)}</option>`);
        });

        const domain = () => data.domains[domainSelect.value];

        function fillPresets() {
            presetSelect.innerHTML = Object.entries(domain().presets)
                .map(([id, preset]) => `<option value="${id}">${esc(preset.title)}</option>`).join('');
        }

        function fillFacts() {
            const preset = domain().presets[presetSelect.value];
            factsBox.innerHTML = domain().goap.facts.map((fact) => {
                const value = Object.prototype.hasOwnProperty.call(preset.state, fact.name) ? preset.state[fact.name]
                    : (fact.type === 'bool' ? false : 0);
                return fact.type === 'bool'
                    ? `<label><input type="checkbox" data-fact="${fact.name}" ${value ? 'checked' : ''}> ${fact.name}</label>`
                    : `<label>${fact.name} <input type="number" step="1" data-fact="${fact.name}" value="${value}"></label>`;
            }).join('');
            const routeCosts = domain().routeCosts;
            costs.innerHTML = domain().goap.actions.map((action) => {
                const cost = Object.prototype.hasOwnProperty.call(routeCosts, action.context) ? routeCosts[action.context] : action.cost;
                return `<tr><td><code>${action.action}/${action.context}</code></td><td>${fmt(cost)}</td></tr>`;
            }).join('');
            reset();
        }

        function readState() {
            const state = {};
            factsBox.querySelectorAll('[data-fact]').forEach((input) => {
                state[input.dataset.fact] = input.type === 'checkbox' ? input.checked : parseInt(input.value || '0', 10);
            });
            return state;
        }

        function reset() {
            iterator = search(domain(), compileActions(domain(), data), readState());
            finished = false;
            out.innerHTML = '<p class="demo-note">Нажмите «Шаг» для одного извлечения из очереди или «До конца».</p>';
            summary.textContent = '';
        }

        function render(event) {
            if (event.type === 'stale') {
                out.insertAdjacentHTML('beforeend', `<p>Извлечена устаревшая запись (cost ${fmt(event.node.cost)}): пропуск.</p>`);
                return;
            }
            if (event.type === 'done') {
                finished = true;
                summary.innerHTML = event.plan
                    ? `<span class="ok">План найден</span>: ${event.plan.length ? event.plan.map((s) => `<code>${s}</code>`).join(' → ') : '(пустой)'}; ` +
                      `стоимость ${fmt(event.cost)}; раскрыто состояний: ${event.expanded}.` + (event.note ? ` ${event.note}.` : '')
                    : `<span class="bad">Плана нет</span>: ${event.note}; раскрыто ${event.expanded}.`;
                return;
            }
            const rows = event.rows.map((r) => `<tr><td><code>${r.context}</code></td><td class="${r.verdict === 'в очередь' ? 'ok' : (r.verdict === 'отклонено' ? 'bad' : '')}">${r.verdict}</td><td>${esc(r.detail)}</td></tr>`).join('');
            const queue = event.queue.slice(0, 8).map((n) => `<li>cost ${fmt(n.cost)}, seq ${n.seq}: ${n.steps.map((s) => `<code>${s}</code>`).join(' → ')}</li>`).join('');
            out.innerHTML = `<p><strong>Извлечение №${event.expanded}</strong>: cost ${fmt(event.node.cost)}, путь ` +
                `${event.node.steps.length ? event.node.steps.map((s) => `<code>${s}</code>`).join(' → ') : '(старт)'}</p>` +
                `<p>Истинные факты: ${esc(stateText(event.node.state, domain().goap))}</p>` +
                `<table><thead><tr><th>Действие</th><th>Итог</th><th>Причина / стоимость</th></tr></thead><tbody>${rows}</tbody></table>` +
                `<p>Очередь (первые 8 из ${event.queue.length}):</p><ol>${queue}</ol>`;
        }

        function step() {
            if (finished || !iterator) {
                return;
            }
            let result = iterator.next();
            while (!result.done && result.value.type === 'stale') {
                render(result.value);
                result = iterator.next();
            }
            if (!result.done) {
                render(result.value);
            }
        }

        function run() {
            let guard = 0;
            while (!finished && guard < 100000) {
                step();
                guard += 1;
            }
        }

        domainSelect.addEventListener('change', () => { fillPresets(); fillFacts(); });
        presetSelect.addEventListener('change', fillFacts);
        factsBox.addEventListener('change', reset);
        root.querySelector('[data-demo-action="step"]').addEventListener('click', step);
        root.querySelector('[data-demo-action="run"]').addEventListener('click', run);
        root.querySelector('[data-demo-action="reset"]').addEventListener('click', reset);
        fillPresets();
        fillFacts();
    }

    // ------------------------------------------------------------------ Arrival / Flee
    const ARRIVAL_TOL = 0.001;
    const EPS = 1e-6;

    function arrival(d, r, R, minimum) {
        r = Math.max(r, 0);
        R = Math.max(R, r);
        if (d <= r + ARRIVAL_TOL || d <= EPS) {
            return {m: 0, branch: `d ≤ r + 0.001 = ${fmt(r + ARRIVAL_TOL)} → прибыл, m = 0`};
        }
        const interval = R - r;
        if (interval <= EPS || d >= R) {
            return {m: 1, branch: interval <= EPS ? 'R − r ≈ 0 → m = 1' : `d ≥ R = ${fmt(R)} → m = 1`};
        }
        const lambda = Math.min(Math.max((d - r) / interval, 0), 1);
        const floor = Math.min(Math.max(minimum, 0), 1);
        const m = Math.max(lambda, floor);
        return {m, branch: `λ = (d − r)/(R − r) = (${fmt(d)} − ${fmt(r)})/${fmt(interval)} = ${fmt(lambda)}; m = max(λ, m_min) = ${fmt(m)}`};
    }

    function plotArrival(svg, r, R, minimum, d) {
        const W = 520, H = 220, x0 = 46, y0 = 190, w = 450, h = 160, dmax = Math.max(3, R * 1.4, d * 1.1);
        const X = (v) => x0 + v / dmax * w;
        const Y = (m) => y0 - m * h;
        let path = '';
        let previous = null;
        for (let i = 0; i <= 400; i += 1) {
            const value = dmax * i / 400;
            const m = arrival(value, r, R, minimum).m;
            const command = previous === null || Math.abs(m - previous) > 0.05 ? 'M' : 'L';
            path += `${command}${X(value).toFixed(1)},${Y(m).toFixed(1)} `;
            previous = m;
        }
        const current = arrival(d, r, R, minimum).m;
        let ticks = '';
        for (let i = 0; i <= 6; i += 1) {
            const value = dmax * i / 6;
            ticks += `<text x="${X(value)}" y="${y0 + 16}" font-size="11" text-anchor="middle" fill="#64748b">${fmt(value)}</text>`;
        }
        svg.setAttribute('viewBox', `0 0 ${W} ${H + 10}`);
        svg.innerHTML = `<rect width="${W}" height="${H + 10}" fill="#fff"/>` +
            `<line x1="${x0}" y1="${y0}" x2="${x0 + w}" y2="${y0}" stroke="#94a3b8"/><line x1="${x0}" y1="${y0}" x2="${x0}" y2="${y0 - h}" stroke="#94a3b8"/>` +
            `<text x="${x0 - 6}" y="${Y(1) + 4}" font-size="11" text-anchor="end" fill="#64748b">1</text><text x="${x0 - 6}" y="${y0 + 4}" font-size="11" text-anchor="end" fill="#64748b">0</text>` +
            ticks +
            `<line x1="${X(r)}" y1="${y0}" x2="${X(r)}" y2="${y0 - h}" stroke="#b45309" stroke-dasharray="3 3"/>` +
            `<line x1="${X(R)}" y1="${y0}" x2="${X(R)}" y2="${y0 - h}" stroke="#b45309" stroke-dasharray="3 3"/>` +
            `<text x="${X(r) + 3}" y="${y0 - h + 12}" font-size="11" fill="#b45309">r</text><text x="${X(R) + 3}" y="${y0 - h + 12}" font-size="11" fill="#b45309">R</text>` +
            `<path d="${path}" fill="none" stroke="#3f6ea3" stroke-width="2.5"/>` +
            `<circle cx="${X(d)}" cy="${Y(current)}" r="5" fill="#c2410c"/>` +
            `<text x="${x0 + w}" y="${y0 + 30}" font-size="11" text-anchor="end" fill="#14212e">горизонтальная дистанция d, м</text>`;
    }

    function setupArrival(root) {
        const inputs = {};
        root.querySelectorAll('[data-arrival]').forEach((input) => { inputs[input.dataset.arrival] = input; });
        const out = root.querySelector('[data-arrival-out]');
        const svg = root.querySelector('svg.demo-svg');
        const run = root.querySelector('[data-arrival-run]');
        function update() {
            const r = parseFloat(inputs.r.value), R = parseFloat(inputs.R.value), minimum = parseFloat(inputs.min.value);
            const d = parseFloat(inputs.d.value);
            root.querySelectorAll('[data-arrival-value]').forEach((span) => { span.textContent = inputs[span.dataset.arrivalValue].value; });
            const result = arrival(d, r, R, minimum);
            const speed = run && run.checked ? 4.5 : 2.0;
            out.textContent = `Вход: r = ${fmt(r)}, R = ${fmt(R)} (после нормализации R = ${fmt(Math.max(R, Math.max(r, 0)))}), m_min = ${fmt(minimum)}, d = ${fmt(d)}\n` +
                `${result.branch}\nmagnitude = ${result.m.toFixed(2)}\n` +
                `целевая скорость мотора = m · ${speed} м/с = ${(result.m * speed).toFixed(2)} м/с (без учёта ускорения и физики)`;
            plotArrival(svg, r, R, minimum, d);
        }
        Object.values(inputs).forEach((input) => input.addEventListener('input', update));
        if (run) {
            run.addEventListener('change', update);
        }
        update();
    }

    function setupFlee(root) {
        const get = (name) => root.querySelector(`[data-flee="${name}"]`);
        const out = root.querySelector('[data-flee-out]');
        const log = root.querySelector('[data-flee-log]');
        let fleeing = false;
        function params() {
            const trigger = Math.max(parseFloat(get('trigger').value), 0);
            const safe = Math.max(parseFloat(get('safe').value), trigger);
            return {trigger, safe};
        }
        function stepWith(d) {
            const {trigger, safe} = params();
            const before = fleeing;
            fleeing = before ? d < safe : d <= trigger;
            const rule = before ? `бежал: d < safe (${fmt(d)} < ${fmt(safe)})` : `стоял: d ≤ trigger (${fmt(d)} ≤ ${fmt(trigger)})`;
            return `${rule} → ${fleeing ? 'БЕЖИТ' : 'стоит'}`;
        }
        function show(text) {
            const {trigger, safe} = params();
            root.querySelectorAll('[data-flee-value]').forEach((span) => { span.textContent = get(span.dataset.fleeValue).value; });
            out.textContent = `trigger = ${fmt(trigger)}, safe = max(safe, trigger) = ${fmt(safe)}\n${text}`;
        }
        get('d').addEventListener('input', () => show(stepWith(parseFloat(get('d').value))));
        ['trigger', 'safe'].forEach((name) => get(name).addEventListener('input', () => show('параметры изменены; состояние сохранено')));
        root.querySelector('[data-flee-action="reset"]').addEventListener('click', () => { fleeing = false; log.textContent = ''; show('сброс: стоит'); });
        root.querySelector('[data-flee-action="sweep"]').addEventListener('click', () => {
            fleeing = false;
            const lines = [];
            [8, 6, 5, 4, 6, 6.9, 7, 6, 5].forEach((d) => lines.push(`d = ${d}: ${stepWith(d)}`));
            log.textContent = lines.join('\n');
            show('прогон последовательности дистанций выполнен');
        });
        show('стоит');
    }

    function initialise() {
        document.querySelectorAll('[data-demo="goap"]').forEach((root) => {
            if (root.dataset.ready !== 'true') { root.dataset.ready = 'true'; setupGoap(root); }
        });
        document.querySelectorAll('[data-demo="arrival"]').forEach((root) => {
            if (root.dataset.ready !== 'true') { root.dataset.ready = 'true'; setupArrival(root); }
        });
        document.querySelectorAll('[data-demo="flee"]').forEach((root) => {
            if (root.dataset.ready !== 'true') { root.dataset.ready = 'true'; setupFlee(root); }
        });
    }

    window.AtlasDemos = {arrival, search};
    if (window.document$ && typeof window.document$.subscribe === 'function') {
        window.document$.subscribe(() => initialise());
    } else if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initialise, {once: true});
    } else {
        initialise();
    }
}());
