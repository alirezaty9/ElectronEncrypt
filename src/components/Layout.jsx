import React from "react";
import Navbar from "./Navbar";
import { Outlet } from "react-router-dom";

export default function Layout() {
  return (
    <>
      <Navbar />
      
      {/* Compact spacer for fixed navbar */}
      <div className="h-20"></div>
      
      <main className="relative min-h-screen">
        <div className="relative z-10 py-4">
          <Outlet />
        </div>
      </main>
    </>
  );
}