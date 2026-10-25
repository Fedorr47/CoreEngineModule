/*
 * CoreEngine AI Atlas — интерактивный просмотрщик схем (адаптирован из образца ArchitectureAtlas, без внешних зависимостей).
 *
 * Схемы встраиваются при сборке как inline SVG внутри .diagram-viewport (hooks/atlas_hooks.py).
 * Политика взаимодействия:
 *   - схема начинается в адаптивной раскладке страницы; первое масштабирование/вписывание/двойной клик/Ctrl+колесо
 *     переводит именно эту схему в режим навигации (translate + scale внутри области); «Сброс» возвращает раскладку;
 *   - Ctrl (Cmd) + колесо масштабирует у указателя; обычное колесо прокручивает страницу;
 *   - перетаскивание панорамирует только в режиме навигации; нажатие с малым смещением остаётся кликом,
 *     поэтому ссылки внутри SVG работают; на touch-устройствах один палец — панорама, два — масштаб;
 *   - клавиатура на схеме в фокусе: + / − масштаб, 0 = 100 %, F = вписать, R = сброс, стрелки — панорама,
 *     Enter = развернуть;
 *   - «Развернуть» переносит исходный узел схемы в модальный слой страницы (без копии); Esc закрывает,
 *     фокус удерживается в слое и возвращается к исходной кнопке; полноэкранный режим — по возможности браузера.
 * Проценты масштаба отсчитываются от собственного размера схемы (100 % = авторский размер).
 */
(function () {
    'use strict';

    const CONFIG = {
        minZoom: 0.25,
        maxZoom: 5,
        buttonStep: 1.25,
        doubleClickStep: 2,
        wheelSensitivity: 0.0015,
        dragThreshold: 5,
        keyboardPanStep: 60,
        expandClickDelay: 260,
    };
    const instances = new Set();
    let modal = null;

    function clamp(value, minimum, maximum) {
        return Math.min(maximum, Math.max(minimum, value));
    }

    function naturalSize(content) {
        if (content.tagName.toLowerCase() === 'img') {
            const width = parseFloat(content.getAttribute('width')) || content.naturalWidth;
            const height = parseFloat(content.getAttribute('height')) || content.naturalHeight;
            return {width, height};
        }
        const viewBox = content.viewBox && content.viewBox.baseVal;
        if (viewBox && viewBox.width && viewBox.height) {
            return {width: viewBox.width, height: viewBox.height};
        }
        const box = content.getBoundingClientRect();
        return {width: box.width || 1, height: box.height || 1};
    }

    function diagramTitle(root, content) {
        const caption = root.querySelector('figcaption .fig-title, figcaption h4, figcaption');
        const svgTitle = content.querySelector ? content.querySelector('title') : null;
        const text = (svgTitle && svgTitle.textContent) || content.getAttribute('alt') || (caption && caption.textContent) || 'Схема';
        return text.trim().replace(/\s+/g, ' ').slice(0, 120);
    }

    function createButton(label, text, action, extraClass) {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'adv-button' + (extraClass ? ' ' + extraClass : '');
        button.setAttribute('aria-label', label);
        button.title = label;
        button.textContent = text;
        button.dataset.action = action;
        return button;
    }

    class DiagramViewer {
        constructor(root, stage, content, options) {
            this.root = root;
            this.stage = stage;
            this.content = content;
            this.options = options || {};
            this.natural = naturalSize(content);
            this.title = diagramTitle(root, content);
            this.navigating = false;
            this.scale = 1;
            this.x = 0;
            this.y = 0;
            this.pointers = new Map();
            this.drag = null;
            this.suppressClick = false;
            this.listeners = [];
            this.buildToolbar();
            this.bind();
            this.updateReadout();
            instances.add(this);
        }

        listen(target, type, handler, options) {
            target.addEventListener(type, handler, options);
            this.listeners.push(() => target.removeEventListener(type, handler, options));
        }

        buildToolbar() {
            const toolbar = document.createElement('div');
            toolbar.className = 'adv-toolbar';
            toolbar.setAttribute('role', 'toolbar');
            toolbar.setAttribute('aria-label', 'Управление схемой: ' + this.title);
            this.readout = document.createElement('output');
            this.readout.className = 'adv-readout';
            this.readout.setAttribute('aria-live', 'polite');
            this.readout.setAttribute('aria-label', 'Текущий масштаб');
            const buttons = [
                createButton('Уменьшить', '−', 'out'),
                this.readout,
                createButton('Увеличить', '+', 'in'),
                createButton('Вписать схему в область', 'Вписать', 'fit'),
                createButton('Исходный размер (100 %)', '1:1', 'actual'),
                createButton('Вернуть раскладку страницы', 'Сброс', 'reset'),
            ];
            if (!this.options.inModal) {
                buttons.push(createButton('Развернуть схему поверх страницы', '⤢ Развернуть', 'expand', 'adv-expand'));
            } else {
                if (document.fullscreenEnabled) {
                    buttons.push(createButton('Полноэкранный режим браузера', 'Во весь экран', 'fullscreen'));
                }
                buttons.push(createButton('Закрыть (Esc)', '✕ Закрыть', 'close', 'adv-close'));
            }
            buttons.forEach((element) => toolbar.appendChild(element));
            this.toolbar = toolbar;
            const parent = this.stage.parentNode;
            const layout = window.getComputedStyle(parent);
            const sideBySide = layout.display.includes('grid')
                || (layout.display.includes('flex') && !layout.flexDirection.startsWith('column'));
            if (sideBySide && !this.options.inModal) {
                // Grid / row-flex containers (quick reference .diagram-components) would place the toolbar beside the
                // diagram; a frame keeps it above and takes over the stage's flex sizing.
                this.frame = document.createElement('div');
                this.frame.className = 'adv-frame';
                this.frame.style.flex = this.stage.style.flex;
                parent.insertBefore(this.frame, this.stage);
                this.frame.appendChild(toolbar);
                this.frame.appendChild(this.stage);
            } else {
                parent.insertBefore(toolbar, this.stage);
            }
        }

        bind() {
            this.stage.classList.add('adv-stage');
            if (!this.stage.hasAttribute('tabindex')) {
                this.stage.setAttribute('tabindex', '0');
            }
            if (!this.stage.hasAttribute('aria-label')) {
                this.stage.setAttribute('aria-label', 'Схема: ' + this.title + '. Клавиши: плюс, минус, F, 0, R, стрелки, Enter.');
            }
            this.listen(this.toolbar, 'click', (event) => {
                const button = event.target.closest('button[data-action]');
                if (button) {
                    this.command(button.dataset.action, button);
                }
            });
            this.listen(this.stage, 'wheel', (event) => this.onWheel(event), {passive: false});
            this.listen(this.stage, 'dblclick', (event) => this.onDoubleClick(event));
            this.listen(this.stage, 'pointerdown', (event) => this.onPointerDown(event));
            this.listen(this.stage, 'pointermove', (event) => this.onPointerMove(event));
            this.listen(this.stage, 'pointerup', (event) => this.onPointerUp(event));
            this.listen(this.stage, 'pointercancel', (event) => this.onPointerUp(event));
            this.listen(this.stage, 'click', (event) => this.onClick(event), true);
            this.listen(this.stage, 'keydown', (event) => this.onKeyDown(event));
            // A native image/link drag would cancel the pointer stream (pointercancel) and stop panning.
            this.listen(this.stage, 'dragstart', (event) => {
                if (this.navigating) {
                    event.preventDefault();
                }
            });
            if (window.ResizeObserver) {
                this.resizeObserver = new ResizeObserver(() => this.onResize());
                this.resizeObserver.observe(this.stage);
            }
        }

        destroy() {
            window.clearTimeout(this.pendingExpand);
            this.listeners.forEach((remove) => remove());
            this.listeners = [];
            if (this.resizeObserver) {
                this.resizeObserver.disconnect();
            }
            this.exitNavigation();
            this.toolbar.remove();
            if (this.frame) {
                this.frame.parentNode.insertBefore(this.stage, this.frame);
                this.frame.remove();
                this.frame = null;
            }
            this.stage.classList.remove('adv-stage');
            instances.delete(this);
        }

        // ----- geometry ----------------------------------------------------------------------------------------------
        stageMetrics() {
            const rect = this.stage.getBoundingClientRect();
            return {
                left: rect.left + this.stage.clientLeft,
                top: rect.top + this.stage.clientTop,
                width: this.stage.clientWidth,
                height: this.stage.clientHeight,
            };
        }

        enterNavigation() {
            if (this.navigating) {
                return;
            }
            const stageRect = this.stage.getBoundingClientRect();
            const metrics = this.stageMetrics();
            const contentRect = this.content.getBoundingClientRect();
            this.frozenHeight = this.options.inModal ? null : Math.max(stageRect.height, 160);
            this.scale = contentRect.width > 0 ? contentRect.width / this.natural.width : 1;
            this.x = contentRect.left - metrics.left;
            this.y = contentRect.top - metrics.top;
            this.navigating = true;
            this.stage.classList.add('adv-navigating');
            if (this.frozenHeight) {
                this.stage.style.height = this.frozenHeight + 'px';
            }
            const style = this.content.style;
            this.savedContentStyle = {
                width: style.width, height: style.height, minWidth: style.minWidth, maxWidth: style.maxWidth,
                transform: style.transform,
            };
            style.width = this.natural.width + 'px';
            style.height = this.natural.height + 'px';
            style.minWidth = '0';
            style.maxWidth = 'none';
            this.content.classList.add('adv-content');
            this.apply(false);
        }

        exitNavigation() {
            if (!this.navigating) {
                return;
            }
            this.navigating = false;
            this.stage.classList.remove('adv-navigating', 'adv-dragging');
            this.stage.style.height = '';
            const saved = this.savedContentStyle || {};
            const style = this.content.style;
            style.width = saved.width || '';
            style.height = saved.height || '';
            style.minWidth = saved.minWidth || '';
            style.maxWidth = saved.maxWidth || '';
            style.transform = saved.transform || '';
            this.content.classList.remove('adv-content');
            this.updateReadout();
        }

        minimumZoom() {
            // Fit may need less than the configured minimum for very large diagrams in small stages.
            return Math.min(CONFIG.minZoom, this.fitScale());
        }

        fitScale() {
            const metrics = this.stageMetrics();
            const padding = 16;
            return Math.max(0.01, Math.min((metrics.width - padding) / this.natural.width,
                (metrics.height - padding) / this.natural.height));
        }

        apply(animate) {
            const style = this.content.style;
            this.stage.classList.toggle('adv-animate', Boolean(animate));
            style.transform = `translate(${this.x}px, ${this.y}px) scale(${this.scale})`;
            this.updateReadout();
        }

        updateReadout() {
            let scale = this.scale;
            if (!this.navigating) {
                const rect = this.content.getBoundingClientRect();
                scale = rect.width > 0 ? rect.width / this.natural.width : 1;
            }
            this.readout.textContent = Math.round(scale * 100) + '%';
            this.toolbar.dataset.mode = this.navigating ? 'navigation' : 'page';
        }

        zoomAt(factor, clientX, clientY, animate) {
            this.enterNavigation();
            const metrics = this.stageMetrics();
            const pointX = clientX === undefined ? metrics.width / 2 : clientX - metrics.left;
            const pointY = clientY === undefined ? metrics.height / 2 : clientY - metrics.top;
            const next = clamp(this.scale * factor, this.minimumZoom(), CONFIG.maxZoom);
            const ratio = next / this.scale;
            this.x = pointX - (pointX - this.x) * ratio;
            this.y = pointY - (pointY - this.y) * ratio;
            this.scale = next;
            this.apply(animate);
        }

        setScaleCentered(scale, animate) {
            this.enterNavigation();
            const metrics = this.stageMetrics();
            this.scale = scale;
            this.x = (metrics.width - this.natural.width * scale) / 2;
            this.y = Math.max(8, (metrics.height - this.natural.height * scale) / 2);
            this.apply(animate);
        }

        fit(animate) {
            this.enterNavigation();
            this.setScaleCentered(clamp(this.fitScale(), 0.01, CONFIG.maxZoom), animate);
        }

        actualSize(animate) {
            this.enterNavigation();
            const metrics = this.stageMetrics();
            // Keep the point currently at the stage centre under the centre.
            this.zoomAt(1 / this.scale, metrics.left + metrics.width / 2, metrics.top + metrics.height / 2, animate);
        }

        panBy(deltaX, deltaY) {
            this.enterNavigation();
            this.x += deltaX;
            this.y += deltaY;
            this.apply(false);
        }

        command(action, button) {
            switch (action) {
                case 'in': this.zoomAt(CONFIG.buttonStep, undefined, undefined, true); break;
                case 'out': this.zoomAt(1 / CONFIG.buttonStep, undefined, undefined, true); break;
                case 'fit': this.fit(true); break;
                case 'actual': this.actualSize(true); break;
                case 'reset': this.exitNavigation(); break;
                case 'expand': openModal(this, button); break;
                case 'close': closeModal(); break;
                case 'fullscreen': toggleFullscreen(); break;
                default: break;
            }
        }

        // ----- input ---------------------------------------------------------------------------------------------------
        onWheel(event) {
            if (!event.ctrlKey && !event.metaKey) {
                return;  // plain wheel: normal page scrolling
            }
            event.preventDefault();
            const factor = Math.exp(-event.deltaY * CONFIG.wheelSensitivity * (event.deltaMode === 1 ? 33 : 1));
            this.zoomAt(factor, event.clientX, event.clientY, false);
        }

        onDoubleClick(event) {
            const link = event.target.closest('a');
            if (link && !link.classList.contains('diagram-link')) {
                return;  // source links inside SVGs keep their native behaviour
            }
            window.clearTimeout(this.pendingExpand);
            this.pendingExpand = null;
            event.preventDefault();
            this.zoomAt(CONFIG.doubleClickStep, event.clientX, event.clientY, true);
        }

        onPointerDown(event) {
            if (!this.navigating || (event.pointerType === 'mouse' && event.button !== 0)) {
                return;
            }
            this.pointers.set(event.pointerId, {x: event.clientX, y: event.clientY});
            if (this.pointers.size === 1) {
                this.drag = {startX: event.clientX, startY: event.clientY, lastX: event.clientX, lastY: event.clientY,
                    moved: false, pointerId: event.pointerId};
            } else if (this.pointers.size === 2) {
                const [first, second] = [...this.pointers.values()];
                this.pinch = {distance: Math.hypot(second.x - first.x, second.y - first.y)};
                this.drag = null;
            }
        }

        onPointerMove(event) {
            if (!this.pointers.has(event.pointerId)) {
                return;
            }
            this.pointers.set(event.pointerId, {x: event.clientX, y: event.clientY});
            if (this.pointers.size === 2 && this.pinch) {
                const [first, second] = [...this.pointers.values()];
                const distance = Math.hypot(second.x - first.x, second.y - first.y);
                if (this.pinch.distance > 0) {
                    this.zoomAt(distance / this.pinch.distance, (first.x + second.x) / 2, (first.y + second.y) / 2, false);
                }
                this.pinch.distance = distance;
                this.suppressClick = true;
                return;
            }
            const drag = this.drag;
            if (!drag || drag.pointerId !== event.pointerId) {
                return;
            }
            if (!drag.moved && Math.hypot(event.clientX - drag.startX, event.clientY - drag.startY) < CONFIG.dragThreshold) {
                return;
            }
            if (!drag.moved) {
                drag.moved = true;
                this.stage.classList.add('adv-dragging');
                try {
                    this.stage.setPointerCapture(event.pointerId);
                } catch (error) {
                    // capture is optional
                }
            }
            this.x += event.clientX - drag.lastX;
            this.y += event.clientY - drag.lastY;
            drag.lastX = event.clientX;
            drag.lastY = event.clientY;
            this.apply(false);
        }

        onPointerUp(event) {
            if (this.drag && this.drag.pointerId === event.pointerId && this.drag.moved) {
                this.suppressClick = true;
            }
            this.pointers.delete(event.pointerId);
            if (this.pointers.size < 2) {
                this.pinch = null;
            }
            if (this.pointers.size === 0) {
                this.drag = null;
                this.stage.classList.remove('adv-dragging');
            }
        }

        onClick(event) {
            if (this.suppressClick) {
                // The press was a pan, not a click: do not follow links or open the diagram.
                this.suppressClick = false;
                event.preventDefault();
                event.stopPropagation();
                return;
            }
            const link = event.target.closest('a.diagram-link');
            if (link && !this.options.inModal && !event.ctrlKey && !event.metaKey && !event.shiftKey) {
                // The build-time full-size link is the no-JavaScript fallback; with the viewer a single click on the
                // page-layout diagram expands it in-page. The expansion waits briefly so a double-click can zoom
                // instead; in navigation mode a click never expands (it would interrupt panning).
                event.preventDefault();
                window.clearTimeout(this.pendingExpand);
                if (this.navigating || event.detail > 1) {
                    return;
                }
                this.pendingExpand = window.setTimeout(() => {
                    this.pendingExpand = null;
                    openModal(this, this.toolbar.querySelector('.adv-expand'));
                }, CONFIG.expandClickDelay);
            }
        }

        onKeyDown(event) {
            if (event.target !== this.stage) {
                return;
            }
            const step = CONFIG.keyboardPanStep;
            switch (event.key) {
                case '+': case '=': this.zoomAt(CONFIG.buttonStep, undefined, undefined, true); break;
                case '-': case '_': this.zoomAt(1 / CONFIG.buttonStep, undefined, undefined, true); break;
                case '0': this.actualSize(true); break;
                case 'f': case 'F': this.fit(true); break;
                case 'r': case 'R': this.exitNavigation(); break;
                case 'Enter': if (!this.options.inModal) { openModal(this, this.toolbar.querySelector('.adv-expand')); } break;
                case 'ArrowLeft': if (!this.navigating) { return; } this.panBy(step, 0); break;
                case 'ArrowRight': if (!this.navigating) { return; } this.panBy(-step, 0); break;
                case 'ArrowUp': if (!this.navigating) { return; } this.panBy(0, step); break;
                case 'ArrowDown': if (!this.navigating) { return; } this.panBy(0, -step); break;
                default: return;
            }
            event.preventDefault();
        }

        onResize() {
            if (!this.navigating) {
                this.updateReadout();
            }
        }
    }

    // ----- expanded (modal) viewer ------------------------------------------------------------------------------------
    function buildModal() {
        const overlay = document.createElement('div');
        overlay.className = 'adv-modal';
        overlay.hidden = true;
        overlay.innerHTML = '<div class="adv-modal__dialog" role="dialog" aria-modal="true">'
            + '<h2 class="adv-modal__title"></h2><div class="adv-modal__stage"></div></div>';
        document.body.appendChild(overlay);
        const dialog = overlay.querySelector('.adv-modal__dialog');
        const title = overlay.querySelector('.adv-modal__title');
        title.id = 'adv-modal-title';
        dialog.setAttribute('aria-labelledby', title.id);
        overlay.addEventListener('mousedown', (event) => {
            if (event.target === overlay) {
                closeModal();
            }
        });
        overlay.addEventListener('keydown', (event) => {
            if (event.key === 'Escape') {
                event.preventDefault();
                closeModal();
            } else if (event.key === 'Tab') {
                const focusable = [...dialog.querySelectorAll('button, [tabindex="0"], a[href]')]
                    .filter((element) => element.offsetParent !== null);
                if (!focusable.length) {
                    return;
                }
                const first = focusable[0];
                const last = focusable[focusable.length - 1];
                if (event.shiftKey && document.activeElement === first) {
                    event.preventDefault();
                    last.focus();
                } else if (!event.shiftKey && document.activeElement === last) {
                    event.preventDefault();
                    first.focus();
                }
            }
        });
        return {overlay, dialog, title, stageHost: overlay.querySelector('.adv-modal__stage')};
    }

    function openModal(source, returnFocus) {
        if (modal && modal.active) {
            return;
        }
        modal = modal || buildModal();
        const wasNavigating = source.navigating;
        const savedView = {scale: source.scale, x: source.x, y: source.y};
        source.exitNavigation();
        const placeholderHeight = source.stage.getBoundingClientRect().height;
        modal.title.textContent = source.title;
        modal.overlay.hidden = false;
        document.documentElement.classList.add('adv-modal-open');
        // Move (never copy) the diagram node so SVG ids, links and accessibility labels stay unique.
        const anchor = document.createComment('adv-diagram-origin');
        source.content.parentNode.insertBefore(anchor, source.content);
        source.stage.style.minHeight = placeholderHeight + 'px';
        const stage = document.createElement('div');
        stage.className = 'adv-modal__viewport';
        modal.stageHost.appendChild(stage);
        stage.appendChild(source.content);
        modal.viewer = new DiagramViewer(modal.dialog, stage, source.content, {inModal: true});
        modal.viewer.title = source.title;
        modal.active = {source, anchor, returnFocus: returnFocus || document.activeElement, wasNavigating, savedView};
        requestAnimationFrame(() => {
            modal.viewer.fit(false);
            const close = modal.dialog.querySelector('.adv-close');
            if (close) {
                close.focus();
            }
        });
    }

    function closeModal() {
        if (!modal || !modal.active) {
            return;
        }
        const {source, anchor, returnFocus, wasNavigating, savedView} = modal.active;
        if (document.fullscreenElement) {
            document.exitFullscreen().catch(() => null);
        }
        modal.viewer.destroy();
        modal.viewer = null;
        anchor.parentNode.insertBefore(source.content, anchor);
        anchor.remove();
        modal.stageHost.textContent = '';
        modal.overlay.hidden = true;
        modal.active = null;
        document.documentElement.classList.remove('adv-modal-open');
        source.stage.style.minHeight = '';
        if (wasNavigating) {
            source.enterNavigation();
            Object.assign(source, savedView);
            source.apply(false);
        } else {
            source.updateReadout();
        }
        if (returnFocus && document.contains(returnFocus)) {
            returnFocus.focus();
        }
    }

    function toggleFullscreen() {
        if (!modal || !modal.active) {
            return;
        }
        if (document.fullscreenElement) {
            document.exitFullscreen().catch(() => null);
        } else if (modal.overlay.requestFullscreen) {
            modal.overlay.requestFullscreen().then(() => modal.viewer && modal.viewer.fit(false)).catch(() => null);
        }
    }

    // ----- discovery / lifecycle ----------------------------------------------------------------------------------
    function discover() {
        const found = [];
        document.querySelectorAll('.md-typeset figure.diagram:not(.screenshot)').forEach((figure) => {
            const image = figure.querySelector('img[style*="--diagram-width"]');
            const stage = image && image.closest('p');
            if (image && stage && figure.contains(stage)) {
                found.push({root: figure, stage, content: image});
            }
        });
        document.querySelectorAll('.diagram-viewport').forEach((viewport) => {
            const svg = viewport.querySelector(':scope > svg');
            if (svg) {
                found.push({root: viewport.closest('figure') || viewport, stage: viewport, content: svg});
            }
        });
        return found;
    }

    function initialise() {
        // Navigation without a full reload (Material instant navigation): drop viewers of detached pages first.
        instances.forEach((instance) => {
            if (!document.contains(instance.stage)) {
                instance.destroy();
            }
        });
        discover().forEach(({root, stage, content}) => {
            if (stage.dataset.advInitialised === 'true') {
                return;
            }
            stage.dataset.advInitialised = 'true';
            new DiagramViewer(root, stage, content); // eslint-disable-line no-new
        });
    }

    window.AtlasDiagramViewer = {config: CONFIG, initialise, instances, close: closeModal};
    if (window.document$ && typeof window.document$.subscribe === 'function') {
        window.document$.subscribe(() => initialise());
    } else if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initialise, {once: true});
    } else {
        initialise();
    }
}());
