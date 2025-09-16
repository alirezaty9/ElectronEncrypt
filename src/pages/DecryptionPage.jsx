import React, { useState, useEffect, useRef } from 'react';
import imageProcessor from '../services/imageProcessor';
import tokenEncryption from '../services/tokenEncryption';

const DecryptionPage = () => {
  const [selectedFiles, setSelectedFiles] = useState([]);
  const [decryptedImages, setDecryptedImages] = useState([]);
  const [processing, setProcessing] = useState(false);
  const [tokenStatus, setTokenStatus] = useState({ connected: false });
  const [showPinDialog, setShowPinDialog] = useState(false);
  const [pin, setPin] = useState('');
  const [error, setError] = useState(null);
  const [success, setSuccess] = useState(null);
  const [currentImage, setCurrentImage] = useState(null);
  const [progress, setProgress] = useState(null);
  const [outputDirectory, setOutputDirectory] = useState(null);
  const [saveToFile, setSaveToFile] = useState(false);
  const [appStatus, setAppStatus] = useState('آماده برای کار');
  
  useEffect(() => {
    checkTokenStatus();
    
    return () => {
      // Cleanup decrypted images from memory
      decryptedImages.forEach(img => {
        if (img.url) {
          URL.revokeObjectURL(img.url);
        }
      });
    };
  }, []);

  // Check token connection status
  const checkTokenStatus = async () => {
    const status = await tokenEncryption.checkStatus();
    setTokenStatus(status);
    if (status?.success || status?.data?.isInitialized) {
      setTokenStatus({ ...status, connected: true });
    } else if (!status?.data?.isInitialized) {
      setError('توکن سخت‌افزاری متصل نیست. لطفاً توکن را متصل کنید.');
    }
  };

  // Handle file selection
  const handleFileSelect = async () => {
    try {
      setAppStatus('در حال انتخاب فایل...');
      const result = await window.electronAPI.selectEncryptedFile();
      
      if (result.success && result.file) {
        setSelectedFiles([result.file]);
        setError(null);
        setDecryptedImages([]);
        setAppStatus('فایل انتخاب شد');
      }
    } catch (err) {
      setError('خطا در انتخاب فایل: ' + err.message);
      setAppStatus('خطا در انتخاب');
    }
  };

  // Handle folder selection for batch decryption
  const handleFolderSelect = async () => {
    try {
      setAppStatus('در حال انتخاب پوشه...');
      const result = await window.electronAPI.selectFolder();
      
      if (result.success) {
        // Get all .enc files from folder
        const encFiles = await window.electronAPI.getEncryptedFilesFromFolder(result.path);
        
        if (encFiles && encFiles.length > 0) {
          setSelectedFiles(encFiles);
          setError(null);
          setDecryptedImages([]);
          setSuccess(`✅ ${encFiles.length} فایل رمزنگاری شده یافت شد`);
          setAppStatus(`${encFiles.length} فایل آماده رمزگشایی`);
        } else {
          setError('هیچ فایل رمزنگاری شده‌ای در این پوشه یافت نشد');
          setAppStatus('فایلی یافت نشد');
        }
      }
    } catch (err) {
      setError('خطا در انتخاب پوشه: ' + err.message);
      setAppStatus('خطا در انتخاب');
    }
  };

  // Handle output directory selection
  const handleSelectOutputDir = async () => {
    try {
      setAppStatus('در حال انتخاب محل ذخیره...');
      const result = await window.electronAPI.selectOutputDirectory();
      
      if (result.success) {
        setOutputDirectory(result.path);
        setSaveToFile(true);
        setSuccess(`📁 فایل‌ها در ${result.path} ذخیره خواهند شد`);
        setAppStatus('محل ذخیره انتخاب شد');
      }
    } catch (err) {
      setError('خطا در انتخاب پوشه: ' + err.message);
    }
  };

  // Handle PIN submission
  const handlePinSubmit = async () => {
    try {
      const result = await tokenEncryption.login(pin);
      
      if (result.success) {
        setShowPinDialog(false);
        setPin('');
        setTokenStatus({ ...tokenStatus, connected: true });
        processDecryption();
      } else {
        setError('پین اشتباه است. لطفاً دوباره تلاش کنید.');
      }
    } catch (err) {
      setError('خطا در ورود به توکن: ' + err.message);
    }
  };

  // Process decryption
  const processDecryption = async () => {
    if (selectedFiles.length === 0) {
      setError('لطفاً ابتدا فایل‌های رمزنگاری شده را انتخاب کنید.');
      return;
    }

    setProcessing(true);
    setError(null);
    setSuccess(null);
    setProgress({ current: 0, total: selectedFiles.length, percentage: 0 });
    setAppStatus('شروع رمزگشایی...');
    
    try {
      // Initialize processor
      await imageProcessor.initialize();
      
      const results = [];
      
      for (let i = 0; i < selectedFiles.length; i++) {
        const file = selectedFiles[i];
        
        // Update progress
        setProgress({
          current: i + 1,
          total: selectedFiles.length,
          percentage: Math.round(((i + 1) / selectedFiles.length) * 100),
          currentFile: file.name
        });
        
        setAppStatus(`در حال رمزگشایی: ${file.name}`);
        
        try {
          // Decrypt the image
          const result = await imageProcessor.decryptImage(file.path, {
            saveToFile: saveToFile,
            outputDirectory: outputDirectory
          });
          
          if (result.success) {
            if (result.buffer) {
              // Create blob URL for display
              const blob = new Blob([result.buffer], { type: result.mimeType || 'image/jpeg' });
              const url = URL.createObjectURL(blob);
              
              results.push({
                name: file.name.replace('.enc', ''),
                url: url,
                size: result.buffer.byteLength,
                mimeType: result.mimeType,
                originalPath: file.path,
                savedPath: result.savedPath
              });
            }
          } else {
            console.error('Decryption failed for', file.name, result.error);
          }
        } catch (err) {
          console.error('Error decrypting file:', file.name, err);
        }
      }
      
      setDecryptedImages(results);
      
      if (results.length > 0) {
        setSuccess(`✅ ${results.length} فایل با موفقیت رمزگشایی شد`);
        setAppStatus('رمزگشایی کامل شد');
        
        if (results.length > 0) {
          setCurrentImage(results[0]);
        }
        
        // Show notification
        await window.electronAPI.showNotification({
          title: 'رمزگشایی کامل شد',
          body: `${results.length} فایل با موفقیت رمزگشایی شد`
        });
      } else {
        setError('هیچ فایلی رمزگشایی نشد');
        setAppStatus('رمزگشایی ناموفق');
      }
      
    } catch (err) {
      setError('خطا در رمزگشایی: ' + err.message);
      setAppStatus('خطا در رمزگشایی');
    } finally {
      setProcessing(false);
      setProgress(null);
    }
  };

  // Start decryption process
  const handleStartDecryption = () => {
    if (!tokenStatus.connected) {
      setShowPinDialog(true);
    } else {
      processDecryption();
    }
  };

  // Save decrypted image to file
  const handleSaveImage = async (image) => {
    try {
      // Convert blob URL back to base64 if needed
      let dataUrl = image.url;
      if (image.url.startsWith('blob:')) {
        // Fetch the blob and convert to data URL
        const response = await fetch(image.url);
        const blob = await response.blob();
        dataUrl = await new Promise((resolve) => {
          const reader = new FileReader();
          reader.onloadend = () => resolve(reader.result);
          reader.readAsDataURL(blob);
        });
      }
      
      const result = await window.electronAPI.saveDecryptedImage({
        data: dataUrl,
        name: image.name,
        outputDirectory: outputDirectory
      });
      
      if (result.success) {
        setSuccess(`✅ فایل در ${result.path} ذخیره شد`);
        
        // Open folder
        await window.electronAPI.revealInFolder(result.path);
      }
    } catch (err) {
      setError('خطا در ذخیره فایل: ' + err.message);
    }
  };

  // Clear selection
  const handleClearSelection = () => {
    setSelectedFiles([]);
    setDecryptedImages([]);
    setCurrentImage(null);
    setError(null);
    setSuccess(null);
    setOutputDirectory(null);
    setSaveToFile(false);
    setAppStatus('آماده برای کار');
  };

  // Format file size
  const formatFileSize = (bytes) => {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-green-900 to-slate-900 p-4 sm:p-6 lg:p-8">
      <div className="max-w-7xl mx-auto">
        {/* Header Card */}
        <div className="bg-white/10 backdrop-blur-sm rounded-2xl shadow-2xl p-6 mb-6 border border-white/20">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-bold text-white mb-2">🔓 رمزگشایی تصاویر</h1>
              <p className="text-slate-300">تصاویر رمزنگاری شده را مشاهده و ذخیره کنید</p>
            </div>
            <div className="text-center">
              <div className={`inline-flex items-center px-4 py-2 rounded-xl ${
                tokenStatus.connected 
                  ? 'bg-green-500/20 text-green-300 border border-green-500/50' 
                  : 'bg-red-500/20 text-red-300 border border-red-500/50'
              }`}>
                <span className="text-2xl mr-2">{tokenStatus.connected ? '🟢' : '🔴'}</span>
                <div className="text-left">
                  <div className="text-xs opacity-70">وضعیت توکن</div>
                  <div className="font-medium">{tokenStatus.connected ? 'متصل' : 'قطع'}</div>
                </div>
              </div>
            </div>
          </div>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Left Panel - Controls */}
          <div className="space-y-4">
            {/* File Selection Card */}
            <div className="bg-white/10 backdrop-blur-sm rounded-xl p-5 border border-white/20">
              <h2 className="text-lg font-semibold text-white mb-4 flex items-center">
                <span className="text-2xl mr-2">📁</span>
                انتخاب فایل‌ها
              </h2>
              
              <div className="space-y-3">
                <button
                  onClick={handleFileSelect}
                  className="w-full bg-gradient-to-r from-green-500 to-green-600 hover:from-green-600 hover:to-green-700 text-white font-medium py-3 px-4 rounded-lg transition-all duration-200 flex items-center justify-center gap-2 shadow-lg hover:shadow-xl"
                  disabled={processing}
                >
                  <span>📄</span>
                  <span>انتخاب فایل .enc</span>
                </button>
                
                <button
                  onClick={handleFolderSelect}
                  className="w-full bg-gradient-to-r from-purple-500 to-purple-600 hover:from-purple-600 hover:to-purple-700 text-white font-medium py-3 px-4 rounded-lg transition-all duration-200 flex items-center justify-center gap-2 shadow-lg hover:shadow-xl"
                  disabled={processing}
                >
                  <span>📂</span>
                  <span>انتخاب پوشه</span>
                </button>
                
                <button
                  onClick={handleSelectOutputDir}
                  className="w-full bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white font-medium py-3 px-4 rounded-lg transition-all duration-200 flex items-center justify-center gap-2 shadow-lg hover:shadow-xl"
                  disabled={processing}
                >
                  <span>💾</span>
                  <span>محل ذخیره</span>
                </button>
              </div>

              {/* Output Directory Display */}
              {outputDirectory && (
                <div className="mt-4 p-3 bg-blue-500/10 rounded-lg border border-blue-500/30">
                  <div className="flex items-start gap-2">
                    <span className="text-blue-400">📁</span>
                    <div className="flex-1">
                      <p className="text-xs text-blue-400 mb-1">محل ذخیره:</p>
                      <p className="text-sm text-white break-all">{outputDirectory}</p>
                    </div>
                  </div>
                </div>
              )}
            </div>

            {/* Selected Files */}
            {selectedFiles.length > 0 && (
              <div className="bg-white/10 backdrop-blur-sm rounded-xl p-5 border border-white/20">
                <div className="flex items-center justify-between mb-3">
                  <h3 className="text-white font-medium">فایل‌های انتخابی</h3>
                  <span className="bg-green-500/20 text-green-300 px-3 py-1 rounded-full text-sm">
                    {selectedFiles.length} فایل
                  </span>
                </div>
                
                <div className="space-y-2 max-h-48 overflow-y-auto">
                  {selectedFiles.slice(0, 5).map((file, index) => (
                    <div key={index} className="p-2 bg-black/20 rounded-lg">
                      <div className="flex items-center gap-2">
                        <span className="text-green-400">🔒</span>
                        <div className="flex-1 min-w-0">
                          <p className="text-sm text-white truncate">{file.name}</p>
                          <p className="text-xs text-slate-400">{formatFileSize(file.size)}</p>
                        </div>
                      </div>
                    </div>
                  ))}
                  {selectedFiles.length > 5 && (
                    <p className="text-center text-slate-400 text-sm">
                      و {selectedFiles.length - 5} فایل دیگر...
                    </p>
                  )}
                </div>

                <button
                  onClick={handleClearSelection}
                  className="w-full mt-3 bg-gray-600/50 hover:bg-gray-600/70 text-white py-2 rounded-lg transition-all duration-200"
                  disabled={processing}
                >
                  پاک کردن همه
                </button>
              </div>
            )}

            {/* Action Button */}
            <button
              onClick={handleStartDecryption}
              className={`w-full py-4 font-bold rounded-xl shadow-lg transition-all duration-200 ${
                processing || selectedFiles.length === 0
                  ? 'bg-gray-600/50 text-gray-400 cursor-not-allowed'
                  : 'bg-gradient-to-r from-green-500 to-emerald-600 hover:from-green-600 hover:to-emerald-700 text-white hover:shadow-xl transform hover:scale-105'
              }`}
              disabled={processing || selectedFiles.length === 0}
            >
              {processing ? (
                <span className="flex items-center justify-center gap-2">
                  <span className="animate-spin">⚙️</span>
                  در حال رمزگشایی...
                </span>
              ) : (
                <span className="flex items-center justify-center gap-2">
                  <span>🔓</span>
                  شروع رمزگشایی
                </span>
              )}
            </button>
          </div>

          {/* Right Panel - Results & Progress */}
          <div className="lg:col-span-2 space-y-4">
            {/* Status Bar */}
            <div className="bg-white/10 backdrop-blur-sm rounded-xl p-4 border border-white/20">
              <div className="flex items-center justify-between">
                <span className="text-slate-300 text-sm">وضعیت برنامه:</span>
                <span className="text-white font-medium">{appStatus}</span>
              </div>
            </div>

            {/* Messages */}
            {error && (
              <div className="bg-red-500/10 backdrop-blur-sm border border-red-500/30 text-red-300 px-5 py-4 rounded-xl flex items-start gap-3">
                <span className="text-xl">⚠️</span>
                <p className="flex-1">{error}</p>
              </div>
            )}
            
            {success && (
              <div className="bg-green-500/10 backdrop-blur-sm border border-green-500/30 text-green-300 px-5 py-4 rounded-xl flex items-start gap-3">
                <span className="text-xl">✅</span>
                <p className="flex-1">{success}</p>
              </div>
            )}

            {/* Progress */}
            {progress && (
              <div className="bg-white/10 backdrop-blur-sm rounded-xl p-6 border border-white/20">
                <h3 className="text-white font-semibold mb-4 flex items-center gap-2">
                  <span className="animate-spin">⚙️</span>
                  در حال پردازش...
                </h3>
                
                <div className="space-y-4">
                  <div>
                    <div className="flex justify-between text-sm text-slate-300 mb-2">
                      <span>فایل {progress.current} از {progress.total}</span>
                      <span>{progress.percentage}%</span>
                    </div>
                    <div className="w-full bg-black/30 rounded-full h-3 overflow-hidden">
                      <div
                        className="h-full bg-gradient-to-r from-green-500 to-emerald-500 rounded-full transition-all duration-300"
                        style={{ width: `${progress.percentage}%` }}
                      />
                    </div>
                  </div>
                  
                  <div className="pt-3 border-t border-white/10">
                    <p className="text-sm text-slate-300">
                      <span className="text-white">فایل جاری:</span> {progress.currentFile}
                    </p>
                  </div>
                </div>
              </div>
            )}

            {/* Decrypted Images Display */}
            {decryptedImages.length > 0 && (
              <div className="bg-white/10 backdrop-blur-sm rounded-xl p-6 border border-white/20">
                <div className="flex items-center justify-between mb-4">
                  <h3 className="text-white font-semibold flex items-center gap-2">
                    <span>🖼️</span>
                    تصاویر رمزگشایی شده
                  </h3>
                  <span className="bg-green-500/20 text-green-300 px-3 py-1 rounded-full text-sm">
                    {decryptedImages.length} تصویر
                  </span>
                </div>
                
                {/* Current Image Display */}
                {currentImage && (
                  <div className="space-y-4">
                    {/* Image Preview */}
                    <div className="bg-black/30 rounded-xl p-4 border border-white/10">
                      <img 
                        src={currentImage.url} 
                        alt={currentImage.name}
                        className="w-full h-auto max-h-[400px] object-contain rounded-lg"
                      />
                    </div>
                    
                    {/* Image Info */}
                    <div className="flex items-center justify-between p-3 bg-black/20 rounded-lg border border-white/10">
                      <div>
                        <p className="text-white font-medium">{currentImage.name}</p>
                        <p className="text-sm text-slate-400">
                          {formatFileSize(currentImage.size)} • {currentImage.mimeType}
                        </p>
                      </div>
                      <button
                        onClick={() => handleSaveImage(currentImage)}
                        className="px-4 py-2 bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white rounded-lg transition-all duration-200 flex items-center gap-2 shadow-lg hover:shadow-xl"
                      >
                        <span>💾</span>
                        <span>ذخیره</span>
                      </button>
                    </div>
                    
                    {/* Thumbnail Gallery */}
                    {decryptedImages.length > 1 && (
                      <div>
                        <p className="text-sm text-slate-300 mb-3">گالری تصاویر:</p>
                        <div className="grid grid-cols-4 sm:grid-cols-6 gap-2">
                          {decryptedImages.map((img, index) => (
                            <button
                              key={index}
                              onClick={() => setCurrentImage(img)}
                              className={`relative aspect-square rounded-lg overflow-hidden border-2 transition-all ${
                                currentImage?.name === img.name 
                                  ? 'border-green-500 shadow-lg scale-105' 
                                  : 'border-transparent hover:border-gray-500'
                              }`}
                            >
                              <img 
                                src={img.url} 
                                alt={img.name}
                                className="w-full h-full object-cover"
                              />
                              {currentImage?.name === img.name && (
                                <div className="absolute inset-0 bg-green-500/20 flex items-center justify-center">
                                  <span className="text-white text-2xl">✓</span>
                                </div>
                              )}
                            </button>
                          ))}
                        </div>
                      </div>
                    )}

                    {/* Save All Button */}
                    {outputDirectory && decryptedImages.length > 1 && (
                      <button
                        onClick={async () => {
                          for (const img of decryptedImages) {
                            await handleSaveImage(img);
                          }
                        }}
                        className="w-full bg-green-500/20 hover:bg-green-500/30 text-green-300 py-2.5 rounded-lg transition-all duration-200 flex items-center justify-center gap-2"
                      >
                        <span>💾</span>
                        ذخیره همه تصاویر
                      </button>
                    )}
                  </div>
                )}
              </div>
            )}
          </div>
        </div>

        {/* PIN Dialog */}
        {showPinDialog && (
          <div className="fixed inset-0 bg-black/70 backdrop-blur-sm flex items-center justify-center z-50 p-4">
            <div className="bg-slate-800/90 backdrop-blur rounded-2xl p-6 w-full max-w-md border border-white/20 shadow-2xl">
              <h3 className="text-xl font-bold text-white mb-4 flex items-center gap-2">
                <span>🔑</span>
                ورود پین توکن
              </h3>
              <input
                type="password"
                value={pin}
                onChange={(e) => setPin(e.target.value)}
                placeholder="پین توکن را وارد کنید"
                className="w-full px-4 py-3 bg-black/30 border border-white/20 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:border-green-500 transition-colors"
                autoFocus
              />
              <div className="flex gap-3 mt-6">
                <button
                  onClick={handlePinSubmit}
                  className="flex-1 bg-gradient-to-r from-green-500 to-emerald-600 hover:from-green-600 hover:to-emerald-700 text-white font-medium py-2.5 rounded-lg transition-all duration-200"
                >
                  تایید
                </button>
                <button
                  onClick={() => {
                    setShowPinDialog(false);
                    setPin('');
                  }}
                  className="flex-1 bg-gray-600/50 hover:bg-gray-600/70 text-white font-medium py-2.5 rounded-lg transition-all duration-200"
                >
                  انصراف
                </button>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default DecryptionPage;