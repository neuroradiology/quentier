#include "EncryptionManager_p.h"
#include <logging/QuteNoteLogger.h>
#include <tools/DesktopServices.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <stdlib.h>

// Evernote service defined constants
#define NUM_ITERATIONS (50000)
#define NUM_SALT_BYTES (16)

namespace qute_note {

EncryptionManagerPrivate::EncryptionManagerPrivate() :
    m_salt(),
    m_saltmac(),
    m_iv()
{
    for(int i = 0; i < NUM_SALT_BYTES; ++i) {
        m_salt[i] = '0';
        m_saltmac[i] = '0';
        m_iv[i] = '0';
    }

    for(int i = 0; i < 64; ++i) {
        m_pbkdf2_key[i] = '0';
    }

    for(int i = 0; i < 129; ++i) {
        m_hash[i] = '0';
    }

    for(int i = 0; i < 32; ++i) {
        m_pkcs5_key[i] = '0';
    }


}

bool EncryptionManagerPrivate::decrypt(const QString & encryptedText, const QString & passphrase,
                                       const QString & cipher, const size_t keyLength,
                                       QString & decryptedText, QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(encryptedText);
    Q_UNUSED(passphrase);
    Q_UNUSED(cipher);
    Q_UNUSED(keyLength);
    Q_UNUSED(decryptedText);
    Q_UNUSED(errorDescription);
    return false;
}

bool EncryptionManagerPrivate::encrypt(const QString & textToEncrypt, const QString & passphrase,
                                       QString & cipher, size_t & keyLength,
                                       QString & encryptedText, QString & errorDescription)
{
    encryptedText.resize(0);
    encryptedText += "ENCO";

    ErrorStringsHolder errorStringsHolder;
    Q_UNUSED(errorStringsHolder)

#define GET_OPENSSL_ERROR \
    unsigned long errorCode = ERR_get_error(); \
    const char * errorLib = ERR_lib_error_string(errorCode); \
    const char * errorFunc = ERR_func_error_string(errorCode); \
    const char * errorReason = ERR_reason_error_string(errorCode)

    bool res = generateRandomBytes(SaltKind::SALT, errorDescription);
    if (!res) {
        return false;
    }

    res = generateRandomBytes(SaltKind::SALTMAC, errorDescription);
    if (!res) {
        return false;
    }

    res = generateRandomBytes(SaltKind::IV, errorDescription);
    if (!res) {
        return false;
    }

    appendUnsignedCharToQString(encryptedText, m_salt, NUM_SALT_BYTES);
    appendUnsignedCharToQString(encryptedText, m_saltmac, NUM_SALT_BYTES);
    appendUnsignedCharToQString(encryptedText, m_iv, NUM_SALT_BYTES);

    QString encryptionKey;
    res = generateKey(passphrase, m_salt, NUM_ITERATIONS, encryptionKey, errorDescription);
    if (!res) {
        return false;
    }

    res = encyptWithAes(textToEncrypt, passphrase, encryptedText, errorDescription);
    if (!res) {
        return false;
    }

    QString hmac;
    res = calculateHmacHash(passphrase, m_saltmac, textToEncrypt, NUM_ITERATIONS, hmac, errorDescription);
    if (!res) {
        return false;
    }

    encryptedText += hmac;

    QByteArray encryptedTextData = encryptedText.toLocal8Bit();
    encryptedText = encryptedTextData.toBase64();

    cipher = "AES";
    keyLength = 128;

    return true;
}

bool EncryptionManagerPrivate::generateRandomBytes(const EncryptionManagerPrivate::SaltKind::type saltKind,
                                                   QString & errorDescription)
{
    unsigned char * salt = nullptr;
    const char * saltText = nullptr;

    switch (saltKind)
    {
    case SaltKind::SALT:
        salt = m_salt;
        saltText = "salt";
        break;
    case SaltKind::SALTMAC:
        salt = m_saltmac;
        saltText = "saltmac";
        break;
    case SaltKind::IV:
        salt = m_iv;
        saltText = "iv";
        break;
    default:
        errorDescription = QT_TR_NOOP("Internal error, detected incorrect salt kind for cryptographic key generation");
        QNCRITICAL(errorDescription);
        return false;
    }

    int res = RAND_bytes(salt, NUM_SALT_BYTES);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographically strong bytes for encryption");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << "; " << saltText << ": lib: " << errorLib
                  << "; func: " << errorFunc << ", reason: " << errorReason);
        return false;
    }

    return true;
}

bool EncryptionManagerPrivate::generateKey(const QString & passphrase, const unsigned char * salt,
                                           const size_t numIterations, QString & key,
                                           QString & errorDescription)
{
    ErrorStringsHolder errorStringsHolder;
    Q_UNUSED(errorStringsHolder)

    const char * rawPassphrase = passphrase.toLocal8Bit().constData();
    int res = PKCS5_PBKDF2_HMAC(rawPassphrase, strlen(rawPassphrase),
                                salt, strlen(reinterpret_cast<const char*>(salt)),
                                numIterations, EVP_sha256(), 32, m_pkcs5_key);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographic key");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl PKCS5_PBKDF2_HMAC failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    for(int i = 0; i < 32; ++i) {
        Q_UNUSED(sprintf(&m_pbkdf2_key[i*2], "%02x", m_pkcs5_key[i]));
    }

    key = QString(m_pbkdf2_key);
    return true;
}

bool EncryptionManagerPrivate::calculateHmacHash(const QString & passphrase, const unsigned char * salt,
                                                 const QString & textToEncrypt, const size_t numIterations,
                                                 QString & hash, QString & errorDescription)
{
    QString key;
    bool res = generateKey(passphrase, salt, numIterations, key, errorDescription);
    if (!res) {
        return false;
    }

    ErrorStringsHolder errorStringsHolder;
    Q_UNUSED(errorStringsHolder)

    QByteArray keyData = key.toLocal8Bit();
    const char * rawKey = keyData.constData();

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();
    const unsigned char * data = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());

    unsigned char * digest = HMAC(EVP_sha256(), rawKey, strlen(rawKey), data,
                                  strlen(reinterpret_cast<const char*>(data)),
                                  NULL, NULL);

    for(int i = 0; i < 64; ++i) {
        Q_UNUSED(sprintf(&m_hash[i*2], "%02x", digest[i]));
    }

    hash = QString(m_hash);
    return true;
}

bool EncryptionManagerPrivate::encyptWithAes(const QString & textToEncrypt,
                                             const QString & key,
                                             QString & encryptedText,
                                             QString & errorDescription)
{
    ErrorStringsHolder errorStringsHolder;
    Q_UNUSED(errorStringsHolder)

    QByteArray keyData = key.toLocal8Bit();
    const unsigned char * rawKey = reinterpret_cast<const unsigned char*>(keyData.constData());

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();
    const unsigned char * rawTextToEncrypt = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());
    size_t rawTextToEncryptSize = strlen(textToEncryptData.constData());

    unsigned char * buffer = reinterpret_cast<unsigned char*>(malloc(rawTextToEncryptSize * 2));
    int out_len;

    EVP_CIPHER_CTX context;
    int res = EVP_CipherInit(&context, EVP_aes_128_cbc(), rawKey, m_iv, /* should encrypt = */ 1);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    res = EVP_CipherUpdate(&context, buffer, &out_len, rawTextToEncrypt, rawTextToEncryptSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    res = EVP_CipherFinal(&context, buffer, &out_len);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    appendUnsignedCharToQString(encryptedText, buffer, out_len);
    return true;
}

EncryptionManagerPrivate::ErrorStringsHolder::ErrorStringsHolder()
{
    SSL_load_error_strings();
}

EncryptionManagerPrivate::ErrorStringsHolder::~ErrorStringsHolder()
{
    ERR_free_strings();
}

} // namespace qute_note
