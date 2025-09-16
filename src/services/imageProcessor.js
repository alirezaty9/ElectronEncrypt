import encryptionService from './encryptionService';
import tokenEncryption from './tokenEncryption';

class ImageProcessor {
  constructor() {
    this.processingQueue = [];
    this.isProcessing = false;
    this.currentProgress = null;
  }

  /**
   * Initialize the processor
   */
  async initialize() {
    // Initialize token connection
    const tokenResult = await tokenEncryption.initialize();
    if (!tokenResult.success) {
      throw new Error(`Failed to initialize token: ${tokenResult.error}`);
    }
    
    return true;
  }

  /**
   * Encrypt a single image
   */
  async encryptImage(imagePath, options = {}) {
    try {
      // Encrypt the image with output directory
      const result = await encryptionService.encryptImageFile(
        imagePath, 
        options.outputDirectory || null
      );
      
      // Delete original if requested
      if (options.deleteOriginal && window.fs) {
        // Note: In browser context, we can't delete files directly
        // This would need to be handled via another IPC call
      }
      
      return {
        success: true,
        ...result
      };
    } catch (error) {
      return {
        success: false,
        originalPath: imagePath,
        error: error.message
      };
    }
  }

  /**
   * Decrypt a single image
   */
  async decryptImage(encryptedPath, options = {}) {
    try {
      // Check if file is encrypted
      const isEncrypted = await encryptionService.isEncryptedFile(encryptedPath);
      if (!isEncrypted) {
        throw new Error('File is not an encrypted image');
      }
      
      // Decrypt the image (returns Buffer in memory)
      const decryptedBuffer = await encryptionService.decryptImageFile(encryptedPath);
      
      // Detect MIME type
      const mimeType = this.detectMimeType(decryptedBuffer);
      
      // If save to file is requested
      if (options.saveToFile && options.outputDirectory) {
        const fileName = encryptedPath.split('/').pop().replace('.enc', '');
        const result = await window.electronAPI.saveDecryptedImage({
          data: this.bufferToDataURL(decryptedBuffer, mimeType),
          name: fileName,
          outputDirectory: options.outputDirectory
        });
        
        return {
          success: true,
          buffer: decryptedBuffer,
          mimeType: mimeType,
          savedPath: result.path
        };
      }
      
      // Return decrypted data in memory
      return {
        success: true,
        buffer: decryptedBuffer,
        mimeType: mimeType
      };
    } catch (error) {
      return {
        success: false,
        encryptedPath,
        error: error.message
      };
    }
  }

  /**
   * Convert buffer to data URL
   */
  bufferToDataURL(buffer, mimeType) {
    const bytes = new Uint8Array(buffer);
    let binary = '';
    for (let i = 0; i < bytes.byteLength; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return `data:${mimeType};base64,${btoa(binary)}`;
  }

  /**
   * Batch encrypt images
   */
  async encryptBatch(imagePaths, options = {}) {
    const results = [];
    const total = imagePaths.length;
    
    // Progress callback
    const progressCallback = options.onProgress || (() => {});
    
    // Extract output directory from options
    const outputDirectory = options.outputDirectory || null;
    
    for (let i = 0; i < total; i++) {
      const imagePath = imagePaths[i];
      
      // Update progress
      progressCallback({
        current: i + 1,
        total,
        percentage: Math.round(((i + 1) / total) * 100),
        currentFile: imagePath.split('/').pop(),
        status: 'encrypting'
      });
      
      // Encrypt image with output directory
      const result = await this.encryptImage(imagePath, {
        ...options,
        outputDirectory: outputDirectory
      });
      results.push(result);
      
      // Check if should stop
      if (options.stopOnError && !result.success) {
        break;
      }
    }
    
    return {
      total: results.length,
      successful: results.filter(r => r.success).length,
      failed: results.filter(r => !r.success).length,
      results
    };
  }

  /**
   * Batch decrypt images
   */
  async decryptBatch(encryptedPaths, options = {}) {
    const results = [];
    const total = encryptedPaths.length;
    
    // Progress callback
    const progressCallback = options.onProgress || (() => {});
    
    for (let i = 0; i < total; i++) {
      const encryptedPath = encryptedPaths[i];
      
      // Update progress
      progressCallback({
        current: i + 1,
        total,
        percentage: Math.round(((i + 1) / total) * 100),
        currentFile: encryptedPath.split('/').pop(),
        status: 'decrypting'
      });
      
      // Decrypt image
      const result = await this.decryptImage(encryptedPath, options);
      results.push(result);
      
      // Check if should stop
      if (options.stopOnError && !result.success) {
        break;
      }
    }
    
    return {
      total: results.length,
      successful: results.filter(r => r.success).length,
      failed: results.filter(r => !r.success).length,
      results
    };
  }

  /**
   * Process a folder recursively
   */
  async processFolder(folderPath, operation = 'encrypt', options = {}) {
    // Get files from folder via IPC
    const files = await window.electronAPI.getImagesFromFolder(folderPath);
    
    if (!files || files.length === 0) {
      return {
        total: 0,
        successful: 0,
        failed: 0,
        results: []
      };
    }
    
    // Extract file paths
    const filePaths = files.map(f => f.path);
    
    // Process the files
    if (operation === 'encrypt') {
      return await this.encryptBatch(filePaths, options);
    } else {
      return await this.decryptBatch(filePaths, options);
    }
  }

  /**
   * Get metadata for encrypted file
   */
  async getEncryptedFileInfo(filePath) {
    try {
      const metadata = await encryptionService.getEncryptedFileMetadata(filePath);
      return {
        success: true,
        metadata
      };
    } catch (error) {
      return {
        success: false,
        error: error.message
      };
    }
  }

  /**
   * Detect MIME type from buffer
   */
  detectMimeType(buffer) {
    // Check magic numbers for image formats
    const arr = buffer instanceof Uint8Array ? buffer : new Uint8Array(buffer);
    
    if (!arr || arr.length < 12) {
      return 'image/jpeg';
    }
    
    // PNG
    if (arr[0] === 0x89 && arr[1] === 0x50 && arr[2] === 0x4E && arr[3] === 0x47) {
      return 'image/png';
    }
    
    // JPEG
    if (arr[0] === 0xFF && arr[1] === 0xD8 && arr[2] === 0xFF) {
      return 'image/jpeg';
    }
    
    // GIF
    if (arr[0] === 0x47 && arr[1] === 0x49 && arr[2] === 0x46) {
      return 'image/gif';
    }
    
    // WebP
    if (arr.length > 11 && arr[8] === 0x57 && arr[9] === 0x45 && arr[10] === 0x42 && arr[11] === 0x50) {
      return 'image/webp';
    }
    
    // BMP
    if (arr[0] === 0x42 && arr[1] === 0x4D) {
      return 'image/bmp';
    }
    
    // Default to JPEG
    return 'image/jpeg';
  }
}

// Export singleton instance
const imageProcessor = new ImageProcessor();
export default imageProcessor;