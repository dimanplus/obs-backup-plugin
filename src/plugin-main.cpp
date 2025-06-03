/*
obs-backup-plugin
Copyright (C) <2025> <https://github.com/dimanplus>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QStandardPaths>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <qdir.h>
#include <QThread>
#include <QProcess>
#include <QtConcurrent>

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

#include <QTranslator>
#include <QLocale>
#include <QMessageBox>

#include "test.hpp"

#include "plugin-support.h"
#include <qfiledialog.h>
#include <windows.h>
#include <shellapi.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	QWidget *main_widget = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!main_widget) {
		obs_log(LOG_ERROR, "Failed to get main QWidget from OBS frontend");
		return false;
	}

	QMainWindow *main_window = qobject_cast<QMainWindow *>(main_widget);
	if (!main_window) {
		obs_log(LOG_ERROR, "Failed to cast QWidget to QMainWindow");
		return false;
	}

	QMenuBar *menu_bar = main_window->menuBar();
	if (!menu_bar) {
		obs_log(LOG_ERROR, "Failed to get QMenuBar");
		return false;
	}

	// Создаем меню BACKUP
	QMenu *backup_menu = new QMenu(obs_module_text("BACKUP_MENU"), menu_bar);

	QAction *action_Create = new QAction(obs_module_text("CREATE_BACKUP"), backup_menu);
	QAction *action_Load = new QAction(obs_module_text("LOAD_BACKUP"), backup_menu);

	QObject::connect(action_Create, &QAction::triggered, []() {
		QString pluginPath = QCoreApplication::applicationDirPath();
		QDir dir(pluginPath);
		dir.cdUp();
		dir.cdUp();
		QString obsPluginsPath = dir.absoluteFilePath("obs-plugins");
		obs_log(LOG_INFO, "[%s] START_CREATE_BACKUP — obsPluginsPath : %s",
            PLUGIN_NAME,
			obsPluginsPath.toStdString().c_str());

		if (!QDir(obsPluginsPath).exists()) {
			obs_log(LOG_ERROR, "[%s] Backup failed: obs-plugins folder not found at %s",
                PLUGIN_NAME,
				obsPluginsPath.toStdString().c_str());
			return;
		}

		QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
		QString timestamp = QDateTime::currentDateTime().toString("dd-MM-yyyy_HH-mm-ss");
		QString zipPath = documentsPath + QString("/OBS_Backup_%1.zip").arg(timestamp);

		QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
		QString tempRootCopyPath = tempPath + "/obs-plugins-backup"; // чистая папка
		QString tempObsPluginsCopyPath = tempRootCopyPath + "/obs-plugins";

		// Запускаем копирование и архивирование в отдельном потоке
		auto archive = QtConcurrent::run([=]() {
			// Удаляем старую копию
			QDir(tempRootCopyPath).removeRecursively();

			// Создаем папку obs-plugins внутри tempRootCopyPath
			QDir().mkpath(tempObsPluginsCopyPath);

			// Копируем содержимое obsPluginsPath в tempObsPluginsCopyPath через PowerShell
			QString copyCmd =
				QString("Copy-Item -Path \"%1\\*\" -Destination \"%2\" -Recurse -ErrorAction SilentlyContinue")
					.arg(obsPluginsPath, tempObsPluginsCopyPath);

			QProcess copyProcess;
			copyProcess.start("powershell", QStringList() << "-NoProfile" << "-Command" << copyCmd);
			copyProcess.waitForFinished();

			if (!QDir(tempObsPluginsCopyPath).exists()) {
				obs_log(LOG_ERROR,
					"[%s] Backup failed: could not copy obs-plugins to temp",
                    PLUGIN_NAME);
				return;
			}

			// Архивируем tempRootCopyPath — в архив попадет папка obs-plugins и её содержимое
			QString zipCmd = QString("Compress-Archive -Path \"%1\\*\" -DestinationPath \"%2\" -Force")
						 .arg(tempRootCopyPath, zipPath);

			QProcess zipProcess;
			zipProcess.start("powershell", QStringList() << "-NoProfile" << "-Command" << zipCmd);
			zipProcess.waitForFinished(-1);

			QString stdOut = zipProcess.readAllStandardOutput();
			QString stdErr = zipProcess.readAllStandardError();

			// obs_log(LOG_INFO, "[%s PowerShell zip STDOUT: %s", PLUGIN_NAME, stdOut.toStdString().c_str());
			// obs_log(LOG_ERROR, "[%s] PowerShell zip STDERR: %s", PLUGIN_NAME, stdErr.toStdString().c_str());

			if (zipProcess.exitStatus() == QProcess::NormalExit && zipProcess.exitCode() == 0) {
				obs_log(LOG_INFO,
					"[%s] STOP_CREATE_BACKUP — Backup created successfully at: %s",
                    PLUGIN_NAME,
					zipPath.toStdString().c_str());

				// Вызов GUI из главного потока - показать сообщение об успешном создании
				QMetaObject::invokeMethod(
					QCoreApplication::instance(),
					[zipPath]() {
						QMessageBox::information(nullptr, obs_module_text("backup_title"),
									 QString("%1\n%2")
										 .arg(obs_module_text("backup_success"))
										 .arg(zipPath));
					},
					Qt::QueuedConnection);

			} else {
				obs_log(LOG_ERROR, "[%s] Backup creation failed",
                PLUGIN_NAME);
			}

			// Очистка временной копии
			QDir(tempRootCopyPath).removeRecursively();
		});
	});

	QObject::connect(action_Load, &QAction::triggered, []() {
		QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

		QString filePath = QFileDialog::getOpenFileName(nullptr, obs_module_text("LOAD_BACKUP"), documentsPath,
								"ZIP Archives (*.zip);;All Files (*)");

		if (filePath.isEmpty()) {
			obs_log(LOG_INFO, "[%s] No backup file selected",
            PLUGIN_NAME);
			return;
		}

		QString fileName = QFileInfo(filePath).fileName();
		obs_log(LOG_INFO, "[%s] START_LOAD_BACKUP — Selected backup file: %s",
            PLUGIN_NAME,
			filePath.toStdString().c_str());

		QMetaObject::invokeMethod(
			QCoreApplication::instance(),
			[filePath, fileName]() {
				QString pluginPath = QCoreApplication::applicationDirPath();
				QDir dir(pluginPath);
				dir.cdUp();
				dir.cdUp();
				QString obsStudioPath = dir.absolutePath(); // путь до obs-studio (два уровня вверх)

				QString zipPathWin = QDir::toNativeSeparators(filePath);
				QString destPathWin = QDir::toNativeSeparators(obsStudioPath);
				QString exePathWin = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

				QString scriptContent = QString(R"(
@echo off
:: Проверка запуска с правами администратора, если нет - перезапускаем с UAC
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Запуск с правами администратора...
    powershell -Command "Start-Process '%~f0' -ArgumentList '%1','%2','%3' -Verb runAs"
    exit /b
)

echo [WAIT] waiting close OBS...
:wait_loop
tasklist | findstr /I "obs64.exe" >nul
if not errorlevel 1 (
    timeout /t 1 >nul
    goto wait_loop
)

echo [RESTORE] EXTRACT ARCHIVE \n FROM -> "%1" \n TO DIR -> "%2"
powershell -NoProfile -Command "Expand-Archive -Path '%1' -DestinationPath '%2' -Force"

echo [START] NOW you can LAUNCH OBS... just do it !
powershell -Command "Add-Type -AssemblyName PresentationFramework;[System.Windows.MessageBox]::Show('SUCCESS','Info',[System.Windows.MessageBoxButton]::OK,[System.Windows.MessageBoxImage]::Information)
)")
								.arg(zipPathWin, destPathWin, exePathWin);

				QString scriptPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
						     "/restore_after_exit.bat";

				QFile scriptFile(scriptPath);
				if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
					obs_log(LOG_ERROR, "[%s] Не удалось создать батник по пути %s",
                        PLUGIN_NAME,
						scriptPath.toStdString().c_str());
					QMessageBox::critical(nullptr, "Error",
							      "Не удалось создать скрипт восстановления.");
					return;
				}
				scriptFile.write(scriptContent.toUtf8());
				scriptFile.close();

				// Запускаем .bat с правами администратора через ShellExecuteExW
				wchar_t batPath[MAX_PATH];
				mbstowcs(batPath, scriptPath.toStdString().c_str(), MAX_PATH);

				SHELLEXECUTEINFOW sei = {sizeof(sei)};
				sei.cbSize = sizeof(sei);
				sei.fMask = SEE_MASK_NOCLOSEPROCESS;
				sei.hwnd = nullptr;
				sei.lpVerb = L"runas"; // Запуск с правами администратора
				sei.lpFile = batPath;
				sei.lpParameters = nullptr;
				sei.lpDirectory = nullptr;
				sei.nShow = SW_SHOWNORMAL;

				if (!ShellExecuteExW(&sei)) {
					DWORD err = GetLastError();
					obs_log(LOG_ERROR,
						"[%s] Не удалось запустить bat с правами администратора. Ошибка: %lu",
                        PLUGIN_NAME,
						err);
					QMessageBox::critical(nullptr, "Error",
							      "Не удалось запустить скрипт с правами администратора.");
					return;
				}

				// Показываем окно с предупреждением, что OBS будет закрыта
				int result = QMessageBox::warning(
					nullptr, obs_module_text("load_title"),
					obs_module_text(
						"obs_will_close_warning"), // Надпись в ресурсах, например "OBS будет закрыта для восстановления из бэкапа. Продолжить?"
					QMessageBox::Ok | QMessageBox::Cancel);

				if (result == QMessageBox::Ok) {
					// Закрываем OBS, батник уже запущен и ждёт закрытия OBS
					QCoreApplication::exit(0);
				} else {
					// Пользователь отменил, батник запущен — можно просто завершить (файл будет удалён системой позже)
					// Либо оставить как есть — батник ждёт закрытия obs64.exe и не выполнится пока OBS не закроют
					obs_log(LOG_INFO,
						"[%s] USER cancelled restore, OBS will not be closed.", 
                        PLUGIN_NAME);
				}
			},
			Qt::QueuedConnection);
	});

	backup_menu->addAction(action_Create);
	backup_menu->addAction(action_Load);

	// Добавляем меню в конец панели
	menu_bar->addMenu(backup_menu);

	obs_log(LOG_INFO, "plugin loaded successfully with BACKUP menu");
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
