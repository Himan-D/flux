export interface ForwardPoint {
  tenor: string;
  price: number;
  change: number;
}

export interface QuoteData {
  bid: number;
  ask: number;
  fairValue: number;
  delta: number;
  gamma: number;
  vega: number;
  theta: number;
  skewBps: number;
}

export interface PositionRisk {
  desk: string;
  asset: string;
  netDeltaBbl: number;
  vegaUsd: number;
  var99Usd: number;
}

export interface VesselLogistics {
  vessel: string;
  imo: string;
  route: string;
  grade: string;
  volumeBbl: number;
  status: 'ON_SCHEDULE' | 'IN_DEMURRAGE' | 'AT_RISK';
  demurrageUsd: number;
}
