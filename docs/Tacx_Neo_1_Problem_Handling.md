## Cadence Filtering for Tacx Neo 1 (T2800)

### The Problem
The Tacx Neo 1 does **not** measure cadence directly.  
Instead, it estimates cadence by analyzing the **power waveform per pedal stroke**.  

- At **normal or high power**, the torque ripple is distinct and the Neo’s algorithm works reliably.  
- At **low power** (downhill sections, recovery riding), the torque ripple is weak and ambiguous.  
- In these cases, the Neo sometimes misinterprets (during 1-12 seconds) the signal and reports a **false doubling of cadence**.  

The result:  
- Short bursts of **140–180 rpm** cadence appear in the data stream.  
- On Zwift with Virtual Shifting, cadence is part of the resistance control loop.  
- These spikes lead to **sudden, unnatural resistance changes** for a few seconds until the value stabilizes.  

A ride of ~60 minutes can easily show **dozens of false cadence spikes** when unfiltered.

---

### The Solution: Power-Aware Filtering
We designed a lightweight filter inside the bridge software that uses two techniques:

1. **Median filter (window = 5 samples)**  
   - Removes isolated outliers or short bursts of 1–2 bad samples.  
   - Ensures only consistent cadence signals move forward.  

2. **Jump rejection with power threshold**  
   - The key insight: *most spikes occur at low power*.  
   - If `power < 120 W`, the filter becomes **very strict** (e.g. max ±15% change allowed).  
   - At higher power, the Neo is stable, so the filter relaxes (normal ±40% tolerance).  
   - This dynamic approach preserves real cadence changes while blocking false “doubling” events.  
   
3. **Startup handling**  
   - When going from 0 rpm to the first pedal strokes, cadence can legitimately rise by 2× or more in just a second.  
   - To avoid clamping this natural acceleration, the filter allows **very large jumps below ~30–50 rpm**.  
   - This ensures smooth ramp-up at ride start or after coasting, without introducing false suppression.  

---

### Why It Works
The filter is effective because it matches the **physics of the Neo’s cadence estimation**:  
- **Low torque → poor signal → false spikes.**  
- **High torque → clear signal → accurate cadence.**  
- **Startup strokes → rapid cadence changes are real, so they must be allowed.**  

By conditioning cadence on **power** and **startup context**, the bridge software only intervenes when the Neo is known to be unreliable. Everywhere else, it passes cadence through unchanged.

---

### Results
In back-to-back ride tests (same course, at same **low** intensity, ~60 minutes each):

- **Without filtering**:  
  > 40+ cadence spikes between 140–180 rpm, each causing a noticeable resistance disruption.  

- **With filtering**:  
  > Only 2 small spikes remained, the rest were completely suppressed.  
  > Overall cadence trace matched true pedaling closely, with no noticeable lag.  

Below are cadence plots from the two test rides:

**Unfiltered cadence (many spikes):**  
![Cadence without filter](../media/cadence_unfiltered.png)

**Filtered cadence (clean signal):**  
![Cadence with filter](../media/cadence_filtered.png)

---

### Summary
The cadence filter for Tacx Neo 1:  
- Eliminates nearly all false cadence spikes.  
- Handles **startup acceleration** naturally, without false suppression.  
- Preserves real cadence changes, even during sprints.  
- Removes the disruptive resistance jumps in Zwift Virtual Shifting.  
- Adds minimal complexity and no extra latency.  

This makes Virtual Shifting much more stable and realistic on the Tacx Neo 1.
