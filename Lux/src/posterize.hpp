// weighted_deterministic_posterize_ucolor.hpp
#pragma once
#include <vector>
#include <cstdint>
#include <limits>
#include <algorithm>
#include "ucolor.hpp"

// -------------------------------------------------------------------
// Distance kind
// -------------------------------------------------------------------
// Add a new distance kind
enum class DistKind { L2_SQR, L1_MANHATTAN, YCOCG_L2 };

// Cheap integer YCoCg from 8-bit RGB (scaled so values stay in 16-bit range)
static inline void rgb_to_ycocg(uint8_t r, uint8_t g, uint8_t b, int& Y, int& Co, int& Cg) {
    // Reversible YCoCg (lossless lifting with small scale)
    int t  = r - b;                  // Co*2
    int co = t;                      // [-255,255] *2
    int y  = b + (t >> 1);           // B + Co
    int cg = g - y;                  // Cg
    int yy = y + (cg >> 1);          // Y
    Y = yy; Co = co; Cg = cg;
}

static inline uint32_t ycocg_l2_ucolor(ucolor a, ucolor b, int wY=1, int wC=2) {
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int Ya, Coa, Cga, Yb, Cob, Cgb;
    rgb_to_ycocg(ar,ag,ab, Ya,Coa,Cga);
    rgb_to_ycocg(br,bg,bb, Yb,Cob,Cgb);
    int dY  = Ya - Yb;
    int dCo = Coa - Cob;
    int dCg = Cga - Cgb;
    // emphasize chroma with wC > wY (try wC=2)
    return uint32_t(wY*dY*dY + wC*(dCo*dCo + dCg*dCg));
}

static inline uint32_t dist_ucolor(ucolor a, ucolor b, DistKind kind) {
    switch (kind) {
        case DistKind::L2_SQR:      // existing
            { int dr=int((a>>16)&0xFF)-int((b>>16)&0xFF);
              int dg=int((a>> 8)&0xFF)-int((b>> 8)&0xFF);
              int db=int( a      &0xFF)-int( b      &0xFF);
              return uint32_t(dr*dr + dg*dg + db*db); }
        case DistKind::L1_MANHATTAN:
            return manhattan(a,b);
        case DistKind::YCOCG_L2:
            return ycocg_l2_ucolor(a,b, /*wY=*/1, /*wC=*/8);
    }
    return 0;
}

// 32-bit-only helpers
static inline uint32_t l2_sqdist_ucolor(ucolor a, ucolor b) {
    int dr = int((a >> 16) & 0xFF) - int((b >> 16) & 0xFF);
    int dg = int((a >>  8) & 0xFF) - int((b >>  8) & 0xFF);
    int db = int((a      ) & 0xFF) - int((b      ) & 0xFF);
    return uint32_t(dr*dr + dg*dg + db*db);
}
static inline uint32_t l1_manhattan_ucolor(ucolor a, ucolor b) {
    return manhattan(a, b); // from ucolor.hpp
}

static inline ucolor make_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return (ucolor(a) << 24) | (ucolor(r) << 16) | (ucolor(g) << 8) | ucolor(b);
}

// -------------------------------------------------------------------
// Mapping: O(K) linear scan (very fast for K ≤ 256)
// -------------------------------------------------------------------
static inline ucolor posterize_linear(const ucolor& c,
                                      const std::vector<ucolor>& palette,
                                      DistKind kind = DistKind::L2_SQR)
{
    uint32_t best = std::numeric_limits<uint32_t>::max();
    size_t besti = 0;
    for (size_t i = 0; i < palette.size(); ++i) {
        uint32_t d = dist_ucolor(c, palette[i], kind);
        // strict '<' keeps earlier palette entries on ties (deterministic)
        if (d < best) { best = d; besti = i; }
    }
    return palette[besti];
}

// -------------------------------------------------------------------
// 3D LUT for O(1) mapping.
// bits_per_channel = 5 (32^3 ~ 32KB) or 6 (64^3 ~ 256KB)
// Stores palette index as uint16_t to allow K > 256 if needed.
// -------------------------------------------------------------------
struct PosterizeLUT {
    int bits = 5;
    int side = 32;
    DistKind kind = DistKind::L2_SQR;
    std::vector<uint16_t> idx;     // quantized RGB -> palette index
    std::vector<ucolor> palette;   // A forced to 0xFF

    void build(const std::vector<ucolor>& pal,
               int bits_per_channel = 5,
               DistKind dist_kind = DistKind::L2_SQR)
    {
        palette = pal;
        kind = dist_kind;
        bits = bits_per_channel;
        side = 1 << bits;
        const size_t entries = size_t(side) * side * side;
        idx.assign(entries, 0);

        for (int r = 0; r < side; ++r) {
            uint8_t rc = uint8_t( (r * 255 + (side-1)/2) / (side-1) );
            for (int g = 0; g < side; ++g) {
                uint8_t gc = uint8_t( (g * 255 + (side-1)/2) / (side-1) );
                for (int b = 0; b < side; ++b) {
                    uint8_t bc = uint8_t( (b * 255 + (side-1)/2) / (side-1) );
                    ucolor cell = make_argb(0xFF, rc, gc, bc);

                    uint32_t best = std::numeric_limits<uint32_t>::max();
                    uint16_t besti = 0;
                    for (uint16_t k = 0; k < palette.size(); ++k) {
                        uint32_t d = dist_ucolor(cell, palette[k], kind);
                        if (d < best) { best = d; besti = k; }
                    }
                    size_t id = (size_t(r) << (2*bits)) | (size_t(g) << bits) | size_t(b);
                    idx[id] = besti;
                }
            }
        }
    }

    inline ucolor map(const ucolor& c) const {
        if (idx.empty() || palette.empty()) {
            return posterize_linear(c, palette, kind);
        }
        const int sh = 8 - bits; // e.g., 3 for 5 bits
        const int r = ((c >> 16) & 0xFF) >> sh;
        const int g = ((c >>  8) & 0xFF) >> sh;
        const int b = ( c        & 0xFF) >> sh;
        const size_t id = (size_t(r) << (2*bits)) | (size_t(g) << bits) | size_t(b);
        return palette[idx[id]];
    }
};

// ------------------------- Weighted samples -------------------------
struct WeightedColor { ucolor c; uint32_t w; };

// 32×32×32 RGB histogram → weighted samples (deterministic)
inline std::vector<WeightedColor>
rgb32_histogram(const std::vector<ucolor>& img){
    constexpr int B = 32, SH = 8 - 5; // 5 bits/chan
    std::vector<uint32_t> bins(B*B*B, 0);
    for (auto p : img) {
        int r = ((p >> 16) & 0xFF) >> SH;
        int g = ((p >>  8) & 0xFF) >> SH;
        int b = ( p        & 0xFF) >> SH;
        bins[(r<<10) | (g<<5) | b] += 1;
    }
    std::vector<WeightedColor> out; out.reserve(32768);
    for (int r = 0; r < B; ++r)
    for (int g = 0; g < B; ++g)
    for (int b = 0; b < B; ++b) {
        uint32_t w = bins[(r<<10)|(g<<5)|b];
        if (!w) continue;
        // bin center back to 8-bit
        uint8_t rc = uint8_t((r*255 + (B-1)/2)/(B-1));
        uint8_t gc = uint8_t((g*255 + (B-1)/2)/(B-1));
        uint8_t bc = uint8_t((b*255 + (B-1)/2)/(B-1));
        out.push_back({ (0xFFu<<24)|(ucolor(rc)<<16)|(ucolor(gc)<<8)|bc, w });
    }
    return out;
}

// ---------------- Deterministic weighted farthest-first init ----------------
// First center: weighted median luminance (ties → lower index).
// Next centers: argmax_i w_i * nearestD_i (ties → higher weight → lower index).
inline std::vector<ucolor>
det_weighted_farthest_first_init(const std::vector<WeightedColor>& pts, int K, DistKind kind)
{
    std::vector<ucolor> centers;
    const size_t N = pts.size();
    if (N == 0 || K <= 0) return centers;
    centers.reserve(K);

    // weighted median luminance
    std::vector<size_t> idx(N);
    for (size_t i = 0; i < N; ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b){
        auto la = luminance(pts[a].c), lb = luminance(pts[b].c);
        if (la != lb) return la < lb;
        return a < b;
    });
    uint64_t totalW = 0; for (auto& p : pts) totalW += p.w;
    uint64_t acc = 0; size_t firstI = idx.back();
    for (size_t j = 0; j < N; ++j) {
        acc += pts[idx[j]].w;
        if (acc * 2 >= totalW) { firstI = idx[j]; break; }
    }
    centers.push_back( (pts[firstI].c & 0x00FFFFFF) | 0xFF000000 );

    // nearest distances
    std::vector<uint32_t> nearestD(N);
    for (size_t i = 0; i < N; ++i)
        nearestD[i] = dist_ucolor(pts[i].c, centers[0], kind);

    // iteratively add farthest by w * D
    for (int k = 1; k < K; ++k) {
        uint64_t bestScore = 0; size_t bestI = 0; uint32_t bestW = 0;
        for (size_t i = 0; i < N; ++i) {
            uint64_t score = uint64_t(nearestD[i]) * pts[i].w;
            if (score > bestScore ||
                (score == bestScore && (pts[i].w > bestW || (pts[i].w == bestW && i < bestI)))) {
                bestScore = score; bestI = i; bestW = pts[i].w;
            }
        }
        centers.push_back( (pts[bestI].c & 0x00FFFFFF) | 0xFF000000 );
        for (size_t i = 0; i < N; ++i) {
            uint32_t d = dist_ucolor(pts[i].c, centers.back(), kind);
            if (d < nearestD[i]) nearestD[i] = d;
        }
    }
    return centers;
}

// ---------------- Weighted centroid updates ----------------
// L2: weighted mean (integers)
inline void update_centroids_weighted_L2(
    const std::vector<WeightedColor>& pts,
    const std::vector<int>& assign, int K,
    std::vector<ucolor>& centers)
{
    std::vector<uint64_t> sr(K,0), sg(K,0), sb(K,0), sw(K,0);
    for (size_t i = 0; i < pts.size(); ++i) {
        int k = assign[i];
        ucolor p = pts[i].c; uint32_t w = pts[i].w;
        sr[k] += uint64_t(((p>>16)&0xFF)) * w;
        sg[k] += uint64_t(((p>> 8)&0xFF)) * w;
        sb[k] += uint64_t(( p     &0xFF)) * w;
        sw[k] += w;
    }
    for (int k = 0; k < K; ++k) {
        if (sw[k]) {
            uint8_t r = uint8_t(sr[k] / sw[k]);
            uint8_t g = uint8_t(sg[k] / sw[k]);
            uint8_t b = uint8_t(sb[k] / sw[k]);
            centers[k] = make_argb(0xFF, r, g, b);
        }
    }
}

// L1: weighted median via 256-bin histograms (no sorting)
inline void update_centroids_weighted_L1(
    const std::vector<WeightedColor>& pts,
    const std::vector<int>& assign, int K,
    std::vector<ucolor>& centers)
{
    // per-cluster histograms
    std::vector<std::array<uint64_t,256>> HR(K), HG(K), HB(K);
    for (auto &a : HR) a.fill(0);
    for (auto &a : HG) a.fill(0);
    for (auto &a : HB) a.fill(0);

    for (size_t i = 0; i < pts.size(); ++i) {
        int k = assign[i];
        ucolor p = pts[i].c; uint32_t w = pts[i].w;
        HR[k][(p>>16)&0xFF] += w;
        HG[k][(p>> 8)&0xFF] += w;
        HB[k][ p     &0xFF] += w;
    }

    auto wmedian = [](const std::array<uint64_t,256>& H)->uint8_t{
        uint64_t total = 0; for (int v=0; v<256; ++v) total += H[v];
        if (!total) return 0;
        uint64_t half = (total + 1) / 2; // lower weighted median
        uint64_t acc = 0;
        for (int v=0; v<256; ++v) { acc += H[v]; if (acc >= half) return uint8_t(v); }
        return 255;
    };

    for (int k = 0; k < K; ++k) {
        uint8_t r = wmedian(HR[k]);
        uint8_t g = wmedian(HG[k]);
        uint8_t b = wmedian(HB[k]);
        centers[k] = make_argb(0xFF, r, g, b);
    }
}

static inline void snap_centers_to_medoids(
    const std::vector<WeightedColor>& pts,
    const std::vector<int>& assign,
    DistKind kind,
    std::vector<ucolor>& centers)
{
    const int K = (int)centers.size();
    for (int k=0; k<K; ++k) {
        uint32_t best = std::numeric_limits<uint32_t>::max();
        ucolor bestc = centers[k];
        for (size_t i=0;i<pts.size();++i) if (assign[i]==k) {
            uint32_t d = dist_ucolor(pts[i].c, centers[k], kind);
            if (d < best) { best = d; bestc = pts[i].c; }
        }
        centers[k] = (bestc & 0x00FFFFFF) | 0xFF000000;
    }
}

static inline void snap_centers_to_maxS_weighted(
    const std::vector<WeightedColor>& pts,
    const std::vector<int>& assign,
    std::vector<ucolor>& centers)
{
    const int K = (int)centers.size();
    for (int k = 0; k < K; ++k) {
        uint8_t   bestS = 0, bestV = 0;
        uint32_t  bestSV = 0;
        uint32_t  bestW = 0;
        size_t    bestI = SIZE_MAX;
        ucolor    bestC = centers[k];

        for (size_t i = 0; i < pts.size(); ++i) if (assign[i] == k) {
            ucolor p = pts[i].c;
            ucolor hsv = rgb_to_hsv(p);
            uint8_t S = uint8_t((hsv >> 8) & 0xFF);
            uint8_t V = uint8_t(hsv & 0xFF);
            uint32_t W = pts[i].w;

            if (S + V > bestSV ) {
                bestSV = S + V; bestC = p;
            }
        }
        centers[k] = (bestC & 0x00FFFFFF) | 0xFF000000;
    }
}

// ---------------- Deterministic weighted K-means ----------------
inline std::vector<ucolor>
kmeans_weighted_palette_ucolor_deterministic(const std::vector<WeightedColor>& pts,
                                             int K,
                                             DistKind kind = DistKind::L2_SQR,
                                             int max_iters = 20,
                                             bool sort_output = true)
{
    std::vector<ucolor> centers = det_weighted_farthest_first_init(pts, K, kind);
    const size_t N = pts.size();
    if (centers.empty()) return centers;

    std::vector<int> assign(N, -1);

    for (int it = 0; it < max_iters; ++it) {
        bool changed = false;

        // assignment (strict '<' for ties → lower center index)
        for (size_t i = 0; i < N; ++i) {
            ucolor p = pts[i].c;
            uint32_t best = std::numeric_limits<uint32_t>::max();
            int bestk = 0;
            for (int k = 0; k < K; ++k) {
                uint32_t d = dist_ucolor(p, centers[k], kind);
                if (d < best) { best = d; bestk = k; }
            }
            if (bestk != assign[i]) { assign[i] = bestk; changed = true; }
        }
        if (!changed) break;

        // update (weighted)
        if (kind == DistKind::L1_MANHATTAN)
            update_centroids_weighted_L1(pts, assign, K, centers);
        else
            update_centroids_weighted_L2(pts, assign, K, centers);

        // deterministic empty-cluster repair: steal max (w * error)
        std::vector<uint64_t> counts(K, 0);
        for (int a : assign) if (a>=0) counts[a]++;
        for (int k = 0; k < K; ++k) if (counts[k] == 0) {
            uint64_t worstScore = 0; size_t worstI = 0; uint32_t bestW = 0;
            for (size_t i = 0; i < N; ++i) {
                uint32_t d = dist_ucolor(pts[i].c, centers[assign[i]], kind);
                uint64_t s = uint64_t(d) * pts[i].w;
                if (s > worstScore || (s == worstScore && (pts[i].w > bestW || (pts[i].w == bestW && i < worstI)))) {
                    worstScore = s; worstI = i; bestW = pts[i].w;
                }
            }
            assign[worstI] = k;
            centers[k] = (pts[worstI].c & 0x00FFFFFF) | 0xFF000000;
        }
    }

    snap_centers_to_maxS_weighted(pts, assign, centers);

    // optional stable ordering
    if (sort_output) {
        std::sort(centers.begin(), centers.end(), [](ucolor a, ucolor b){
            auto la = luminance(a), lb = luminance(b);
            if (la != lb) return la < lb;
            if (((a>>16)&0xFF) != ((b>>16)&0xFF)) return ((a>>16)&0xFF) < ((b>>16)&0xFF);
            if (((a>>8)&0xFF)  != ((b>>8)&0xFF))  return ((a>>8)&0xFF)  < ((b>>8)&0xFF);
            return (a&0xFF) < (b&0xFF);
        });
    }

    for (auto& c : centers) c = (c & 0x00FFFFFF) | 0xFF000000;
    return centers;
}

// --------------- Convenience: weighted palette + LUT ---------------
struct WeightedPaletteAndLUT {
    std::vector<ucolor> palette;
    PosterizeLUT lut;
};

inline WeightedPaletteAndLUT make_weighted_palette_and_lut(
        const std::vector<WeightedColor>& pts,
        int K,
        DistKind kind = DistKind::L2_SQR,
        int max_iters = 20,
        int bits_per_channel = 5,
        bool sort_output = true)
{
    WeightedPaletteAndLUT out;
    out.palette = kmeans_weighted_palette_ucolor_deterministic(pts, K, kind, max_iters, sort_output);
    out.lut.build(out.palette, bits_per_channel, kind);
    return out;
}

// --- Ensure centers vector is exactly K, seeded from prev palette (deterministic) ---
inline std::vector<ucolor>
seed_from_previous_or_init(const std::vector<ucolor>& prev_palette,
                           const std::vector<WeightedColor>& pts,
                           int K,
                           DistKind kind)
{
    std::vector<ucolor> centers;
    centers.reserve(K);

    // 1) if prev palette is compatible, use it (force A=FF)
    if (!prev_palette.empty()) {
        if ((int)prev_palette.size() >= K) {
            centers.assign(prev_palette.begin(), prev_palette.begin()+K);
        } else {
            centers = prev_palette;
            // deterministically add more centers by farthest-first on weighted points
            auto extra = det_weighted_farthest_first_init(pts, K - (int)prev_palette.size(), kind);
            // append
            centers.insert(centers.end(), extra.begin(), extra.end());
        }
    } else {
        // no previous palette: deterministic weighted farthest-first
        centers = det_weighted_farthest_first_init(pts, K, kind);
    }

    // force alpha, just in case
    for (auto& c : centers) c = (c & 0x00FFFFFF) | 0xFF000000;
    return centers;
}

// --- Warm-start deterministic weighted k-means, few Lloyd iterations (fast per-frame) ---
inline std::vector<ucolor>
kmeans_weighted_palette_warm_start(const std::vector<WeightedColor>& pts,
                                   const std::vector<ucolor>& prev_palette,
                                   int K,
                                   DistKind kind = DistKind::L2_SQR,
                                   int max_iters = 6,           // fewer iters for per-frame updates
                                   bool sort_output = false)    // keep order stable across frames
{
    std::vector<ucolor> centers = seed_from_previous_or_init(prev_palette, pts, K, kind);
    const size_t N = pts.size();
    if (centers.empty() || N == 0) return centers;

    std::vector<int> assign(N, -1);

    for (int it = 0; it < max_iters; ++it) {
        bool changed = false;

        // assignment (strict '<' → deterministic)
        for (size_t i = 0; i < N; ++i) {
            ucolor p = pts[i].c;
            uint32_t best = std::numeric_limits<uint32_t>::max();
            int bestk = 0;
            for (int k = 0; k < K; ++k) {
                uint32_t d = dist_ucolor(p, centers[k], kind);
                if (d < best) { best = d; bestk = k; }
            }
            if (bestk != assign[i]) { assign[i] = bestk; changed = true; }
        }
        if (!changed && it > 0) break;

        // update (weighted)
        if (kind == DistKind::L1_MANHATTAN)
            update_centroids_weighted_L1(pts, assign, K, centers);
        else
            update_centroids_weighted_L2(pts, assign, K, centers);

        // deterministic empty-cluster repair: steal max (w*error)
        std::vector<uint64_t> counts(K, 0);
        for (int a : assign) if (a >= 0) counts[a]++;
        for (int k = 0; k < K; ++k) if (counts[k] == 0) {
            uint64_t worstScore = 0; size_t worstI = 0; uint32_t bestW = 0;
            for (size_t i = 0; i < N; ++i) {
                uint32_t d = dist_ucolor(pts[i].c, centers[assign[i]], kind);
                uint64_t s = uint64_t(d) * pts[i].w;
                if (s > worstScore || (s == worstScore && (pts[i].w > bestW || (pts[i].w == bestW && i < worstI)))) {
                    worstScore = s; worstI = i; bestW = pts[i].w;
                }
            }
            assign[worstI] = k;
            centers[k] = (pts[worstI].c & 0x00FFFFFF) | 0xFF000000;
        }
    }

    // keep order fixed for temporal consistency (don’t sort), unless user wants sort_output
    if (sort_output) {
        std::sort(centers.begin(), centers.end(), [](ucolor a, ucolor b){
            auto la = luminance(a), lb = luminance(b);
            if (la != lb) return la < lb;
            if (((a>>16)&0xFF) != ((b>>16)&0xFF)) return ((a>>16)&0xFF) < ((b>>16)&0xFF);
            if (((a>>8)&0xFF)  != ((b>>8)&0xFF))  return ((a>>8)&0xFF)  < ((b>>8)&0xFF);
            return (a&0xFF) < (b&0xFF);
        });
    }

    for (auto& c : centers) c = (c & 0x00FFFFFF) | 0xFF000000;
    return centers;
}

// --- Optional temporal smoothing to reduce flicker (deterministic) ---
// prop: 0..256. 0 = keep prev entirely, 256 = use new entirely.
inline void smooth_palette_in_place(std::vector<ucolor>& new_palette,
                                    const std::vector<ucolor>& prev_palette,
                                    unsigned prop /*0..256*/)
{
    const size_t K = std::min(new_palette.size(), prev_palette.size());
    const unsigned p = std::min<unsigned>(prop, 256);
    for (size_t i = 0; i < K; ++i) {
        // blend(prev, new, p) uses your integer blend (prop is weight of 'a')
        // We want result ← blend(prev, new, p), i.e. p/256 of 'a' (prev) and (1-p/256) of 'b' (new)
        new_palette[i] = blend(prev_palette[i], new_palette[i], p);
        // force alpha
        new_palette[i] = (new_palette[i] & 0x00FFFFFF) | 0xFF000000;
    }
}

// --- Palette change metric for LUT rebuild decisions ---
inline uint32_t palette_max_rgb_delta(const std::vector<ucolor>& a,
                                      const std::vector<ucolor>& b)
{
    const size_t K = std::min(a.size(), b.size());
    uint32_t maxd = 0;
    for (size_t i = 0; i < K; ++i) {
        int dr = int((a[i]>>16)&0xFF) - int((b[i]>>16)&0xFF);
        int dg = int((a[i]>> 8)&0xFF) - int((b[i]>> 8)&0xFF);
        int db = int( a[i]     &0xFF) - int( b[i]     &0xFF);
        uint32_t d = (dr<0?-dr:dr) + (dg<0?-dg:dg) + (db<0?-db:db); // L1 per entry
        if (d > maxd) maxd = d;
    }
    return maxd; // 0..765
}

// --- One-shot convenience: warm-start + (optional) smoothing + LUT (re)build gating ---
struct WarmStartResult {
    std::vector<ucolor> palette;
    PosterizeLUT lut;
    bool lut_rebuilt = false;
};

inline WarmStartResult update_palette_and_lut_warm(
        const std::vector<WeightedColor>& pts,
        const std::vector<ucolor>& prev_palette,
        const PosterizeLUT* prev_lut,    // may be null
        int K,
        DistKind kind,
        int max_iters = 6,
        int bits_per_channel = 5,
        int smooth_prop_prev = 0,        // 0..256; e.g. 32 (≈12.5%) smooths a bit
        uint32_t rebuild_threshold = 6   // rebuild LUT if any palette color moves > ~6 L1 units
)
{
    WarmStartResult out;

    // 1) warm-start k-means from previous palette
    out.palette = kmeans_weighted_palette_warm_start(pts, prev_palette, K, kind, max_iters, /*sort_output=*/false);

    // 2) optional temporal smoothing
    if (!prev_palette.empty() && smooth_prop_prev > 0)
        smooth_palette_in_place(out.palette, prev_palette, (unsigned)smooth_prop_prev);

    // 3) decide whether to rebuild the LUT
    bool need_rebuild = true;
    if (prev_lut && !prev_lut->palette.empty()
        && prev_lut->palette.size() == out.palette.size()
        && prev_lut->kind == kind
        && (1<<prev_lut->bits) == prev_lut->side) {

        uint32_t delta = palette_max_rgb_delta(prev_lut->palette, out.palette);
        need_rebuild = (delta > rebuild_threshold);
        if (!need_rebuild) {
            // reuse previous LUT
            out.lut = *prev_lut;
        }
    }

    // 4) (re)build LUT if needed
    if (need_rebuild) {
        out.lut.build(out.palette, bits_per_channel, kind);
        out.lut_rebuilt = true;
    }

    return out;
}
