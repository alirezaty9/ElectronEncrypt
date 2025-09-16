class TokenEncryptionService {
  constructor() {
    this.isConnected = false;
    this.tokenInfo = null;
  }

  /**
   * Initialize token connection
   */
  async initialize() {
    try {
      const result = await window.electronAPI.verifyToken({
        forceRefresh: true
      });
      
      if (result.success) {
        this.isConnected = true;
        this.tokenInfo = result.details;
        return { success: true };
      }
      
      return { 
        success: false, 
        error: result.message || 'Failed to connect to token' 
      };
    } catch (error) {
      return { 
        success: false, 
        error: error.message 
      };
    }
  }

  /**
   * Check token status
   */
  async checkStatus() {
    try {
      const result = await window.electronAPI.checkTokenStatus();
      return result;
    } catch (error) {
      return { 
        success: false, 
        error: error.message 
      };
    }
  }

  /**
   * Encrypt data using token's public key
   */
  async encryptWithPublicKey(data) {
    try {
      const result = await window.electronAPI.tokenEncryptRSA({
        data: data.toString('base64')
      });
      
      if (result.success) {
        return Buffer.from(result.encrypted, 'base64');
      }
      
      throw new Error(result.error || 'Encryption failed');
    } catch (error) {
      throw new Error(`Token encryption failed: ${error.message}`);
    }
  }

  /**
   * Decrypt data using token's private key
   */
  async decryptWithPrivateKey(encryptedData) {
    try {
      const result = await window.electronAPI.tokenDecryptRSA({
        data: encryptedData.toString('base64')
      });
      
      if (result.success) {
        return Buffer.from(result.decrypted, 'base64');
      }
      
      throw new Error(result.error || 'Decryption failed');
    } catch (error) {
      throw new Error(`Token decryption failed: ${error.message}`);
    }
  }

  /**
   * Login to token with PIN
   */
  async login(pin) {
    try {
      const result = await window.electronAPI.verifyToken({
        pin: pin,
        forceRefresh: true
      });
      
      if (result.success) {
        this.isConnected = true;
        this.tokenInfo = result.details;
        return { success: true };
      }
      
      return { 
        success: false, 
        error: result.message || 'Login failed' 
      };
    } catch (error) {
      return { 
        success: false, 
        error: error.message 
      };
    }
  }

  /**
   * Get token public key
   */
  async getPublicKey() {
    try {
      const result = await window.electronAPI.getTokenPublicKey();
      
      if (result.success) {
        return result.publicKey;
      }
      
      throw new Error(result.error || 'Failed to get public key');
    } catch (error) {
      throw new Error(`Failed to get token public key: ${error.message}`);
    }
  }

  /**
   * Check if token is connected
   */
  isTokenConnected() {
    return this.isConnected;
  }

  /**
   * Get token information
   */
  getTokenInfo() {
    return this.tokenInfo;
  }

  /**
   * Disconnect from token
   */
  async disconnect() {
    this.isConnected = false;
    this.tokenInfo = null;
    // Additional cleanup if needed
  }
}

export default new TokenEncryptionService();