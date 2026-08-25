import React from 'react';
import { VesselLogistics } from '../types';
import { Anchor } from 'lucide-react';

export const LogisticsDesk: React.FC = () => {
  const voyages: VesselLogistics[] = [
    {
      vessel: 'DHT HAWK (VLCC)',
      imo: 'IMO-9812345',
      route: 'Ras Tanura -> Rotterdam',
      grade: 'Arab Light',
      volumeBbl: 2000000,
      status: 'IN_DEMURRAGE',
      demurrageUsd: 65000
    },
    {
      vessel: 'FRONT ALTAIR (Suezmax)',
      imo: 'IMO-9745120',
      route: 'Corpus Christi -> Le Havre',
      grade: 'WTI Midland',
      volumeBbl: 1000000,
      status: 'ON_SCHEDULE',
      demurrageUsd: 0
    },
    {
      vessel: 'NORDIC FREEDOM (Aframax)',
      imo: 'IMO-9654311',
      route: 'Primorsk -> Wilhelmshaven',
      grade: 'Urals/Gasoil',
      volumeBbl: 600000,
      status: 'AT_RISK',
      demurrageUsd: 12500
    }
  ];

  return (
    <div className="bg-fluxPanel border border-fluxBorder rounded-lg p-5 flex flex-col h-full">
      <div className="flex items-center justify-between border-b border-fluxBorder pb-3 mb-4">
        <div className="flex items-center space-x-2">
          <Anchor className="w-4 h-4 text-amber-400" />
          <h2 className="font-semibold text-sm tracking-wide text-white uppercase">Physical CTRM Logistics & Demurrage Monitor</h2>
        </div>
        <span className="text-xs bg-amber-950 text-amber-400 border border-amber-800 px-2 py-0.5 rounded">
          SHELLVOY5 CONTRACT TRACKING
        </span>
      </div>

      <div className="space-y-3">
        {voyages.map((v, i) => (
          <div key={i} className="p-3 bg-fluxDark rounded border border-fluxBorder flex items-center justify-between text-xs">
            <div>
              <div className="flex items-center space-x-2">
                <span className="font-bold text-gray-100">{v.vessel}</span>
                <span className="text-gray-400 text-[10px]">({v.imo})</span>
              </div>
              <div className="text-gray-400 mt-0.5">
                Route: <span className="text-gray-200">{v.route}</span> | Cargo: <span className="text-gray-200">{v.volumeBbl.toLocaleString()} bbl ({v.grade})</span>
              </div>
            </div>

            <div className="text-right">
              <span className={`inline-block px-2 py-0.5 rounded text-[10px] font-bold ${
                v.status === 'IN_DEMURRAGE' ? 'bg-red-950 text-red-400 border border-red-800' :
                v.status === 'AT_RISK' ? 'bg-amber-950 text-amber-400 border border-amber-800' :
                'bg-emerald-950 text-emerald-400 border border-emerald-800'
              }`}>
                {v.status}
              </span>
              <div className="text-xs text-gray-300 font-semibold mt-1">
                Demurrage: <span className={v.demurrageUsd > 0 ? 'text-red-400' : 'text-gray-400'}>${v.demurrageUsd.toLocaleString()}</span>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};
