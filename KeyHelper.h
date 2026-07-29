#pragma once
#include <QString>

/**
 * KeyHelper — mirrors the PHP App\Helpers\KeyHelper class exactly.
 *
 * Derives KID and CENC_Key from a master_key + library_id pair using
 * HMAC-SHA256.  The shared APP_SALT constant MUST stay in sync with the
 * PHP server and the player app.
 *
 * Algorithm (matches PHP):
 *   KID      = HMAC-SHA256( "KID:{master_key}:{library_id}", APP_SALT ) [first 32 hex chars]
 *   CENC_Key = HMAC-SHA256( "CENC_KEY:{master_key}:{library_id}", APP_SALT ) [first 32 hex chars]
 */
namespace KeyHelper {

    // Shared salt — must match PHP KeyHelper::APP_SALT and the player.
    static constexpr const char* APP_SALT =
        "VIDCRYPT_V1_53A9B2E7C4D8F1A6B9C3D2E5F8A1B4C7";

    /**
     * Derives the 16-byte KID as a lowercase hex string (32 chars).
     * @param masterKey  The raw master key (e.g. "E40C050015175EAD...")
     * @param libraryId  The numeric library ID from the server.
     */
    QString deriveKid(const QString& masterKey, int libraryId);

    /**
     * Derives the 16-byte CENC encryption key as a lowercase hex string (32 chars).
     */
    QString deriveCencKey(const QString& masterKey, int libraryId);

} // namespace KeyHelper
