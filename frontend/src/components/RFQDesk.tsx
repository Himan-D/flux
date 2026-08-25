import React, { useState } from 'react';
import { QuoteData } from '../types';
import { TrendingUp, CheckCircle2 } from 'lucide-react';

export const RFQDesk: React.FC = () => {
  const [strike, setStrike] = useState(82.50);
  const [qty, setQty] = useState(50000);
  const [instrument, setInstrument] = useState('ASIAN_APO');
  const [underlying, setUnderlying] = useState('BRENT_CRUDE');
  const [isExecuting, setIsExecuting] = useState(false);
  const [executedTrade, setExecutedTrade] = useState<string | null>(null);

  const quote: QuoteData = {
    fairValue: 3.3749,
    bid: 3.3249 + (8.75 / 10000) * 82.5,
    ask: 3.4249 + (8.75 / 10000) * 82.5,
    delta: 0.4062,
    gamma: 0.0477,
    vega: 12.4391,
    theta: -6.4627,
    skewBps: 8.75
  };

  const handleExecute = (side: 'BUY' | 'SELL') => {
    setIsExecuting(true);
    setTimeout(() => {
      setIsExecuting(false);
      setExecutedTrade(`UTR-FLUX-${side}-${Math.floor(100000 + Math.random() * 900000)}`);
    }, 300);
  };

  return (
    <div className="bg-fluxPanel border border-fluxBorder rounded-lg p-5 flex flex-col h-full">
      <div className="flex items-center justify-between border-b border-fluxBorder pb-3 mb-4">
        <div className="flex items-center space-x-2">
          <TrendingUp className="w-4 h-4 text-blue-400" />
          <h2 className="font-semibold text-sm tracking-wide text-white uppercase">Systematic OTC RFQ & Quoting Desk</h2>
        </div>
        <span className="text-xs bg-emerald-950 text-emerald-400 border border-emerald-800 px-2 py-0.5 rounded">
          STREAMING FIRM
        </span>
      </div>

      {/* Input Parameters */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-4 text-xs">
        <div>
          <label className="text-gray-400 block mb-1">UNDERLYING</label>
          <select 
            value={underlying} 
            onChange={(e) => setUnderlying(e.target.value)}
            className="w-full bg-fluxDark border border-fluxBorder rounded p-2 text-gray-200 focus:outline-none focus:border-blue-500"
          >
            <option value="BRENT_CRUDE">ICE Dated Brent ($/bbl)</option>
            <option value="WTI_CRUDE">NYMEX WTI ($/bbl)</option>
            <option value="GASOIL_CRACK">Gasoil Crack ($/bbl)</option>
          </select>
        </div>

        <div>
          <label className="text-gray-400 block mb-1">STRUCTURE</label>
          <select 
            value={instrument} 
            onChange={(e) => setInstrument(e.target.value)}
            className="w-full bg-fluxDark border border-fluxBorder rounded p-2 text-gray-200 focus:outline-none focus:border-blue-500"
          >
            <option value="ASIAN_APO">Monthly Asian APO (21 Fixings)</option>
            <option value="CRACK_SPREAD">Crack Spread Swap</option>
            <option value="CALENDAR_SPREAD">Calendar Spread Option</option>
          </select>
        </div>

        <div>
          <label className="text-gray-400 block mb-1">STRIKE PRICE ($)</label>
          <input 
            type="number" 
            step="0.10"
            value={strike} 
            onChange={(e) => setStrike(parseFloat(e.target.value))}
            className="w-full bg-fluxDark border border-fluxBorder rounded p-2 text-gray-200 focus:outline-none focus:border-blue-500"
          />
        </div>

        <div>
          <label className="text-gray-400 block mb-1">NOTIONAL (BBL)</label>
          <input 
            type="number" 
            step="5000"
            value={qty} 
            onChange={(e) => setQty(parseInt(e.target.value))}
            className="w-full bg-fluxDark border border-fluxBorder rounded p-2 text-gray-200 focus:outline-none focus:border-blue-500"
          />
        </div>
      </div>

      {/* Two-Way Market Quoting Box */}
      <div className="grid grid-cols-2 gap-4 bg-fluxDark border border-fluxBorder rounded-lg p-4 mb-4">
        <div className="flex flex-col items-center justify-center p-3 bg-red-950/20 border border-red-900/40 rounded">
          <span className="text-xs text-red-400 uppercase font-semibold mb-1">FIRM BID (SELL TO FLUX)</span>
          <span className="text-2xl font-bold text-red-400 tracking-wider">${quote.bid.toFixed(4)}</span>
          <span className="text-xs text-gray-400 mt-1">Notional: ${(quote.bid * qty).toLocaleString('en-US', {maximumFractionDigits: 2})}</span>
          <button 
            onClick={() => handleExecute('SELL')}
            disabled={isExecuting}
            className="mt-3 w-full py-2 bg-red-600 hover:bg-red-500 text-white rounded font-bold text-xs transition"
          >
            HIT BID (SELL)
          </button>
        </div>

        <div className="flex flex-col items-center justify-center p-3 bg-emerald-950/20 border border-emerald-900/40 rounded">
          <span className="text-xs text-emerald-400 uppercase font-semibold mb-1">FIRM ASK (BUY FROM FLUX)</span>
          <span className="text-2xl font-bold text-emerald-400 tracking-wider">${quote.ask.toFixed(4)}</span>
          <span className="text-xs text-gray-400 mt-1">Notional: ${(quote.ask * qty).toLocaleString('en-US', {maximumFractionDigits: 2})}</span>
          <button 
            onClick={() => handleExecute('BUY')}
            disabled={isExecuting}
            className="mt-3 w-full py-2 bg-emerald-600 hover:bg-emerald-500 text-white rounded font-bold text-xs transition"
          >
            LIFT ASK (BUY)
          </button>
        </div>
      </div>

      {/* Real-time Greeks Sensitivities */}
      <div className="grid grid-cols-5 gap-2 text-center text-xs bg-fluxDark/60 p-2.5 rounded border border-fluxBorder">
        <div>
          <span className="text-gray-400 block">FAIR VAL</span>
          <span className="font-bold text-gray-200">${quote.fairValue.toFixed(4)}</span>
        </div>
        <div>
          <span className="text-gray-400 block">DELTA (Δ)</span>
          <span className="font-bold text-blue-400">{quote.delta.toFixed(4)}</span>
        </div>
        <div>
          <span className="text-gray-400 block">GAMMA (Γ)</span>
          <span className="font-bold text-purple-400">{quote.gamma.toFixed(4)}</span>
        </div>
        <div>
          <span className="text-gray-400 block">VEGA (ν)</span>
          <span className="font-bold text-amber-400">{quote.vega.toFixed(4)}</span>
        </div>
        <div>
          <span className="text-gray-400 block">AI SKEW</span>
          <span className="font-bold text-emerald-400">+{quote.skewBps.toFixed(2)} bps</span>
        </div>
      </div>

      {executedTrade && (
        <div className="mt-4 p-3 bg-emerald-950/80 border border-emerald-700 text-emerald-300 text-xs rounded flex items-center justify-between">
          <div className="flex items-center space-x-2">
            <CheckCircle2 className="w-4 h-4" />
            <span>TRADE CONFIRMED & COMMITTED TO AERON RAFT CLUSTER: <strong>{executedTrade}</strong></span>
          </div>
          <span className="text-[10px] bg-emerald-900 px-2 py-0.5 rounded">CFTC PART 43 LOGGED</span>
        </div>
      )}
    </div>
  );
};
