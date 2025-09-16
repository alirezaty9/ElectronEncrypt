import React, { useState } from "react";
import { NavLink } from "react-router-dom";

export default function Navbar() {
  const [isMenuOpen, setIsMenuOpen] = useState(false);

  const navItems = [
    {
      path: "/",
      icon: "🏠",
      label: "صفحه اصلی",
      description: "خانه"
    },
    {
      path: "/encrypt",
      icon: "🔐",
      label: "رمزنگاری",
      description: "رمزنگاری تصاویر"
    },
    {
      path: "/decrypt",
      icon: "🔓",
      label: "رمزگشایی",
      description: "رمزگشایی تصاویر"
    }
  ];

  return (
    <nav className="fixed top-0 left-0 right-0 z-50 backdrop-blur-lg border-b border-white/10">
      <div className="absolute inset-0 bg-gradient-to-r from-slate-900/95 via-blue-900/95 to-slate-900/95"></div>
      
      <div className="relative max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          
          {/* Logo Section */}
          <div className="flex items-center gap-3">
            <div className="relative group cursor-pointer">
              <div className="absolute -inset-1 bg-gradient-to-r from-blue-500 to-cyan-500 rounded-lg blur opacity-25 group-hover:opacity-50 transition duration-200"></div>
              <div className="relative w-10 h-10 bg-gradient-to-br from-blue-500 to-cyan-600 rounded-lg flex items-center justify-center shadow-lg">
                <span className="text-white font-bold text-lg">🔒</span>
              </div>
            </div>
            <div className="hidden md:block">
              <h1 className="text-lg font-bold text-white">
                رمزنگار تصاویر
              </h1>
              <p className="text-xs text-slate-400">
                Image Encryption Tool
              </p>
            </div>
          </div>

          {/* Desktop Menu */}
          <div className="hidden md:flex items-center space-x-2 space-x-reverse">
            {navItems.map((item) => (
              <NavLink
                key={item.path}
                to={item.path}
                className={({ isActive }) =>
                  `group relative flex items-center gap-2 px-4 py-2 rounded-lg transition-all duration-200 ${
                    isActive
                      ? 'bg-blue-500/20 text-white border border-blue-500/50'
                      : 'text-slate-300 hover:text-white hover:bg-white/5 border border-transparent'
                  }`
                }
              >
                <span className="text-xl">{item.icon}</span>
                <div className="flex flex-col">
                  <span className="font-medium text-sm">{item.label}</span>
                  <span className="text-xs opacity-70">{item.description}</span>
                </div>
              </NavLink>
            ))}
          </div>

          {/* Mobile Menu Button */}
          <div className="md:hidden">
            <button
              onClick={() => setIsMenuOpen(!isMenuOpen)}
              className="p-2 rounded-lg text-slate-400 hover:text-white hover:bg-white/10 transition-all duration-200"
            >
              <svg className="w-6 h-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path 
                  strokeLinecap="round" 
                  strokeLinejoin="round" 
                  strokeWidth={2} 
                  d={isMenuOpen ? "M6 18L18 6M6 6l12 12" : "M4 6h16M4 12h16M4 18h16"}
                />
              </svg>
            </button>
          </div>
        </div>
      </div>

      {/* Mobile Menu */}
      <div className={`md:hidden transition-all duration-300 ${
        isMenuOpen ? 'max-h-64 opacity-100' : 'max-h-0 opacity-0 overflow-hidden'
      }`}>
        <div className="relative px-4 py-3 space-y-2 bg-slate-900/98 backdrop-blur-xl border-t border-white/10">
          {navItems.map((item) => (
            <NavLink
              key={item.path}
              to={item.path}
              onClick={() => setIsMenuOpen(false)}
              className={({ isActive }) =>
                `flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 ${
                  isActive
                    ? 'bg-blue-500/20 text-white border border-blue-500/50'
                    : 'text-slate-300 hover:text-white hover:bg-white/5 border border-transparent'
                }`
              }
            >
              <span className="text-xl">{item.icon}</span>
              <div>
                <div className="font-medium">{item.label}</div>
                <div className="text-sm opacity-70">{item.description}</div>
              </div>
            </NavLink>
          ))}
        </div>
      </div>
    </nav>
  );
}