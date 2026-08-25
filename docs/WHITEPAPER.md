# Flux: Quantitative Specification & Mathematical Foundations

This document provides the formal mathematical specifications, derivations, and algorithmic implementations for the **Flux** quantitative trading engine.

---

## 1. Turnbull-Wakeman (1991) Discrete Moment-Matching for Asian Options

Arithmetic Asian options (Average Price Options, or APOs) represent the core hedging instrument for physical crude and refined product streams settled against Platts or Argus daily publications.

### 1.1 Problem Formulation
Let $S_t$ follow a geometric Brownian motion under the risk-neutral measure $\mathbb{Q}$:

$$\frac{dS_t}{S_t} = r dt + \sigma dW_t$$

The payoff of a discrete arithmetic Asian call option with strike $K$ across $N$ observation dates $t_1, t_2, \dots, t_N \le T$ is given by:

$$\Pi_{\text{call}} = \max\left( \bar{S} - K, 0 \right), \quad \text{where } \bar{S} = \frac{1}{N} \sum_{i=1}^{N} S_{t_i}$$

Because the sum of lognormal variables is not lognormal, an exact closed-form density does not exist. Turnbull and Wakeman approximate the distribution of $\bar{S}$ by matching its first two analytical moments to a lognormal distribution with adjusted parameters.

### 1.2 Analytical Moment Derivations
The first two uncentered moments of $\bar{S}$ are derived as:

$$M_1 = \mathbb{E}[\bar{S}] = \frac{1}{N} \sum_{i=1}^{N} S_0 e^{r t_i}$$

$$M_2 = \mathbb{E}[\bar{S}^2] = \frac{1}{N^2} \sum_{i=1}^{N} \sum_{j=1}^{N} S_0^2 e^{r(t_i + t_j) + \sigma^2 \min(t_i, t_j)}$$

The effective annualized volatility $\sigma_A$ and forward price $F_A$ are obtained via:

$$\sigma_A^2 = \frac{1}{T} \ln\left( \frac{M_2}{M_1^2} \right)$$

$$d_1 = \frac{\ln(M_1 / K) + \frac{1}{2} \sigma_A^2 T}{\sigma_A \sqrt{T}}, \quad d_2 = d_1 - \sigma_A \sqrt{T}$$

The analytical price $C_{\text{Asian}}$ is given by:

$$C_{\text{Asian}} = e^{-rT} \left[ M_1 N(d_1) - K N(d_2) \right]$$

### 1.3 Branchless Cody Rational Minimax Polynomial Approximation
To achieve sub-microsecond latency ($542\text{ ns}$), standard transcendental error functions (`erf`/`erfc`) are replaced with Cody's rational Chebyshev minimax polynomial approximation for the standard normal CDF $N(z)$:

$$N(z) = \begin{cases}
1 - \frac{1}{\sqrt{2\pi}} e^{-z^2/2} R(z), & z \ge 0 \\
\frac{1}{\sqrt{2\pi}} e^{-z^2/2} R(-z), & z < 0
\end{cases}$$

where $R(z)$ is a rational function of polynomials $P(z)/Q(z)$ with pre-computed minimax coefficients.

---

## 2. Kirk's Approximation for Bivariate Crack Spread Options

Refinery crack spread options price the margin between crude input $S_1$ and refined product output $S_2$ with strike $K$ and conversion factor $w$.

### 2.1 Payoff Function
$$\Pi_{\text{crack}} = \max\left( S_{2,T} - w S_{1,T} - K, 0 \right)$$

### 2.2 Volatility Aggregation & Pricing Formula
Under joint lognormal dynamics with instantaneous correlation $\rho = \text{Corr}(dW_1, dW_2)$, Kirk approximates the volatility $\sigma_{\text{eff}}$ of the composite forward $F_{\text{eff}} = \frac{F_2}{w F_1 + K e^{-rT}}$:

$$\sigma_{\text{eff}} = \sqrt{\sigma_2^2 - 2 \rho \sigma_2 \sigma_1 \left( \frac{w F_1}{w F_1 + K e^{-rT}} \right) + \sigma_1^2 \left( \frac{w F_1}{w F_1 + K e^{-rT}} \right)^2}$$

$$d_1 = \frac{\ln\left( \frac{F_2}{w F_1 + K e^{-rT}} \right) + \frac{1}{2} \sigma_{\text{eff}}^2 T}{\sigma_{\text{eff}} \sqrt{T}}, \quad d_2 = d_1 - \sigma_{\text{eff}} \sqrt{T}$$

$$C_{\text{crack}} = e^{-rT} \left( w F_1 + K e^{-rT} \right) \left[ \left( \frac{F_2}{w F_1 + K e^{-rT}} \right) N(d_1) - N(d_2) \right]$$

---

## 3. Avellaneda-Stoikov Systematic Market Making Model

Flux computes optimal two-way firm quotes by dynamically adjusting reservation prices according to net inventory and order-flow alpha skew.

### 3.1 Reservation Price
Given mid-price $s$, inventory $q$, risk aversion parameter $\gamma$, volatility $\sigma$, and remaining time horizon $T - t$:

$$r(s, q, t) = s - q \gamma \sigma^2 (T - t)$$

### 3.2 Optimal Bid-Ask Spread
The optimal half-spreads $\delta^a$ and $\delta^b$ with order arrival intensity $\kappa$ are determined by:

$$\delta^a + \delta^b = \gamma \sigma^2 (T - t) + \frac{2}{\gamma} \ln\left( 1 + \frac{\gamma}{\kappa} \right)$$

$$\text{Bid} = r(s, q, t) - \delta^b + \alpha_{\text{skew}}, \quad \text{Ask} = r(s, q, t) + \delta^a + \alpha_{\text{skew}}$$

where $\alpha_{\text{skew}}$ is the directional skew synthesized from physical satellite radar storage telemetry.

---

## 4. Almgren-Chriss Optimal Execution & Central Risk Book (CRB)

When residual macro risk cannot be internalized across desks, Flux computes the optimal TWAP/VWAP execution trajectory minimizing temporary and permanent market impact.

### 4.1 Optimal Trajectory
Let $x_0$ be the initial residual inventory and $T$ be the total liquidation horizon. The optimal trajectory $x(t)$ is given by:

$$x(t) = x_0 \frac{\sinh(\kappa (T - t))}{\sinh(\kappa T)}$$

where the urgency parameter $\kappa$ is defined as:

$$\kappa = \sqrt{\frac{\lambda \sigma^2}{\eta}}$$

with risk-aversion parameter $\lambda$, temporary price impact parameter $\eta$, and asset volatility $\sigma$.

---

## 5. ISDA SIMM v2.6 Initial Margin Model

For non-cleared OTC bilateral trades, Initial Margin (IM) is calculated using the standard ISDA sensitivity aggregation method:

$$\text{IM} = \sqrt{\sum_{k} \text{WS}_k^2 + \sum_{k} \sum_{l \neq k} \rho_{kl} \text{WS}_k \text{WS}_l}$$

where:
* $\text{WS}_k = \text{RW}_k \cdot s_k \cdot \text{CR}_{b}$ is the weighted sensitivity for risk factor $k$.
* $\text{RW}_k$ is the regulatory risk weight (e.g., 28% for crude oil, 35% for distillates).
* $\rho_{kl}$ is the intra-commodity correlation matrix prescribed by ISDA SIMM v2.6.

---

## 6. ASTM D1298 Non-Linear Specific Gravity Blending Physics

Crude and refined oil blending is governed by non-linear specific gravity equations subject to total mass and volume conservation.

### 6.1 Specific Gravity Inversion
$$\text{SG} = \frac{141.5}{\text{API} + 131.5}$$

Given $K$ input streams with volumes $V_i$ and API gravities $\text{API}_i$:

$$\text{Total Volume } V_{\text{total}} = \sum_{i=1}^{K} V_i$$

$$\text{Blended Specific Gravity } \text{SG}_{\text{blend}} = \sum_{i=1}^{K} \left( \frac{V_i}{V_{\text{total}}} \right) \text{SG}_i$$

$$\text{Blended API} = \frac{141.5}{\text{SG}_{\text{blend}}} - 131.5$$

### 6.2 Mass-Weighted Sulfur Conservation
$$\text{Mass}_i = V_i \cdot \text{SG}_i \cdot \rho_{\text{water}}$$

$$\text{Blended Sulfur } (\%) = \frac{\sum_{i=1}^{K} \text{Mass}_i \cdot S_i}{\sum_{i=1}^{K} \text{Mass}_i}$$
