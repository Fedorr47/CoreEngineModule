# Демонстрация: профиль Arrival и гистерезис Flee

!!! warning "Учебная модель"
    Формулы повторяют `BuildGameplayArrivalSteering` и правило бегства `AIFleeTargetActionRuntime`
    (главы [13](../chapters/13-steering.md), [14](../chapters/14-temporal-behaviors.md)). Это не симуляция:
    нет мотора с ограничением ускорения, физики, кадрового цикла и интервала выборки 0.05 с; дистанции —
    горизонтальные (XZ).

## Arrival

$$
m(d) =
\begin{cases}
0, & d \le r + 0.001,\\
1, & R - r \le 10^{-6} \;\text{или}\; d \ge R,\\
\max\!\left(\dfrac{d - r}{R - r},\; m_{\min}\right), & \text{иначе}.
\end{cases}
$$

<div class="demo" data-demo="arrival" markdown="0">
  <div class="demo-grid">
    <div>
      <label>r (acceptance) = <span data-arrival-value="r"></span><input type="range" min="0" max="2" step="0.01" value="0.5" data-arrival="r"></label>
      <label>R (slowing) = <span data-arrival-value="R"></span><input type="range" min="0" max="4" step="0.01" value="2.5" data-arrival="R"></label>
      <label>m_min = <span data-arrival-value="min"></span><input type="range" min="0" max="1" step="0.01" value="0.1" data-arrival="min"></label>
      <label>d = <span data-arrival-value="d"></span><input type="range" min="0" max="5" step="0.0001" value="1.5" data-arrival="d"></label>
      <label><input type="checkbox" data-arrival-run> бег (4.5 м/с вместо 2.0)</label>
      <div class="demo-out" data-arrival-out></div>
    </div>
    <svg class="demo-svg" role="img" aria-label="Профиль Arrival"></svg>
  </div>
</div>

Контрольные случаи (проверены `tools/verify_examples.py`): r = 0.5, R = 2.5, m_min = 0.1 → d = 1.5 даёт 0.50;
d = 0.6 даёт 0.10 (плато); d = 0.5005 — 0 (внутри допуска); d = 0.5011 — 0.10 (скачок); R < r приводится к R = r.

## Гистерезис Flee

$$
s_k = \begin{cases} [d_k < \theta_s], & s_{k-1} = 1,\\ [d_k \le \theta_t], & s_{k-1} = 0, \end{cases}
\qquad \theta_s \leftarrow \max(\theta_s, \theta_t).
$$

<div class="demo" data-demo="flee" markdown="0">
  <div class="demo-grid">
    <div>
      <label>trigger θ_t = <span data-flee-value="trigger"></span><input type="range" min="0" max="10" step="0.1" value="5" data-flee="trigger"></label>
      <label>safe θ_s = <span data-flee-value="safe"></span><input type="range" min="0" max="12" step="0.1" value="7" data-flee="safe"></label>
      <label>дистанция d = <span data-flee-value="d"></span><input type="range" min="0" max="12" step="0.1" value="8" data-flee="d"></label>
      <p>
        <button type="button" data-flee-action="sweep">Прогон 8 → 6 → 5 → 4 → 6 → 6.9 → 7 → 6 → 5</button>
        <button type="button" data-flee-action="reset">Сброс</button>
      </p>
      <div class="demo-out" data-flee-out></div>
    </div>
    <div class="demo-out" data-flee-log></div>
  </div>
</div>

При параметрах Steering Playground (5 и 7) прогон показывает память: на d = 6 по пути «вперёд» агент стоит,
после старта на d = 5 — продолжает бежать на 6 и 6.9 и останавливается только на 7. Двигайте ползунок дистанции
вперёд-назад около 5 и 7, чтобы увидеть, что при одном пороге (θ_s = θ_t) состояние переключалось бы на каждом
пересечении.
