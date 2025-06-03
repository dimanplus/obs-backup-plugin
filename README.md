## Development Status
[![Push](https://github.com/dimanplus/obs-backup-plugin/actions/workflows/push.yaml/badge.svg)](https://github.com/dimanplus/obs-backup-plugin/actions/workflows/push.yaml)
![Windows Support](https://img.shields.io/badge/Windows-Supported-green)
![Linux Support](https://img.shields.io/badge/Linux-Planned-yellow)
![macOS Support](https://img.shields.io/badge/macOS-Planned-yellow)

# OBS Backup Plugin

<details>
<summary><strong>RUS описание</strong></summary>

## Плагин резервного копирования для OBS

Плагин для создания резервных копий плагинов, сцен и профилей в OBS. На текущий момент реализованы:

### Возможности:
- ✅ Создание резервной копии папки с плагинами (`obs-plugins`)
- ✅ Восстановление плагинов из резервной копии
- 🚧 Поддержка резервирования сцен и профилей (в будущем)

### Особенности:
- Работает только на Windows
- Для применения восстановленных плагинов требуется перезапуск OBS
- Простой и интуитивно понятный интерфейс

### Установка:
1. Скачайте последнюю версию плагина из [раздела Releases](https://github.com/dimanplus/obs-backup-plugin/releases)
2. Распакуйте архив в папку `obs-studio` вашей установки OBS (стандартный путь C:\Program Files\obs-studio)
3. Перезапустите OBS

### Использование:
1. Откройте OBS
2. Перейдите в `Резервная копия` в строке меню (там где Файл...Вид, Профиль... Справка...)

![image](https://github.com/user-attachments/assets/b6e90191-63e9-43c9-91ac-c98f360ba6db)

3. Выберите нужное действие:
   - "Создать резервную копию" - сохранит текущие плагины
   - "Загрузить резервную копию" - восстановит плагины из сохраненной копии

</details>

<details>
<summary><strong>ENG description</strong></summary>

## OBS Backup Plugin

A plugin for backing up and restoring OBS plugins, scenes and profiles. Currently implemented:

### Features:
- ✅ Backup of `obs-plugins` folder
- ✅ Restore plugins from backup
- 🚧 Scene and profile backup (in future)

### Notes:
- Windows only
- Requires OBS restart after restore
- Simple and intuitive interface

### Installation:
1. Download the latest version from [Releases section](https://github.com/dimanplus/obs-backup-plugin/releases)
2. Extract to `obs-studio` folder of your OBS installation (default path C:\Program Files\obs-studio)
3. Restart OBS

### Usage:
1. Open OBS
2. Go to `BACKUP` on Menu Bar

![image](https://github.com/user-attachments/assets/cb6f2b84-4a4d-40a5-85e5-1bb165e79e2b)

3. Select action:
   - "Create BACKUP" - saves current plugins
   - "Load BACKUP" - restores plugins from saved backup

</details>

## License
MIT License - see [LICENSE](LICENSE) file for details
