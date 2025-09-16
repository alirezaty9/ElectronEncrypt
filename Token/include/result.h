#ifndef RESULT_H
#define RESULT_H

#include <string>

namespace PKCS11Lib {

enum class Status {
    // Success
    OK = 0,
    
    // General errors
    ERROR_GENERAL = 1,
    ERROR_CANCEL = 2,
    ERROR_HOST_MEMORY = 3,
    ERROR_SLOT_ID_INVALID = 4,
    ERROR_FUNCTION_FAILED = 5,
    ERROR_ARGUMENTS_BAD = 6,
    ERROR_NO_EVENT = 7,
    ERROR_NEED_TO_CREATE_THREADS = 8,
    ERROR_CANT_LOCK = 9,
    
    // Attribute errors
    ERROR_ATTRIBUTE_READ_ONLY = 10,
    ERROR_ATTRIBUTE_SENSITIVE = 11,
    ERROR_ATTRIBUTE_TYPE_INVALID = 12,
    ERROR_ATTRIBUTE_VALUE_INVALID = 13,
    
    // Data errors
    ERROR_DATA_INVALID = 20,
    ERROR_DATA_LEN_RANGE = 21,
    
    // Device errors
    ERROR_DEVICE_ERROR = 30,
    ERROR_DEVICE_MEMORY = 31,
    ERROR_DEVICE_REMOVED = 32,
    
    // Encryption errors
    ERROR_ENCRYPTED_DATA_INVALID = 40,
    ERROR_ENCRYPTED_DATA_LEN_RANGE = 41,
    
    // Function errors
    ERROR_FUNCTION_CANCELED = 50,
    ERROR_FUNCTION_NOT_PARALLEL = 51,
    ERROR_FUNCTION_NOT_SUPPORTED = 52,
    ERROR_FUNCTION_REJECTED = 53,
    
    // Key errors
    ERROR_KEY_HANDLE_INVALID = 60,
    ERROR_KEY_SIZE_RANGE = 61,
    ERROR_KEY_TYPE_INCONSISTENT = 62,
    ERROR_KEY_NOT_NEEDED = 63,
    ERROR_KEY_CHANGED = 64,
    ERROR_KEY_NEEDED = 65,
    ERROR_KEY_INDIGESTIBLE = 66,
    ERROR_KEY_FUNCTION_NOT_PERMITTED = 67,
    ERROR_KEY_NOT_WRAPPABLE = 68,
    ERROR_KEY_UNEXTRACTABLE = 69,
    
    // Mechanism errors
    ERROR_MECHANISM_INVALID = 70,
    ERROR_MECHANISM_PARAM_INVALID = 71,
    
    // Object errors
    ERROR_OBJECT_HANDLE_INVALID = 80,
    ERROR_OBJECT_NOT_FOUND = 81,
    
    // Operation errors
    ERROR_OPERATION_ACTIVE = 90,
    ERROR_OPERATION_NOT_INITIALIZED = 91,
    
    // PIN errors
    ERROR_PIN_INCORRECT = 100,
    ERROR_PIN_INVALID = 101,
    ERROR_PIN_LEN_RANGE = 102,
    ERROR_PIN_EXPIRED = 103,
    ERROR_PIN_LOCKED = 104,
    
    // Session errors
    ERROR_SESSION_CLOSED = 110,
    ERROR_SESSION_COUNT = 111,
    ERROR_SESSION_HANDLE_INVALID = 112,
    ERROR_SESSION_PARALLEL_NOT_SUPPORTED = 113,
    ERROR_SESSION_READ_ONLY = 114,
    ERROR_SESSION_EXISTS = 115,
    ERROR_SESSION_READ_ONLY_EXISTS = 116,
    ERROR_SESSION_READ_WRITE_SO_EXISTS = 117,
    
    // Signature errors
    ERROR_SIGNATURE_INVALID = 120,
    ERROR_SIGNATURE_LEN_RANGE = 121,
    
    // Template errors
    ERROR_TEMPLATE_INCOMPLETE = 130,
    ERROR_TEMPLATE_INCONSISTENT = 131,
    
    // Token errors
    ERROR_TOKEN_NOT_PRESENT = 140,
    ERROR_TOKEN_NOT_RECOGNIZED = 141,
    ERROR_TOKEN_WRITE_PROTECTED = 142,
    
    // Wrapping errors
    ERROR_UNWRAPPING_KEY_HANDLE_INVALID = 150,
    ERROR_UNWRAPPING_KEY_SIZE_RANGE = 151,
    ERROR_UNWRAPPING_KEY_TYPE_INCONSISTENT = 152,
    ERROR_WRAPPED_KEY_INVALID = 153,
    ERROR_WRAPPED_KEY_LEN_RANGE = 154,
    ERROR_WRAPPING_KEY_HANDLE_INVALID = 155,
    ERROR_WRAPPING_KEY_SIZE_RANGE = 156,
    ERROR_WRAPPING_KEY_TYPE_INCONSISTENT = 157,
    
    // User errors
    ERROR_USER_ALREADY_LOGGED_IN = 160,
    ERROR_USER_NOT_LOGGED_IN = 161,
    ERROR_USER_PIN_NOT_INITIALIZED = 162,
    ERROR_USER_TYPE_INVALID = 163,
    ERROR_USER_ANOTHER_ALREADY_LOGGED_IN = 164,
    ERROR_USER_TOO_MANY_TYPES = 165,
    
    // Random number errors
    ERROR_RANDOM_SEED_NOT_SUPPORTED = 170,
    ERROR_RANDOM_NO_RNG = 171,
    
    // Domain parameter errors
    ERROR_DOMAIN_PARAMS_INVALID = 180,
    
    // Buffer errors
    ERROR_BUFFER_TOO_SMALL = 190,
    
    // State errors
    ERROR_SAVED_STATE_INVALID = 200,
    ERROR_INFORMATION_SENSITIVE = 201,
    ERROR_STATE_UNSAVEABLE = 202,
    
    // Cryptoki initialization errors
    ERROR_CRYPTOKI_NOT_INITIALIZED = 210,
    ERROR_CRYPTOKI_ALREADY_INITIALIZED = 211,
    
    // Mutex errors
    ERROR_MUTEX_BAD = 220,
    ERROR_MUTEX_NOT_LOCKED = 221,
    
    // New PIN mode errors (PKCS #11 v2.20 amendment 3)
    ERROR_NEW_PIN_MODE = 230,
    ERROR_NEXT_OTP = 231,
    
    // Vendor defined errors
    ERROR_VENDOR_DEFINED = 240,
    
    // Library specific errors
    ERROR_LIBRARY_LOAD_FAILED = 250,
    ERROR_FUNCTION_LIST_NOT_AVAILABLE = 251,
    ERROR_AUX_FUNCTION_NOT_AVAILABLE = 252,
    ERROR_INVALID_PARAMETER = 253,
    ERROR_MEMORY = 254,
    ERROR_FILE_IO = 255,
    ERROR_UNSUPPORTED_ALGORITHM = 256,
    ERROR_UNSUPPORTED_OPERATION = 257
};

template<typename T>
class Result {
public:
    bool success;
    T value;
    Status errorCode;
    std::string errorMessage;
    unsigned long pkcs11Error;

    Result(bool success, const T& value, Status errorCode, 
           const std::string& errorMessage = "", unsigned long pkcs11Error = 0)
        : success(success), value(value), errorCode(errorCode), 
          errorMessage(errorMessage), pkcs11Error(pkcs11Error) {}

    static Result Ok(const T& value) {
        return Result(true, value, Status::OK);
    }

    static Result Error(Status errorCode, const std::string& errorMessage = "", 
                       unsigned long pkcs11Error = 0) {
        return Result(false, T{}, errorCode, errorMessage, pkcs11Error);
    }

    bool isOk() const { return success; }
    bool isError() const { return !success; }
    
    // Helper methods for common error types
    bool isTokenNotPresent() const { 
        return errorCode == Status::ERROR_TOKEN_NOT_PRESENT; 
    }
    
    bool isPinError() const {
        return errorCode == Status::ERROR_PIN_INCORRECT || 
               errorCode == Status::ERROR_PIN_INVALID || 
               errorCode == Status::ERROR_PIN_LOCKED ||
               errorCode == Status::ERROR_PIN_EXPIRED;
    }
    
    bool isSessionError() const {
        return errorCode == Status::ERROR_SESSION_CLOSED ||
               errorCode == Status::ERROR_SESSION_HANDLE_INVALID ||
               errorCode == Status::ERROR_SESSION_READ_ONLY;
    }
    
    bool isKeyError() const {
        return errorCode == Status::ERROR_KEY_HANDLE_INVALID ||
               errorCode == Status::ERROR_KEY_SIZE_RANGE ||
               errorCode == Status::ERROR_KEY_TYPE_INCONSISTENT;
    }
    
    bool isObjectError() const {
        return errorCode == Status::ERROR_OBJECT_HANDLE_INVALID ||
               errorCode == Status::ERROR_OBJECT_NOT_FOUND;
    }
    
    // Get human-readable error description
    std::string getErrorDescription() const {
        switch (errorCode) {
            case Status::OK: return "Success";
            case Status::ERROR_GENERAL: return "General error";
            case Status::ERROR_CANCEL: return "Operation was cancelled";
            case Status::ERROR_HOST_MEMORY: return "Host memory allocation error";
            case Status::ERROR_SLOT_ID_INVALID: return "Invalid slot ID";
            case Status::ERROR_FUNCTION_FAILED: return "Function failed";
            case Status::ERROR_ARGUMENTS_BAD: return "Invalid arguments";
            case Status::ERROR_NO_EVENT: return "No event available";
            case Status::ERROR_NEED_TO_CREATE_THREADS: return "Need to create threads";
            case Status::ERROR_CANT_LOCK: return "Cannot lock";
            case Status::ERROR_ATTRIBUTE_READ_ONLY: return "Attribute is read-only";
            case Status::ERROR_ATTRIBUTE_SENSITIVE: return "Attribute is sensitive";
            case Status::ERROR_ATTRIBUTE_TYPE_INVALID: return "Invalid attribute type";
            case Status::ERROR_ATTRIBUTE_VALUE_INVALID: return "Invalid attribute value";
            case Status::ERROR_DATA_INVALID: return "Invalid data";
            case Status::ERROR_DATA_LEN_RANGE: return "Data length out of range";
            case Status::ERROR_DEVICE_ERROR: return "Device error";
            case Status::ERROR_DEVICE_MEMORY: return "Device memory error";
            case Status::ERROR_DEVICE_REMOVED: return "Device removed";
            case Status::ERROR_ENCRYPTED_DATA_INVALID: return "Invalid encrypted data";
            case Status::ERROR_ENCRYPTED_DATA_LEN_RANGE: return "Encrypted data length out of range";
            case Status::ERROR_FUNCTION_CANCELED: return "Function was canceled";
            case Status::ERROR_FUNCTION_NOT_PARALLEL: return "Function not parallel";
            case Status::ERROR_FUNCTION_NOT_SUPPORTED: return "Function not supported";
            case Status::ERROR_FUNCTION_REJECTED: return "Function rejected";
            case Status::ERROR_KEY_HANDLE_INVALID: return "Invalid key handle";
            case Status::ERROR_KEY_SIZE_RANGE: return "Key size out of range";
            case Status::ERROR_KEY_TYPE_INCONSISTENT: return "Inconsistent key type";
            case Status::ERROR_KEY_NOT_NEEDED: return "Key not needed";
            case Status::ERROR_KEY_CHANGED: return "Key changed";
            case Status::ERROR_KEY_NEEDED: return "Key needed";
            case Status::ERROR_KEY_INDIGESTIBLE: return "Key indigestible";
            case Status::ERROR_KEY_FUNCTION_NOT_PERMITTED: return "Key function not permitted";
            case Status::ERROR_KEY_NOT_WRAPPABLE: return "Key not wrappable";
            case Status::ERROR_KEY_UNEXTRACTABLE: return "Key unextractable";
            case Status::ERROR_MECHANISM_INVALID: return "Invalid mechanism";
            case Status::ERROR_MECHANISM_PARAM_INVALID: return "Invalid mechanism parameter";
            case Status::ERROR_OBJECT_HANDLE_INVALID: return "Invalid object handle";
            case Status::ERROR_OBJECT_NOT_FOUND: return "Object not found";
            case Status::ERROR_OPERATION_ACTIVE: return "Operation active";
            case Status::ERROR_OPERATION_NOT_INITIALIZED: return "Operation not initialized";
            case Status::ERROR_PIN_INCORRECT: return "Incorrect PIN";
            case Status::ERROR_PIN_INVALID: return "Invalid PIN";
            case Status::ERROR_PIN_LEN_RANGE: return "PIN length out of range";
            case Status::ERROR_PIN_EXPIRED: return "PIN expired";
            case Status::ERROR_PIN_LOCKED: return "PIN locked";
            case Status::ERROR_SESSION_CLOSED: return "Session closed";
            case Status::ERROR_SESSION_COUNT: return "Session count exceeded";
            case Status::ERROR_SESSION_HANDLE_INVALID: return "Invalid session handle";
            case Status::ERROR_SESSION_PARALLEL_NOT_SUPPORTED: return "Parallel sessions not supported";
            case Status::ERROR_SESSION_READ_ONLY: return "Session is read-only";
            case Status::ERROR_SESSION_EXISTS: return "Session exists";
            case Status::ERROR_SESSION_READ_ONLY_EXISTS: return "Read-only session exists";
            case Status::ERROR_SESSION_READ_WRITE_SO_EXISTS: return "Read/write SO session exists";
            case Status::ERROR_SIGNATURE_INVALID: return "Invalid signature";
            case Status::ERROR_SIGNATURE_LEN_RANGE: return "Signature length out of range";
            case Status::ERROR_TEMPLATE_INCOMPLETE: return "Template incomplete";
            case Status::ERROR_TEMPLATE_INCONSISTENT: return "Template inconsistent";
            case Status::ERROR_TOKEN_NOT_PRESENT: return "Token not present";
            case Status::ERROR_TOKEN_NOT_RECOGNIZED: return "Token not recognized";
            case Status::ERROR_TOKEN_WRITE_PROTECTED: return "Token is write-protected";
            case Status::ERROR_UNWRAPPING_KEY_HANDLE_INVALID: return "Invalid unwrapping key handle";
            case Status::ERROR_UNWRAPPING_KEY_SIZE_RANGE: return "Unwrapping key size out of range";
            case Status::ERROR_UNWRAPPING_KEY_TYPE_INCONSISTENT: return "Inconsistent unwrapping key type";
            case Status::ERROR_WRAPPED_KEY_INVALID: return "Invalid wrapped key";
            case Status::ERROR_WRAPPED_KEY_LEN_RANGE: return "Wrapped key length out of range";
            case Status::ERROR_WRAPPING_KEY_HANDLE_INVALID: return "Invalid wrapping key handle";
            case Status::ERROR_WRAPPING_KEY_SIZE_RANGE: return "Wrapping key size out of range";
            case Status::ERROR_WRAPPING_KEY_TYPE_INCONSISTENT: return "Inconsistent wrapping key type";
            case Status::ERROR_USER_ALREADY_LOGGED_IN: return "User already logged in";
            case Status::ERROR_USER_NOT_LOGGED_IN: return "User not logged in";
            case Status::ERROR_USER_PIN_NOT_INITIALIZED: return "User PIN not initialized";
            case Status::ERROR_USER_TYPE_INVALID: return "Invalid user type";
            case Status::ERROR_USER_ANOTHER_ALREADY_LOGGED_IN: return "Another user already logged in";
            case Status::ERROR_USER_TOO_MANY_TYPES: return "Too many user types";
            case Status::ERROR_RANDOM_SEED_NOT_SUPPORTED: return "Random seed not supported";
            case Status::ERROR_RANDOM_NO_RNG: return "No random number generator";
            case Status::ERROR_DOMAIN_PARAMS_INVALID: return "Invalid domain parameters";
            case Status::ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
            case Status::ERROR_SAVED_STATE_INVALID: return "Invalid saved state";
            case Status::ERROR_INFORMATION_SENSITIVE: return "Information sensitive";
            case Status::ERROR_STATE_UNSAVEABLE: return "State unsaveable";
            case Status::ERROR_CRYPTOKI_NOT_INITIALIZED: return "Cryptoki not initialized";
            case Status::ERROR_CRYPTOKI_ALREADY_INITIALIZED: return "Cryptoki already initialized";
            case Status::ERROR_MUTEX_BAD: return "Bad mutex";
            case Status::ERROR_MUTEX_NOT_LOCKED: return "Mutex not locked";
            case Status::ERROR_NEW_PIN_MODE: return "New PIN mode";
            case Status::ERROR_NEXT_OTP: return "Next OTP";
            case Status::ERROR_VENDOR_DEFINED: return "Vendor defined error";
            case Status::ERROR_LIBRARY_LOAD_FAILED: return "Failed to load library";
            case Status::ERROR_FUNCTION_LIST_NOT_AVAILABLE: return "Function list not available";
            case Status::ERROR_AUX_FUNCTION_NOT_AVAILABLE: return "Auxiliary function not available";
            case Status::ERROR_INVALID_PARAMETER: return "Invalid parameter";
            case Status::ERROR_MEMORY: return "Memory allocation error";
            case Status::ERROR_FILE_IO: return "File I/O error";
            case Status::ERROR_UNSUPPORTED_ALGORITHM: return "Unsupported algorithm";
            case Status::ERROR_UNSUPPORTED_OPERATION: return "Unsupported operation";
            default: return "Unknown error";
        }
    }
};

// Specialization for void
template<>
class Result<void> {
public:
    bool success;
    Status errorCode;
    std::string errorMessage;
    unsigned long pkcs11Error;

    Result(bool success, Status errorCode, 
           const std::string& errorMessage = "", unsigned long pkcs11Error = 0)
        : success(success), errorCode(errorCode), 
          errorMessage(errorMessage), pkcs11Error(pkcs11Error) {}

    static Result Ok() {
        return Result(true, Status::OK);
    }

    static Result Error(Status errorCode, const std::string& errorMessage = "", 
                       unsigned long pkcs11Error = 0) {
        return Result(false, errorCode, errorMessage, pkcs11Error);
    }

    bool isOk() const { return success; }
    bool isError() const { return !success; }
    
    // Helper methods for common error types
    bool isTokenNotPresent() const { 
        return errorCode == Status::ERROR_TOKEN_NOT_PRESENT; 
    }
    
    bool isPinError() const {
        return errorCode == Status::ERROR_PIN_INCORRECT || 
               errorCode == Status::ERROR_PIN_INVALID || 
               errorCode == Status::ERROR_PIN_LOCKED ||
               errorCode == Status::ERROR_PIN_EXPIRED;
    }
    
    bool isSessionError() const {
        return errorCode == Status::ERROR_SESSION_CLOSED ||
               errorCode == Status::ERROR_SESSION_HANDLE_INVALID ||
               errorCode == Status::ERROR_SESSION_READ_ONLY;
    }
    
    bool isKeyError() const {
        return errorCode == Status::ERROR_KEY_HANDLE_INVALID ||
               errorCode == Status::ERROR_KEY_SIZE_RANGE ||
               errorCode == Status::ERROR_KEY_TYPE_INCONSISTENT;
    }
    
    bool isObjectError() const {
        return errorCode == Status::ERROR_OBJECT_HANDLE_INVALID ||
               errorCode == Status::ERROR_OBJECT_NOT_FOUND;
    }
    
    // Get human-readable error description
    std::string getErrorDescription() const {
        switch (errorCode) {
            case Status::OK: return "Success";
            case Status::ERROR_GENERAL: return "General error";
            case Status::ERROR_CANCEL: return "Operation was cancelled";
            case Status::ERROR_HOST_MEMORY: return "Host memory allocation error";
            case Status::ERROR_SLOT_ID_INVALID: return "Invalid slot ID";
            case Status::ERROR_FUNCTION_FAILED: return "Function failed";
            case Status::ERROR_ARGUMENTS_BAD: return "Invalid arguments";
            case Status::ERROR_NO_EVENT: return "No event available";
            case Status::ERROR_NEED_TO_CREATE_THREADS: return "Need to create threads";
            case Status::ERROR_CANT_LOCK: return "Cannot lock";
            case Status::ERROR_ATTRIBUTE_READ_ONLY: return "Attribute is read-only";
            case Status::ERROR_ATTRIBUTE_SENSITIVE: return "Attribute is sensitive";
            case Status::ERROR_ATTRIBUTE_TYPE_INVALID: return "Invalid attribute type";
            case Status::ERROR_ATTRIBUTE_VALUE_INVALID: return "Invalid attribute value";
            case Status::ERROR_DATA_INVALID: return "Invalid data";
            case Status::ERROR_DATA_LEN_RANGE: return "Data length out of range";
            case Status::ERROR_DEVICE_ERROR: return "Device error";
            case Status::ERROR_DEVICE_MEMORY: return "Device memory error";
            case Status::ERROR_DEVICE_REMOVED: return "Device removed";
            case Status::ERROR_ENCRYPTED_DATA_INVALID: return "Invalid encrypted data";
            case Status::ERROR_ENCRYPTED_DATA_LEN_RANGE: return "Encrypted data length out of range";
            case Status::ERROR_FUNCTION_CANCELED: return "Function was canceled";
            case Status::ERROR_FUNCTION_NOT_PARALLEL: return "Function not parallel";
            case Status::ERROR_FUNCTION_NOT_SUPPORTED: return "Function not supported";
            case Status::ERROR_FUNCTION_REJECTED: return "Function rejected";
            case Status::ERROR_KEY_HANDLE_INVALID: return "Invalid key handle";
            case Status::ERROR_KEY_SIZE_RANGE: return "Key size out of range";
            case Status::ERROR_KEY_TYPE_INCONSISTENT: return "Inconsistent key type";
            case Status::ERROR_KEY_NOT_NEEDED: return "Key not needed";
            case Status::ERROR_KEY_CHANGED: return "Key changed";
            case Status::ERROR_KEY_NEEDED: return "Key needed";
            case Status::ERROR_KEY_INDIGESTIBLE: return "Key indigestible";
            case Status::ERROR_KEY_FUNCTION_NOT_PERMITTED: return "Key function not permitted";
            case Status::ERROR_KEY_NOT_WRAPPABLE: return "Key not wrappable";
            case Status::ERROR_KEY_UNEXTRACTABLE: return "Key unextractable";
            case Status::ERROR_MECHANISM_INVALID: return "Invalid mechanism";
            case Status::ERROR_MECHANISM_PARAM_INVALID: return "Invalid mechanism parameter";
            case Status::ERROR_OBJECT_HANDLE_INVALID: return "Invalid object handle";
            case Status::ERROR_OBJECT_NOT_FOUND: return "Object not found";
            case Status::ERROR_OPERATION_ACTIVE: return "Operation active";
            case Status::ERROR_OPERATION_NOT_INITIALIZED: return "Operation not initialized";
            case Status::ERROR_PIN_INCORRECT: return "Incorrect PIN";
            case Status::ERROR_PIN_INVALID: return "Invalid PIN";
            case Status::ERROR_PIN_LEN_RANGE: return "PIN length out of range";
            case Status::ERROR_PIN_EXPIRED: return "PIN expired";
            case Status::ERROR_PIN_LOCKED: return "PIN locked";
            case Status::ERROR_SESSION_CLOSED: return "Session closed";
            case Status::ERROR_SESSION_COUNT: return "Session count exceeded";
            case Status::ERROR_SESSION_HANDLE_INVALID: return "Invalid session handle";
            case Status::ERROR_SESSION_PARALLEL_NOT_SUPPORTED: return "Parallel sessions not supported";
            case Status::ERROR_SESSION_READ_ONLY: return "Session is read-only";
            case Status::ERROR_SESSION_EXISTS: return "Session exists";
            case Status::ERROR_SESSION_READ_ONLY_EXISTS: return "Read-only session exists";
            case Status::ERROR_SESSION_READ_WRITE_SO_EXISTS: return "Read/write SO session exists";
            case Status::ERROR_SIGNATURE_INVALID: return "Invalid signature";
            case Status::ERROR_SIGNATURE_LEN_RANGE: return "Signature length out of range";
            case Status::ERROR_TEMPLATE_INCOMPLETE: return "Template incomplete";
            case Status::ERROR_TEMPLATE_INCONSISTENT: return "Template inconsistent";
            case Status::ERROR_TOKEN_NOT_PRESENT: return "Token not present";
            case Status::ERROR_TOKEN_NOT_RECOGNIZED: return "Token not recognized";
            case Status::ERROR_TOKEN_WRITE_PROTECTED: return "Token is write-protected";
            case Status::ERROR_UNWRAPPING_KEY_HANDLE_INVALID: return "Invalid unwrapping key handle";
            case Status::ERROR_UNWRAPPING_KEY_SIZE_RANGE: return "Unwrapping key size out of range";
            case Status::ERROR_UNWRAPPING_KEY_TYPE_INCONSISTENT: return "Inconsistent unwrapping key type";
            case Status::ERROR_WRAPPED_KEY_INVALID: return "Invalid wrapped key";
            case Status::ERROR_WRAPPED_KEY_LEN_RANGE: return "Wrapped key length out of range";
            case Status::ERROR_WRAPPING_KEY_HANDLE_INVALID: return "Invalid wrapping key handle";
            case Status::ERROR_WRAPPING_KEY_SIZE_RANGE: return "Wrapping key size out of range";
            case Status::ERROR_WRAPPING_KEY_TYPE_INCONSISTENT: return "Inconsistent wrapping key type";
            case Status::ERROR_USER_ALREADY_LOGGED_IN: return "User already logged in";
            case Status::ERROR_USER_NOT_LOGGED_IN: return "User not logged in";
            case Status::ERROR_USER_PIN_NOT_INITIALIZED: return "User PIN not initialized";
            case Status::ERROR_USER_TYPE_INVALID: return "Invalid user type";
            case Status::ERROR_USER_ANOTHER_ALREADY_LOGGED_IN: return "Another user already logged in";
            case Status::ERROR_USER_TOO_MANY_TYPES: return "Too many user types";
            case Status::ERROR_RANDOM_SEED_NOT_SUPPORTED: return "Random seed not supported";
            case Status::ERROR_RANDOM_NO_RNG: return "No random number generator";
            case Status::ERROR_DOMAIN_PARAMS_INVALID: return "Invalid domain parameters";
            case Status::ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
            case Status::ERROR_SAVED_STATE_INVALID: return "Invalid saved state";
            case Status::ERROR_INFORMATION_SENSITIVE: return "Information sensitive";
            case Status::ERROR_STATE_UNSAVEABLE: return "State unsaveable";
            case Status::ERROR_CRYPTOKI_NOT_INITIALIZED: return "Cryptoki not initialized";
            case Status::ERROR_CRYPTOKI_ALREADY_INITIALIZED: return "Cryptoki already initialized";
            case Status::ERROR_MUTEX_BAD: return "Bad mutex";
            case Status::ERROR_MUTEX_NOT_LOCKED: return "Mutex not locked";
            case Status::ERROR_NEW_PIN_MODE: return "New PIN mode";
            case Status::ERROR_NEXT_OTP: return "Next OTP";
            case Status::ERROR_VENDOR_DEFINED: return "Vendor defined error";
            case Status::ERROR_LIBRARY_LOAD_FAILED: return "Failed to load library";
            case Status::ERROR_FUNCTION_LIST_NOT_AVAILABLE: return "Function list not available";
            case Status::ERROR_AUX_FUNCTION_NOT_AVAILABLE: return "Auxiliary function not available";
            case Status::ERROR_INVALID_PARAMETER: return "Invalid parameter";
            case Status::ERROR_MEMORY: return "Memory allocation error";
            case Status::ERROR_FILE_IO: return "File I/O error";
            case Status::ERROR_UNSUPPORTED_ALGORITHM: return "Unsupported algorithm";
            case Status::ERROR_UNSUPPORTED_OPERATION: return "Unsupported operation";
            default: return "Unknown error";
        }
    }
};

} // namespace PKCS11Lib

#endif // RESULT_H