# Google Round Iconizer (Windows)

Приложение для автоматического преобразования иконок всех установленных программ Windows в **круглые адаптивные иконки в стиле Google Pixel / Material You**.

## Возможности
- **Авто-поиск всех ярлыков Windows (`.lnk`)**: Рабочий стол (`%UserProfile%\Desktop`, `%Public%\Desktop`), меню «Пуск» (`%AppData%`, `%ProgramData%`) и Панель задач (`User Pinned\TaskBar`).
- **Извлечение оригинальных иконок в максимальном качестве (`256×256`)** напрямую из PE-ресурсов `.exe` / `.dll` через `icoextract` и Win32 API `PrivateExtractIconsW`.
- **Суперсэмплинг `1024×1024 → 256×256`**: идеально гладкие края круга и сохранение всех стандартных слоёв `.ico` (`256, 128, 64, 48, 32, 24, 16 px`).
- **4 фирменных стиля Google**:
  1. **Google White (Pixel Classic)** — чисто-белый круг с тенью Material Elevation.
  2. **Material You Pastel** — автоматическое извлечение доминирующего цвета каждой иконки и генерация мягкого пастельного круга в тон.
  3. **Dark Material** — графитовый круг `#1E1F22` для тёмной темы Windows 11.
  4. **Adaptive Edge / Custom Color** — заливка цветом краёв иконки или любым выбранным цветом + опциональный монохромный режим **Themed Icons**.
- **Безопасность, Бэкап и Откат в 1 клик**: системные `.exe` файлы не изменяются; перед заменой создаётся резервная копия ярлыков в `%LocalAppData%\GoogleRoundIconizer\backups\`.
- **Фоновый мониторинг (`watchdog`)**: автоматически скругляет иконки новых программ сразу после их установки.

---

## Быстрый запуск

### На Windows
```bat
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

### Сборка в один `.exe` файл на Windows
Запустите `build_windows.bat` двойным кликом — готовый файл появится в `dist\GoogleRoundIconizer.exe`.

### Запуск превью на macOS
```bash
./.venv/bin/python main.py
```

### CLI-режим (без интерфейса, например для автозагрузки)
```bash
python main.py --apply-all --preset material_pastel
python main.py --restore
python main.py --export ./exported_icons
```
