# GTA III water paths

`routes.json` defines connected coastal circuits around Portland, Staunton Island,
and Shoreside Vale, including the docks and airport coast. The routes use sea
level (Z = 0); the elevated reservoir is intentionally excluded.

Regenerate the embedded graph from the repository root:

```powershell
python resources/iii-water-paths/generate.py
```

Optionally validate the routes against the stock GTA III water coverage:

```powershell
python resources/iii-water-paths/generate.py --water-map Z:/GitHub/re3/assets/data/waterpro.dat
```

The generator checks sea-level coverage with 40 metres of clearance and writes
`source/IIILodLights/WaterPaths.hpp`. Both the graph and the existing boat mesh
are compiled into the ASI; no resource files need to be installed with it.
The water coverage check does not replace in-game checks for docks or bridge
piers. Custom maps may require edited routes.

`MaritimeTraffic` enables these routes. Boats reserve up to 5% of
`MaxNumberOfDistantCars`, capped at 32 boats, within the existing total budget.
`DistantCars3D` and `DistantBoats3D` enable the boat models; otherwise the
existing navigation coronas are used. Roads and boats use separate corona IDs.

The optional `CWaterLevel::GetWaterLevelNoWaves` binding in III's `dllmain.cpp`
can supply modified water heights. Stock routes work without it at sea level.
