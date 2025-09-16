#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <map>

// Include result template
#include "result.h"

// Include PKCS#11 headers
extern "C" {
    #include "cryptoki_ext.h"
    #include "auxiliary.h"
}

namespace PKCS11Lib {

// Data structures
struct TokenInfo {
    std::string label;
    std::string manufacturerId;
    std::string model;
    std::string serialNumber;
    CK_FLAGS flags;
    CK_ULONG maxSessionCount;
    CK_ULONG sessionCount;
    CK_ULONG maxRwSessionCount;
    CK_ULONG rwSessionCount;
    CK_ULONG maxPinLen;
    CK_ULONG minPinLen;
    CK_ULONG totalPublicMemory;
    CK_ULONG freePublicMemory;
    CK_ULONG totalPrivateMemory;
    CK_ULONG freePrivateMemory;
    CK_VERSION hardwareVersion;
    CK_VERSION firmwareVersion;
};

struct SlotInfo {
    std::string slotDescription;
    std::string manufacturerId;
    CK_FLAGS flags;
    CK_VERSION hardwareVersion;
    CK_VERSION firmwareVersion;
};

struct PinInfo {
    CK_BYTE soMaxRetries;
    CK_BYTE soCurCounter;
    CK_BYTE userMaxRetries;
    CK_BYTE userCurCounter;
    CK_FLAGS pinFlags;
};

struct CertificateInfo {
    CK_OBJECT_HANDLE handle;
    std::string label;
    std::vector<CK_BYTE> subject;
    std::vector<CK_BYTE> id;
    std::vector<CK_BYTE> value;
    CK_CERTIFICATE_TYPE type;
};

struct KeyInfo {
    CK_OBJECT_HANDLE handle;
    std::string label;
    CK_KEY_TYPE keyType;
    CK_OBJECT_CLASS objectClass;
    std::vector<CK_BYTE> id;
    bool canEncrypt;
    bool canDecrypt;
    bool canSign;
    bool canVerify;
    bool canWrap;
    bool canUnwrap;
    bool canDerive;
    bool isSensitive;
    bool isExtractable;
};

struct KeyPair {
    KeyInfo publicKey;
    KeyInfo privateKey;
};

// Cryptographic mechanisms
enum class HashAlgorithm {
    SHA1,
    SHA224,
    SHA256,
    SHA384,
    SHA512,
    MD5
};

enum class SymmetricAlgorithm {
    DES,
    DES3,
    RC2,
    RC4,
    AES
};

enum class AsymmetricAlgorithm {
    RSA,
    DSA,
    ECDSA
};

enum class CipherMode {
    ECB,
    CBC,
    CBC_PAD
};

// Main PKCS11 Library class
class PKCS11Library {
public:
    PKCS11Library();
    ~PKCS11Library();

    // Library management
    Result<void> initialize(const std::string& libraryPath = "");
    Result<void> finalize();
    bool isInitialized() const { return initialized_; }

    // Slot and token management
    Result<std::vector<CK_SLOT_ID>> getSlotList(bool tokenPresent = true);
    Result<SlotInfo> getSlotInfo(CK_SLOT_ID slotId);
    Result<TokenInfo> getTokenInfo(CK_SLOT_ID slotId);
    
    // Session management
    Result<void> openSession(CK_SLOT_ID slotId, bool readWrite = true);
    Result<void> closeSession();
    Result<void> login(const std::string& pin, CK_USER_TYPE userType = CKU_USER);
    Result<void> logout();
    bool isLoggedIn() const { return loggedIn_; }

    // PIN management
    Result<PinInfo> getPinInfo();
    Result<void> changePin(const std::string& oldPin, const std::string& newPin);
    Result<void> initPin(const std::string& pin);

    // Token configuration
    Result<void> setTokenLabel(const std::string& label);
    Result<void> setTokenTimeout(CK_ULONG timeoutSeconds);
    Result<CK_ULONG> getTokenTimeout();
    Result<void> blankToken(const std::string& soPin);

    // Object enumeration
    Result<std::vector<CertificateInfo>> findCertificates();
    Result<std::vector<KeyInfo>> findKeys(CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY);
    Result<std::vector<CK_OBJECT_HANDLE>> findDataObjects();

    // Certificate operations
    Result<std::vector<CK_BYTE>> exportCertificate(CK_OBJECT_HANDLE certHandle);
    Result<void> exportCertificateToFile(CK_OBJECT_HANDLE certHandle, const std::string& filename);

    // Key generation
    Result<KeyPair> generateRSAKeyPair(CK_ULONG modulusBits, const std::string& label);
    Result<KeyInfo> generateSymmetricKey(SymmetricAlgorithm algorithm, CK_ULONG keyLength, 
                                        const std::string& label);

    // Cryptographic operations
    Result<std::vector<CK_BYTE>> sign(CK_OBJECT_HANDLE privateKeyHandle, const std::vector<CK_BYTE>& data,
                                     HashAlgorithm hashAlg = HashAlgorithm::SHA1);
    
    Result<void> verify(CK_OBJECT_HANDLE publicKeyHandle, const std::vector<CK_BYTE>& data,
                       const std::vector<CK_BYTE>& signature, HashAlgorithm hashAlg = HashAlgorithm::SHA1);
    
    Result<std::vector<CK_BYTE>> encrypt(CK_OBJECT_HANDLE keyHandle, const std::vector<CK_BYTE>& plaintext,
                                        SymmetricAlgorithm algorithm = SymmetricAlgorithm::DES,
                                        CipherMode mode = CipherMode::CBC, const std::vector<CK_BYTE>& iv = {});
    
    Result<std::vector<CK_BYTE>> decrypt(CK_OBJECT_HANDLE keyHandle, const std::vector<CK_BYTE>& ciphertext,
                                        SymmetricAlgorithm algorithm = SymmetricAlgorithm::DES,
                                        CipherMode mode = CipherMode::CBC, const std::vector<CK_BYTE>& iv = {});

    Result<std::vector<CK_BYTE>> encryptRSA(CK_OBJECT_HANDLE publicKeyHandle, 
                                           const std::vector<CK_BYTE>& plaintext);
    
    Result<std::vector<CK_BYTE>> decryptRSA(CK_OBJECT_HANDLE privateKeyHandle, 
                                           const std::vector<CK_BYTE>& ciphertext);

    // Object management
    Result<void> destroyObject(CK_OBJECT_HANDLE objectHandle);
    Result<std::vector<CK_BYTE>> getObjectAttribute(CK_OBJECT_HANDLE objectHandle, CK_ATTRIBUTE_TYPE attrType);

    // Event handling
    Result<std::pair<CK_SLOT_ID, CK_ULONG>> waitForSlotEvent(bool blocking = true);

    // Utility functions
    std::string getErrorString(CK_RV rv) const;
    static std::string bytesToHex(const std::vector<CK_BYTE>& bytes);
    static std::vector<CK_BYTE> hexToBytes(const std::string& hex);

private:
    // Internal state
    bool initialized_;
    bool sessionOpen_;
    bool loggedIn_;
    void* libraryHandle_;
    CK_FUNCTION_LIST_PTR functionList_;
    AUX_FUNC_LIST_PTR auxFunctionList_;
    CK_SESSION_HANDLE session_;
    CK_SLOT_ID currentSlotId_;

    // Internal helper methods
    Result<void> loadLibrary(const std::string& path);
    Result<void> loadAuxFunctions();
    CK_MECHANISM createMechanism(SymmetricAlgorithm algorithm, CipherMode mode, 
                               const std::vector<CK_BYTE>& iv);
    CK_MECHANISM createHashMechanism(HashAlgorithm hashAlg, AsymmetricAlgorithm asymAlg);
    Status convertPKCS11Error(CK_RV rv);
    std::string trimString(const char* str, size_t maxLen);

    // Template helpers
    template<typename T>
    Result<T> getAttribute(CK_OBJECT_HANDLE handle, CK_ATTRIBUTE_TYPE type);
    
    Result<std::vector<CK_BYTE>> getAttributeBytes(CK_OBJECT_HANDLE handle, CK_ATTRIBUTE_TYPE type);
};

// RAII Session helper
class SessionGuard {
public:
    SessionGuard(PKCS11Library& lib, CK_SLOT_ID slotId, const std::string& pin = "")
        : lib_(lib), success_(false) {
        if (lib_.openSession(slotId).isOk()) {
            if (!pin.empty()) {
                success_ = lib_.login(pin).isOk();
            } else {
                success_ = true;
            }
        }
    }

    ~SessionGuard() {
        if (success_) {
            if (lib_.isLoggedIn()) {
                lib_.logout();
            }
            lib_.closeSession();
        }
    }

    bool isValid() const { return success_; }

private:
    PKCS11Library& lib_;
    bool success_;
};

} // namespace PKCS11Lib