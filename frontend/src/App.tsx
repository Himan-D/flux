import React from 'react';
import { Header } from './components/Header';
import { RFQDesk } from './components/RFQDesk';
import { RiskDashboard } from './components/RiskDashboard';
import { VolSurface3D } from './components/VolSurface3D';
import { LogisticsDesk } from './components/LogisticsDesk';

export const App: React.FC = () => {
  return (
    <div className="min-h-screen bg-fluxDark flex flex-col text-gray-100">
      <Header />
      <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div className="flex flex-col space-y-6">
          <RFQDesk />
          <VolSurface3D />
        </div>
        <div className="flex flex-col space-y-6">
          <RiskDashboard />
          <LogisticsDesk />
        </div>
      </main>
    </div>
  );
};
export default App;
