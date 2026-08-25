package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"
)

type RFQRequest struct {
	TenantID          string  `json:"tenant_id"`
	DeskID            string  `json:"desk_id"`
	InstrumentType    string  `json:"instrument_type"` // "ASIAN_APO", "CRACK_SPREAD"
	Underlying        string  `json:"underlying"`        // "BRENT_CRUDE", "WTI_CRUDE"
	StrikePrice       float64 `json:"strike_price"`
	NotionalQuantity  float64 `json:"notional_quantity"`
	QuantityUnit      string  `json:"quantity_unit"`
}

type RFQResponse struct {
	RFQID            string    `json:"rfq_id"`
	TenantID         string    `json:"tenant_id"`
	Status           string    `json:"status"`
	FairValue        float64   `json:"fair_value"`
	FirmBid          float64   `json:"firm_bid"`
	FirmAsk          float64   `json:"firm_ask"`
	QuoteExpiry      time.Time `json:"quote_expiry"`
	GreeksDelta      float64   `json:"greeks_delta"`
	ServerTimestamp  time.Time `json:"server_timestamp"`
}

type TradeExecutionRequest struct {
	RFQID       string  `json:"rfq_id"`
	TenantID    string  `json:"tenant_id"`
	Side        string  `json:"side"` // "BUY" or "SELL"
	Price       float64 `json:"price"`
	Quantity    float64 `json:"quantity"`
}

type TradeExecutionResponse struct {
	TradeID         string    `json:"trade_id"`
	TradeUTR        string    `json:"trade_utr"` // MiFID II / CFTC UTI
	Status          string    `json:"status"`
	ExecutedPrice   float64   `json:"executed_price"`
	ExecutedQty     float64   `json:"executed_qty"`
	NotionalUSD     float64   `json:"notional_usd"`
	ExecutionTime   time.Time `json:"execution_time"`
}

type ServerState struct {
	sync.RWMutex
	rfqs   map[string]RFQResponse
	trades map[string]TradeExecutionResponse
}

var state = ServerState{
	rfqs:   make(map[string]RFQResponse),
	trades: make(map[string]TradeExecutionResponse),
}

func handleRFQ(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req RFQRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	rfqID := fmt.Sprintf("rfq-%d", time.Now().UnixNano())
	
	// Fast pricing heuristic (simulated integration with Turnbull-Wakeman kernel)
	fairVal := 2.4192
	bid := 2.4414
	ask := 2.5414

	resp := RFQResponse{
		RFQID:           rfqID,
		TenantID:        req.TenantID,
		Status:          "QUOTED",
		FairValue:       fairVal,
		FirmBid:         bid,
		FirmAsk:         ask,
		QuoteExpiry:     time.Now().Add(5 * time.Second),
		GreeksDelta:     0.3900,
		ServerTimestamp: time.Now().UTC(),
	}

	state.Lock()
	state.rfqs[rfqID] = resp
	state.Unlock()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}

func handleTradeExecute(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req TradeExecutionRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	tradeID := fmt.Sprintf("trd-%d", time.Now().UnixNano())
	utr := fmt.Sprintf("FLUX-OTC-%s-%d", req.TenantID, time.Now().Unix())

	resp := TradeExecutionResponse{
		TradeID:       tradeID,
		TradeUTR:      utr,
		Status:        "EXECUTED",
		ExecutedPrice: req.Price,
		ExecutedQty:   req.Quantity,
		NotionalUSD:   req.Price * req.Quantity,
		ExecutionTime: time.Now().UTC(),
	}

	state.Lock()
	state.trades[tradeID] = resp
	state.Unlock()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}

func handleHealth(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":      "UP",
		"service":     "flux-saas-control",
		"version":     "1.0.0",
		"timestamp":   time.Now().UTC(),
	})
}

func main() {
	http.HandleFunc("/v1/health", handleHealth)
	http.HandleFunc("/v1/rfq/request", handleRFQ)
	http.HandleFunc("/v1/trade/execute", handleTradeExecute)

	port := ":8080"
	fmt.Printf("=========================================================\n")
	fmt.Printf("  FLUX SAAS CONTROL PLANE & RFQ API GATEWAY\n")
	fmt.Printf("  Listening on http://localhost%s\n", port)
	fmt.Printf("=========================================================\n")

	log.Fatal(http.ListenAndServe(port, nil))
}
