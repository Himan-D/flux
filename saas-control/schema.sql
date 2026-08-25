-- Enable UUID and Cryptographic Extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- -------------------------------------------------------------
-- 1. Tenant & Organization Hierarchy
-- -------------------------------------------------------------
CREATE TABLE tenants (
    tenant_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    organization_name VARCHAR(128) NOT NULL,
    account_status VARCHAR(32) NOT NULL DEFAULT 'ACTIVE',
    risk_profile JSONB NOT NULL DEFAULT '{"max_notional_usd": 100000000, "var_limit_99": 5000000}',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- -------------------------------------------------------------
-- 2. Trading Books & Desks
-- -------------------------------------------------------------
CREATE TABLE trading_desks (
    desk_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    tenant_id UUID NOT NULL REFERENCES tenants(tenant_id) ON DELETE CASCADE,
    desk_code VARCHAR(32) NOT NULL,
    currency VARCHAR(3) NOT NULL DEFAULT 'USD',
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE(tenant_id, desk_code)
);

-- -------------------------------------------------------------
-- 3. OTC RFQ Lifecycle & Negotiation Ledger
-- -------------------------------------------------------------
CREATE TYPE rfq_status_enum AS ENUM (
    'REQUESTED', 'QUOTED', 'ACCEPTED', 'EXPIRED', 'REJECTED', 'CANCELLED'
);

CREATE TABLE otc_rfq_orders (
    rfq_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    tenant_id UUID NOT NULL REFERENCES tenants(tenant_id),
    desk_id UUID NOT NULL REFERENCES trading_desks(desk_id),
    counterparty_lei VARCHAR(20) NOT NULL,
    instrument_type VARCHAR(64) NOT NULL,
    underlying_primary VARCHAR(32) NOT NULL,
    underlying_secondary VARCHAR(32),
    strike_price NUMERIC(16, 4) NOT NULL,
    notional_quantity NUMERIC(16, 2) NOT NULL,
    quantity_unit VARCHAR(16) NOT NULL DEFAULT 'BBL',
    averaging_start_date DATE NOT NULL,
    averaging_end_date DATE NOT NULL,
    rfq_status rfq_status_enum NOT NULL DEFAULT 'REQUESTED',
    firm_bid NUMERIC(16, 4),
    firm_ask NUMERIC(16, 4),
    quote_expiry TIMESTAMPTZ,
    executed_price NUMERIC(16, 4),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- -------------------------------------------------------------
-- 4. Trade Capture & Immutable Audit Sourced Trades
-- -------------------------------------------------------------
CREATE TABLE executed_trades (
    trade_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    tenant_id UUID NOT NULL REFERENCES tenants(tenant_id),
    rfq_id UUID REFERENCES otc_rfq_orders(rfq_id),
    desk_id UUID NOT NULL REFERENCES trading_desks(desk_id),
    trade_utr VARCHAR(64) UNIQUE NOT NULL,
    side VARCHAR(4) NOT NULL CHECK (side IN ('BUY', 'SELL')),
    executed_price NUMERIC(16, 4) NOT NULL,
    quantity NUMERIC(16, 2) NOT NULL,
    total_consideration NUMERIC(18, 2) NOT NULL,
    clearing_venue VARCHAR(32) NOT NULL DEFAULT 'BILATERAL_OTC',
    pricing_snapshot JSONB NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Row-Level Security for SaaS Isolation
ALTER TABLE tenants ENABLE ROW LEVEL SECURITY;
ALTER TABLE trading_desks ENABLE ROW LEVEL SECURITY;
ALTER TABLE otc_rfq_orders ENABLE ROW LEVEL SECURITY;
ALTER TABLE executed_trades ENABLE ROW LEVEL SECURITY;

CREATE POLICY tenant_isolation_policy_rfq ON otc_rfq_orders
    USING (tenant_id = NULLIF(current_setting('app.current_tenant_id', true), '')::uuid);

CREATE POLICY tenant_isolation_policy_trades ON executed_trades
    USING (tenant_id = NULLIF(current_setting('app.current_tenant_id', true), '')::uuid);

-- -------------------------------------------------------------
-- 5. Physical Logistics, Vessel Charters & Demurrage Ledger
-- -------------------------------------------------------------
CREATE TABLE physical_vessel_fixtures (
    fixture_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    tenant_id UUID NOT NULL REFERENCES tenants(tenant_id),
    vessel_name VARCHAR(128) NOT NULL,
    imo_number VARCHAR(16) NOT NULL,
    charter_party_type VARCHAR(32) NOT NULL DEFAULT 'SHELLVOY5',
    origin_port VARCHAR(64) NOT NULL,
    destination_port VARCHAR(64) NOT NULL,
    cargo_grade VARCHAR(64) NOT NULL,
    volume_bbl NUMERIC(16, 2) NOT NULL,
    laytime_allowed_hours NUMERIC(8, 2) NOT NULL DEFAULT 72.0,
    actual_laytime_used_hours NUMERIC(8, 2) NOT NULL DEFAULT 0.0,
    demurrage_rate_per_day_usd NUMERIC(12, 2) NOT NULL DEFAULT 65000.0,
    demurrage_incurred_usd NUMERIC(14, 2) NOT NULL DEFAULT 0.0,
    laytime_status VARCHAR(32) NOT NULL DEFAULT 'ON_SCHEDULE',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- -------------------------------------------------------------
-- 6. Credit Support Annex (CSA) & ISDA SIMM Collateral Ledger
-- -------------------------------------------------------------
CREATE TABLE counterparty_csa_agreements (
    csa_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    tenant_id UUID NOT NULL REFERENCES tenants(tenant_id),
    counterparty_lei VARCHAR(20) NOT NULL,
    counterparty_name VARCHAR(128) NOT NULL,
    threshold_usd NUMERIC(16, 2) NOT NULL DEFAULT 5000000.0,
    mta_usd NUMERIC(16, 2) NOT NULL DEFAULT 500000.0, -- Minimum Transfer Amount
    current_collateral_posted_usd NUMERIC(16, 2) NOT NULL DEFAULT 0.0,
    isda_simm_im_required_usd NUMERIC(16, 2) NOT NULL DEFAULT 0.0,
    last_margin_call_usd NUMERIC(16, 2) NOT NULL DEFAULT 0.0,
    last_margin_call_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- -------------------------------------------------------------
-- 7. Multi-Curve Counterparty XVA Valuation Metrics
-- -------------------------------------------------------------
CREATE TABLE counterparty_xva_valuations (
    valuation_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    tenant_id UUID NOT NULL REFERENCES tenants(tenant_id),
    counterparty_lei VARCHAR(20) NOT NULL,
    cva_usd NUMERIC(16, 2) NOT NULL,
    dva_usd NUMERIC(16, 2) NOT NULL,
    fva_usd NUMERIC(16, 2) NOT NULL,
    net_xva_usd NUMERIC(16, 2) NOT NULL,
    calculated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

ALTER TABLE physical_vessel_fixtures ENABLE ROW LEVEL SECURITY;
ALTER TABLE counterparty_csa_agreements ENABLE ROW LEVEL SECURITY;
ALTER TABLE counterparty_xva_valuations ENABLE ROW LEVEL SECURITY;

CREATE POLICY tenant_isolation_fixtures ON physical_vessel_fixtures
    USING (tenant_id = NULLIF(current_setting('app.current_tenant_id', true), '')::uuid);

CREATE POLICY tenant_isolation_csa ON counterparty_csa_agreements
    USING (tenant_id = NULLIF(current_setting('app.current_tenant_id', true), '')::uuid);

CREATE POLICY tenant_isolation_xva ON counterparty_xva_valuations
    USING (tenant_id = NULLIF(current_setting('app.current_tenant_id', true), '')::uuid);
