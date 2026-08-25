package main

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"sync"
	"sync/atomic"
	"testing"
	"time"
)

func TestConcurrentScaleLoad(t *testing.T) {
	mux := http.NewServeMux()
	mux.HandleFunc("/v1/rfq/request", handleRFQ)
	server := httptest.NewServer(mux)
	defer server.Close()

	const totalRequests = 50000
	const concurrency = 50

	var successCount int64
	var errorCount int64

	requestsPerWorker := totalRequests / concurrency
	var wg sync.WaitGroup

	reqBody, _ := json.Marshal(RFQRequest{
		TenantID:         "TENANT_TRAFIGURA_PTE",
		DeskID:           "DESK_OIL_DERIVATIVES_LONDON",
		InstrumentType:   "ASIAN_APO",
		Underlying:       "BRENT",
		StrikePrice:      82.50,
		NotionalQuantity: 50000.0,
		QuantityUnit:     "BBL",
	})

	transport := &http.Transport{
		MaxIdleConns:        1000,
		MaxIdleConnsPerHost: 500,
		IdleConnTimeout:     30 * time.Second,
		DisableKeepAlives:   false,
	}

	start := time.Now()

	for c := 0; c < concurrency; c++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			client := &http.Client{
				Transport: transport,
				Timeout:   10 * time.Second,
			}
			for i := 0; i < requestsPerWorker; i++ {
				resp, err := client.Post(server.URL+"/v1/rfq/request", "application/json", bytes.NewReader(reqBody))
				if err != nil {
					atomic.AddInt64(&errorCount, 1)
					continue
				}
				if resp.StatusCode == http.StatusOK {
					atomic.AddInt64(&successCount, 1)
				} else {
					atomic.AddInt64(&errorCount, 1)
				}
				io.Copy(io.Discard, resp.Body)
				resp.Body.Close()
			}
		}()
	}

	wg.Wait()
	duration := time.Since(start)
	rps := float64(totalRequests) / duration.Seconds()

	t.Logf("====================================================================")
	t.Logf("  GO SAAS CONTROL PLANE CONCURRENT LOAD TEST (50,000 REQUESTS)      ")
	t.Logf("====================================================================")
	t.Logf("  • Total Requests Processed: %d", totalRequests)
	t.Logf("  • Concurrency Level:        %d parallel workers", concurrency)
	t.Logf("  • Total Duration:           %v", duration)
	t.Logf("  • Measured Throughput:      %.2f Requests / sec", rps)
	t.Logf("  • Successful Quotes:        %d (%.2f%% Success Rate)", successCount, (float64(successCount)/float64(totalRequests))*100.0)
	t.Logf("  • Failed / Dropped:         %d", errorCount)
	t.Logf("====================================================================")

	if errorCount > 0 {
		t.Fatalf("Encountered %d failed requests during scale load test", errorCount)
	}
}
