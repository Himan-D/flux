import React from 'react';
import { Activity } from 'lucide-react';

export const VolSurface3D: React.FC = () => {
  const strikes = ['0.10Δ', '0.25Δ', '0.50Δ (ATM)', '0.75Δ', '0.90Δ'];
  const tenors = ['M01 (Prompt)', 'M02', 'M03', 'M06', 'M12'];
  const surface = [
    [0.34, 0.30, 0.28, 0.27, 0.29],
    [0.32, 0.28, 0.26, 0.25, 0.27],
    [0.30, 0.26, 0.25, 0.24, 0.26],
    [0.28, 0.25, 0.24, 0.23, 0.25],
    [0.27, 0.24, 0.23, 0.22, 0.24],
  ];

  return (
    <div className="bg-fluxPanel border border-fluxBorder rounded-lg p-5 flex flex-col h-full">
      <div className="flex items-center justify-between border-b border-fluxBorder pb-3 mb-4">
        <div className="flex items-center space-x-2">
          <Activity className="w-4 h-4 text-emerald-400" />
          <h2 className="font-semibold text-sm tracking-wide text-white uppercase">SABR Calibrated Implied Volatility Surface</h2>
        </div>
        <span className="text-xs bg-blue-950 text-blue-400 border border-blue-800 px-2 py-0.5 rounded">
          SABR / MONOTONE CONVEX
        </span>
      </div>

      <div className="overflow-x-auto">
        <table className="w-full text-center text-xs">
          <thead className="bg-fluxDark text-gray-400 border-b border-fluxBorder">
            <tr>
              <th className="p-2 text-left">TENOR</th>
              {strikes.map((s, idx) => (
                <th key={idx} className="p-2">{s}</th>
              ))}
            </tr>
          </thead>
          <tbody className="divide-y divide-fluxBorder text-gray-300">
            {tenors.map((t, rowIdx) => (
              <tr key={rowIdx} className="hover:bg-fluxDark/40">
                <td className="p-2 text-left font-bold text-gray-200">{t}</td>
                {surface[rowIdx].map((vol, colIdx) => (
                  <td key={colIdx} className="p-2">
                    <span className="px-2 py-1 bg-blue-950/40 text-blue-300 rounded border border-blue-900/30">
                      {(vol * 100).toFixed(1)}%
                    </span>
                  </td>
                ))}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
};
