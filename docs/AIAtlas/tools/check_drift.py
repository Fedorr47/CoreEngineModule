"""Проверка дрейфа исходников относительно последней сборки атласа.

Сравнивает docs/data/source-references.json (записано при сборке) с текущим рабочим деревом:
изменившийся SHA-256 файла, сдвиг строки якоря, изменившийся фрагмент из 8 строк, пропавший якорь.
Запускать ПЕРЕД обновлением атласа: список показывает, какие главы и карточки перепроверить.
Код выхода 1, если найден дрейф.
"""
import hashlib
import json
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ATLAS / 'tools'))
import references  # noqa: E402


def main():
    recorded = json.loads((ATLAS / 'docs' / 'data' / 'source-references.json').read_text(encoding='utf-8'))
    current, failures = references.resolve()
    current = {entry['id']: entry for entry in current}
    head = references.snapshot()['head']
    drift = []
    if recorded['head'] != head:
        drift.append(f'HEAD изменился: {recorded["head"][:12]} → {head[:12]}')
    drift += [f'якорь не найден: {failure}' for failure in failures]
    for old in recorded['references']:
        new = current.get(old['id'])
        if new is None:
            continue
        if new['file_sha256'] != old['file_sha256']:
            _, _, lines = references.excerpt(old['id'], 8)
            excerpt = hashlib.sha256('\n'.join(lines).encode('utf-8')).hexdigest()
            what = 'фрагмент изменён' if excerpt != old.get('excerpt_sha256') else 'файл изменён, фрагмент тот же'
            drift.append(f'{old["id"]}: {old["path"]}:{old["line"]} → :{new["line"]} ({what})')
    for line in drift:
        print(line)
    print('drift:', len(drift))
    return 1 if drift else 0


if __name__ == '__main__':
    sys.exit(main())
