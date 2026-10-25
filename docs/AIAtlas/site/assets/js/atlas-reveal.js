/*
 * CoreEngine AI Atlas — раскрытие скрытых фрагментов и фильтры индекса.
 *  1) Переход по якорю (#id), находящемуся внутри свёрнутого <details>, раскрывает все родительские <details>.
 *  2) Переход из поиска Material (?h=термин) раскрывает <details>, текст которых содержит искомый термин,
 *     и прокручивает к первому совпадению.
 *  3) Таблица с атрибутом data-filter-table получает фильтры по домену, виду сущности и тексту.
 * Без JavaScript содержимое <details> остаётся доступным по клику, таблицы — полными.
 */
(function () {
    'use strict';

    function openAncestors(element) {
        let node = element;
        while (node) {
            if (node.tagName === 'DETAILS' && !node.open) {
                node.open = true;
            }
            node = node.parentElement;
        }
    }

    function revealHash() {
        if (!location.hash || location.hash.length < 2) {
            return;
        }
        let target = null;
        try {
            target = document.getElementById(decodeURIComponent(location.hash.slice(1)));
        } catch (error) {
            target = null;
        }
        if (target) {
            openAncestors(target);
            target.scrollIntoView({block: 'start'});
        }
    }

    function revealSearchTerms() {
        const params = new URLSearchParams(location.search);
        const raw = params.get('h');
        if (!raw) {
            return;
        }
        const terms = raw.toLowerCase().split(/\s+/).filter((term) => term.length > 1);
        if (!terms.length) {
            return;
        }
        let first = null;
        document.querySelectorAll('.md-typeset details').forEach((details) => {
            const text = details.textContent.toLowerCase();
            if (terms.some((term) => text.includes(term))) {
                details.open = true;
                first = first || details;
            }
        });
        if (first && !location.hash) {
            first.scrollIntoView({block: 'center'});
        }
    }

    function setupFilters() {
        document.querySelectorAll('table[data-filter-table], div[data-filter-table] table').forEach((table) => {
            if (table.dataset.filterReady === 'true') {
                return;
            }
            table.dataset.filterReady = 'true';
            const rows = [...table.querySelectorAll('tbody tr')];
            const header = [...table.querySelectorAll('thead th')].map((th) => th.textContent.trim());
            const domainColumn = header.findIndex((name) => /домен/i.test(name));
            const kindColumn = header.findIndex((name) => /^вид/i.test(name));
            const values = (column) => [...new Set(rows.map((row) => (row.cells[column] || {}).textContent || '')
                .map((value) => value.trim()).filter(Boolean))].sort();
            const bar = document.createElement('div');
            bar.className = 'index-filters';
            const makeSelect = (label, column) => {
                if (column < 0) {
                    return null;
                }
                const wrapper = document.createElement('label');
                wrapper.textContent = label + ' ';
                const select = document.createElement('select');
                select.innerHTML = '<option value="">все</option>' + values(column)
                    .map((value) => `<option>${value.replace(/</g, '&lt;')}</option>`).join('');
                wrapper.appendChild(select);
                bar.appendChild(wrapper);
                return select;
            };
            const domain = makeSelect('Домен:', domainColumn);
            const kind = makeSelect('Вид:', kindColumn);
            const textLabel = document.createElement('label');
            textLabel.textContent = 'Текст: ';
            const text = document.createElement('input');
            text.type = 'search';
            text.placeholder = 'имя, файл, назначение';
            textLabel.appendChild(text);
            bar.appendChild(textLabel);
            const counter = document.createElement('span');
            bar.appendChild(counter);
            const container = table.closest('.md-typeset__scrollwrap') || table;
            container.parentNode.insertBefore(bar, container);
            const apply = () => {
                let shown = 0;
                rows.forEach((row) => {
                    const cell = (column) => ((row.cells[column] || {}).textContent || '').trim();
                    const visible = (!domain || !domain.value || cell(domainColumn) === domain.value)
                        && (!kind || !kind.value || cell(kindColumn) === kind.value)
                        && (!text.value || row.textContent.toLowerCase().includes(text.value.toLowerCase()));
                    row.classList.toggle('index-hidden', !visible);
                    shown += visible ? 1 : 0;
                });
                counter.textContent = `показано ${shown} из ${rows.length}`;
            };
            [domain, kind].forEach((select) => select && select.addEventListener('change', apply));
            text.addEventListener('input', apply);
            apply();
        });
    }

    // Подписи столбцов для карточной раскладки таблиц на узких экранах (CSS: atlas.css, max-width 600px).
    function labelTables() {
        document.querySelectorAll('.md-typeset table:not([class])').forEach((table) => {
            const header = [...table.querySelectorAll('thead th')].map((th) => th.textContent.trim());
            if (header.length < 3) {
                return;
            }
            table.dataset.stack = 'true';   // атрибут, а не класс: стили Material привязаны к table:not([class])
            table.querySelectorAll('tbody tr').forEach((row) => {
                [...row.cells].forEach((cell, index) => { cell.dataset.label = header[index] || ''; });
            });
        });
    }

    function initialise() {
        labelTables();
        setupFilters();
        revealHash();
        revealSearchTerms();
    }

    window.addEventListener('hashchange', revealHash);
    if (window.document$ && typeof window.document$.subscribe === 'function') {
        window.document$.subscribe(() => initialise());
    } else if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initialise, {once: true});
    } else {
        initialise();
    }
}());
