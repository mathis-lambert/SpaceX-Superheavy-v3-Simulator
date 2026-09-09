# Site geography and photographic controls

This batch updates the existing simulator environment and presentation layer.
Flight guidance, forces, mass, physical contacts and the catch corridor are unchanged.

## Geography

The geodetic origin is 25.9973 N, 97.1569 W. Unreal X is east, Y is true north,
and Z is up. The Earth meshes retain their existing `(1,-1,1)` import correction.

The former NAIP download requested square physical footprints in geographic
coordinates, but the image service silently expanded their latitude bounds to
make the angular pixels square. One measured tile overlapped its northern
neighbor by approximately 338 m. The corrected downloader explicitly disables
aspect adjustment and verifies the extent returned by the service before accepting
an image. All 16 tiles were fetched again. Internal tile-edge fallback bands were
removed; imagery fades only at the outside of the regional coverage.
Local orthoimagery remains fully weighted up to 12 km viewing distance, then
transitions to regional imagery by 50 km. A geographic water-mask texture keeps
shorelines independent of Nanite's distance-dependent triangle simplification.

Source: [Esri exportImage parameter documentation](https://developers.arcgis.com/rest/services-reference/enterprise/export-image/).
Imagery: USGS/USDA NAIP. Each source record includes its request, returned extent,
dimensions and SHA-256. A 4000-pixel export is not a claim about native sensor resolution.
Original sources remain in `../ArtSource/Earth/Registered4000`.

Four central terrain meshes now use a 512-by-512 cell grid, replacing the previous
meshes rather than adding another terrain layer. The outer edges match the original
neighbor geometry. USGS 3DEP elevation is retained; authored sub-metre detail adds
dune texture within the measured coastal dune band. The engineered pad datum is
unchanged. Vegetation placements are resampled from the registered imagery and terrain.

## Living site

- A continuous service circuit and a western access branch share a metric source layout.
- Asphalt markings, aggregate shoulders, drainage grates, maintenance bays,
  roller doors, bollards, parking areas, container workshops and cable reels.
- Four service pickups: two circulate and pause periodically, two park at the workshops.
  Moving traffic decelerates and stops when countdown starts.
- Two tank conditioning vents reuse the existing non-emissive heterogeneous
  condensation material. They fade beyond 550 m and disappear at 800 m.
- Four additional bounded workshop/tank-farm light beams. They add no shadow maps.
- A curved coastal water mesh follows the sampled shoreline, with two scales of
  wave normals and a shallow surf band.

The detailed facility parts share eight instanced mesh batches, totaling 4,281
instances. The new service scenery has no flight collisions. Only the existing
physical mount, tower and catcher define vehicle contact. This is an authored
simulator layout over historical imagery, not a survey of the current facility.

## Photography

Open **Settings → Photography** from home or pause. The hierarchy separates
**Camera optics**, **Color & exposure**, **Environment**, and **Saved looks**.

Five factory looks are included: Natural daylight, Broadcast telephoto, Onboard
camera, Golden hour, and Industrial night. Three user slots store the current
look, including camera mode, optics, color, motion, atmosphere and solar calendar.

Controls include 12–600 mm focal length on a 36 mm sensor, exposure compensation,
white balance, tint, saturation, contrast, f-stop, subject/manual focus, vibration,
motion blur, grain and automatic orbit speed. Manual telephoto views use the mouse
wheel for focal length. Other orbit views retain distance zoom.

The world remains physically paused while lens and lighting changes preview live.
Camera history continues updating; motion blur is suppressed in pause and restored
on resume. This avoids a frozen zoom smear when adjusting a stopped shot.

## Sun and clock

The environment clock now represents local civil time at Starbase. The solar
direction uses latitude, longitude, day of year, equation of time and UTC offset,
following the [NOAA solar-position approximation](https://gml.noaa.gov/grad/solcalc/solareqns.PDF).
The calendar is the non-leap year 2026. Select CDT (UTC−5) or CST (UTC−6) explicitly;
the application does not infer daylight-saving transitions from the date slider.

On September 9, solar noon occurs near 13:25 CDT. The sun rises on the eastern
side, culminates to the south, and sets on the western side. Exact sunrise/set
azimuth varies with date. The calculation is geometric and excludes atmospheric
refraction. The moon remains an artistic night light, not a lunar ephemeris.

## Rebuild and validation

1. `Tools/Data/fetch_earth_detail.py` verifies and fetches the NAIP tiles.
2. `Tools/Data/prepare_earth_scenery.py` updates terrain-bound foliage.
3. Run Blender in background with `Tools/Art/build_site_landscape.py`.
4. Build the Editor target, then run `Tools/Editor/build_site_photography.py` in Unreal.
5. `Tools/Tests/verify_site_geography.py` verifies source bounds, axes and route geometry.
6. `Tools/Tests/test_site_photography.ps1` runs solar tests and two isolated game
   processes for rendered controls and persistence across restart.
7. Run the rendered Crosswind/Chase recovery and asset contracts before release.

The asset builder backs up affected assets under `Saved/Recovery/BeforeSitePhotography`.
Local validation captures and reports are under `Saved/Recovery/Photography-*`.
