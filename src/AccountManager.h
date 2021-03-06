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

#ifndef QUENTIER_ACCOUNT_MANAGER_H
#define QUENTIER_ACCOUNT_MANAGER_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/exception/IQuentierException.h>
#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QDir>
#include <QNetworkProxy>

namespace quentier {

class AccountManager: public QObject
{
    Q_OBJECT
public:
    class AccountInitializationException: public IQuentierException
    {
    public:
        explicit AccountInitializationException(const ErrorString & message);

    protected:
        virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
    };

public:
    AccountManager(QObject * parent = Q_NULLPTR);

    const QVector<Account> & availableAccounts() const
    { return m_availableAccounts; }

    /**
     * Attempts to retrieve the last used account from the app settings, in case of failure creates and returns
     * the default local account
     *
     * @param pCreatedDefaultAccount - optional pointer to bool parameter which, if not null,
     * is used to report whether no existing account was found so the default account was created
     */
    Account currentAccount(bool * pCreatedDefaultAccount = Q_NULLPTR);

    void raiseAddAccountDialog();
    void raiseManageAccountsDialog();

Q_SIGNALS:
    void evernoteAccountAuthenticationRequested(QString host, QNetworkProxy proxy);
    void switchedAccount(Account account);
    void accountUpdated(Account account);
    void notifyError(ErrorString error);
    void accountAdded(Account account);

public Q_SLOTS:
    void switchAccount(const Account & account);

private Q_SLOTS:
    void onLocalAccountAdditionRequested(QString name, QString fullName);
    void onAccountDisplayNameChanged(Account account);

private:
    void detectAvailableAccounts();

    QSharedPointer<Account> createDefaultAccount(ErrorString & errorDescription, bool * pCreatedDefaultAccount);
    QSharedPointer<Account> createLocalAccount(const QString & name, const QString & displayName,
                                               ErrorString & errorDescription);
    bool createAccountInfo(const Account & account);

    bool writeAccountInfo(const QString & name, const QString & displayName, const bool isLocal,
                          const qevercloud::UserID id, const QString & evernoteAccountType,
                          const QString & evernoteHost, const QString & shardId,
                          ErrorString & errorDescription);

    QString evernoteAccountTypeToString(const Account::EvernoteAccountType::type type) const;

    void readComplementaryAccountInfo(Account & account);

    // Tries to find the account corresponding to the specified environment variables
    // specifying the details of the account; in case of success returns
    // non-null pointer to the account, null otherwise
    QSharedPointer<Account> accountFromEnvVarHints();

    // Tries to restore the last used account from the app settings;
    // in case of success returns non-null pointer to account, null otherwise
    QSharedPointer<Account> lastUsedAccount();

    QSharedPointer<Account> findAccount(const bool isLocal, const QString & accountName,
                                        const qevercloud::UserID id, const Account::EvernoteAccountType::type type,
                                        const QString & evernoteHost);

    void updateLastUsedAccount(const Account & account);

private:
    QVector<Account>   m_availableAccounts;
};

} // namespace quentier

#endif // QUENTIER_ACCOUNT_MANAGER_H
