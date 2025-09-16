#include "pkcs11_lib.h"
#include <dlfcn.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace PKCS11Lib {

PKCS11Library::PKCS11Library() 
    : initialized_(false), sessionOpen_(false), loggedIn_(false),
      libraryHandle_(nullptr), functionList_(nullptr), auxFunctionList_(nullptr),
      session_(0), currentSlotId_(0) {
}

PKCS11Library::~PKCS11Library() {
    finalize();
}

Result<void> PKCS11Library::initialize(const std::string& libraryPath) {
    if (initialized_) {
        return Result<void>::Ok();
    }

    std::string path = libraryPath.empty() ? 
        "libshuttle_p11v220.so.1.0.0" : libraryPath;
        
    auto result = loadLibrary(path);
    if (!result.isOk()) {
        return result;
    }

    CK_RV rv = functionList_->C_Initialize(nullptr);
    if (rv != CKR_OK && rv != CKR_CRYPTOKI_ALREADY_INITIALIZED) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Failed to initialize PKCS#11", rv);
    }

    loadAuxFunctions(); // Best effort, don't fail if aux functions not available

    initialized_ = true;
    return Result<void>::Ok();
}

Result<void> PKCS11Library::finalize() {
    if (!initialized_) {
        return Result<void>::Ok();
    }

    if (sessionOpen_) {
        closeSession();
    }

    if (functionList_) {
        functionList_->C_Finalize(nullptr);
        functionList_ = nullptr;
    }

    if (libraryHandle_) {
        dlclose(libraryHandle_);
        libraryHandle_ = nullptr;
    }

    auxFunctionList_ = nullptr;
    initialized_ = false;
    return Result<void>::Ok();
}

Result<void> PKCS11Library::loadLibrary(const std::string& path) {
    libraryHandle_ = dlopen(path.c_str(), RTLD_NOW);
    if (!libraryHandle_) {
        return Result<void>::Error(Status::ERROR_GENERAL, 
            "Failed to load library: " + std::string(dlerror()));
    }

    typedef CK_RV (*C_GetFunctionListFunc)(CK_FUNCTION_LIST_PTR_PTR);
    auto getFunctionList = (C_GetFunctionListFunc)dlsym(libraryHandle_, "C_GetFunctionList");
    
    if (!getFunctionList) {
        dlclose(libraryHandle_);
        libraryHandle_ = nullptr;
        return Result<void>::Error(Status::ERROR_GENERAL, "Failed to get C_GetFunctionList");
    }

    CK_RV rv = getFunctionList(&functionList_);
    if (rv != CKR_OK || !functionList_) {
        dlclose(libraryHandle_);
        libraryHandle_ = nullptr;
        return Result<void>::Error(Status::ERROR_GENERAL, "Failed to get function list", rv);
    }

    return Result<void>::Ok();
}

Result<void> PKCS11Library::loadAuxFunctions() {
    if (!libraryHandle_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Library not loaded");
    }

    typedef CK_RV (*E_GetAuxFunctionListFunc)(AUX_FUNC_LIST_PTR_PTR);
    auto getAuxFunctionList = (E_GetAuxFunctionListFunc)dlsym(libraryHandle_, "E_GetAuxFunctionList");
    
    if (getAuxFunctionList) {
        getAuxFunctionList(&auxFunctionList_);
    }

    return Result<void>::Ok(); // Non-critical
}

Result<std::vector<CK_SLOT_ID>> PKCS11Library::getSlotList(bool tokenPresent) {
    if (!initialized_) {
        return Result<std::vector<CK_SLOT_ID>>::Error(Status::ERROR_GENERAL, "Library not initialized");
    }

    CK_ULONG count = 0;
    CK_RV rv = functionList_->C_GetSlotList(tokenPresent ? CK_TRUE : CK_FALSE, nullptr, &count);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_SLOT_ID>>::Error(convertPKCS11Error(rv), "Failed to get slot count", rv);
    }

    if (count == 0) {
        return Result<std::vector<CK_SLOT_ID>>::Ok(std::vector<CK_SLOT_ID>());
    }

    std::vector<CK_SLOT_ID> slots(count);
    rv = functionList_->C_GetSlotList(tokenPresent ? CK_TRUE : CK_FALSE, slots.data(), &count);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_SLOT_ID>>::Error(convertPKCS11Error(rv), "Failed to get slot list", rv);
    }

    slots.resize(count);
    return Result<std::vector<CK_SLOT_ID>>::Ok(slots);
}

Result<SlotInfo> PKCS11Library::getSlotInfo(CK_SLOT_ID slotId) {
    if (!initialized_) {
        return Result<SlotInfo>::Error(Status::ERROR_GENERAL, "Library not initialized");
    }

    CK_SLOT_INFO slotInfo;
    CK_RV rv = functionList_->C_GetSlotInfo(slotId, &slotInfo);
    if (rv != CKR_OK) {
        return Result<SlotInfo>::Error(convertPKCS11Error(rv), "Failed to get slot info", rv);
    }

    SlotInfo info;
    info.slotDescription = trimString((char*)slotInfo.slotDescription, 64);
    info.manufacturerId = trimString((char*)slotInfo.manufacturerID, 32);
    info.flags = slotInfo.flags;
    info.hardwareVersion = slotInfo.hardwareVersion;
    info.firmwareVersion = slotInfo.firmwareVersion;

    return Result<SlotInfo>::Ok(info);
}

Result<TokenInfo> PKCS11Library::getTokenInfo(CK_SLOT_ID slotId) {
    if (!initialized_) {
        return Result<TokenInfo>::Error(Status::ERROR_GENERAL, "Library not initialized");
    }

    CK_TOKEN_INFO tokenInfo;
    CK_RV rv = functionList_->C_GetTokenInfo(slotId, &tokenInfo);
    if (rv != CKR_OK) {
        return Result<TokenInfo>::Error(convertPKCS11Error(rv), "Failed to get token info", rv);
    }

    TokenInfo info;
    info.label = trimString((char*)tokenInfo.label, 32);
    info.manufacturerId = trimString((char*)tokenInfo.manufacturerID, 32);
    info.model = trimString((char*)tokenInfo.model, 16);
    info.serialNumber = trimString((char*)tokenInfo.serialNumber, 16);
    info.flags = tokenInfo.flags;
    info.maxSessionCount = tokenInfo.ulMaxSessionCount;
    info.sessionCount = tokenInfo.ulSessionCount;
    info.maxRwSessionCount = tokenInfo.ulMaxRwSessionCount;
    info.rwSessionCount = tokenInfo.ulRwSessionCount;
    info.maxPinLen = tokenInfo.ulMaxPinLen;
    info.minPinLen = tokenInfo.ulMinPinLen;
    info.totalPublicMemory = tokenInfo.ulTotalPublicMemory;
    info.freePublicMemory = tokenInfo.ulFreePublicMemory;
    info.totalPrivateMemory = tokenInfo.ulTotalPrivateMemory;
    info.freePrivateMemory = tokenInfo.ulFreePrivateMemory;
    info.hardwareVersion = tokenInfo.hardwareVersion;
    info.firmwareVersion = tokenInfo.firmwareVersion;

    return Result<TokenInfo>::Ok(info);
}

Result<void> PKCS11Library::openSession(CK_SLOT_ID slotId, bool readWrite) {
    if (!initialized_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Library not initialized");
    }

    if (sessionOpen_) {
        closeSession();
    }

    CK_FLAGS flags = CKF_SERIAL_SESSION;
    if (readWrite) {
        flags |= CKF_RW_SESSION;
    }

    CK_RV rv = functionList_->C_OpenSession(slotId, flags, nullptr, nullptr, &session_);
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to open session", rv);
    }

    sessionOpen_ = true;
    currentSlotId_ = slotId;
    return Result<void>::Ok();
}

Result<void> PKCS11Library::closeSession() {
    if (!sessionOpen_) {
        return Result<void>::Ok();
    }

    if (loggedIn_) {
        logout();
    }

    CK_RV rv = functionList_->C_CloseSession(session_);
    sessionOpen_ = false;
    session_ = 0;
    
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to close session", rv);
    }

    return Result<void>::Ok();
}

Result<void> PKCS11Library::login(const std::string& pin, CK_USER_TYPE userType) {
    if (!sessionOpen_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_RV rv = functionList_->C_Login(session_, userType, 
                                     (CK_UTF8CHAR_PTR)pin.c_str(), pin.length());
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to login", rv);
    }

    loggedIn_ = true;
    return Result<void>::Ok();
}

Result<void> PKCS11Library::logout() {
    if (!loggedIn_) {
        return Result<void>::Ok();
    }

    CK_RV rv = functionList_->C_Logout(session_);
    loggedIn_ = false;
    
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to logout", rv);
    }

    return Result<void>::Ok();
}

Result<PinInfo> PKCS11Library::getPinInfo() {
    if (!sessionOpen_ || !auxFunctionList_) {
        return Result<PinInfo>::Error(Status::ERROR_GENERAL, "Session not open or aux functions not available");
    }

    AUX_PIN_INFO pinInfo;
    auto getPinInfoFunc = (EP_GetPinInfo)auxFunctionList_->pFunc[EP_GET_PIN_INFO];
    if (!getPinInfoFunc) {
        return Result<PinInfo>::Error(Status::ERROR_FUNCTION_FAILED, "GetPinInfo function not available");
    }

    CK_RV rv = getPinInfoFunc(currentSlotId_, &pinInfo);
    if (rv != CKR_OK) {
        return Result<PinInfo>::Error(convertPKCS11Error(rv), "Failed to get PIN info", rv);
    }

    PinInfo info;
    info.soMaxRetries = pinInfo.bSOPinMaxRetries;
    info.soCurCounter = pinInfo.bSOPinCurCounter;
    info.userMaxRetries = pinInfo.bUserPinMaxRetries;
    info.userCurCounter = pinInfo.bUserPinCurCounter;
    info.pinFlags = pinInfo.pinflags;

    return Result<PinInfo>::Ok(info);
}

Result<void> PKCS11Library::setTokenLabel(const std::string& label) {
    if (!sessionOpen_ || !auxFunctionList_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Session not open or aux functions not available");
    }

    auto setLabelFunc = (EP_SetTokenLabel)auxFunctionList_->pFunc[EP_SET_TOKEN_LABEL];
    if (!setLabelFunc) {
        return Result<void>::Error(Status::ERROR_FUNCTION_FAILED, "SetTokenLabel function not available");
    }

    CK_RV rv = setLabelFunc(currentSlotId_, CKU_USER, nullptr, 0, 
                           (CK_UTF8CHAR_PTR)label.c_str());
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to set token label", rv);
    }

    return Result<void>::Ok();
}

Result<void> PKCS11Library::setTokenTimeout(CK_ULONG timeoutSeconds) {
    if (!sessionOpen_ || !auxFunctionList_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Session not open or aux functions not available");
    }

    auto setTimeoutFunc = (EP_SetTokenTimeout)auxFunctionList_->pFunc[EP_SET_TOKEN_TIMEOUT];
    if (!setTimeoutFunc) {
        return Result<void>::Error(Status::ERROR_FUNCTION_FAILED, "SetTokenTimeout function not available");
    }

    CK_RV rv = setTimeoutFunc(currentSlotId_, timeoutSeconds * 1000); // Convert to milliseconds
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to set token timeout", rv);
    }

    return Result<void>::Ok();
}

Result<CK_ULONG> PKCS11Library::getTokenTimeout() {
    if (!sessionOpen_ || !auxFunctionList_) {
        return Result<CK_ULONG>::Error(Status::ERROR_GENERAL, "Session not open or aux functions not available");
    }

    auto getTimeoutFunc = (EP_GetTokenTimeout)auxFunctionList_->pFunc[EP_GET_TOKEN_TIMEOUT];
    if (!getTimeoutFunc) {
        return Result<CK_ULONG>::Error(Status::ERROR_FUNCTION_FAILED, "GetTokenTimeout function not available");
    }

    CK_ULONG timeoutMs;
    CK_RV rv = getTimeoutFunc(currentSlotId_, &timeoutMs);
    if (rv != CKR_OK) {
        return Result<CK_ULONG>::Error(convertPKCS11Error(rv), "Failed to get token timeout", rv);
    }

    return Result<CK_ULONG>::Ok(timeoutMs / 1000); // Convert from milliseconds
}

Result<std::vector<CertificateInfo>> PKCS11Library::findCertificates() {
    if (!sessionOpen_) {
        return Result<std::vector<CertificateInfo>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    std::vector<CertificateInfo> certificates;

    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_BBOOL isToken = CK_TRUE;
    CK_ATTRIBUTE template_[] = {
        {CKA_CLASS, &certClass, sizeof(certClass)},
        {CKA_TOKEN, &isToken, sizeof(isToken)}
    };

    CK_RV rv = functionList_->C_FindObjectsInit(session_, template_, 2);
    if (rv != CKR_OK) {
        return Result<std::vector<CertificateInfo>>::Error(convertPKCS11Error(rv), "Failed to init certificate search", rv);
    }

    CK_OBJECT_HANDLE handle;
    CK_ULONG count;
    
    while (true) {
        rv = functionList_->C_FindObjects(session_, &handle, 1, &count);
        if (rv != CKR_OK || count == 0) {
            break;
        }

        CertificateInfo cert;
        cert.handle = handle;

        // Get certificate attributes
        auto labelBytes = getAttributeBytes(handle, CKA_LABEL);
        if (labelBytes.isOk()) {
            cert.label = std::string(labelBytes.value.begin(), labelBytes.value.end());
        }

        auto subject = getAttributeBytes(handle, CKA_SUBJECT);
        if (subject.isOk()) {
            cert.subject = subject.value;
        }

        auto id = getAttributeBytes(handle, CKA_ID);
        if (id.isOk()) {
            cert.id = id.value;
        }

        auto value = getAttributeBytes(handle, CKA_VALUE);
        if (value.isOk()) {
            cert.value = value.value;
        }

        auto type = getAttribute<CK_CERTIFICATE_TYPE>(handle, CKA_CERTIFICATE_TYPE);
        if (type.isOk()) {
            cert.type = type.value;
        }

        certificates.push_back(cert);
    }

    functionList_->C_FindObjectsFinal(session_);
    return Result<std::vector<CertificateInfo>>::Ok(certificates);
}

Result<std::vector<KeyInfo>> PKCS11Library::findKeys(CK_OBJECT_CLASS keyClass) {
    if (!sessionOpen_) {
        return Result<std::vector<KeyInfo>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    std::vector<KeyInfo> keys;

    CK_BBOOL isToken = CK_TRUE;
    CK_ATTRIBUTE template_[] = {
        {CKA_CLASS, &keyClass, sizeof(keyClass)},
        {CKA_TOKEN, &isToken, sizeof(isToken)}
    };

    CK_RV rv = functionList_->C_FindObjectsInit(session_, template_, 2);
    if (rv != CKR_OK) {
        return Result<std::vector<KeyInfo>>::Error(convertPKCS11Error(rv), "Failed to init key search", rv);
    }

    CK_OBJECT_HANDLE handle;
    CK_ULONG count;
    
    while (true) {
        rv = functionList_->C_FindObjects(session_, &handle, 1, &count);
        if (rv != CKR_OK || count == 0) {
            break;
        }

        KeyInfo key;
        key.handle = handle;
        key.objectClass = keyClass;

        // Get key attributes
        auto labelBytes = getAttributeBytes(handle, CKA_LABEL);
        if (labelBytes.isOk()) {
            key.label = std::string(labelBytes.value.begin(), labelBytes.value.end());
        }

        auto keyType = getAttribute<CK_KEY_TYPE>(handle, CKA_KEY_TYPE);
        if (keyType.isOk()) {
            key.keyType = keyType.value;
        }

        auto id = getAttributeBytes(handle, CKA_ID);
        if (id.isOk()) {
            key.id = id.value;
        }

        // Get capability flags
        auto flag = getAttribute<CK_BBOOL>(handle, CKA_ENCRYPT);
        key.canEncrypt = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_DECRYPT);
        key.canDecrypt = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_SIGN);
        key.canSign = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_VERIFY);
        key.canVerify = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_WRAP);
        key.canWrap = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_UNWRAP);
        key.canUnwrap = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_DERIVE);
        key.canDerive = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_SENSITIVE);
        key.isSensitive = flag.isOk() && flag.value;
        
        flag = getAttribute<CK_BBOOL>(handle, CKA_EXTRACTABLE);
        key.isExtractable = flag.isOk() && flag.value;

        keys.push_back(key);
    }

    functionList_->C_FindObjectsFinal(session_);
    return Result<std::vector<KeyInfo>>::Ok(keys);
}

Result<KeyPair> PKCS11Library::generateRSAKeyPair(CK_ULONG modulusBits, const std::string& label) {
    if (!sessionOpen_) {
        return Result<KeyPair>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = {CKM_RSA_PKCS_KEY_PAIR_GEN, nullptr, 0};
    CK_BBOOL bTrue = CK_TRUE;
    CK_ULONG keyType = CKK_RSA;

    // Public key template
    CK_OBJECT_CLASS pubClass = CKO_PUBLIC_KEY;
    CK_ATTRIBUTE pubTemplate[] = {
        {CKA_CLASS, &pubClass, sizeof(pubClass)},
        {CKA_KEY_TYPE, &keyType, sizeof(keyType)},
        {CKA_LABEL, (void*)label.c_str(), label.length()},
        {CKA_MODULUS_BITS, &modulusBits, sizeof(modulusBits)},
        {CKA_ENCRYPT, &bTrue, sizeof(bTrue)},
        {CKA_VERIFY, &bTrue, sizeof(bTrue)},
        {CKA_WRAP, &bTrue, sizeof(bTrue)},
        {CKA_TOKEN, &bTrue, sizeof(bTrue)}
    };

    // Private key template
    CK_OBJECT_CLASS priClass = CKO_PRIVATE_KEY;
    CK_ATTRIBUTE priTemplate[] = {
        {CKA_CLASS, &priClass, sizeof(priClass)},
        {CKA_KEY_TYPE, &keyType, sizeof(keyType)},
        {CKA_LABEL, (void*)label.c_str(), label.length()},
        {CKA_DECRYPT, &bTrue, sizeof(bTrue)},
        {CKA_SIGN, &bTrue, sizeof(bTrue)},
        {CKA_UNWRAP, &bTrue, sizeof(bTrue)},
        {CKA_PRIVATE, &bTrue, sizeof(bTrue)},
        {CKA_SENSITIVE, &bTrue, sizeof(bTrue)},
        {CKA_TOKEN, &bTrue, sizeof(bTrue)},
        {CKA_EXTRACTABLE, &bTrue, sizeof(bTrue)}
    };

    CK_OBJECT_HANDLE pubKey, priKey;
    CK_RV rv = functionList_->C_GenerateKeyPair(session_, &mechanism,
                                               pubTemplate, sizeof(pubTemplate)/sizeof(CK_ATTRIBUTE),
                                               priTemplate, sizeof(priTemplate)/sizeof(CK_ATTRIBUTE),
                                               &pubKey, &priKey);
    if (rv != CKR_OK) {
        return Result<KeyPair>::Error(convertPKCS11Error(rv), "Failed to generate RSA key pair", rv);
    }

    // Fill key pair info
    KeyPair keyPair;
    keyPair.publicKey.handle = pubKey;
    keyPair.publicKey.label = label;
    keyPair.publicKey.keyType = CKK_RSA;
    keyPair.publicKey.objectClass = CKO_PUBLIC_KEY;
    keyPair.publicKey.canEncrypt = true;
    keyPair.publicKey.canVerify = true;
    keyPair.publicKey.canWrap = true;

    keyPair.privateKey.handle = priKey;
    keyPair.privateKey.label = label;
    keyPair.privateKey.keyType = CKK_RSA;
    keyPair.privateKey.objectClass = CKO_PRIVATE_KEY;
    keyPair.privateKey.canDecrypt = true;
    keyPair.privateKey.canSign = true;
    keyPair.privateKey.canUnwrap = true;
    keyPair.privateKey.isSensitive = true;
    keyPair.privateKey.isExtractable = true;

    return Result<KeyPair>::Ok(keyPair);
}

Result<KeyInfo> PKCS11Library::generateSymmetricKey(SymmetricAlgorithm algorithm, CK_ULONG keyLength, 
                                                   const std::string& label) {
    if (!sessionOpen_) {
        return Result<KeyInfo>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism;
    CK_KEY_TYPE keyType;
    
    switch (algorithm) {
        case SymmetricAlgorithm::DES:
            mechanism = {CKM_DES_KEY_GEN, nullptr, 0};
            keyType = CKK_DES;
            keyLength = 8; // DES is always 8 bytes
            break;
        case SymmetricAlgorithm::DES3:
            mechanism = {CKM_DES3_KEY_GEN, nullptr, 0};
            keyType = CKK_DES3;
            keyLength = 24; // 3DES is always 24 bytes
            break;
        case SymmetricAlgorithm::RC2:
            mechanism = {CKM_RC2_KEY_GEN, nullptr, 0};
            keyType = CKK_RC2;
            break;
        case SymmetricAlgorithm::RC4:
            mechanism = {CKM_RC4_KEY_GEN, nullptr, 0};
            keyType = CKK_RC4;
            break;
        case SymmetricAlgorithm::AES:
            mechanism = {CKM_AES_KEY_GEN, nullptr, 0};
            keyType = CKK_AES;
            break;
        default:
            return Result<KeyInfo>::Error(Status::ERROR_INVALID_PARAMETER, "Unsupported algorithm");
    }

    CK_BBOOL bTrue = CK_TRUE;
    CK_BBOOL bFalse = CK_FALSE;
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    
    CK_ATTRIBUTE keyTemplate[] = {
        {CKA_CLASS, &keyClass, sizeof(keyClass)},
        {CKA_KEY_TYPE, &keyType, sizeof(keyType)},
        {CKA_LABEL, (void*)label.c_str(), label.length()},
        {CKA_TOKEN, &bFalse, sizeof(bFalse)},
        {CKA_PRIVATE, &bTrue, sizeof(bTrue)},
        {CKA_ENCRYPT, &bTrue, sizeof(bTrue)},
        {CKA_DECRYPT, &bTrue, sizeof(bTrue)},
        {CKA_VALUE_LEN, &keyLength, sizeof(keyLength)}
    };

    CK_OBJECT_HANDLE keyHandle;
    CK_RV rv = functionList_->C_GenerateKey(session_, &mechanism, keyTemplate, 
                                           sizeof(keyTemplate)/sizeof(CK_ATTRIBUTE), &keyHandle);
    if (rv != CKR_OK) {
        return Result<KeyInfo>::Error(convertPKCS11Error(rv), "Failed to generate symmetric key", rv);
    }

    KeyInfo keyInfo;
    keyInfo.handle = keyHandle;
    keyInfo.label = label;
    keyInfo.keyType = keyType;
    keyInfo.objectClass = CKO_SECRET_KEY;
    keyInfo.canEncrypt = true;
    keyInfo.canDecrypt = true;

    return Result<KeyInfo>::Ok(keyInfo);
}

Result<std::vector<CK_BYTE>> PKCS11Library::sign(CK_OBJECT_HANDLE privateKeyHandle, const std::vector<CK_BYTE>& data,
                                                 HashAlgorithm hashAlg) {
    if (!sessionOpen_) {
        return Result<std::vector<CK_BYTE>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = createHashMechanism(hashAlg, AsymmetricAlgorithm::RSA);
    
    CK_RV rv = functionList_->C_SignInit(session_, &mechanism, privateKeyHandle);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to initialize signing", rv);
    }

    CK_ULONG signatureLen = 0;
    rv = functionList_->C_Sign(session_, (CK_BYTE_PTR)data.data(), data.size(), nullptr, &signatureLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get signature length", rv);
    }

    std::vector<CK_BYTE> signature(signatureLen);
    rv = functionList_->C_Sign(session_, (CK_BYTE_PTR)data.data(), data.size(), 
                              signature.data(), &signatureLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to sign data", rv);
    }

    signature.resize(signatureLen);
    return Result<std::vector<CK_BYTE>>::Ok(signature);
}

Result<void> PKCS11Library::verify(CK_OBJECT_HANDLE publicKeyHandle, const std::vector<CK_BYTE>& data,
                                  const std::vector<CK_BYTE>& signature, HashAlgorithm hashAlg) {
    if (!sessionOpen_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = createHashMechanism(hashAlg, AsymmetricAlgorithm::RSA);
    
    CK_RV rv = functionList_->C_VerifyInit(session_, &mechanism, publicKeyHandle);
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to initialize verification", rv);
    }

    rv = functionList_->C_Verify(session_, (CK_BYTE_PTR)data.data(), data.size(),
                                (CK_BYTE_PTR)signature.data(), signature.size());
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Verification failed", rv);
    }

    return Result<void>::Ok();
}

Result<std::vector<CK_BYTE>> PKCS11Library::encrypt(CK_OBJECT_HANDLE keyHandle, const std::vector<CK_BYTE>& plaintext,
                                                    SymmetricAlgorithm algorithm, CipherMode mode, 
                                                    const std::vector<CK_BYTE>& iv) {
    if (!sessionOpen_) {
        return Result<std::vector<CK_BYTE>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = createMechanism(algorithm, mode, iv);
    
    CK_RV rv = functionList_->C_EncryptInit(session_, &mechanism, keyHandle);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to initialize encryption", rv);
    }

    CK_ULONG ciphertextLen = 0;
    rv = functionList_->C_Encrypt(session_, (CK_BYTE_PTR)plaintext.data(), plaintext.size(), 
                                 nullptr, &ciphertextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get ciphertext length", rv);
    }

    std::vector<CK_BYTE> ciphertext(ciphertextLen);
    rv = functionList_->C_Encrypt(session_, (CK_BYTE_PTR)plaintext.data(), plaintext.size(),
                                 ciphertext.data(), &ciphertextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to encrypt data", rv);
    }

    ciphertext.resize(ciphertextLen);
    return Result<std::vector<CK_BYTE>>::Ok(ciphertext);
}

Result<std::vector<CK_BYTE>> PKCS11Library::decrypt(CK_OBJECT_HANDLE keyHandle, const std::vector<CK_BYTE>& ciphertext,
                                                    SymmetricAlgorithm algorithm, CipherMode mode, 
                                                    const std::vector<CK_BYTE>& iv) {
    if (!sessionOpen_) {
        return Result<std::vector<CK_BYTE>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = createMechanism(algorithm, mode, iv);
    
    CK_RV rv = functionList_->C_DecryptInit(session_, &mechanism, keyHandle);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to initialize decryption", rv);
    }

    CK_ULONG plaintextLen = 0;
    rv = functionList_->C_Decrypt(session_, (CK_BYTE_PTR)ciphertext.data(), ciphertext.size(),
                                 nullptr, &plaintextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get plaintext length", rv);
    }

    std::vector<CK_BYTE> plaintext(plaintextLen);
    rv = functionList_->C_Decrypt(session_, (CK_BYTE_PTR)ciphertext.data(), ciphertext.size(),
                                 plaintext.data(), &plaintextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to decrypt data", rv);
    }

    plaintext.resize(plaintextLen);
    return Result<std::vector<CK_BYTE>>::Ok(plaintext);
}

Result<std::vector<CK_BYTE>> PKCS11Library::encryptRSA(CK_OBJECT_HANDLE publicKeyHandle, 
                                                       const std::vector<CK_BYTE>& plaintext) {
    if (!sessionOpen_) {
        return Result<std::vector<CK_BYTE>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = {CKM_RSA_PKCS, nullptr, 0};
    
    CK_RV rv = functionList_->C_EncryptInit(session_, &mechanism, publicKeyHandle);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to initialize RSA encryption", rv);
    }

    CK_ULONG ciphertextLen = 0;
    rv = functionList_->C_Encrypt(session_, (CK_BYTE_PTR)plaintext.data(), plaintext.size(),
                                 nullptr, &ciphertextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get RSA ciphertext length", rv);
    }

    std::vector<CK_BYTE> ciphertext(ciphertextLen);
    rv = functionList_->C_Encrypt(session_, (CK_BYTE_PTR)plaintext.data(), plaintext.size(),
                                 ciphertext.data(), &ciphertextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to encrypt with RSA", rv);
    }

    ciphertext.resize(ciphertextLen);
    return Result<std::vector<CK_BYTE>>::Ok(ciphertext);
}

Result<std::vector<CK_BYTE>> PKCS11Library::decryptRSA(CK_OBJECT_HANDLE privateKeyHandle, 
                                                       const std::vector<CK_BYTE>& ciphertext) {
    if (!sessionOpen_) {
        return Result<std::vector<CK_BYTE>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_MECHANISM mechanism = {CKM_RSA_PKCS, nullptr, 0};
    
    CK_RV rv = functionList_->C_DecryptInit(session_, &mechanism, privateKeyHandle);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to initialize RSA decryption", rv);
    }

    CK_ULONG plaintextLen = 0;
    rv = functionList_->C_Decrypt(session_, (CK_BYTE_PTR)ciphertext.data(), ciphertext.size(),
                                 nullptr, &plaintextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get RSA plaintext length", rv);
    }

    std::vector<CK_BYTE> plaintext(plaintextLen);
    rv = functionList_->C_Decrypt(session_, (CK_BYTE_PTR)ciphertext.data(), ciphertext.size(),
                                 plaintext.data(), &plaintextLen);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to decrypt with RSA", rv);
    }

    plaintext.resize(plaintextLen);
    return Result<std::vector<CK_BYTE>>::Ok(plaintext);
}

Result<std::vector<CK_BYTE>> PKCS11Library::exportCertificate(CK_OBJECT_HANDLE certHandle) {
    return getAttributeBytes(certHandle, CKA_VALUE);
}

Result<void> PKCS11Library::exportCertificateToFile(CK_OBJECT_HANDLE certHandle, const std::string& filename) {
    auto certData = exportCertificate(certHandle);
    if (!certData.isOk()) {
        return Result<void>::Error(certData.errorCode, certData.errorMessage, certData.pkcs11Error);
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Failed to open file for writing");
    }

    file.write(reinterpret_cast<const char*>(certData.value.data()), certData.value.size());
    if (!file) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Failed to write certificate data");
    }

    return Result<void>::Ok();
}

Result<void> PKCS11Library::destroyObject(CK_OBJECT_HANDLE objectHandle) {
    if (!sessionOpen_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_RV rv = functionList_->C_DestroyObject(session_, objectHandle);
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to destroy object", rv);
    }

    return Result<void>::Ok();
}

Result<std::pair<CK_SLOT_ID, CK_ULONG>> PKCS11Library::waitForSlotEvent(bool blocking) {
    if (!sessionOpen_ || !auxFunctionList_) {
        return Result<std::pair<CK_SLOT_ID, CK_ULONG>>::Error(Status::ERROR_GENERAL, 
            "Session not open or aux functions not available");
    }

    auto waitFunc = (EP_WaitForSlotEvent)auxFunctionList_->pFunc[EP_WAITFORSLOTEVENT];
    if (!waitFunc) {
        return Result<std::pair<CK_SLOT_ID, CK_ULONG>>::Error(Status::ERROR_FUNCTION_FAILED, 
            "WaitForSlotEvent function not available");
    }

    CK_FLAGS flags = blocking ? 0 : CKF_DONT_BLOCK;
    CK_SLOT_ID slotId;
    CK_ULONG event;
    CK_ULONG extData;
    
    CK_RV rv = waitFunc(flags, &slotId, &event, &extData, nullptr);
    if (rv != CKR_OK) {
        return Result<std::pair<CK_SLOT_ID, CK_ULONG>>::Error(convertPKCS11Error(rv), 
            "Failed to wait for slot event", rv);
    }

    return Result<std::pair<CK_SLOT_ID, CK_ULONG>>::Ok(std::make_pair(slotId, event));
}

Result<std::vector<CK_OBJECT_HANDLE>> PKCS11Library::findDataObjects() {
    if (!sessionOpen_) {
        return Result<std::vector<CK_OBJECT_HANDLE>>::Error(Status::ERROR_GENERAL, "No session open");
    }

    std::vector<CK_OBJECT_HANDLE> objects;

    CK_OBJECT_CLASS dataClass = CKO_DATA;
    CK_BBOOL isToken = CK_TRUE;
    CK_ATTRIBUTE template_[] = {
        {CKA_CLASS, &dataClass, sizeof(dataClass)},
        {CKA_TOKEN, &isToken, sizeof(isToken)}
    };

    CK_RV rv = functionList_->C_FindObjectsInit(session_, template_, 2);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_OBJECT_HANDLE>>::Error(convertPKCS11Error(rv), 
            "Failed to init data object search", rv);
    }

    CK_OBJECT_HANDLE handle;
    CK_ULONG count;
    
    while (true) {
        rv = functionList_->C_FindObjects(session_, &handle, 1, &count);
        if (rv != CKR_OK || count == 0) {
            break;
        }
        objects.push_back(handle);
    }

    functionList_->C_FindObjectsFinal(session_);
    return Result<std::vector<CK_OBJECT_HANDLE>>::Ok(objects);
}

Result<std::vector<CK_BYTE>> PKCS11Library::getObjectAttribute(CK_OBJECT_HANDLE objectHandle, 
                                                               CK_ATTRIBUTE_TYPE attrType) {
    return getAttributeBytes(objectHandle, attrType);
}

Result<void> PKCS11Library::changePin(const std::string& oldPin, const std::string& newPin) {
    if (!sessionOpen_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_RV rv = functionList_->C_SetPIN(session_, 
                                      (CK_UTF8CHAR_PTR)oldPin.c_str(), oldPin.length(),
                                      (CK_UTF8CHAR_PTR)newPin.c_str(), newPin.length());
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to change PIN", rv);
    }

    return Result<void>::Ok();
}

Result<void> PKCS11Library::initPin(const std::string& pin) {
    if (!sessionOpen_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "No session open");
    }

    CK_RV rv = functionList_->C_InitPIN(session_, 
                                       (CK_UTF8CHAR_PTR)pin.c_str(), pin.length());
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to initialize PIN", rv);
    }

    return Result<void>::Ok();
}

Result<void> PKCS11Library::blankToken(const std::string& soPin) {
    if (!sessionOpen_ || !auxFunctionList_) {
        return Result<void>::Error(Status::ERROR_GENERAL, "Session not open or aux functions not available");
    }

    auto blankFunc = (EP_BlankToken)auxFunctionList_->pFunc[EP_BLANK_TOKEN];
    if (!blankFunc) {
        return Result<void>::Error(Status::ERROR_FUNCTION_FAILED, "BlankToken function not available");
    }

    CK_RV rv = blankFunc(currentSlotId_, (CK_UTF8CHAR_PTR)soPin.c_str(), soPin.length());
    if (rv != CKR_OK) {
        return Result<void>::Error(convertPKCS11Error(rv), "Failed to blank token", rv);
    }

    return Result<void>::Ok();
}

// Helper methods implementation

CK_MECHANISM PKCS11Library::createMechanism(SymmetricAlgorithm algorithm, CipherMode mode, 
                                           const std::vector<CK_BYTE>& iv) {
    CK_MECHANISM mechanism;
    
    switch (algorithm) {
        case SymmetricAlgorithm::DES:
            switch (mode) {
                case CipherMode::ECB: mechanism.mechanism = CKM_DES_ECB; break;
                case CipherMode::CBC: mechanism.mechanism = CKM_DES_CBC; break;
                case CipherMode::CBC_PAD: mechanism.mechanism = CKM_DES_CBC_PAD; break;
            }
            break;
        case SymmetricAlgorithm::DES3:
            switch (mode) {
                case CipherMode::ECB: mechanism.mechanism = CKM_DES3_ECB; break;
                case CipherMode::CBC: mechanism.mechanism = CKM_DES3_CBC; break;
                case CipherMode::CBC_PAD: mechanism.mechanism = CKM_DES3_CBC_PAD; break;
            }
            break;
        case SymmetricAlgorithm::RC2:
            switch (mode) {
                case CipherMode::ECB: mechanism.mechanism = CKM_RC2_ECB; break;
                case CipherMode::CBC: mechanism.mechanism = CKM_RC2_CBC; break;
                case CipherMode::CBC_PAD: mechanism.mechanism = CKM_RC2_CBC_PAD; break;
            }
            break;
        case SymmetricAlgorithm::RC4:
            mechanism.mechanism = CKM_RC4;
            break;
        case SymmetricAlgorithm::AES:
            switch (mode) {
                case CipherMode::ECB: mechanism.mechanism = CKM_AES_ECB; break;
                case CipherMode::CBC: mechanism.mechanism = CKM_AES_CBC; break;
                case CipherMode::CBC_PAD: mechanism.mechanism = CKM_AES_CBC_PAD; break;
            }
            break;
    }

    if (mode == CipherMode::CBC || mode == CipherMode::CBC_PAD) {
        mechanism.pParameter = (void*)iv.data();
        mechanism.ulParameterLen = iv.size();
    } else {
        mechanism.pParameter = nullptr;
        mechanism.ulParameterLen = 0;
    }

    return mechanism;
}

CK_MECHANISM PKCS11Library::createHashMechanism(HashAlgorithm hashAlg, AsymmetricAlgorithm asymAlg) {
    CK_MECHANISM mechanism;
    
    if (asymAlg == AsymmetricAlgorithm::RSA) {
        switch (hashAlg) {
            case HashAlgorithm::SHA1: mechanism.mechanism = CKM_SHA1_RSA_PKCS; break;
            case HashAlgorithm::SHA224: mechanism.mechanism = CKM_SHA224_RSA_PKCS; break;
            case HashAlgorithm::SHA256: mechanism.mechanism = CKM_SHA256_RSA_PKCS; break;
            case HashAlgorithm::SHA384: mechanism.mechanism = CKM_SHA384_RSA_PKCS; break;
            case HashAlgorithm::SHA512: mechanism.mechanism = CKM_SHA512_RSA_PKCS; break;
            case HashAlgorithm::MD5: mechanism.mechanism = CKM_MD5_RSA_PKCS; break;
        }
    }
    
    mechanism.pParameter = nullptr;
    mechanism.ulParameterLen = 0;
    return mechanism;
}

Status PKCS11Library::convertPKCS11Error(CK_RV rv) {
    switch (rv) {
        case CKR_OK: return Status::OK;
        case CKR_TOKEN_NOT_PRESENT: return Status::ERROR_TOKEN_NOT_PRESENT;
        case CKR_PIN_INCORRECT: 
        case CKR_PIN_INVALID: return Status::ERROR_PIN_INVALID;
        case CKR_PIN_LOCKED: return Status::ERROR_PIN_LOCKED;
        case CKR_HOST_MEMORY: return Status::ERROR_MEMORY;
        case CKR_FUNCTION_FAILED: return Status::ERROR_FUNCTION_FAILED;
        case CKR_OBJECT_HANDLE_INVALID: return Status::ERROR_OBJECT_NOT_FOUND;
        case CKR_ARGUMENTS_BAD: return Status::ERROR_INVALID_PARAMETER;
        default: return Status::ERROR_GENERAL;
    }
}

std::string PKCS11Library::trimString(const char* str, size_t maxLen) {
    std::string result(str, maxLen);
    result.erase(result.find_last_not_of(" \0") + 1);
    return result;
}

template<typename T>
Result<T> PKCS11Library::getAttribute(CK_OBJECT_HANDLE handle, CK_ATTRIBUTE_TYPE type) {
    T value;
    CK_ATTRIBUTE attr = {type, &value, sizeof(T)};
    CK_RV rv = functionList_->C_GetAttributeValue(session_, handle, &attr, 1);
    if (rv != CKR_OK) {
        return Result<T>::Error(convertPKCS11Error(rv), "Failed to get attribute", rv);
    }
    return Result<T>::Ok(value);
}

Result<std::vector<CK_BYTE>> PKCS11Library::getAttributeBytes(CK_OBJECT_HANDLE handle, CK_ATTRIBUTE_TYPE type) {
    CK_ATTRIBUTE attr = {type, nullptr, 0};
    CK_RV rv = functionList_->C_GetAttributeValue(session_, handle, &attr, 1);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get attribute length", rv);
    }

    if (attr.ulValueLen == 0) {
        return Result<std::vector<CK_BYTE>>::Ok(std::vector<CK_BYTE>());
    }

    std::vector<CK_BYTE> value(attr.ulValueLen);
    attr.pValue = value.data();
    
    rv = functionList_->C_GetAttributeValue(session_, handle, &attr, 1);
    if (rv != CKR_OK) {
        return Result<std::vector<CK_BYTE>>::Error(convertPKCS11Error(rv), "Failed to get attribute value", rv);
    }

    return Result<std::vector<CK_BYTE>>::Ok(value);
}

std::string PKCS11Library::getErrorString(CK_RV rv) const {
    switch (rv) {
        case CKR_OK: return "OK";
        case CKR_CANCEL: return "Cancel";
        case CKR_HOST_MEMORY: return "Host memory";
        case CKR_SLOT_ID_INVALID: return "Slot ID invalid";
        case CKR_GENERAL_ERROR: return "General error";
        case CKR_FUNCTION_FAILED: return "Function failed";
        case CKR_ARGUMENTS_BAD: return "Arguments bad";
        case CKR_NO_EVENT: return "No event";
        case CKR_NEED_TO_CREATE_THREADS: return "Need to create threads";
        case CKR_CANT_LOCK: return "Can't lock";
        case CKR_ATTRIBUTE_READ_ONLY: return "Attribute read only";
        case CKR_ATTRIBUTE_SENSITIVE: return "Attribute sensitive";
        case CKR_ATTRIBUTE_TYPE_INVALID: return "Attribute type invalid";
        case CKR_ATTRIBUTE_VALUE_INVALID: return "Attribute value invalid";
        case CKR_DATA_INVALID: return "Data invalid";
        case CKR_DATA_LEN_RANGE: return "Data len range";
        case CKR_DEVICE_ERROR: return "Device error";
        case CKR_DEVICE_MEMORY: return "Device memory";
        case CKR_DEVICE_REMOVED: return "Device removed";
        case CKR_ENCRYPTED_DATA_INVALID: return "Encrypted data invalid";
        case CKR_ENCRYPTED_DATA_LEN_RANGE: return "Encrypted data len range";
        case CKR_FUNCTION_CANCELED: return "Function canceled";
        case CKR_FUNCTION_NOT_PARALLEL: return "Function not parallel";
        case CKR_FUNCTION_NOT_SUPPORTED: return "Function not supported";
        case CKR_KEY_HANDLE_INVALID: return "Key handle invalid";
        case CKR_KEY_SIZE_RANGE: return "Key size range";
        case CKR_KEY_TYPE_INCONSISTENT: return "Key type inconsistent";
        case CKR_KEY_NOT_NEEDED: return "Key not needed";
        case CKR_KEY_CHANGED: return "Key changed";
        case CKR_KEY_NEEDED: return "Key needed";
        case CKR_KEY_INDIGESTIBLE: return "Key indigestible";
        case CKR_KEY_FUNCTION_NOT_PERMITTED: return "Key function not permitted";
        case CKR_KEY_NOT_WRAPPABLE: return "Key not wrappable";
        case CKR_KEY_UNEXTRACTABLE: return "Key unextractable";
        case CKR_MECHANISM_INVALID: return "Mechanism invalid";
        case CKR_MECHANISM_PARAM_INVALID: return "Mechanism param invalid";
        case CKR_OBJECT_HANDLE_INVALID: return "Object handle invalid";
        case CKR_OPERATION_ACTIVE: return "Operation active";
        case CKR_OPERATION_NOT_INITIALIZED: return "Operation not initialized";
        case CKR_PIN_INCORRECT: return "PIN incorrect";
        case CKR_PIN_INVALID: return "PIN invalid";
        case CKR_PIN_LEN_RANGE: return "PIN len range";
        case CKR_PIN_EXPIRED: return "PIN expired";
        case CKR_PIN_LOCKED: return "PIN locked";
        case CKR_SESSION_CLOSED: return "Session closed";
        case CKR_SESSION_COUNT: return "Session count";
        case CKR_SESSION_HANDLE_INVALID: return "Session handle invalid";
        case CKR_SESSION_PARALLEL_NOT_SUPPORTED: return "Session parallel not supported";
        case CKR_SESSION_READ_ONLY: return "Session read only";
        case CKR_SESSION_EXISTS: return "Session exists";
        case CKR_SESSION_READ_ONLY_EXISTS: return "Session read only exists";
        case CKR_SESSION_READ_WRITE_SO_EXISTS: return "Session read write SO exists";
        case CKR_SIGNATURE_INVALID: return "Signature invalid";
        case CKR_SIGNATURE_LEN_RANGE: return "Signature len range";
        case CKR_TEMPLATE_INCOMPLETE: return "Template incomplete";
        case CKR_TEMPLATE_INCONSISTENT: return "Template inconsistent";
        case CKR_TOKEN_NOT_PRESENT: return "Token not present";
        case CKR_TOKEN_NOT_RECOGNIZED: return "Token not recognized";
        case CKR_TOKEN_WRITE_PROTECTED: return "Token write protected";
        case CKR_UNWRAPPING_KEY_HANDLE_INVALID: return "Unwrapping key handle invalid";
        case CKR_UNWRAPPING_KEY_SIZE_RANGE: return "Unwrapping key size range";
        case CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT: return "Unwrapping key type inconsistent";
        case CKR_USER_ALREADY_LOGGED_IN: return "User already logged in";
        case CKR_USER_NOT_LOGGED_IN: return "User not logged in";
        case CKR_USER_PIN_NOT_INITIALIZED: return "User PIN not initialized";
        case CKR_USER_TYPE_INVALID: return "User type invalid";
        case CKR_USER_ANOTHER_ALREADY_LOGGED_IN: return "User another already logged in";
        case CKR_USER_TOO_MANY_TYPES: return "User too many types";
        case CKR_WRAPPED_KEY_INVALID: return "Wrapped key invalid";
        case CKR_WRAPPED_KEY_LEN_RANGE: return "Wrapped key len range";
        case CKR_WRAPPING_KEY_HANDLE_INVALID: return "Wrapping key handle invalid";
        case CKR_WRAPPING_KEY_SIZE_RANGE: return "Wrapping key size range";
        case CKR_WRAPPING_KEY_TYPE_INCONSISTENT: return "Wrapping key type inconsistent";
        case CKR_RANDOM_SEED_NOT_SUPPORTED: return "Random seed not supported";
        case CKR_RANDOM_NO_RNG: return "Random no RNG";
        case CKR_DOMAIN_PARAMS_INVALID: return "Domain params invalid";
        case CKR_BUFFER_TOO_SMALL: return "Buffer too small";
        case CKR_SAVED_STATE_INVALID: return "Saved state invalid";
        case CKR_INFORMATION_SENSITIVE: return "Information sensitive";
        case CKR_STATE_UNSAVEABLE: return "State unsaveable";
        case CKR_CRYPTOKI_NOT_INITIALIZED: return "Cryptoki not initialized";
        case CKR_CRYPTOKI_ALREADY_INITIALIZED: return "Cryptoki already initialized";
        case CKR_MUTEX_BAD: return "Mutex bad";
        case CKR_MUTEX_NOT_LOCKED: return "Mutex not locked";
        case CKR_FUNCTION_REJECTED: return "Function rejected";
        case CKR_VENDOR_DEFINED: return "Vendor defined";
        default: return "Unknown error: " + std::to_string(rv);
    }
}

std::string PKCS11Library::bytesToHex(const std::vector<CK_BYTE>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : bytes) {
        oss << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return oss.str();
}

std::vector<CK_BYTE> PKCS11Library::hexToBytes(const std::string& hex) {
    std::vector<CK_BYTE> bytes;
    if (hex.length() % 2 != 0) {
        return bytes; // Invalid hex string
    }

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        CK_BYTE byte = static_cast<CK_BYTE>(std::stoul(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

} // namespace PKCS11Lib