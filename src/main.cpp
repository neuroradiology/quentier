/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MainWindow.h"
#include "SystemTrayIconManager.h"
#include "initialization/Initialize.h"
#include "initialization/LoadDependencies.h"
#include <quentier/utility/QuentierApplication.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/exception/DatabaseLockedException.h>
#include <quentier/exception/DatabaseOpeningException.h>
#include <quentier/exception/IQuentierException.h>
#include <QScopedPointer>
#include <iostream>
#include <exception>

using namespace quentier;

int main(int argc, char *argv[])
{
    // Loading the dependencies manually - required on Windows
    loadDependencies();

    ParseCommandLineResult parseCmdResult;
    parseCommandLine(argc, argv, parseCmdResult);
    if (parseCmdResult.m_shouldQuit)
    {
        if (!parseCmdResult.m_errorDescription.isEmpty()) {
            std::cerr << parseCmdResult.m_errorDescription.nonLocalizedString().toLocal8Bit().constData();
            return 1;
        }

        std::cout << parseCmdResult.m_responseMessage.toLocal8Bit().constData();
        return 0;
    }

    QuentierApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("Quentier"));
    app.setQuitOnLastWindowClosed(false);

    int res = initialize(app, parseCmdResult.m_cmdOptions);
    if (res != 0) {
        return res;
    }

    QScopedPointer<MainWindow> pMainWindow;

    try
    {
        pMainWindow.reset(new MainWindow);

        const SystemTrayIconManager & systemTrayIconManager = pMainWindow->systemTrayIconManager();
        if (!systemTrayIconManager.shouldStartMinimizedToSystemTray()) {
            pMainWindow->show();
        }
        else {
            QNDEBUG(QStringLiteral("Not showing the main window on startup because the start "
                                   "minimized to system tray was requested"));
        }
    }
    catch(const quentier::DatabaseLockedException & dbLockedException)
    {
        criticalMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start"),
                           QObject::tr("Database is locked"),
                           QObject::tr("Quentier cannot start because its database is locked. "
                                       "It should only happen if another instance is already running "
                                       "and using the same account. Please either use the already running "
                                       "instance or quit it before opening the new one. If there is no "
                                       "already running instance, please report the problem to the developers "
                                       "of Quentier and try restarting your computer. Sorry for the inconvenience."
                                       "\n\n"
                                       "Exception message: ") + dbLockedException.localizedErrorMessage());
        qWarning() << QStringLiteral("Caught DatabaseLockedException: ") << dbLockedException.nonLocalizedErrorMessage();
        return 1;
    }
    catch(const quentier::DatabaseOpeningException & dbOpeningException)
    {
        criticalMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start"),
                           QObject::tr("Failed to open the local storage database"),
                           QObject::tr("Quentier cannot start because it could not open the database. "
                                       "On Windows it might happen if another instance is already running "
                                       "and using the same account. Otherwise it might indicate the corruption "
                                       "of account database file.\n\nException message: ") + dbOpeningException.localizedErrorMessage());
        qWarning() << QStringLiteral("Caught DatabaseOpeningException: ") << dbOpeningException.nonLocalizedErrorMessage();
        return 1;
    }
    catch(const quentier::IQuentierException & exception)
    {
        internalErrorMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start, exception occurred: ") + exception.localizedErrorMessage());
        qWarning() << QStringLiteral("Caught IQuentierException: ") << exception.nonLocalizedErrorMessage();
        return 1;
    }
    catch(const std::exception & exception)
    {
        internalErrorMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start, exception occurred: ") + QString::fromUtf8(exception.what()));
        qWarning() << QStringLiteral("Caught IQuentierException: ") << exception.what();
        return 1;
    }

    return app.exec();
}
