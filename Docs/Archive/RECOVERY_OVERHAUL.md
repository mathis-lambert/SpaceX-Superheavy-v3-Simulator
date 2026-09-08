# Refonte du 7 septembre 2026

La direction visuelle suit les six références fournies : acier éclairé par le ciel, brume côtière, aube et crépuscule, panache lumineux entouré de vapeur et télémétrie sobre. Les installations et modèles reconstruits sont des interprétations destinées au simulateur.

## Utilisation

Lancer `Scripts/launch_recovery.ps1`. **Lumière & caméra** donne accès au curseur solaire sur 24 heures, au brouillard volumétrique, au flou de mouvement, au grain et à la profondeur de champ. Ces réglages sont également disponibles avec **Échap** pendant le vol ; ils changent le rendu en gardant la simulation en pause. **V / Tab** ouvre les quatorze vues, **F** active la caméra libre et **H** masque le bandeau de vol.

Le soleil suit une géométrie solaire locale à 25,9973° N, avec une déclinaison représentative de septembre. L'exposition, l'éclairage lunaire et les lumières du site suivent l'heure choisie. Le brouillard est volumétrique près du sol puis s'efface à haute altitude. La profondeur de champ focalise sur le véhicule ; elle se désactive complètement avec sa case. Le HUD Slate est dessiné après les effets caméra à la résolution de sortie.

## Capture et vol

Le pas de tir et la capture sont sur le même axe. Le booster approche l'ouverture des bras par l'avant, au-dessus de la tour, puis descend. Les deux ergots sont des volumes de collision soudés au corps rigide principal. Chaque bras possède un volume structurel bloquant et un rail porteur. La capsule du booster ne peut pas traverser les bras.

Le premier appui lent et bien placé déclenche l'arrêt des moteurs et des corrections d'attitude. La gravité transfère le poids aux rails. Après stabilisation sur les deux appuis, le maintien est évalué pendant huit secondes. Aucune pose ni vitesse n'est imposée à la capture. La contrainte historique est conservée dans le Blueprint pour compatibilité, mais n'est attachée à aucun corps.

Trois essais indépendants sans propulsion vérifient les contacts : un booster bien orienté est porté ; un mauvais cap laisse les ergots tomber entre les rails ; une arrivée latérale heurte réellement la structure. Les contacts sont simplifiés par des boîtes rigides avec frottement, sans déformation des bras ni simulation détaillée de leurs roulements.

Le contrôle de réaction utilise six forces appliquées aux modules modélisés, organisées en couples qui évitent une translation artificielle. Les jets visibles suivent ces commandes. La disposition, l'Isp et les capacités de ces buses sont estimées. La masse décroît avec les ergols principaux et ceux de réaction ; le vol conserve boostback, côte balistique sans moteurs, entrée guidée par les grid fins et landing burn.

## Assets et rendu

- Starship de 54 m, quatre volets avec charnières, joints de coque, panneaux, jupe et six tuyères ; environ 52 000 sommets dans la source Blender. Sa trajectoire après séparation est illustrative.
- Matériau du booster dérivé des textures originales avec les canaux métal/rugosité corrigés (G : rugosité, B : métal), et copies linéaires des cartes de données. Les matériaux d'origine sont conservés.
- Vapeur au sol : pool de 1 280 volutes, émission sur trois niveaux, durée de 22 s. Traînée GPU : jusqu'à 330 particules/s, modulées par la densité atmosphérique et la poussée. Les volutes reçoivent désormais la lumière de la scène.
- Flammes plus lumineuses, cœur presque blanc et mélange orangé ; 33 sources lumineuses liées aux moteurs, dont trois projettent des ombres.
- Terre : source NASA 21 600 × 10 800, limite de rendu 16 384 × 8 192 ; région NAIP 8 000² ; seize tuiles locales 4 000². Les limites de résolution et la provenance sont dans RECOVERY_EARTH.md et `../assets/overhaul/sources.json`.
- Nuages calculés en pleine résolution (`r.VolumetricRenderTarget.Mode=3`) avec composition des transparences ; le nombre d'échantillons en profondeur diminue au-dessus de la couche nuageuse. Ombres virtuelles, anisotropie 16 et brouillard volumétrique affiné.

Les panaches restent des effets artistiques, pas une simulation CFD ; le modèle de vol n'est pas une télémétrie SpaceX. Le réglage volumétrique et la fumée dense ont un coût GPU : les paramètres graphiques restent disponibles pour adapter le rendu.

## Reconstruction

Fermer l'éditeur et tous les jeux avant d'écrire les assets ; un jeu de test peut conserver un verrou même sans rendu.

1. Conserver ou reconstruire la scène géographique et les effets historiques selon RECOVERY_LAB.md et RECOVERY_EARTH.md.
2. Exécuter `build_overhaul_art.py` avec Blender et `fetch_overhaul_textures.py` avec Python/Pillow.
3. Compiler le module Unreal 5.8.2 avec `-gather`.
4. Exécuter `apply_physical_site.py`, puis `build_overhaul_presentation.py`, `polish_overhaul_materials.py` et `polish_earth_transitions.py` dans le commandlet PythonScript avec rendu autorisé.
5. Exécuter les audits des assets, `test_physical_recovery.ps1`, les fixtures de contact et les audits rendus. Inspecter les captures ; les contrôles numériques seuls ne prouvent pas la qualité visuelle.

Les sources Blender se trouvent dans `../assets/overhaul/StarshipAndRCS.blend`. Une sauvegarde de l'état initial de cette refonte est conservée dans `Saved/Recovery/OverhaulBaseline`.

## Validation

Les rapports actuels sont rassemblés dans `OVERHAUL_VALIDATION.json`. La matrice physique couvre Nominal, Crosswind et Offset à 60, 30 et 15 Hz. Les fixtures vérifient le soutien et le blocage structurel ; l'audit de présentation vérifie l'heure, le rendu et la pause. Le vol rendu complet contrôle également les phases et les deux appuis réels. Les essais valident ce modèle estimé, pas les performances d'un véhicule réel.
