const { contextBridge, ipcRenderer } = require('electron');

console.log('Preload script loaded');

// ====================================================================
// ENHANCED ELECTRON FETCH WRAPPER - FIXED FOR FORMDATA
// ====================================================================

const electronFetch = async (url, options = {}) => {
  try {
    console.log('electronFetch called with:', { url, method: options.method });
    console.log('Body type:', typeof options.body);
    console.log('Body constructor:', options.body?.constructor?.name);
    console.log('Is FormData:', options.body instanceof FormData);
    
    // Handle FormData specially
    if (options.body instanceof FormData) {
      console.log('Processing FormData in electronFetch...');
      
      // Convert FormData to a format that can be sent via IPC
      const formDataObj = {};
      const files = {};
      
      for (const [key, value] of options.body.entries()) {
        if (value instanceof File) {
          console.log(`Processing file: ${key} - ${value.name}`);
          
          // Convert File to base64 for IPC transmission
          const base64 = await new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = () => resolve(reader.result);
            reader.onerror = reject;
            reader.readAsDataURL(value);
          });
          
          files[key] = {
            name: value.name,
            type: value.type,
            size: value.size,
            data: base64
          };
        } else {
          console.log(`Processing field: ${key} = ${value}`);
          formDataObj[key] = value;
        }
      }
      
      console.log('Sending FormData via IPC:', { formDataFields: Object.keys(formDataObj), files: Object.keys(files) });
      
      // Use the api-request handler for FormData
      const result = await electronAPI.apiRequest({
        url,
        method: options.method || 'POST',
        headers: options.headers || {},
        body: { formData: formDataObj, files }
      });

      if (result.success) {
        return {
          ok: result.status >= 200 && result.status < 300,
          status: result.status,
          statusText: result.status >= 200 && result.status < 300 ? 'OK' : 'Error',
          headers: new Headers(result.headers || {}),
          json: async () => {
            if (typeof result.data === 'object') {
              return result.data;
            }
            try {
              return JSON.parse(result.data);
            } catch {
              return result.data;
            }
          },
          text: async () => typeof result.data === 'string' ? result.data : JSON.stringify(result.data),
          blob: async () => new Blob([typeof result.data === 'string' ? result.data : JSON.stringify(result.data)], { 
            type: 'application/json' 
          }),
          arrayBuffer: async () => {
            const text = typeof result.data === 'string' ? result.data : JSON.stringify(result.data);
            return new TextEncoder().encode(text).buffer;
          }
        };
      } else {
        throw new Error(result.error || 'Network request failed');
      }
    }

    // Regular JSON requests
    console.log('Processing regular request...');
    const result = await electronAPI.apiRequest({
      url,
      method: options.method || 'GET',
      headers: options.headers || {},
      body: options.body || null
    });

    if (result.success) {
      return {
        ok: result.status >= 200 && result.status < 300,
        status: result.status,
        statusText: result.status >= 200 && result.status < 300 ? 'OK' : 'Error',
        headers: new Headers(result.headers || {}),
        json: async () => {
          if (typeof result.data === 'object') {
            return result.data;
          }
          try {
            return JSON.parse(result.data);
          } catch {
            return result.data;
          }
        },
        text: async () => typeof result.data === 'string' ? result.data : JSON.stringify(result.data)
      };
    } else {
      throw new Error(result.error || 'Network request failed');
    }
  } catch (error) {
    console.error('electronFetch error:', error.message);
    throw error;
  }
};

// ====================================================================
// CORE ELECTRON API - Enhanced & Unified
// ====================================================================

const electronAPI = {
  // Token Operations
  checkTokenStatus: () => ipcRenderer.invoke('check-token-status'),
  verifyToken: (options) => ipcRenderer.invoke('verify-token', options),
  testDriver: () => ipcRenderer.invoke('test-driver'),
  
  // Legacy Support
  checkHardwareToken: (vendorId, productId) => ipcRenderer.invoke('check-hardware-token', vendorId, productId),
  requestTokenAccess: (vendorId, productId) => ipcRenderer.invoke('request-token-access', vendorId, productId),
  signAndVerifyFile: (options) => ipcRenderer.invoke('sign-and-verify-file', options),

  // File Operations
  createFile: (fileName, content) => ipcRenderer.invoke('create-file', fileName, content),
  readFile: (filePath) => ipcRenderer.invoke('read-file', filePath),
  saveFileDialog: (data, filters) => ipcRenderer.invoke('save-file-dialog', data, filters),
  
  // Image Operations
  selectImageFiles: () => ipcRenderer.invoke('select-image-files'),
  compareImages: (imageData) => ipcRenderer.invoke('compare-images', imageData),
  selectEncryptedFile: () => ipcRenderer.invoke('select-encrypted-file'),
  selectFolder: () => ipcRenderer.invoke('select-folder'),
  getImagesFromFolder: (folderPath) => ipcRenderer.invoke('get-images-from-folder', folderPath),
  
  // RSA Encryption/Decryption
  tokenEncryptRSA: (data) => ipcRenderer.invoke('token-encrypt-rsa', data),
  tokenDecryptRSA: (data) => ipcRenderer.invoke('token-decrypt-rsa', data),
  getTokenPublicKey: () => ipcRenderer.invoke('get-token-public-key'),
  
  // Image Encryption/Decryption
  encryptImage: (imagePath, outputDirectory) => ipcRenderer.invoke('encrypt-image', imagePath, outputDirectory),
  decryptImage: (encryptedPath) => ipcRenderer.invoke('decrypt-image', encryptedPath),
  isEncryptedFile: (filePath) => ipcRenderer.invoke('is-encrypted-file', filePath),
  getEncryptedFileMetadata: (filePath) => ipcRenderer.invoke('get-encrypted-file-metadata', filePath),

  // System Operations
  getSystemInfo: () => ipcRenderer.invoke('get-system-info'),
  showItemInFolder: (fullPath) => ipcRenderer.invoke('show-item-in-folder', fullPath),
  openExternal: (url) => ipcRenderer.invoke('open-external', url),

  // Command Execution
  executeShellCommand: (command, options) => ipcRenderer.invoke('execute-shell-command', command, options),
  runTest123Command: (args) => ipcRenderer.invoke('run-test123-command', args),
  verifyPkcs11Signature: (options) => ipcRenderer.invoke('verify-pkcs11-signature', options),

  // Network Operations - Enhanced for FormData
  apiRequest: (options) => ipcRenderer.invoke('api-request', options),

  // Event Listeners
  onTokenVerificationResult: (callback) => {
    const handler = (event, ...args) => callback(event, ...args);
    ipcRenderer.on('token-verification-result', handler);
    return () => ipcRenderer.removeListener('token-verification-result', handler);
  },

  onTokenConnected: (callback) => {
    const handler = (event, data) => {
      if (data && (data.connected || data.vendorId)) {
        callback(event, data);
      }
    };
    ipcRenderer.on('token-connected', handler);
    ipcRenderer.on('token-verification-result', (event, result) => {
      if (result.success) {
        callback(event, { connected: true, timestamp: new Date() });
      }
    });
    return () => {
      ipcRenderer.removeListener('token-connected', handler);
    };
  },
  
  onTokenDisconnected: (callback) => {
    const handler = (event, data) => {
      if (data && (data.connected === false || data.vendorId)) {
        callback(event, data);
      }
    };
    ipcRenderer.on('token-disconnected', handler);
    ipcRenderer.on('token-verification-result', (event, result) => {
      if (!result.success) {
        callback(event, { connected: false, timestamp: new Date() });
      }
    });
    return () => {
      ipcRenderer.removeListener('token-disconnected', handler);
    };
  },

  onFileSignResult: (callback) => {
    const handler = (event, ...args) => callback(event, ...args);
    ipcRenderer.on('token-verification-result', handler);
    ipcRenderer.on('file-sign-result', handler);
    return () => {
      ipcRenderer.removeListener('token-verification-result', handler);
      ipcRenderer.removeListener('file-sign-result', handler);
    };
  },
  
  onFilesSelected: (callback) => {
    const handler = (event, ...args) => callback(event, ...args);
    ipcRenderer.on('files-selected', handler);
    return () => ipcRenderer.removeListener('files-selected', handler);
  },
  
  onCommandLog: (callback) => {
    const handler = (event, ...args) => callback(event, ...args);
    ipcRenderer.on('command-log', handler);
    return () => ipcRenderer.removeListener('command-log', handler);
  },

  removeAllListeners: (channel) => ipcRenderer.removeAllListeners(channel),

  getTokenId: () => ipcRenderer.invoke('get-token-id'),
  verifyLicense: () => ipcRenderer.invoke('verify-license'),
  
  // New UX improvement handlers
  selectOutputDirectory: () => ipcRenderer.invoke('select-output-directory'),
  showNotification: (options) => ipcRenderer.invoke('show-notification', options),
  revealInFolder: (filePath) => ipcRenderer.invoke('reveal-in-folder', filePath),
  setProcessingState: (isProcessing) => ipcRenderer.invoke('set-processing-state', isProcessing),
  getEncryptedFilesFromFolder: (folderPath) => ipcRenderer.invoke('get-encrypted-files-from-folder', folderPath),
  saveDecryptedImage: (options) => ipcRenderer.invoke('save-decrypted-image', options),
};

// ====================================================================
// FILE SYSTEM API
// ====================================================================

const fileSystemAPI = {
  readFile: async (fileName, options = {}) => {
    try {
      if (fileName && typeof fileName === 'object' && fileName.name) {
        return new Promise((resolve, reject) => {
          const reader = new FileReader();
          
          reader.onload = () => {
            try {
              if (options.encoding === 'utf8') {
                resolve(reader.result);
              } else {
                resolve(new Uint8Array(reader.result));
              }
            } catch (error) {
              reject(new Error('File processing failed: ' + error.message));
            }
          };
          
          reader.onerror = () => reject(new Error('File reading failed'));
          
          if (options.encoding === 'utf8') {
            reader.readAsText(fileName);
          } else {
            reader.readAsArrayBuffer(fileName);
          }
        });
      }
      
      if (typeof fileName === 'string') {
        const result = await electronAPI.readFile(fileName);
        if (result.success) {
          if (options.encoding === 'utf8') {
            return Buffer.from(result.data).toString('utf8');
          } else {
            return new Uint8Array(result.data);
          }
        } else {
          throw new Error(result.error);
        }
      }
      
      throw new Error('Invalid file parameter');
    } catch (error) {
      console.error('File system error:', error.message);
      throw error;
    }
  },

  writeFile: async (fileName, content) => {
    try {
      const result = await electronAPI.createFile(fileName, content);
      if (!result.success) {
        throw new Error(result.error);
      }
      return result;
    } catch (error) {
      console.error('File write error:', error.message);
      throw error;
    }
  }
};

// ====================================================================
// STORAGE UTILITIES - Fixed JSON parsing
// ====================================================================

const storageUtilities = {
  set: (key, value, encrypted = false) => {
    try {
      let serialized;
      if (encrypted) {
        const jsonStr = JSON.stringify({ value, timestamp: Date.now(), type: typeof value });
        serialized = btoa(jsonStr);
      } else {
        serialized = JSON.stringify({ value, timestamp: Date.now(), type: typeof value });
      }
      
      localStorage.setItem(key, serialized);
      return true;
    } catch (error) {
      console.error('Storage set error:', error.message);
      return false;
    }
  },
  
  get: (key, defaultValue = null, encrypted = false) => {
    try {
      const item = localStorage.getItem(key);
      if (!item) return defaultValue;
      
      let parsed;
      if (encrypted) {
        try {
          // Try to decrypt first
          const jsonStr = atob(item);
          parsed = JSON.parse(jsonStr);
        } catch (decryptError) {
          // If decryption fails, try regular JSON parse
          try {
            parsed = JSON.parse(item);
          } catch (parseError) {
            console.warn('Storage decrypt/parse failed for key:', key);
            return defaultValue;
          }
        }
      } else {
        try {
          parsed = JSON.parse(item);
        } catch (parseError) {
          console.warn('Storage parse failed for key:', key);
          return defaultValue;
        }
      }
      
      return parsed.value !== undefined ? parsed.value : parsed;
    } catch (error) {
      console.error('Storage get error:', error.message);
      return defaultValue;
    }
  },
  
  remove: (key) => {
    try {
      localStorage.removeItem(key);
      return true;
    } catch (error) {
      console.error('Storage remove error:', error.message);
      return false;
    }
  },
  
  clear: () => {
    try {
      localStorage.clear();
      return true;
    } catch (error) {
      console.error('Storage clear error:', error.message);
      return false;
    }
  },

  exists: (key) => {
    return localStorage.getItem(key) !== null;
  },

  size: () => {
    return localStorage.length;
  },

  keys: () => {
    const keys = [];
    for (let i = 0; i < localStorage.length; i++) {
      keys.push(localStorage.key(i));
    }
    return keys;
  }
};

// ====================================================================
// IMAGE UTILITIES
// ====================================================================

const imageUtilities = {
  fileToBase64: (file) => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(reader.result);
      reader.onerror = () => reject(new Error('Base64 conversion failed'));
      reader.readAsDataURL(file);
    });
  },
  
  getImageDimensions: (imageSrc) => {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.onload = () => resolve({ 
        width: img.width, 
        height: img.height,
        aspectRatio: img.width / img.height
      });
      img.onerror = () => reject(new Error('Image dimensions calculation failed'));
      img.src = imageSrc;
    });
  },
  
  validateImageFile: (file) => {
    const validTypes = [
      'image/jpeg', 'image/jpg', 'image/png', 'image/gif', 
      'image/bmp', 'image/webp', 'image/tiff', 'image/svg+xml'
    ];
    const maxSize = 50 * 1024 * 1024; // 50MB
    
    return {
      isValid: validTypes.includes(file.type) && file.size <= maxSize,
      type: file.type,
      size: file.size,
      sizeInMB: (file.size / (1024 * 1024)).toFixed(2),
      name: file.name,
      maxSizeExceeded: file.size > maxSize,
      unsupportedType: !validTypes.includes(file.type)
    };
  },

  resizeImage: (file, maxWidth, maxHeight, quality = 0.8) => {
    return new Promise((resolve, reject) => {
      const canvas = document.createElement('canvas');
      const ctx = canvas.getContext('2d');
      const img = new Image();

      img.onload = () => {
        let { width, height } = img;
        if (width > height) {
          if (width > maxWidth) {
            height *= maxWidth / width;
            width = maxWidth;
          }
        } else {
          if (height > maxHeight) {
            width *= maxHeight / height;
            height = maxHeight;
          }
        }

        canvas.width = width;
        canvas.height = height;
        ctx.drawImage(img, 0, 0, width, height);

        canvas.toBlob(resolve, file.type, quality);
      };

      img.onerror = () => reject(new Error('Image resize failed'));
      img.src = URL.createObjectURL(file);
    });
  }
};

// ====================================================================
// COMMAND LOGGER
// ====================================================================

const commandLogger = {
  onLog: (callback) => {
    const handler = (event, log) => {
      callback({
        timestamp: new Date().toISOString(),
        level: log.level || 'info',
        message: log.message || log,
        source: log.source || 'unknown',
        ...log
      });
    };
    
    electronAPI.onCommandLog(handler);
    return () => electronAPI.removeAllListeners('command-log');
  },
  
  removeListeners: () => {
    electronAPI.removeAllListeners('command-log');
  },

  log: (level, message, data = {}) => {
    const logEntry = {
      timestamp: new Date().toISOString(),
      level,
      message,
      data,
      source: 'renderer'
    };
    
    console[level] ? console[level](message, data) : console.log(message, data);
    
    try {
      const logs = JSON.parse(localStorage.getItem('app_logs') || '[]');
      logs.push(logEntry);
      if (logs.length > 100) {
        logs.splice(0, logs.length - 100);
      }
      localStorage.setItem('app_logs', JSON.stringify(logs));
    } catch (e) {
      // Ignore storage errors
    }
  }
};

// ====================================================================
// SHELL API
// ====================================================================

const shellAPI = {
  execute: async (command, options = {}) => {
    try {
      return await electronAPI.executeShellCommand(command, options);
    } catch (error) {
      throw error;
    }
  },
  
  verifyPkcs11: async (options = {}) => {
    try {
      return await electronAPI.verifyPkcs11Signature(options);
    } catch (error) {
      throw error;
    }
  },

  runTest123: async (args = '') => {
    try {
      return await electronAPI.runTest123Command(args);
    } catch (error) {
      throw error;
    }
  }
};

// ====================================================================
// CRYPTO UTILITIES
// ====================================================================

const cryptoUtilities = {
  generateChallenge: (length = 32) => {
    const array = new Uint8Array(length);
    crypto.getRandomValues(array);
    return Array.from(array, byte => byte.toString(16).padStart(2, '0')).join('');
  },

  hexToUint8Array: (hexString) => {
    const matches = hexString.match(/.{1,2}/g);
    return new Uint8Array(matches ? matches.map(byte => parseInt(byte, 16)) : []);
  },

  uint8ArrayToHex: (uint8Array) => {
    return Array.from(uint8Array, byte => byte.toString(16).padStart(2, '0')).join('');
  },

  simpleHash: async (data) => {
    const encoder = new TextEncoder();
    const dataBytes = typeof data === 'string' ? encoder.encode(data) : data;
    const hashBuffer = await crypto.subtle.digest('SHA-256', dataBytes);
    return new Uint8Array(hashBuffer);
  }
};

// ====================================================================
// EXPOSE APIS TO RENDERER PROCESS
// ====================================================================

contextBridge.exposeInMainWorld('electronAPI', electronAPI);
contextBridge.exposeInMainWorld('electronFetch', electronFetch);
contextBridge.exposeInMainWorld('fs', fileSystemAPI);
contextBridge.exposeInMainWorld('imageUtils', imageUtilities);
contextBridge.exposeInMainWorld('storage', storageUtilities);
contextBridge.exposeInMainWorld('commandLogger', commandLogger);
contextBridge.exposeInMainWorld('shell', shellAPI);
contextBridge.exposeInMainWorld('cryptoUtils', cryptoUtilities);

contextBridge.exposeInMainWorld('fetchPolyfill', electronFetch);

window.addEventListener('error', (event) => {
  commandLogger.log('error', 'Global error caught', {
    message: event.error?.message,
    stack: event.error?.stack,
    filename: event.filename,
    lineno: event.lineno,
    colno: event.colno
  });
});

window.addEventListener('unhandledrejection', (event) => {
  commandLogger.log('error', 'Unhandled promise rejection', {
    reason: event.reason,
    promise: event.promise
  });
});

console.log('Enhanced preload script loaded successfully');
console.log('Available APIs:', Object.keys({
  electronAPI,
  electronFetch,
  fs: fileSystemAPI,
  imageUtils: imageUtilities,
  storage: storageUtilities,
  commandLogger,
  shell: shellAPI,
  cryptoUtils: cryptoUtilities
}));