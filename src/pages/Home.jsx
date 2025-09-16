import React from 'react';
import { Link } from 'react-router-dom';

const Home = () => {
  const features = [
    {
      icon: '🔐',
      title: 'رمزنگاری امن',
      description: 'رمزنگاری AES-256 با کلید محافظت شده توسط توکن سخت‌افزاری'
    },
    {
      icon: '🔑',
      title: 'توکن سخت‌افزاری',
      description: 'استفاده از توکن USB برای امنیت بالا'
    },
    {
      icon: '💾',
      title: 'ذخیره‌سازی محلی',
      description: 'فایل‌های شما هرگز از سیستم خارج نمی‌شوند'
    },
    {
      icon: '⚡',
      title: 'سرعت بالا',
      description: 'پردازش سریع با رمزنگاری بخشی'
    }
  ];

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-slate-900">
      {/* Hero Section */}
      <div className="relative overflow-hidden">
        <div className="absolute inset-0">
          <div className="absolute inset-0 bg-gradient-to-br from-blue-500/20 via-transparent to-cyan-500/20"></div>
          <div className="absolute inset-0 opacity-10">
            <div className="h-full w-full" style={{
              backgroundImage: `repeating-linear-gradient(45deg, transparent, transparent 35px, rgba(255,255,255,.1) 35px, rgba(255,255,255,.1) 70px)`
            }}></div>
          </div>
        </div>
        
        <div className="relative max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 pt-32 pb-20">
          <div className="text-center">
            <div className="inline-flex items-center justify-center w-24 h-24 mb-8 bg-gradient-to-br from-blue-500 to-cyan-600 rounded-3xl shadow-2xl animate-pulse">
              <span className="text-5xl">🔒</span>
            </div>
            
            <h1 className="text-5xl md:text-6xl font-bold text-white mb-6">
              رمزنگار 
              <span className="bg-gradient-to-r from-blue-400 to-cyan-400 bg-clip-text text-transparent"> تصاویر</span>
            </h1>
            
            <p className="text-xl text-slate-300 mb-12 max-w-2xl mx-auto">
              تصاویر خود را با استفاده از توکن سخت‌افزاری و رمزنگاری پیشرفته محافظت کنید
            </p>
            
            <div className="flex flex-col sm:flex-row gap-4 justify-center">
              <Link to="/encrypt">
                <button className="px-8 py-4 bg-gradient-to-r from-blue-500 to-cyan-600 text-white font-bold rounded-xl shadow-lg hover:shadow-2xl transition-all duration-200 flex items-center gap-3 hover:scale-105">
                  <span className="text-2xl">🔐</span>
                  <span>رمزنگاری تصاویر</span>
                </button>
              </Link>
              
              <Link to="/decrypt">
                <button className="px-8 py-4 bg-white/10 backdrop-blur-sm text-white font-bold rounded-xl shadow-lg hover:shadow-2xl hover:bg-white/20 transition-all duration-200 border border-white/20 flex items-center gap-3 hover:scale-105">
                  <span className="text-2xl">🔓</span>
                  <span>رمزگشایی تصاویر</span>
                </button>
              </Link>
            </div>
          </div>
        </div>
      </div>

      {/* Features Section */}
      <div className="relative py-20 px-4 sm:px-6 lg:px-8">
        <div className="max-w-6xl mx-auto">
          <div className="text-center mb-12">
            <h2 className="text-3xl font-bold text-white mb-4">ویژگی‌های کلیدی</h2>
            <p className="text-slate-400">امنیت و سرعت در یک برنامه</p>
          </div>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
            {features.map((feature, index) => (
              <div
                key={index}
                className="bg-white/5 backdrop-blur-sm rounded-xl p-6 border border-white/10 hover:border-blue-500/50 transition-all duration-200 hover:scale-105"
              >
                <div className="text-4xl mb-4">{feature.icon}</div>
                <h3 className="text-lg font-bold text-white mb-2">{feature.title}</h3>
                <p className="text-sm text-slate-400">{feature.description}</p>
              </div>
            ))}
          </div>
        </div>
      </div>

      {/* How It Works Section */}
      <div className="relative py-20 px-4 sm:px-6 lg:px-8">
        <div className="max-w-4xl mx-auto">
          <div className="bg-gradient-to-br from-blue-500/10 to-cyan-500/10 backdrop-blur-sm rounded-2xl p-8 border border-white/10">
            <h2 className="text-2xl font-bold text-white mb-6 text-center">نحوه کار</h2>
            
            <div className="space-y-6">
              <div className="flex items-start gap-4">
                <div className="w-10 h-10 bg-blue-500 rounded-full flex items-center justify-center text-white font-bold flex-shrink-0">1</div>
                <div>
                  <h3 className="font-bold text-white mb-1">اتصال توکن</h3>
                  <p className="text-slate-400 text-sm">توکن USB خود را به سیستم متصل کنید</p>
                </div>
              </div>
              
              <div className="flex items-start gap-4">
                <div className="w-10 h-10 bg-blue-500 rounded-full flex items-center justify-center text-white font-bold flex-shrink-0">2</div>
                <div>
                  <h3 className="font-bold text-white mb-1">انتخاب تصاویر</h3>
                  <p className="text-slate-400 text-sm">تصاویر مورد نظر را برای رمزنگاری انتخاب کنید</p>
                </div>
              </div>
              
              <div className="flex items-start gap-4">
                <div className="w-10 h-10 bg-blue-500 rounded-full flex items-center justify-center text-white font-bold flex-shrink-0">3</div>
                <div>
                  <h3 className="font-bold text-white mb-1">رمزنگاری امن</h3>
                  <p className="text-slate-400 text-sm">تصاویر با AES-256 رمزنگاری و کلید در توکن محافظت می‌شود</p>
                </div>
              </div>
              
              <div className="flex items-start gap-4">
                <div className="w-10 h-10 bg-blue-500 rounded-full flex items-center justify-center text-white font-bold flex-shrink-0">4</div>
                <div>
                  <h3 className="font-bold text-white mb-1">ذخیره‌سازی</h3>
                  <p className="text-slate-400 text-sm">فایل‌های رمزنگاری شده در محل دلخواه شما ذخیره می‌شوند</p>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* CTA Section */}
      <div className="relative py-20 px-4 sm:px-6 lg:px-8">
        <div className="max-w-4xl mx-auto text-center">
          <div className="bg-gradient-to-r from-blue-600 to-cyan-600 rounded-2xl p-12 shadow-2xl">
            <h2 className="text-3xl font-bold text-white mb-4">
              همین حالا شروع کنید
            </h2>
            <p className="text-white/90 mb-8 text-lg">
              تصاویر مهم خود را با بالاترین سطح امنیت محافظت کنید
            </p>
            <Link to="/encrypt">
              <button className="px-8 py-4 bg-white text-blue-600 font-bold rounded-xl shadow-lg hover:shadow-2xl transition-all duration-200 hover:scale-105">
                شروع رمزنگاری
              </button>
            </Link>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Home;