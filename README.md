# Modo

**Modo** — кроссплатформенная нативная дизайн-студия для создания адаптивных иконок для macOS и Windows в едином гармоничном стиле.

Разработчик: **Atoma**

---

## Ключевые возможности

- **100% нативная реализация под каждую ОС**:
  - **macOS**: Нативное приложение на **SwiftUI** и **SwiftData** с трехколоночной навигацией `NavigationSplitView`, плавной физикой сглаживания жестов и поддержкой тем оформления macOS.
  - **Windows**: Нативное высокопроизводительное приложение на **C++17** и **GDI+** без внешних зависимостей (чистый Win32 API, никакого Electron или WebView2). Трехколоночная компоновка (боковая панель 210px, адаптивная галерея и инспектор 360px), нативный системный фон Windows 11 **Mica** (`DWMWA_SYSTEMBACKDROP_TYPE`), аппаратное скругление углов и нулевое время отклика (чистый PE32+).
- **Интерактивная 3D-физика наклона и блика (3D Parallax Tilt & Specular Glare)**:
  - Живой холст предпросмотра иконки рассчитывает угол наклона в 3D-пространстве в реальном времени при движении курсора (`WM_MOUSEMOVE` / `GeometryReader`).
  - Динамический световой блик (Specular Glare sweep) с перемещением светового пятна по поверхности материала.
  - Плавная кинематическая пружинная интерполяция возврата в исходное положение при отведении курсора.
- **Набор инструментов разработчика (Developer Export Suite)**:
  - Мультиразмерный файл `.ico` для Windows (256px PNG-фрейм + 64/48/32/16px 32-bpp DIB-фреймы).
  - Мастер-изображение сверхвысокого разрешения `1024×1024 .png` и контейнер `.icns`.
  - **Apple Xcode Asset Catalog**: пакет `AppIcon.appiconset` со сгенерированным манифестом `Contents.json` и полным набором иконок (16, 32, 64, 128, 256, 512, 1024 px).
  - **Web Favicon Kit**: готовый комплект для веб-сайтов (`favicon.ico`, `favicon-16x16.png`, `favicon-32x32.png`, `apple-touch-icon.png` 180px, `android-chrome-192x192.png`, `android-chrome-512x512.png`, `site.webmanifest`).
  - **Чистый векторный SVG**: экспорт геометрии формы, шейдеров градиентов и нативных фильтров теней `feDropShadow` в валидный формат XML SVG.
  - Универсальный ZIP-архив со всеми форматами (`Ctrl+E`).
- **Генератор презентационных баннеров Showcase (1920×1080)**:
  - Создание полиграфического баннера с разрешением 1920×1080 для витрин магазинов приложений, портфолио и README.
  - Освещение студийного пьедестала, градиентное свечение подложки, типографика студии и плавающие миниатюры силуэтов форм.
- **Пакетная конвертация изображений в иконки (Batch Icon Creator)**:
  - Встроенный диалог (`Ctrl+Shift+B` или меню «Инструменты -> Пакетная конвертация изображений…»).
  - Быстрое сканирование каталогов на наличие файлов `.png`, `.jpg`, `.jpeg`, `.bmp`, `.ico` с автоматической генерацией мультиразмерных `.ico` и мастер-PNG 1024px.
  - Индикатор прогресса в реальном времени и итоговый отчет.
- **Кастомизация папок и интеграция с проводником**:
  - Нативная стилизация любых папок файловой системы через `desktop.ini` с атрибутами `FILE_ATTRIBUTE_READONLY` / `SYSTEM` и автоматическим обновлением кэша Windows Shell (`SHChangeNotify`).
  - Полный Drag-and-Drop: перетаскивание папок и исполняемых файлов прямо в окно приложения для мгновенной стилизации.
- **Интеграция системного акцентного цвета Windows (Dynamic Accent Color)**:
  - Поддержка актуального цвета персонализации Windows 10/11 через API `DwmGetColorizationColor` с фоллбэком на системные цвета интерфейса.
  - Быстрое применение через `Ctrl+Shift+A` или меню «Стиль».
  - Живой 9-й свотч в панели «Стиль» инспектора, автоматически синхронизирующийся с системной темой.
- **Встроенный консольный интерфейс (Native Headless CLI)**:
  - Единый исполняемый файл `Modo.exe` автоматически распознает запуск из командной строки Windows (`AttachConsole`) и предоставляет полноценный инструментарий автоматизации:
    - `modo style <app_path> [style]` — применение стиля к приложению.
    - `modo restore <app_path>` — возврат оригинальной иконки.
    - `modo folder <folder_path> [style]` — стилизация каталога с записью `desktop.ini`.
    - `modo batch <input_dir> [output_dir] [style]` — пакетная конвертация каталога.
    - `modo showcase [output_path]` — генерация презентационного баннера 1920×1080.
    - `modo export <app_path> <ico|png|svg|favicon|xcode|all> [output_dir]` — экспорт пакетов иконок.
    - `modo theme <system|light|dark>` — переключение цветовой темы оформления.
    - `modo shell <refresh|clean-cache>` — сброс и перестроение системного кэша иконок Проводника.
    - `modo version` / `modo help` — вывод версии и справки.
  - Консольная обертка `modo.cmd` для удобного запуска из любого терминала (cmd.exe, PowerShell).
- **Сетка и список приложений (Gallery & Compact List)**:
  - Режим галереи карточек с динамическим ползунком масштаба иконок (48–96 px на Windows, 36–140 px на macOS) и авто-подбором колонок.
  - Компактный режим списка для быстрой пакетной работы.
- **19 курируемых дизайн-пакетов**:
  - Встроенный каталог из 19 стилей: *Pixel Pure, Material You, Modo Noir, Atoma Mono, WhiteSur macOS, Tela Minimal, Lawnicons Themed, macOSicons Hub, Tabler, Simple Icons, Squircle, Paper, Cyber Hex, Catppuccin, Nordic, Tokyo Night, Dracula, Sunset Pebble, Fluent Soft*.
- **Встроенное автообновление (OTA Updates)**:
  - Фоновая проверка релизов на GitHub с проверкой контрольных сумм SHA-256 и обновлением в один клик прямо из приложения.
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
  - **Интерактивный холст (Drag-to-Pan)**: перетаскивание глифа внутри формы мышью или трекпадом с авто-центрированием по двойному клику.
  - Масштабирование глифа от 30% до 250% (Zoom / Full-Bleed).
  - Смещение по осям X и Y, интенсивность тени (Elevation Shadow).
  - Пресеты оформления: *Google White*, *Material You Pastel* (автоанализ цветов исходной иконки), *Dark Material*, *Adaptive Edge*, *Custom Color* с пипеткой Eyedropper.
  - Переключатели: *Auto-Trim Margins*, *Inner Glyph Shadow*, *Full-Bleed Square Mask*, *Material Monochrome Tint*.
- **Безопасность и откат в 1 клик**:
  - Системные исполняемые файлы `.exe` не модифицируются.
  - Исходные иконки и оригинальные `.lnk` сохраняются в защищенном каталоге бэкапов (`%LOCALAPPDATA%\Modo\backup`).
  - Мгновенный откат к заводскому виду в один клик.

---

## Быстрый запуск и сборка

### Сборка всех установщиков одной командой
В корне репозитория находится мастер-скрипт сборки:
```bash
python3 build_installers.py
```
Он компилирует нативный macOS-билд, собирает чистые Windows PE32+ бинарники и упаковывает все дистрибутивы в каталог `dist/` (а также автоматически синхронизирует их с `/Users/tasonai/Developer/Modo/dist/`).

---

### Сборка для macOS
Требуется: macOS 14+, Xcode / Command Line Tools.
```bash
# Сборка и установка в ~/Applications/Modo.app
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

## Доступные установщики (`dist/`)

### macOS
- **Homebrew Cask**:
  ```bash
  brew install --cask tasonai/tap/modo
  ```
- **`Modo-macOS.dmg`** — классический образ диска с Drag-and-Drop в `/Applications`.
- **`Modo-macOS-Installer.pkg`** — стандартный системный мастер установки macOS.
- **`Modo-macOS.zip`** — готовый архив с подписанным `Modo.app`.

### Windows
- **`Modo-Windows-Setup.exe`** — нативный установщик (размещает в `%LOCALAPPDATA%\Programs\Modo`, создает ярлыки на Рабочем столе и в «Пуск»).
- **`Modo-Windows-Portable.exe`** — портативная версия (не требует установки).
- **`Modo-Windows-x64.zip`** — портативный ZIP-архив.

---

## Консольный интерфейс (CLI)

Приложение `Modo.exe` может запускаться в режиме командной строки:

```text
Использование: modo <команда> [параметры]

Команды:
  style <app_or_path> [style]                     Применить адаптивный стиль к приложению
  restore <app_or_path>                           Восстановить исходную иконку
  folder <folder_path> [style]                    Стилизовать папку через desktop.ini
  batch <input_folder> [out_dir] [style]          Пакетная конвертация изображений
  showcase [out_png]                              Сгенерировать баннер презентации 1920x1080
  export <app> <ico|png|svg|favicon|xcode|all>    Экспортировать пакет форматов
  theme <system|light|dark>                       Переключить цветовую тему Modo
  shell <refresh|clean-cache>                     Перестроить кэш иконок Windows Shell
  version                                         Вывести версию Modo
  help                                            Показать эту справку
```

Примеры:
```bash
modo style "Google Chrome" squircle
modo folder "D:\Projects\Modo" dark
modo batch "C:\Assets\Logos" "C:\Assets\Icons"
modo showcase "C:\Assets\banner.png"
modo export "Visual Studio Code" xcode "C:\Assets\Export"
```

---

## Интеграции с лаунчерами и оболочками

В каталоге `integrations/` доступны расширения и скрипты для популярных лаунчеров:

- **Raycast (macOS)**: расширение для быстрого поиска и стилизации иконок установленных приложений (`integrations/raycast-modo`).
- **Alfred (macOS)**: рабочий процесс (workflow) для вызова стилизации по ключевому слову `modo` (`integrations/alfred`).
- **PowerShell (Windows)**: модуль `Modo.psm1` с командлетами `Set-ModoStyle`, `Set-ModoFolder`, `New-ModoBatch`, `New-ModoShowcase`, `Export-ModoIcon` и полной поддержкой автодополнения по клавише Tab (`integrations/powershell`).
- **Microsoft PowerToys Run (Windows)**: вызов команд стилизации через шорткат `Alt+Space` и команду `> modo` (`integrations/powertoys-run`).

---

## Архитектура репозитория

```
Modo/
├── Sources/
│   ├── Modo/                     # macOS SwiftUI приложение
│   │   ├── Views/
│   │   │   ├── SidebarView.swift         # Боковая панель категорий и стилей
│   │   │   ├── AppListView.swift         # Список приложений, фильтры-капсулы
│   │   │   └── DetailInspectorView.swift # Студия редактирования и жесты Drag-to-Pan
│   │   ├── ContentView.swift             # Корневой SplitView и тулбар
│   │   ├── Models.swift                  # Модели данных и состояние приложения
│   │   └── IconRenderer.swift            # CoreGraphics рендерер адаптивных иконок
│   └── ModoKit/                  # Библиотека правил и математики преобразований
├── windows-native/               # Нативный C++ движок Windows
│   ├── types.h                   # Общие структуры (AppEntry, RoundStyle, Swatches)
│   ├── icon_engine.h / .cpp      # GDI+ движок, автообрезка, парсинг, шейдеры и экспорт
│   ├── shell_integration.h / .cpp# Shell API, IShellLink, реестр CLSID, кэш Explorer
│   ├── modo_cli.h / .cpp         # Консольный интерфейс (Headless CLI engine)
│   ├── installer.h / .cpp        # Встроенная логика развертывания установщика
│   ├── updater.h / .cpp          # Модуль проверки и установки обновлений OTA
│   ├── modo_win32.cpp            # Оконная процедура Win32, холст и контролы
│   ├── modo.cmd                  # Пакетный скрипт вызова CLI в командной строке
│   ├── modo.rc                   # Ресурсы, VERSIONINFO, UAC Manifest
│   └── build_windows_installers.py # Скрипт сборки PE32+ бинарников
├── integrations/                 # Расширения для внешних лаунчеров и оболочек
│   ├── raycast-modo/             # Расширение для Raycast (macOS)
│   ├── alfred/                   # Workflow для Alfred (macOS)
│   ├── powershell/               # Модуль Modo.psm1 и автодополнение (Windows)
│   └── powertoys-run/            # Инструкция и интеграция с Microsoft PowerToys Run
├── tools/                        # Утилиты верификации и упаковки
│   ├── package_macos.py          # Генератор DMG, PKG и ZIP для macOS
│   ├── snap.sh                   # Headless-снятие снэпшотов интерфейса
│   └── selftest.sh               # Комплексный набор автотестов (114 проверок)
├── build.sh                      # Скрипт сборки macOS приложения
└── build_installers.py           # Мастер-скрипт сборки всех установщиков
```

---

## Лицензия

Copyright © 2026 **Atoma**. Все права защищены.
