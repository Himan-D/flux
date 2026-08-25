package main

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
)

func TestHandleHealth(t *testing.T) {
	req, err := http.NewRequest("GET", "/v1/health", nil)
	if err != nil {
		t.Fatal(err)
	}

	rr := httptest.NewRecorder()
	handler := http.HandlerFunc(handleHealth)
	handler.ServeHTTP(rr, req)

	if status := rr.Code; status != http.StatusOK {
		t.Errorf("handler returned wrong status code: got %v want %v", status, http.StatusOK)
	}

	var resp map[string]interface{}
	if err := json.NewDecoder(rr.Body).Decode(&resp); err != nil {
		t.Fatal(err)
	}

	if resp["status"] != "UP" {
		t.Errorf("expected status 'UP', got %v", resp["status"])
	}
}

func TestHandleRFQAndTradeWorkflow(t *testing.T) {
	// 1. Create RFQ
	rfqReq := RFQRequest{
		TenantID:         "TENANT_TRAFIGURA",
		DeskID:           "DESK_OIL",
		InstrumentType:   "ASIAN_APO",
		Underlying:       "BRENT_CRUDE",
		StrikePrice:      82.50,
		NotionalQuantity: 50000.0,
		QuantityUnit:     "BBL",
	}

	reqBody, _ := json.Marshal(rfqReq)
	req, _ := http.NewRequest("POST", "/v1/rfq/request", bytes.NewBuffer(reqBody))
	rr := httptest.NewRecorder()
	handleRFQ(rr, req)

	if rr.Code != http.StatusOK {
		t.Fatalf("expected 200 OK for RFQ, got %d", rr.Code)
	}

	var rfqResp RFQResponse
	json.NewDecoder(rr.Body).Decode(&rfqResp)
	if rfqResp.Status != "QUOTED" || rfqResp.FirmBid <= 0 || rfqResp.FirmAsk <= 0 {
		t.Fatalf("invalid RFQ quote response: %+v", rfqResp)
	}

	// 2. Execute Trade on Quoted RFQ
	tradeReq := TradeExecutionRequest{
		RFQID:    rfqResp.RFQID,
		TenantID: rfqResp.TenantID,
		Side:     "BUY",
		Price:    rfqResp.FirmAsk,
		Quantity: 50000.0,
	}

	tradeBody, _ := json.Marshal(tradeReq)
	req2, _ := http.NewRequest("POST", "/v1/trade/execute", bytes.NewBuffer(tradeBody))
	rr2 := httptest.NewRecorder()
	handleTradeExecute(rr2, req2)

	if rr2.Code != http.StatusOK {
		t.Fatalf("expected 200 OK for Trade execution, got %d", rr2.Code)
	}

	var tradeResp TradeExecutionResponse
	json.NewDecoder(rr2.Body).Decode(&tradeResp)
	if tradeResp.Status != "EXECUTED" || tradeResp.NotionalUSD != rfqResp.FirmAsk*50000.0 {
		t.Fatalf("trade execution mismatch: %+v", tradeResp)
	}
}
