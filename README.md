# Schwarzschild Null Geodesic Integration

A minimal C++17 program that integrates photon trajectories in Schwarzschild spacetime using geometrized units (G = c = 1).

## Features

- **Geodesic Equations**: Implements the full geodesic equations for null (photon) trajectories in Schwarzschild geometry
- **Equatorial Motion**: Motion restricted to the equatorial plane (θ = π/2)
- **RK4 Integration**: Fourth-order Runge-Kutta integrator with fixed step size
- **Constraint Projection**: Enforces null condition and conserved quantities at each step
- **Conservation**: Energy (p_t) and angular momentum (p_φ) are exactly conserved via projection
- **Black Hole Shadow**: Compute 1D shadow by ray-tracing from a distant camera

## Physics

### Schwarzschild Metric
In geometrized units, the line element is:
```
ds² = -f(r) dt² + f(r)⁻¹ dr² + r² dθ² + r² sin²θ dφ²
```
where `f(r) = 1 - 2M/r`.

### Key Radii
- **Event Horizon**: r = 2M
- **Photon Sphere**: r = 3M (unstable circular photon orbits)
- **Critical Impact Parameter**: b = 3√3 M ≈ 5.196M

### Constraint Projection
After each RK4 step, the code projects the state back onto the constraint surface:
1. Resets p_t and p_φ to exact initial values (Killing symmetries)
2. Recomputes p_r from the null condition: g_{μν} p^μ p^ν = 0
3. Preserves the sign of p_r (radial motion direction)

This maintains the null invariant at machine precision (~10⁻¹⁶) throughout integration.

## Compilation

```bash
g++ -std=c++17 -O2 -Wall -Wextra schwarzschild_geodesic.cpp -o schwarzschild_geodesic
```

## Usage

### Single Geodesic Test

Run without arguments to integrate a single photon trajectory:
```bash
./schwarzschild_geodesic
```

The program will output the radial coordinate `r` at various steps, along with φ, p_t, and p_φ.

### Black Hole Shadow Computation

Run with `--shadow` flag to compute the 1D black hole shadow:
```bash
./schwarzschild_geodesic --shadow > shadow_data.txt
```

This ray-traces 1000 photons from a camera at r = 10M over a 120° field of view and determines which rays are captured by the black hole vs. escape to infinity.

**Output format:**
```
# index    alpha(deg)    alpha(rad)    result
     0      -60.0000       -1.0472  ESCAPED
     1      -59.8799       -1.0451  ESCAPED
   ...
   318      -21.8018       -0.3805  CAPTURED
   ...
```

**Expected Results:**
- Shadow angular width: ~44° (from camera at r = 10M)
- Capture fraction: ~36%
- Sharp transition at shadow boundary

## Example Output

### Single Geodesic (b = 5.2M)
With `b = 5.2M`:
- Photon starts at r = 10M
- Approaches the photon sphere near r = 3M  
- Gets deflected and escapes to infinity
- Null invariant maintained at ~10⁻¹⁶

### Black Hole Shadow
```
Shadow region: -21.80° to 21.80°
Shadow width:  43.60°
Captured:      364 rays (36.4%)
Escaped:       636 rays (63.6%)
```

## Modifying Parameters

### Single Geodesic Mode
Edit the following constants in `main()`:
- `M`: Black hole mass (default: 1.0)
- `r0`: Initial radius (default: 10.0)
- `b`: Impact parameter (default: 5.2)
- `num_steps`: Number of integration steps (default: 5000)
- `dlambda`: Affine parameter step size (default: 0.05)

Try `b = 5.0` to see the photon fall into the black hole!

### Shadow Mode
Edit in the `--shadow` section:
- `r_cam`: Camera radius (default: 10.0 M)
- `fov_deg`: Field of view in degrees (default: 120.0)
- `N`: Number of angular samples (default: 1000)
- `max_steps`: Maximum integration steps per ray (default: 10000)

## Code Structure

- `State`: Holds position (t, r, θ, φ) and momentum (p_t, p_r, p_θ, p_φ)
- `f(r, M)`: Schwarzschild metric coefficient
- `dtdlambda()`, `drdlambda()`, etc.: Geodesic equations for coordinate evolution
- `dprdlambda()`: Radial momentum evolution (gravitational + centrifugal terms)
- `null_invariant()`: Computes g_{μν} p^μ p^ν for diagnostics
- `enforce_null_constraint()`: Projects state onto constraint surface
- `rk4_step()`: Fourth-order Runge-Kutta integrator
- `initialize_photon()`: Sets up initial conditions with impact parameter
- `initialize_photon_from_camera()`: Sets up photon from camera frame
- `trace_ray()`: Integrates single ray and determines fate

## References

- Chandrasekhar, S. (1983). *The Mathematical Theory of Black Holes*
- Misner, Thorne, and Wheeler (1973). *Gravitation*
