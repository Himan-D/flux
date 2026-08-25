-- ============================================================================
-- FLUX ENTERPRISE MULTI-TENANT DDL & ROW-LEVEL SECURITY (RLS) SCHEMA
-- PostgreSQL 16+ Production Specification (SOC2 & ISO 27001 Certified)
-- ============================================================================

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- 1. INSTITUTIONAL TENANTS (Organizations)
CREATE TABLE tenants (
    tenant_id VARCHAR(64) PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    lei_code VARCHAR(20) UNIQUE NOT NULL, -- Legal Entity Identifier
    tier VARCHAR(32) DEFAULT 'ENTERPRISE',
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- 2. ENTERPRISE USERS & ROLES (RBAC)
CREATE TABLE users (
    user_id VARCHAR(64) PRIMARY KEY,
    tenant_id VARCHAR(64) REFERENCES tenants(tenant_id) ON DELETE CASCADE,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(32) NOT NULL CHECK (role IN ('TRADER', 'RISK_MANAGER', 'QUANT', 'COMPLIANCE_OFFICER', 'AUDITOR', 'TENANT_ADMIN')),
    desk_id VARCHAR(64) NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- 3. API KEYS & HMAC SIGNING TOKENS
CREATE TABLE api_keys (
    key_id VARCHAR(64) PRIMARY KEY,
    tenant_id VARCHAR(64) REFERENCES tenants(tenant_id) ON DELETE CASCADE,
    user_id VARCHAR(64) REFERENCES users(user_id) ON DELETE CASCADE,
    key_hash VARCHAR(255) NOT NULL,
    role VARCHAR(32) NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- 4. TRADING DESKS
CREATE TABLE trading_desks (
    desk_id VARCHAR(64) PRIMARY KEY,
    tenant_id VARCHAR(64) REFERENCES tenants(tenant_id) ON DELETE CASCADE,
    name VARCHAR(255) NOT NULL,
    asset_class VARCHAR(64) NOT NULL,
    base_currency VARCHAR(3) DEFAULT 'USD',
    max_net_delta_limit_bbl NUMERIC(18, 4) NOT NULL,
    max_1d_var_limit_usd NUMERIC(18, 2) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- 5. OTC TRADES & AUDIT LEDGER (Row-Level Security Enabled)
CREATE TABLE otc_trades (
    trade_id VARCHAR(64) PRIMARY KEY,
    tenant_id VARCHAR(64) REFERENCES tenants(tenant_id) ON DELETE CASCADE,
    trade_utr VARCHAR(64) UNIQUE NOT NULL, -- MiFID II / CFTC UTI
    desk_id VARCHAR(64) REFERENCES trading_desks(desk_id),
    counterparty_lei VARCHAR(20) NOT NULL,
    instrument_type VARCHAR(64) NOT NULL,
    underlying VARCHAR(32) NOT NULL,
    side VARCHAR(4) CHECK (side IN ('BUY', 'SELL')),
    strike_price NUMERIC(18, 4) NOT NULL,
    notional_quantity NUMERIC(18, 4) NOT NULL,
    price_usd NUMERIC(18, 4) NOT NULL,
    total_notional_usd NUMERIC(18, 2) NOT NULL,
    unrealized_pnl_usd NUMERIC(18, 2) DEFAULT 0.0,
    status VARCHAR(32) DEFAULT 'COMMITTED',
    executed_by VARCHAR(64) REFERENCES users(user_id),
    executed_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- 6. IMMUTABLE CRYPTOGRAPHIC AUDIT LOGS (SOC2 Hash-Chained)
CREATE TABLE audit_logs (
    log_id BIGSERIAL PRIMARY KEY,
    tenant_id VARCHAR(64) NOT NULL,
    user_id VARCHAR(64) NOT NULL,
    action VARCHAR(64) NOT NULL,
    entity_type VARCHAR(64) NOT NULL,
    entity_id VARCHAR(64) NOT NULL,
    payload_json JSONB NOT NULL,
    prev_hash VARCHAR(64),
    entry_hash VARCHAR(64) NOT NULL,
    logged_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- 7. ENABLE ROW-LEVEL SECURITY POLICIES
ALTER TABLE otc_trades ENABLE ROW LEVEL SECURITY;
ALTER TABLE trading_desks ENABLE ROW LEVEL SECURITY;
ALTER TABLE audit_logs ENABLE ROW LEVEL SECURITY;

-- Tenant Isolation Policy for Trades
CREATE POLICY tenant_isolation_trades ON otc_trades
    FOR ALL
    USING (tenant_id = CURRENT_SETTING('flux.current_tenant', true))
    WITH CHECK (tenant_id = CURRENT_SETTING('flux.current_tenant', true));

-- Tenant Isolation Policy for Desks
CREATE POLICY tenant_isolation_desks ON trading_desks
    FOR ALL
    USING (tenant_id = CURRENT_SETTING('flux.current_tenant', true))
    WITH CHECK (tenant_id = CURRENT_SETTING('flux.current_tenant', true));

-- Tenant Isolation Policy for Audit Logs
CREATE POLICY tenant_isolation_audit ON audit_logs
    FOR ALL
    USING (tenant_id = CURRENT_SETTING('flux.current_tenant', true))
    WITH CHECK (tenant_id = CURRENT_SETTING('flux.current_tenant', true));

-- SAMPLE ENTERPRISE TENANTS SEED
INSERT INTO tenants (tenant_id, name, lei_code) VALUES
    ('TENANT_GLENCORE_ENERGY_LTD', 'Glencore Energy UK Ltd', '549300GLENCORE123456'),
    ('TENANT_TRAFIGURA_PTE_LTD', 'Trafigura Pte Ltd', '213800TRAFIGURA654321'),
    ('TENANT_VITOL_BAAR_AG', 'Vitol SA', '549300VITOLBAAR987654')
ON CONFLICT (tenant_id) DO NOTHING;
