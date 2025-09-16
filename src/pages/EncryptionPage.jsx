import React, { useState, useEffect, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import imageProcessor from '../services/imageProcessor';
import tokenEncryption from '../services/tokenEncryption';

const EncryptionPage = () => {
  const navigate = useNavigate();
  const [selectedFiles, setSelectedFiles] = useState([]);
  const [encryptedFiles, setEncryptedFiles] = useState([]);
  const [processing, setProcessing] = useState(false);
  const [progress, setProgress] = useState(null);
  const [tokenStatus, setTokenStatus] = useState({ connected: false });
  const [showPinDialog, setShowPinDialog] = useState(false);
  const [pin, setPin] = useState('');
  const [error, setError] = useState(null);
  const [success, setSuccess] = useState(null);
  const [outputDirectory, setOutputDirectory] = useState(null);
  const [appStatus, setAppStatus] = useState('آماده برای کار');
  const [estimatedTime, setEstimatedTime] = useState(null);
  const [startTime, setStartTime] = useState(null);

  // Initialize token on component mount
  useEffect(() => {
    checkTokenStatus();
  }, []);

  // Check token connection status
  const checkTokenStatus = async () => {
    const status = await tokenEncryption.checkStatus();
    console.log('Token check status result:', status);
    setTokenStatus(status);
    
    // Update connected flag if token is initialized
    if (status?.success || status?.data?.isInitialized) {
      setTokenStatus({ ...status, connected: true });
    } else if (!status?.data?.isInitialized) {
      setError('توکن سخت‌افزاری متصل نیست. لطفاً توکن را متصل کنید.');
    }
  };

  // Handle output directory selection
  const handleSelectOutputDir = async () => {
    try {
      setAppStatus('در حال انتخاب پوشه...');
      const result = await window.electronAPI.selectOutputDirectory();
      
      if (result.success) {
        setOutputDirectory(result.path);
        setAppStatus('پوشه خروجی انتخاب شد');
        setSuccess(`📁 فایل‌ها در ${result.path} ذخیره خواهند شد`);
      }
    } catch (err) {
      setError('خطا در انتخاب پوشه: ' + err.message);
    }
  };

  // Handle file selection
  const handleFileSelect = async () => {
    try {
      setAppStatus('در حال انتخاب فایل‌ها...');
      const result = await window.electronAPI.selectImageFiles();
      
      if (result.success && result.files) {
        setSelectedFiles(result.files);
        setError(null);
        setAppStatus(`${result.files.length} فایل انتخاب شد`);
        
        // Calculate estimated time
        const totalSize = result.files.reduce((sum, file) => sum + file.size, 0);
        const estimatedSeconds = Math.ceil(totalSize / (1024 * 1024 * 5));
        setEstimatedTime(estimatedSeconds);
      }
    } catch (err) {
      setError('خطا در انتخاب فایل‌ها: ' + err.message);
    }
  };

  // Handle folder selection
  const handleFolderSelect = async () => {
    try {
      setAppStatus('در حال انتخاب پوشه...');
      const result = await window.electronAPI.selectFolder();
      
      if (result.success) {
        const files = await window.electronAPI.getImagesFromFolder(result.path);
        if (files && files.length > 0) {
          setSelectedFiles(files);
          setError(null);
          setAppStatus(`${files.length} تصویر از پوشه انتخاب شد`);
        } else {
          setError('هیچ تصویری در این پوشه یافت نشد');
        }
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
        await processEncryption();
      } else {
        setError('پین اشتباه است. لطفاً دوباره تلاش کنید.');
      }
    } catch (err) {
      setError('خطا در ورود به توکن: ' + err.message);
    }
  };

  // Process encryption
  const processEncryption = async () => {
    if (selectedFiles.length === 0) {
      setError('لطفاً ابتدا فایل‌هایی را انتخاب کنید.');
      return;
    }

    setProcessing(true);
    setError(null);
    setSuccess(null);
    setStartTime(Date.now());
    setAppStatus('شروع فرآیند رمزنگاری...');
    
    // Notify main process
    await window.electronAPI.setProcessingState(true);
    
    try {
      // Initialize processor
      await imageProcessor.initialize();
      setAppStatus('شروع رمزنگاری فایل‌ها...');
      
      // Process files
      const results = await imageProcessor.encryptBatch(
        selectedFiles.map(f => f.path),
        {
          deleteOriginal: false,
          outputDirectory: outputDirectory,
          onProgress: (prog) => {
            setProgress(prog);
            
            // Update time estimation
            if (startTime && prog.percentage > 0) {
              const elapsed = (Date.now() - startTime) / 1000;
              const totalEstimated = (elapsed / prog.percentage) * 100;
              const remaining = Math.ceil(totalEstimated - elapsed);
              setEstimatedTime(remaining);
              setAppStatus(`در حال رمزنگاری: ${prog.currentFile}`);
            }
          }
        }
      );
      
      setEncryptedFiles(results.results.filter(r => r.success));
      
      // Show notification
      const notificationBody = results.failed === 0
        ? `همه ${results.successful} فایل با موفقیت رمزنگاری شدند`
        : `${results.successful} فایل موفق، ${results.failed} فایل ناموفق`;
        
      await window.electronAPI.showNotification({
        title: 'رمزنگاری کامل شد',
        body: notificationBody
      });
      
      if (results.successful > 0) {
        setSuccess(`✅ ${results.successful} فایل با موفقیت رمزنگاری شد`);
        setAppStatus('رمزنگاری با موفقیت کامل شد');
      }
      
      if (results.failed > 0) {
        setError(`❌ ${results.failed} فایل رمزنگاری نشد`);
      }
      
    } catch (err) {
      setError('خطا در رمزنگاری: ' + err.message);
      setAppStatus('خطا در رمزنگاری');
    } finally {
      setProcessing(false);
      setProgress(null);
      setEstimatedTime(null);
      setStartTime(null);
      
      // Notify main process
      await window.electronAPI.setProcessingState(false);
      
      if (!error) {
        setAppStatus('آماده برای کار');
      }
    }
  };

  // Start encryption
  const handleStartEncryption = () => {
    if (!tokenStatus.connected) {
      setShowPinDialog(true);
    } else {
      processEncryption();
    }
  };

  // Clear selection
  const handleClearSelection = () => {
    setSelectedFiles([]);
    setEncryptedFiles([]);
    setError(null);
    setSuccess(null);
    setOutputDirectory(null);
    setAppStatus('آماده برای کار');
  };
  
  // Open file in folder
  const handleRevealInFolder = async (filePath) => {
    try {
      await window.electronAPI.revealInFolder(filePath);
    } catch (err) {
      console.error('Error revealing file:', err);
    }
  };

  // Remove a file from selection
  const removeFile = (index) => {
    setSelectedFiles(files => files.filter((_, i) => i !== index));
  };

  // Format file size
  const formatFileSize = (bytes) => {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-slate-900 p-4 sm:p-6 lg:p-8">
      <div className="max-w-7xl mx-auto">
        {/* Header Card */}
        <div className="bg-white/10 backdrop-blur-sm rounded-2xl shadow-2xl p-6 mb-6 border border-white/20">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-bold text-white mb-2">🔐 رمزنگاری تصاویر</h1>
              <p className="text-slate-300">تصاویر خود را با توکن سخت‌افزاری رمزنگاری کنید</p>
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
                  className="w-full bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white font-medium py-3 px-4 rounded-lg transition-all duration-200 flex items-center justify-center gap-2 shadow-lg hover:shadow-xl"
                  disabled={processing}
                >
                  <span>🖼️</span>
                  <span>انتخاب تصاویر</span>
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
                  className="w-full bg-gradient-to-r from-teal-500 to-teal-600 hover:from-teal-600 hover:to-teal-700 text-white font-medium py-3 px-4 rounded-lg transition-all duration-200 flex items-center justify-center gap-2 shadow-lg hover:shadow-xl"
                  disabled={processing}
                >
                  <span>💾</span>
                  <span>محل ذخیره</span>
                </button>
              </div>

              {/* Output Directory Display */}
              {outputDirectory && (
                <div className="mt-4 p-3 bg-green-500/10 rounded-lg border border-green-500/30">
                  <div className="flex items-start gap-2">
                    <span className="text-green-400">📁</span>
                    <div className="flex-1">
                      <p className="text-xs text-green-400 mb-1">محل ذخیره:</p>
                      <p className="text-sm text-white break-all">{outputDirectory}</p>
                    </div>
                  </div>
                </div>
              )}
            </div>

            {/* Selected Files Count */}
            {selectedFiles.length > 0 && (
              <div className="bg-white/10 backdrop-blur-sm rounded-xl p-5 border border-white/20">
                <div className="flex items-center justify-between mb-3">
                  <h3 className="text-white font-medium">فایل‌های انتخابی</h3>
                  <span className="bg-blue-500/20 text-blue-300 px-3 py-1 rounded-full text-sm">
                    {selectedFiles.length} فایل
                  </span>
                </div>
                
                <div className="space-y-2 max-h-48 overflow-y-auto">
                  {selectedFiles.slice(0, 5).map((file, index) => (
                    <div key={index} className="flex items-center justify-between p-2 bg-black/20 rounded-lg">
                      <div className="flex items-center gap-2 flex-1 min-w-0">
                        <span className="text-blue-400">📄</span>
                        <div className="flex-1 min-w-0">
                          <p className="text-sm text-white truncate">{file.name}</p>
                          <p className="text-xs text-slate-400">{formatFileSize(file.size)}</p>
                        </div>
                      </div>
                      <button
                        onClick={() => removeFile(index)}
                        className="text-red-400 hover:text-red-300 p-1"
                      >
                        ✕
                      </button>
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
              onClick={handleStartEncryption}
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
                  در حال رمزنگاری...
                </span>
              ) : (
                <span className="flex items-center justify-center gap-2">
                  <span>🔐</span>
                  شروع رمزنگاری
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
                        className="h-full bg-gradient-to-r from-blue-500 to-cyan-500 rounded-full transition-all duration-300"
                        style={{ width: `${progress.percentage}%` }}
                      />
                    </div>
                  </div>
                  
                  <div className="pt-3 border-t border-white/10">
                    <p className="text-sm text-slate-300 mb-1">
                      <span className="text-white">فایل جاری:</span> {progress.currentFile}
                    </p>
                    {estimatedTime && estimatedTime > 0 && (
                      <p className="text-sm text-slate-300">
                        <span className="text-white">زمان باقیمانده:</span> {
                          estimatedTime > 60 
                            ? `${Math.floor(estimatedTime / 60)} دقیقه`
                            : `${estimatedTime} ثانیه`
                        }
                      </p>
                    )}
                  </div>
                </div>
              </div>
            )}

            {/* Encrypted Files Results */}
            {encryptedFiles.length > 0 && (
              <div className="bg-white/10 backdrop-blur-sm rounded-xl p-6 border border-white/20">
                <h3 className="text-white font-semibold mb-4 flex items-center justify-between">
                  <span className="flex items-center gap-2">
                    <span>✅</span>
                    فایل‌های رمزنگاری شده
                  </span>
                  <span className="bg-green-500/20 text-green-300 px-3 py-1 rounded-full text-sm">
                    {encryptedFiles.length} فایل
                  </span>
                </h3>
                
                <div className="space-y-2 max-h-64 overflow-y-auto">
                  {encryptedFiles.map((file, index) => (
                    <div key={index} className="flex items-center justify-between p-3 bg-green-500/10 rounded-lg border border-green-500/20">
                      <div className="flex items-center gap-3 flex-1 min-w-0">
                        <span className="text-green-400 text-xl">🔒</span>
                        <div className="flex-1 min-w-0">
                          <p className="text-sm text-white font-medium truncate">
                            {file.metadata?.originalPath?.split('/').pop() || 'فایل'}
                          </p>
                          <p className="text-xs text-slate-400 truncate">
                            {file.outputPath}
                          </p>
                        </div>
                      </div>
                      <button
                        onClick={() => handleRevealInFolder(file.outputPath)}
                        className="px-3 py-1.5 bg-blue-500/20 hover:bg-blue-500/30 text-blue-300 rounded-lg transition-all duration-200 flex items-center gap-1 text-sm"
                      >
                        <span>📂</span>
                        نمایش
                      </button>
                    </div>
                  ))}
                </div>
                
                {outputDirectory && (
                  <button
                    onClick={() => handleRevealInFolder(outputDirectory)}
                    className="w-full mt-4 bg-green-500/20 hover:bg-green-500/30 text-green-300 py-2.5 rounded-lg transition-all duration-200 flex items-center justify-center gap-2"
                  >
                    <span>📂</span>
                    باز کردن پوشه خروجی
                  </button>
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
                className="w-full px-4 py-3 bg-black/30 border border-white/20 rounded-lg text-white placeholder-gray-400 focus:outline-none focus:border-blue-500 transition-colors"
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

export default EncryptionPage;