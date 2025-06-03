/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

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
		obs_log(LOG_INFO, "[FU-test-plugin] START_CREATE_BACKUP — obsPluginsPath : %s", obsPluginsPath.toStdString().c_str());

		if (!QDir(obsPluginsPath).exists()) {
			obs_log(LOG_ERROR, "[FU-test-plugin] Backup failed: obs-plugins folder not found at %s",
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
					"[FU-test-plugin] Backup failed: could not copy obs-plugins to temp");
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

			// obs_log(LOG_INFO, "[FU-test-plugin] PowerShell zip STDOUT: %s", stdOut.toStdString().c_str());
			// obs_log(LOG_ERROR, "[FU-test-plugin] PowerShell zip STDERR: %s", stdErr.toStdString().c_str());

			if (zipProcess.exitStatus() == QProcess::NormalExit && zipProcess.exitCode() == 0) {
				obs_log(LOG_INFO, "[FU-test-plugin] STOP_CREATE_BACKUP — Backup created successfully at: %s",
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
				obs_log(LOG_ERROR, "[FU-test-plugin] Backup creation failed");
			}

			// Очистка временной копии
			QDir(tempRootCopyPath).removeRecursively();
		});
	});

	QObject::connect(action_Load, &QAction::triggered,
			 []() { obs_log(LOG_INFO, "Backup -> LOAD Action triggered"); });

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
