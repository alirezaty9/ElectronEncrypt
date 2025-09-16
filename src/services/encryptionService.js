// Encryption Service for Client-side
// This service communicates with the main process for encryption operations

class EncryptionService {
  constructor() {
    this.isInitialized = false;
  }

  /**
   * Encrypt an image file
   * @param {string} imagePath - Path to the image file
   * @param {string} outputDirectory - Optional output directory
   * @returns {Promise<{outputPath: string, metadata: object}>}
   */
  async encryptImageFile(imagePath, outputDirectory = null) {
    try {
      // Call main process to handle encryption with output directory
      const result = await window.electronAPI.encryptImage(imagePath, outputDirectory);
      
      if (!result.success) {
        throw new Error(result.error || 'Encryption failed');
      }
      
      return {
        outputPath: result.outputPath,
        metadata: result.metadata
      };
    } catch (error) {
      throw new Error(`Failed to encrypt image: ${error.message}`);
    }
  }

  /**
   * Decrypt an image file
   * @param {string} encryptedPath - Path to the encrypted file
   * @returns {Promise<Buffer>} - Decrypted image data (in memory only)
   */
  async decryptImageFile(encryptedPath) {
    try {
      // Call main process to handle decryption
      const result = await window.electronAPI.decryptImage(encryptedPath);
      
      if (!result.success) {
        throw new Error(result.error || 'Decryption failed');
      }
      
      // Convert base64 back to buffer for display
      const base64Data = result.data;
      const binaryString = atob(base64Data);
      const bytes = new Uint8Array(binaryString.length);
      for (let i = 0; i < binaryString.length; i++) {
        bytes[i] = binaryString.charCodeAt(i);
      }
      
      return bytes.buffer;
    } catch (error) {
      throw new Error(`Failed to decrypt image: ${error.message}`);
    }
  }

  /**
   * Check if a file is encrypted
   */
  async isEncryptedFile(filePath) {
    try {
      const result = await window.electronAPI.isEncryptedFile(filePath);
      return result.isEncrypted;
    } catch (error) {
      return false;
    }
  }

  /**
   * Batch encrypt multiple images
   */
  async encryptBatch(imagePaths, progressCallback, outputDirectory = null) {
    const results = [];
    const total = imagePaths.length;
    
    for (let i = 0; i < total; i++) {
      const imagePath = imagePaths[i];
      
      try {
        if (progressCallback) {
          progressCallback({
            current: i + 1,
            total,
            currentFile: imagePath.split('/').pop(),
            status: 'encrypting'
          });
        }
        
        // Pass output directory to encryption
        const result = await this.encryptImageFile(imagePath, outputDirectory);
        results.push({
          success: true,
          ...result
        });
      } catch (error) {
        results.push({
          success: false,
          originalPath: imagePath,
          error: error.message
        });
      }
    }
    
    return results;
  }

  /**
   * Get file metadata without decrypting
   */
  async getEncryptedFileMetadata(filePath) {
    try {
      const result = await window.electronAPI.getEncryptedFileMetadata(filePath);
      
      if (!result.success) {
        throw new Error(result.error || 'Failed to get metadata');
      }
      
      return result.metadata;
    } catch (error) {
      throw new Error(`Failed to read metadata: ${error.message}`);
    }
  }
}

export default new EncryptionService();