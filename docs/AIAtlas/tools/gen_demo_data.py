"""Данные учебных демонстраций, встроенные как JS (без fetch, работает из file://).

Пишет docs/assets/js/atlas-demo-data.js: определения Access Key и Target Recovery из assets/ai,
route costs (Дейкстра по авторскому графу и позициям уровня), стартовые состояния сценариев.
"""
import json
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ATLAS / 'tools'))
import goap_model as g  # noqa: E402
import references  # noqa: E402


def main():
    data = g.load_access_key()
    recovery = g.load_json('ai/goap/target_recovery.goap.json')
    marker = g.load_json('ai/goap/marker_visit.goap.json')
    payload = dict(
        snapshot=references.snapshot()['head'][:12],
        semanticIds=g.SEMANTIC_IDS,
        domains=dict(
            access_key=dict(title='Access Key (реальные ассеты, route costs)', goap=data['goap'],
                            routeCosts={k: round(v, 6) for k, v in data['route_costs'].items()},
                            presets=dict(
                                start=dict(title='Старт сценария', state=dict(atStart=True, coinAAvailable=True,
                                                                             coinBAvailable=True, coinCAvailable=True)),
                                replan=dict(title='У монеты C, монета A исчезла', state=dict(
                                    atCoinC=True, coinCCollected=True, coins=1, coinBAvailable=True)),
                                broke=dict(title='Нет доступных монет', state=dict(atCoinC=True, coinCCollected=True, coins=1)))),
            target_recovery=dict(title='Target Recovery', goap=recovery, routeCosts={},
                                 presets=dict(available=dict(title='Цель доступна', state=dict(goalAvailable=True)),
                                              missing=dict(title='Цель временно недоступна', state=dict(goalAvailable=False)))),
            marker_visit=dict(title='Marker Visit', goap=marker, routeCosts={},
                              presets=dict(present=dict(title='Маркер есть', state=dict(markerPresent=True)),
                                           reached=dict(title='Маркер уже достигнут', state=dict(markerPresent=True, markerReached=True))))))
    text = ('/* Сгенерировано tools/gen_demo_data.py из assets/ai и уровня Access Key. Не редактировать вручную. */\n'
            'window.ATLAS_DEMO_DATA = ' + json.dumps(payload, ensure_ascii=False, indent=1) + ';\n')
    (ATLAS / 'docs' / 'assets' / 'js' / 'atlas-demo-data.js').write_text(text, encoding='utf-8')
    print('demo data written')


if __name__ == '__main__':
    main()
