import React from 'react';
import { PositionRisk } from '../types';
import { Layers } from 'lucide-react';

export const RiskDashboard: React.FC = () => {
  const positions: PositionRisk[] = [
    { desk: 'DESK_CRUDE_LONDON', asset: 'ICE Brent', netDeltaBbl: 100000, vegaUsd: 45000, var99Usd: 111194 },
    { desk: 'DESK_DISTILLATES_GENEVA', asset: 'Low Sulphur Gasoil', netDeltaBbl: -60000, vegaUsd: 20000, var99Usd: 42000 },
    { desk: 'DESK_FUELOIL_SINGAPORE', asset: '380cst Fuel Oil', netDeltaBbl: -15000, vegaUsd: 8000, var99Usd: 18500 },
    { desk: 'DESK_LIGHTENDS_HOUSTON', asset: 'NYMEX WTI', netDeltaBbl: 40000, vegaUsd: 18000, var99Usd: 38000 },
  ];

  return (
    <div className="bg-fluxPanel border border-fluxBorder rounded-lg p-5 flex flex-col h-full">
      <div className="flex items-center justify-between border-b border-fluxBorder pb-3 mb-4">
        <div className="flex items-center space-x-2">
          <Layers className="w-4 h-4 text-purple-400" />
          <h2 className="font-semibold text-sm tracking-wide text-white uppercase">Central Risk Book (CRB) & Cross-Desk Netting</h2>
        </div>
        <span className="text-xs bg-purple-950 text-purple-400 border border-purple-800 px-2 py-0.5 rounded">
          75,000 BBL INTERNALIZED ($0 SLIPPAGE)
        </span>
      </div>

      <div className="grid grid-cols-3 gap-3 mb-4 text-xs">
        <div className="bg-fluxDark p-3 rounded border border-fluxBorder">
          <span className="text-gray-400 block mb-1">99% 1-DAY HISTORICAL VaR</span>
          <span className="text-lg font-bold text-red-400">$111,194.16</span>
          <span className="text-[10px] text-gray-500 block">500 Full-Revaluation Days</span>
        </div>
        <div className="bg-fluxDark p-3 rounded border border-fluxBorder">
          <span className="text-gray-400 block mb-1">EXPECTED SHORTFALL (97.5%)</span>
          <span className="text-lg font-bold text-amber-400">$125,001.61</span>
          <span className="text-[10px] text-gray-500 block">FRTB Tail Risk Standard</span>
        </div>
        <div className="bg-fluxDark p-3 rounded border border-fluxBorder">
          <span className="text-gray-400 block mb-1">NET RESIDUAL CRUDE DELTA</span>
          <span className="text-lg font-bold text-blue-400">+25,000 bbl</span>
          <span className="text-[10px] text-emerald-400 block">Almgren-Chriss Auto-Hedged</span>
        </div>
      </div>

      <div className="overflow-x-auto">
        <table className="w-full text-left text-xs">
          <thead className="bg-fluxDark text-gray-400 border-b border-fluxBorder">
            <tr>
              <th className="p-2">TRADING DESK</th>
              <th className="p-2">BENCHMARK</th>
              <th className="p-2 text-right">NET DELTA (BBL)</th>
              <th className="p-2 text-right">VEGA ($)</th>
              <th className="p-2 text-right">DESK VaR ($)</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-fluxBorder text-gray-300">
            {positions.map((p, idx) => (
              <tr key={idx} className="hover:bg-fluxDark/40">
                <td className="p-2 font-semibold text-gray-200">{p.desk}</td>
                <td className="p-2">{p.asset}</td>
                <td className={`p-2 text-right font-bold ${p.netDeltaBbl >= 0 ? 'text-emerald-400' : 'text-red-400'}`}>
                  {p.netDeltaBbl > 0 ? `+${p.netDeltaBbl.toLocaleString()}` : p.netDeltaBbl.toLocaleString()}
                </td>
                <td className="p-2 text-right">${p.vegaUsd.toLocaleString()}</td>
                <td className="p-2 text-right text-red-400">${p.var99Usd.toLocaleString()}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
};
