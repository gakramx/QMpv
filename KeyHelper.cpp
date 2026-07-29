#include "KeyHelper.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

namespace KeyHelper {

// ---------------------------------------------------------------------------
// Internal helper: HKDF-Extract(salt, IKM) -> PRK
// We use APP_SALT as the salt and the masterkey (as string) as the input key material.
// ---------------------------------------------------------------------------
static QByteArray extractPrk(const QString& masterKeyHex)
{
    const QByteArray salt(APP_SALT);
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256, salt);
    // the PHP code hashes the string directly: hash_hmac('sha256', $masterKeyHex, APP_SALT, true)
    mac.addData(masterKeyHex.toUtf8());
    return mac.result();
}

// ---------------------------------------------------------------------------
// Internal helper: HKDF-Expand(PRK, info) -> OKM (16 bytes)
// ---------------------------------------------------------------------------
static QString expand(const QByteArray& prk, const QByteArray& info)
{
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256, prk);
    QByteArray data = info;
    data.append(static_cast<char>(0x01));
    mac.addData(data);
    
    // Take the first 16 bytes of the hash and convert them to a 32-character hex string
    return QString::fromLatin1(mac.result().left(16).toHex());
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

QString deriveKid(const QString& masterKey, int libraryId)
{
    QByteArray prk = extractPrk(masterKey);
    QByteArray info = QString("KID:%1").arg(libraryId).toUtf8();
    return expand(prk, info);
}

QString deriveCencKey(const QString& masterKey, int libraryId)
{
    QByteArray prk = extractPrk(masterKey);
    QByteArray info = QString("CENC:%1").arg(libraryId).toUtf8();
    return expand(prk, info);
}

} // namespace KeyHelper
