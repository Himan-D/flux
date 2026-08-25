package main

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

func TestHandleHealthAndReady(t *testing.T) {
	reqHealth := httptest.NewRequest("GET", "/v1/health", nil)
	wHealth := httptest.NewRecorder()
	handleHealth(wHealth, reqHealth)

	if wHealth.Code != http.StatusOK {
		t.Fatalf("Expected status 200 from /v1/health, got %d", wHealth.Code)
	}

	reqReady := httptest.NewRequest("GET", "/v1/ready", nil)
	wReady := httptest.NewRecorder()
	handleReady(wReady, reqReady)

	if wReady.Code != http.StatusOK {
		t.Fatalf("Expected status 200 from /v1/ready, got %d", wReady.Code)
	}
}

func TestEnterpriseJWTAndRBACWorkflow(t *testing.T) {
	claims := JWTClaims{
		UserID:    "usr-quant-01",
		TenantID:  "TENANT_GLENCORE_ENERGY_LTD",
		DeskID:    "DESK_OIL_DERIVATIVES_LONDON",
		Role:      "TRADER",
		ExpiresAt: time.Now().Add(1 * time.Hour).Unix(),
	}

	token, err := generateJWT(claims)
	if err != nil {
		t.Fatalf("Failed to generate JWT: %v", err)
	}

	parsed, err := validateJWT(token)
	if err != nil {
		t.Fatalf("Failed to validate JWT: %v", err)
	}

	if parsed.UserID != claims.UserID || parsed.Role != "TRADER" {
		t.Fatalf("Claims mismatch: expected %s / TRADER, got %s / %s", claims.UserID, parsed.UserID, parsed.Role)
	}
}

func TestHandleRFQAndTradeWorkflow(t *testing.T) {
	reqBody, _ := json.Marshal(RFQRequest{
		TenantID:         "TENANT_TEST",
		DeskID:           "DESK_TEST",
		InstrumentType:   "ASIAN_APO",
		Underlying:       "BRENT",
		StrikePrice:      82.50,
		NotionalQuantity: 50000.0,
		QuantityUnit:     "BBL",
	})

	req := httptest.NewRequest("POST", "/v1/rfq/request", bytes.NewReader(reqBody))
	w := httptest.NewRecorder()
	handleRFQ(w, req)

	if w.Code != http.StatusOK {
		t.Fatalf("Expected status 200, got %d", w.Code)
	}

	var rfqResp RFQResponse
	json.NewDecoder(w.Body).Decode(&rfqResp)
	if rfqResp.Status != "QUOTED" || rfqResp.FairValue <= 0 {
		t.Fatalf("Invalid RFQ Response: %+v", rfqResp)
	}
}
