// Браузерная проверка собранного атласа из file:// с отключённой сетью (puppeteer + системный Chrome).
//   node tools/qa_browser.js [--pages a,b] [--widths 390,1280,1920,3440] [--zoom 1,1.25,2] [--out audit/screenshots]
// Делает скриншоты, собирает ошибки JS/консоли и запросы в сеть, проверяет горизонтальный overflow страницы,
// работу viewer (zoom/fit/reset/expand/Esc + возврат фокуса), поиск (русский и C++-идентификатор,
// раскрытие <details>), фильтры индекса, переключение темы и обе демонстрации.
// Пишет audit/generated/browser-qa.json. Код выхода 1 при провале.
'use strict';
const fs = require('fs');
const path = require('path');
const puppeteer = require(path.join(__dirname, '..', '.toolchain', 'node', 'node_modules', 'puppeteer'));

const ATLAS = path.resolve(__dirname, '..');
const SITE = path.join(ATLAS, 'site');
const CHROME = 'C:/Program Files/Google/Chrome/Application/chrome.exe';
const args = process.argv.slice(2);
const option = (name, fallback) => {
    const index = args.indexOf(name);
    return index >= 0 ? args[index + 1] : fallback;
};
const PAGES = option('--pages', 'index.html,chapters/05-planner.html,chapters/13-steering.html').split(',');
const WIDTHS = option('--widths', '390,1280,1920,3440').split(',').map(Number);
const ZOOMS = option('--zoom', '1').split(',').map(Number);
const OUT = path.join(ATLAS, option('--out', 'audit/screenshots'));
const FULL = args.includes('--full-page');
const INTERACT = !args.includes('--no-interact');

function url(page) {
    return 'file:///' + path.join(SITE, page).replace(/\\/g, '/');
}

async function openPage(browser, page, width, zoom, report) {
    const tab = await browser.newPage();
    await tab.setViewport({width: Math.round(width / zoom), height: Math.round(1000 / zoom), deviceScaleFactor: zoom});
    await tab.setRequestInterception(true);
    tab.on('request', (request) => {
        const target = request.url();
        if (/^(https?|wss?):/i.test(target)) {
            report.network.push(`${page}: ${target}`);
            request.abort();
        } else {
            request.continue();
        }
    });
    tab.on('pageerror', (error) => report.jsErrors.push(`${page} @${width}: ${error.message}`));
    tab.on('console', (message) => {
        if (message.type() === 'error') {
            report.consoleErrors.push(`${page} @${width}: ${message.text()}`);
        }
    });
    await tab.goto(url(page), {waitUntil: 'load'});
    await new Promise((resolve) => setTimeout(resolve, 300));
    return tab;
}

async function layoutChecks(tab, page, width, zoom, report) {
    const result = await tab.evaluate(() => {
        const doc = document.documentElement;
        const overflow = doc.scrollWidth - doc.clientWidth;
        const article = document.querySelector('.md-content__inner');
        const paragraph = article && article.querySelector(':scope > p');
        const lineLength = paragraph ? paragraph.getBoundingClientRect().width : 0;
        const fontSize = paragraph ? parseFloat(getComputedStyle(paragraph).fontSize) : 0;
        const clipped = [...document.querySelectorAll('.md-typeset table:not([class]) td')]
            .filter((cell) => cell.scrollWidth > cell.clientWidth + 2).length;
        const svgs = [...document.querySelectorAll('.diagram-viewport svg')].map((svg) => {
            const box = svg.getBoundingClientRect();
            const intrinsic = parseFloat(svg.getAttribute('width')) || box.width;
            return Math.round(box.width / intrinsic * 100) / 100;
        });
        return {overflow, lineLength, chars: fontSize ? Math.round(lineLength / (fontSize * 0.5)) : 0, clipped, svgs};
    });
    report.layout.push(Object.assign({page, width, zoom}, result));
    if (result.overflow > 1) {
        report.failures.push(`${page} @${width}x${zoom}: горизонтальный overflow страницы ${result.overflow}px`);
    }
}

async function viewerChecks(tab, page, report) {
    const has = await tab.$('.diagram-viewport');
    if (!has) {
        return;
    }
    const outcome = await tab.evaluate(async () => {
        const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
        const toolbar = document.querySelector('.adv-toolbar');
        if (!toolbar) {
            return {error: 'toolbar missing'};
        }
        const readout = () => toolbar.querySelector('.adv-readout').textContent;
        const click = (action) => toolbar.querySelector(`[data-action="${action}"]`).click();
        const before = readout();
        click('in'); await sleep(250);
        const zoomed = readout();
        click('fit'); await sleep(250);
        const mode = toolbar.dataset.mode;
        click('reset'); await sleep(100);
        const reset = toolbar.dataset.mode;
        const expand = toolbar.querySelector('[data-action="expand"]');
        expand.focus();
        expand.click(); await sleep(400);
        const modal = document.querySelector('.adv-modal');
        const opened = modal && !modal.hidden;
        document.querySelector('.adv-modal').dispatchEvent(new KeyboardEvent('keydown', {key: 'Escape', bubbles: true}));
        await sleep(200);
        const closed = modal.hidden;
        const focusBack = document.activeElement === expand;
        return {before, zoomed, mode, reset, opened, closed, focusBack};
    });
    report.viewer.push(Object.assign({page}, outcome));
    if (outcome.error || outcome.before === outcome.zoomed || outcome.mode !== 'navigation' || outcome.reset !== 'page'
        || !outcome.opened || !outcome.closed || !outcome.focusBack) {
        report.failures.push(`${page}: viewer ${JSON.stringify(outcome)}`);
    }
}

async function searchChecks(browser, report) {
    const cases = [['warm-up', 'прогрев (не оценивается)'], ['планировщик', 'русское слово'],
        ['гистерезис', 'русское слово в теории'], ['FindAIPlan', 'C++-идентификатор'],
        ['Decision', 'часть camelCase'], ['physicallyBlocked', 'поле структуры']];
    const searchTab = await openPage(browser, 'index.html', 1280, 1, report);
    await new Promise((resolve) => setTimeout(resolve, 2000));
    for (const [query, label] of cases) {
        await searchTab.keyboard.press('/');
        await searchTab.$eval('.md-search__input', (input) => {
            input.value = '';
            input.dispatchEvent(new Event('input'));
        });
        await searchTab.type('.md-search__input', query, {delay: 20});
        let count = 0;
        for (let attempt = 0; attempt < 30 && count === 0; attempt += 1) {
            await new Promise((resolve) => setTimeout(resolve, 200));
            count = await searchTab.$$eval('.md-search-result__item', (items) => items.length);
        }
        const first = await searchTab.$eval('.md-search-result__link', (link) => link.getAttribute('href')).catch(() => '');
        if (query === 'warm-up') {
            continue;
        }
        report.search.push({query, label, results: count, first: first.replace(/^.*\/site\//, '')});
        if (!count) {
            report.failures.push(`search "${query}" (${label}): нет результатов`);
        }
    }
    await searchTab.close();
    // Переход по якорю внутри свёрнутого блока раскрывает его
    const tab = await openPage(browser, 'reference/source-references.html?h=bitset', 1280, 1, report);
    const opened = await tab.evaluate(() => [...document.querySelectorAll('details')].filter((d) => d.open).length);
    report.search.push({query: '?h=bitset (reveal details)', results: opened});
    if (!opened) {
        report.failures.push('reveal: ?h= не раскрыл ни одного <details>');
    }
    await tab.close();
}

async function themeCheck(browser, report) {
    const tab = await openPage(browser, 'chapters/13-steering.html', 1280, 1, report);
    const before = await tab.evaluate(() => document.body.getAttribute('data-md-color-scheme'));
    await tab.click('label.md-header__button[for="__palette_1"], label[for="__palette_1"]').catch(() => null);
    await new Promise((resolve) => setTimeout(resolve, 300));
    const after = await tab.evaluate(() => document.body.getAttribute('data-md-color-scheme'));
    report.theme = {before, after};
    if (before === after) {
        report.failures.push(`theme toggle: ${before} → ${after}`);
    }
    await tab.screenshot({path: path.join(OUT, 'theme-dark-13-steering-1280.png')});
    await tab.close();
}

async function filterCheck(browser, report) {
    const tab = await openPage(browser, 'reference/class-index.html', 1280, 1, report);
    const result = await tab.evaluate(async () => {
        const select = document.querySelector('.index-filters select');
        if (!select) {
            return {error: 'no filters'};
        }
        const total = document.querySelectorAll('table tbody tr').length;
        select.value = select.options[1].value;
        select.dispatchEvent(new Event('change'));
        await new Promise((resolve) => setTimeout(resolve, 50));
        const visible = [...document.querySelectorAll('table tbody tr')].filter((row) => !row.classList.contains('index-hidden')).length;
        return {total, visible, domain: select.value};
    });
    report.filters = result;
    if (result.error || !(result.visible > 0 && result.visible < result.total)) {
        report.failures.push(`class index filter: ${JSON.stringify(result)}`);
    }
    await tab.close();
}

async function demoChecks(browser, report) {
    const goap = path.join(SITE, 'demos', 'goap-search.html');
    if (fs.existsSync(goap)) {
        const tab = await openPage(browser, 'demos/goap-search.html', 1280, 1, report);
        const result = await tab.evaluate(async () => {
            const run = document.querySelector('[data-demo-action="run"]');
            if (!run) {
                return {error: 'no run button'};
            }
            run.click();
            await new Promise((resolve) => setTimeout(resolve, 100));
            return {summary: (document.querySelector('[data-demo-summary]') || {}).textContent || ''};
        });
        report.demos.goap = result;
        if (result.error || !/42\.08/.test(result.summary)) {
            report.failures.push(`demo goap: ${JSON.stringify(result)}`);
        }
        await tab.screenshot({path: path.join(OUT, 'demo-goap-1280.png'), fullPage: true});
        await tab.close();
    }
    const arrival = path.join(SITE, 'demos', 'arrival-flee.html');
    if (fs.existsSync(arrival)) {
        const tab = await openPage(browser, 'demos/arrival-flee.html', 1280, 1, report);
        const result = await tab.evaluate(async () => {
            const set = (name, value) => {
                const input = document.querySelector(`[data-arrival="${name}"]`);
                input.value = value;
                input.dispatchEvent(new Event('input'));
            };
            if (!document.querySelector('[data-arrival="d"]')) {
                return {error: 'no arrival inputs'};
            }
            set('r', '0.5'); set('R', '2.5'); set('min', '0.1'); set('d', '1.5');
            await new Promise((resolve) => setTimeout(resolve, 50));
            const out = document.querySelector('[data-arrival-out]').textContent;
            return {out};
        });
        report.demos.arrival = result;
        if (result.error || !/0\.50/.test(result.out)) {
            report.failures.push(`demo arrival: ${JSON.stringify(result)}`);
        }
        await tab.close();
    }
}

(async () => {
    fs.mkdirSync(OUT, {recursive: true});
    const report = {pages: PAGES, widths: WIDTHS, zooms: ZOOMS, failures: [], jsErrors: [], consoleErrors: [], network: [],
        layout: [], viewer: [], search: [], demos: {}, screenshots: []};
    const browser = await puppeteer.launch({executablePath: CHROME, headless: 'new', args: ['--no-sandbox', '--disable-gpu']});
    try {
        for (const page of PAGES) {
            for (const width of WIDTHS) {
                for (const zoom of ZOOMS) {
                    const tab = await openPage(browser, page, width, zoom, report);
                    await layoutChecks(tab, page, width, zoom, report);
                    const file = `${page.replace(/[\\/]/g, '_').replace('.html', '')}-${width}${zoom !== 1 ? '-z' + zoom : ''}.png`;
                    await tab.screenshot({path: path.join(OUT, file), fullPage: FULL});
                    report.screenshots.push(file);
                    if (INTERACT && width === 1280 && zoom === 1) {
                        await viewerChecks(tab, page, report);
                    }
                    await tab.close();
                }
            }
        }
        if (INTERACT) {
            await searchChecks(browser, report);
            await themeCheck(browser, report);
            await filterCheck(browser, report);
            await demoChecks(browser, report);
        }
    } finally {
        await browser.close();
    }
    report.failures.push(...report.jsErrors.map((e) => 'JS error: ' + e), ...report.network.map((n) => 'network: ' + n));
    const out = path.join(ATLAS, 'audit', 'generated', 'browser-qa.json');
    fs.mkdirSync(path.dirname(out), {recursive: true});
    fs.writeFileSync(out, JSON.stringify(report, null, 1));
    console.log(report.failures.length ? 'FAIL' : 'PASS', report.failures.length, 'failures');
    report.failures.slice(0, 40).forEach((failure) => console.log('  ', failure));
    process.exit(report.failures.length ? 1 : 0);
})();
