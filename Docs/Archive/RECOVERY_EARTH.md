# Terre, Starbase et caméras

La carte `L_RecoveryLab` utilise désormais une sphère terrestre complète de rayon 6 371 km, centrée 6 371 km sous l'origine. Le repère local est Est / Nord / Haut, à 25,9973° N et 97,1569° O. La géométrie remplace les anciens patches de mer et de littoral procédural. Le guidage et la gravité restent calculés dans le repère local du modèle de vol : cette évolution du décor n'est pas un passage à une mécanique orbitale mondiale.

## Données et résolution

| Couche | Données | Usage |
|---|---|---|
| Globe entier | NASA Blue Marble Next Generation, septembre 2004, topographie et bathymétrie | Source 21 600 × 10 800, limite de rendu 16 384 × 8 192 |
| Golfe du Mexique | NASA GIBS, BlueMarble Shaded Relief Bathymetry | Texture 4 096² sur une zone de 24° × 24° |
| Environs de Boca Chica | USGS / USDA NAIP | Mosaïque source de 8 000² sur 120 × 120 km, environ 15 m par texel |
| Boca Chica | USGS / USDA NAIP | 16 tuiles source de 4 000² sur 12 × 12 km, environ 0,75 m par texel |
| Relief local | USGS 3DEP | Échantillonnage de 1 024² sur 12 × 12 km ; grille géométrique à environ 23 m |

Les coordonnées, URL exactes et empreintes SHA-256 sont conservées dans `../assets/earth/sources.json`. Les images sont des acquisitions/composites historiques, pas une représentation en temps réel des installations. Les trous NAIP utilisent la texture régionale. Un filtre médian et un masque marin rejettent les pics des zones d'élévation sans données ; celles-ci sont ramenées au niveau de la mer. Le terrain du plateau industriel est aplani à la cote du modèle existant avec une transition de 45 m.

Sources : [NASA Earth Observatory — Blue Marble](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/), [NASA GIBS](https://gibs.earthdata.nasa.gov/wms/epsg4326/best/wms.cgi), [USGS NAIP](https://imagery.nationalmap.gov/arcgis/rest/services/USGSNAIPImagery/ImageServer), [USGS 3DEP](https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer). Crédits NASA Earth Observatory, NASA GIBS, USGS et USDA. Les données NAIP et 3DEP utilisées sont publiques.

## Matériaux et décor

Les photographies locales sont complétées par les textures de sable et leurs normales déjà présentes dans le projet. Les zones basses et sombres reçoivent des normales d'eau animées et des reflets plus lisses. Il s'agit d'un masque de rendu estimé, pas d'une classification hydrologique officielle. Les grass/rocks existants sont placés selon la couleur et l'élévation du terrain, en excluant la plateforme et la route de service.

`DA_RecoveryEnvironment` conserve les transformations modifiables de la végétation et des rochers. Les acteurs du dossier `Recovery/Earth` contiennent les surfaces géographiques ainsi que les compresseurs, ventilations, barrières et équipements de service ajoutés. Les installations sont une composition destinée au simulateur, pas un relevé industriel exhaustif de Starbase. Le décor n'ajoute pas de collision au modèle de vol.

L'atmosphère Unreal partage le centre du globe. Sa limite supérieure se situe à 100 km, avec diffusion Rayleigh et Mie, atténuation solaire par pixel et nuages volumétriques entre 2,1 et 5,2 km. Le sol analytique est placé 500 m sous le rayon nominal pour que les cordes du maillage global ne soient pas considérées comme enfouies et privées de lumière solaire. Le brouillard volumétrique local diminue entre 1,5 et 16 km d'altitude caméra ; l'atmosphère sphérique prend ensuite le relais. L'heure solaire locale règle le soleil et l'exposition ; l'éclairage lunaire et celui du site prennent le relais la nuit. La déclinaison solaire représente septembre et n'est pas une éphéméride annuelle.

La passe de refonte utilise `fetch_overhaul_textures.py` et `build_overhaul_presentation.py` après la construction géographique. Les nouvelles URL et empreintes se trouvent dans `../assets/overhaul/sources.json`. La résolution par texel indique l'échantillonnage demandé au service ; elle ne garantit pas une résolution optique identique dans chaque acquisition historique. Les mipmaps et le streaming déterminent le détail affiché selon la distance.

`polish_earth_transitions.py` fond progressivement les orthophotos locales dans la couverture régionale entre 3,5 et 16 km d'altitude caméra. Les frontières des acquisitions aériennes ne forment ainsi plus de patch rectangulaire clair dans les vues suborbitales. Les images détaillées restent utilisées près du sol.

## Navigation

`V` ou `Tab` ouvre un sélecteur de 14 vues pendant que le vol continue. Cliquer une vue ferme le sélecteur ; `V`, `Tab` ou `Échap` revient au vol. Le menu pause propose aussi ce sélecteur. La sélection depuis la pause reprend le vol.

Les cinq nouvelles vues sont **Spectateur 3 km**, **Spectateur 8 km**, **Panorama de Starbase**, **Horizon terrestre** et **Terre entière**. Les vues spectateur conservent un champ large, sans téléobjectif automatique ; la molette ajuste le cadrage. Le booster est naturellement petit à ces distances. `C` parcourt les vues ; `F` active la caméra libre. La vitesse libre s'adapte à l'altitude lors de son activation et reste réglable à la molette. Les transitions de position aboutissent en 1,2 seconde, y compris au retour de la vue Terre entière.

## Reconstruction et vérification

1. `fetch_earth_data.py` avec le Python disposant de Pillow télécharge et vérifie les sources.
2. `prepare_earth_water_mask.py` produit le masque marin à partir de NAIP.
3. `prepare_earth_scenery.py` produit les placements sur le relief nettoyé.
4. Blender en arrière-plan exécute `build_earth_art.py` et sauvegarde `StarbaseEarth.blend` et les FBX.
5. Compiler le module Unreal, puis exécuter `build_earth_environment.py` dans le commandlet PythonScript, après les scripts de présentation historiques. Ajouter `-EarthReimport` au commandlet pour réimporter des FBX modifiés.

`-RecoveryEarthAudit` contrôle la sélection en vol, les distances de caméra, le retour depuis la vue globe, la caméra libre et la pause. Il écrit ses résultats et captures dans `Saved/Recovery/EarthAudit`. `-RecoveryReview -RecoveryEarthReview -RecoveryAutoExit` effectue un vol complet et ajoute une capture de l'horizon en altitude. Les captures doivent être inspectées : un test de navigation réussi ne garantit pas à lui seul la qualité du rendu.
