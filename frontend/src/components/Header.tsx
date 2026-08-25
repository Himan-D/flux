import React from 'react';
import { Activity, ShieldCheck, Zap, Server } from 'lucide-react';

export const Header: React.FC = () => {
  return (
    <header className="flex items-center justify-between px-6 py-3 bg-fluxPanel border-b border-fluxBorder">
      <div className="flex items-center space-x-4">
        <div className="flex items-center space-x-2">
          <div className="w-3 h-3 rounded-full bg-blue-500 animate-pulse"></div>
          <span className="text-xl font-bold tracking-wider text-white">FLUX</span>
          <span className="text-xs bg-blue-900/60 text-blue-400 border border-blue-700/50 px-2 py-0.5 rounded">
            OTC DERIVATIVES SaaS
          </span>
        </div>
        <div className="hidden md:flex items-center space-x-3 text-xs text-gray-400 border-l border-fluxBorder pl-4">
          <span>TENANT: <strong className="text-gray-200">GLENCORE_ENERGY_LTD</strong></span>
          <span>DESK: <strong className="text-gray-200">OIL_DERIVATIVES_LONDON</strong></span>
        </div>
      </div>

      <div className="flex items-center space-x-6 text-xs">
        <div className="flex items-center space-x-1.5 text-emerald-400">
          <Zap className="w-3.5 h-3.5" />
          <span>FAST-PATH: <strong>2.45 µs</strong></span>
        </div>
        <div className="flex items-center space-x-1.5 text-blue-400">
          <Server className="w-3.5 h-3.5" />
          <span>AERON RAFT: <strong>QUORUM (3 NODES)</strong></span>
        </div>
        <div className="flex items-center space-x-1.5 text-purple-400">
          <ShieldCheck className="w-3.5 h-3.5" />
          <span>RLS TENANT SECURE</span>
        </div>
      </div>
    </header>
  );
};
