# IconMaker

**IconMaker** — кроссплатформенная нативная дизайн-студия для автоматического и ручного преобразования иконок приложений в **круглые адаптивные иконки в стиле Google Pixel и Material You**.

Разработчик: **Atoma**

---

## 🌟 Ключевые возможности

- **100% нативная реализация под каждую ОС**:
  - **macOS**: Нативное приложение на **SwiftUI** и **SwiftData** с трехколоночной навигацией `NavigationSplitView`, плавной физикой сглаживания жестов и поддержкой тем оформления macOS.
  - **Windows**: Нативное приложение на **C++17** и **GDI+** без тяжелых сторонних фреймворков и без среды выполнения Python. Чистый PE32+ бинарник без DWARF-секций и оверлеев (полная совместимость с Windows Defender).
- **Строгий нейтральный дизайн (Achromatic Minimal)**:
  - Лаконичный интерфейс без навязчивых цветных акцентов.
  - Четкая визуальная иерархия, высокая контрастность (WCAG AAA) в светлой и тёмной темах.
- **Поддержка системных компонентов Windows**:
  - **Проводник (File Explorer)**: патчинг `explorer.exe` в реестре CLSID и ярлыков в Панели задач (`TaskBar`).
  - Системные значки Рабочего стола: **Этот компьютер**, **Корзина** (состояния пустая/полная), **Папка пользователя**, **Панель управления**, **Сеть**.
- **Умное сканирование ярлыков**:
  - Автоматическое обнаружение программ на Рабочем столе, в меню «Пуск» и на Панели задач.
  - Фильтрация деинсталляторов и дубликатов с объединением нескольких ссылок на один исполняемый файл.
- **Интерактивная студия кастомизации**:
  - **Интерактивный холст (Drag-to-Pan)**: перетаскивание глифа внутри круга мышью или трекпадом с авто-центрированием по двойному клику.
  - Масштабирование глифа от 30% до 250% (Zoom / Full-Bleed).
  - Смещение по осям X и Y, интенсивность тени (Elevation Shadow).
  - Пресеты фона: *Google White*, *Material You Pastel* (автоанализ цветов исходной иконки), *Dark Material*, *Adaptive Edge*, *Custom Color* с пипеткой Eyedropper.
  - Переключатели: *Auto-Trim Margins*, *Inner Glyph Shadow*, *Full-Bleed Square Mask*, *Material Monochrome Tint*.
- **Безопасность и откат в 1 клик**:
  - Системные исполняемые файлы `.exe` не модифицируются.
  - Исходные иконки и оригинальные `.lnk` сохраняются в защищенном каталоге бэкапов (`%LOCALAPPDATA%\IconMaker\backup`).
  - Мгновенный откат к заводскому виду в один клик.
- **Экспорт полного набора форматов**:
  - Windows: мультиразмерный `.ico` (256px PNG-frame + 64/48/32/16px 32-bpp DIB frames).
  - macOS: `.icns` высокого разрешения и мастер-изображение `1024×1024 .png`.

---

## 🚀 Быстрый запуск и сборка

### Сборка всех установщиков одной командой
В корне репозитория находится мастер-скрипт сборки:
```bash
python3 build_installers.py
```
Он компилирует нативный macOS-билд, собирает чистые Windows PE32+ бинарники и упаковывает все дистрибутивы в каталог `dist/` (а также автоматически синхронизирует их с `/Users/tasonai/Developer/IconMaker/dist/`).

---

### Сборка для macOS
Требуется: macOS 14+, Xcode / Command Line Tools.
```bash
# Сборка и установка в ~/Applications/IconMaker.app
./build.sh

# Запуск приложения после сборки
./build.sh --open

# Упаковка дистрибутивов macOS (.dmg, .pkg, .zip)
python3 tools/package_macos.py
```

### Сборка для Windows (кросс-компиляция или нативная сборка)
Требуется: компилятор `zig` (0.13.0+) или MinGW-w64 / MSVC.
```bash
python3 windows-native/build_windows_installers.py
```

---

## 📦 Доступные установщики (`dist/`)

### 🍏 macOS
- **`IconMaker-macOS.dmg`** — классический образ диска с Drag-and-Drop в `/Applications`.
- **`IconMaker-macOS-Installer.pkg`** — стандартный системный мастер установки macOS.
- **`IconMaker-macOS.zip`** — готовый архив с подписанным `IconMaker.app`.

### 🪟 Windows
- **`IconMaker-Windows-Setup.exe`** — нативный установщик (размещает в `%LOCALAPPDATA%\Programs\IconMaker`, создает ярлыки на Рабочем столе и в «Пуск»).
- **`IconMaker-Windows-Portable.exe`** — портативная версия (не требует установки).
- **`IconMaker-Windows-x64.zip`** — портативный ZIP-архив.
- **`GoogleRoundIconizer-Setup.exe`** — установочный алиас.

---

## 🏛️ Архитектура репозитория

```
Icon Maker/
├── Sources/
│   ├── IconMaker/                # macOS SwiftUI приложение
│   │   ├── Views/
│   │   │   ├── SidebarView.swift         # Боковая панель категорий и стилей
│   │   │   ├── AppListView.swift         # Список приложений, фильтры-капсулы
│   │   │   └── DetailInspectorView.swift # Студия редактирования и жесты Drag-to-Pan
│   │   ├── ContentView.swift             # Корневой SplitView и тулбар
│   │   ├── Models.swift                  # Модели данных и состояние приложения
│   │   └── IconRenderer.swift            # CoreGraphics рендерер круглых иконок
│   └── IconMakerKit/             # Библиотека правил и математики преобразований
├── windows-native/               # Нативный C++ движок Windows
│   ├── types.h                   # Общие структуры (AppEntry, RoundStyle, Swatches)
│   ├── icon_engine.h / .cpp      # GDI+ движок, автообрезка, парсинг и ICO-кодер
│   ├── shell_integration.h / .cpp# Shell API, IShellLink, реестр CLSID, кэш Explorer
│   ├── iconmaker_win32.cpp       # Оконная процедура, GDI+ холст и контролы
│   ├── iconmaker.rc              # Ресурсы, VERSIONINFO, UAC Manifest
│   └── build_windows_installers.py # Скрипт сборки PE32+ бинарников
├── tools/                        # Утилиты верификации и упаковки
│   ├── package_macos.py          # Генератор DMG, PKG и ZIP для macOS
│   ├── snap.sh                   # Headless-снятие снэпшотов интерфейса
│   └── selftest.sh               # Автотесты функционала приложения
├── build.sh                      # Скрипт сборки macOS приложения
└── build_installers.py           # Мастер-скрипт сборки всех установщиков
```

---

## 📄 Лицензия

Copyright © 2026 **Atoma**. Все права защищены.
