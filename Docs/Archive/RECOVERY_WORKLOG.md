# Autonomous recovery implementation

Target: a repeatable local launch, powered return and tower capture demonstration in Unreal 5.8, using the existing booster artwork. This is a local flight demonstrator, not an orbital/reentry model.

Acceptance: actual Chaos flight, independent site/mission configuration, gated capture without teleporting the airborne vehicle, restart and scenario selection, usable cameras and telemetry, machine-readable results from repeatable runs.

Baseline: existing test map flew but has an unconfigured landing target, no Boostback/Entry profile, and fixed-rate catch-up steps sample the same physics state while dropping elapsed time. Runtime body mass was ~200,029 kg despite a 500,000 kg Blueprint default.

Implemented in `Recovery/`: a reusable tower Blueprint, DataAsset mission profile, native mission director, force-driven actuator model, capture constraint, three cameras, telemetry HUD, CSV/JSON recorder and reproducible Blender/Unreal authoring scripts. The default startup and game map now point to Recovery Lab.

The existing booster meshes, Blueprint assembly and Niagara exhaust are reused. The original autopilot is disabled only on the vehicle spawned by the new director; the legacy code remains available. The old test map was saved after the initial live inspection with automatic mission start restored to false.

## Validation, 7 September 2026

Unreal 5.8.2 / Blender 5.2.1. Editor target builds successfully without compiler warnings in the new module. Asset authoring and the separate asset contract audit complete with zero errors. The tower Blueprint retains its service core and both arm meshes when loaded from disk, and the generated map resolves its tower and mission profile.

| Flight | Cadence | Duration, simulated seconds | Lateral capture error | Capture speed | Restraint drift after 3 s |
|---|---:|---:|---:|---:|---:|
| Nominal | 60 Hz | 79.08 | 0.433 m | 0.102 m/s | 0 m |
| Crosswind, 14 m/s | 60 Hz | 90.28 | 0.664 m | 0.033 m/s | 0 m |
| Offset, mass reduced 10% | 60 Hz | 81.82 | 0.443 m | 0.103 m/s | 0 m |
| Nominal | 15 Hz | 79.13 | 0.431 m | 0.099 m/s | 0 m |
| Nominal | 30 Hz | 79.07 | 0.432 m | 0.105 m/s | 0 m |
| Nominal | 120 Hz | 79.09 | 0.433 m | 0.100 m/s | 0 m |

A separate 3 Hz run verifies the long-frame clamp used when the editor throttles in the background; it also captures successfully. A rendered standalone DX12 run completes in 79.11 simulated seconds, with 0.433 m error and zero speed after restraint. `RECOVERY_VALIDATION.json` preserves the measured results. Full source logs and CSV traces are in `Saved/Recovery/Tests/`.

Manual interface checks also pass: a complete second capture after R; F1/F2/F3 prepare the intended scenario without invoking Unreal debug view modes; Space launches; X aborts and shuts the engines down; scenario selection resets the vehicle after an abort. All three camera modes were inspected, including the corrected engine close-up. Screenshots are provided in `RecoveryLab.png` and `RecoveryEngines.png`.

The controller was corrected to preserve the commanded thrust axis, use center-of-mass position for translation guidance, account for the initial reaction of an under-slung gimbal, and synchronize its time with the actual Chaos step budget. Capture tolerances are evaluated separately at the base of the booster. A successful result requires measured hold stability, not merely reaching a phase label.

The model is a local demonstrator with constant mass, aggregate engine thrust, simplified drag and a rigid capture restraint. It does not implement orbital reentry, propellant consumption, calibrated grid-fin aerodynamics or deformable arm contacts. See `RECOVERY_LAB.md` for usage and extension points.

## Évolution suborbitale et revue visuelle — 7 septembre 2026

Les sections précédentes décrivent le prototype local initial et ses validations historiques. Elles sont remplacées pour l'utilisation actuelle par RECOVERY_LAB.md et FLIGHT_MODEL.md.

La mission actuelle atteint environ 97 km avec séparation, boostback, plus de 134 secondes sans poussée principale, commande aérodynamique des trois grid fins puis landing burn et capture. La masse et les deux stocks de propulsion diminuent réellement. Les données publiées de SpaceX et l'atmosphère standard NASA sont séparées des hypothèses non publiques.

La capture vise les deux ferrures mesurées dans Blender puis dans Unreal, à moins de 35 cm chacune. L'approche tient compte de l'inclinaison dans le vent. Cette correction a supprimé l'attente indéfinie du scénario Crosswind ; les trois scénarios passent à 60, 30 et 15 Hz. Les rapports détaillés sont agrégés dans RECOVERY_VALIDATION.json.

La présentation ajoute des panaches attachés aux 33 tuyères, un étage supérieur synchronisé sur la même pose de fin de pas physique, neuf caméras dont une libre, le masquage du HUD, une table de lancement, un littoral et un océan courbes, des nuages volumétriques et de nouveaux matériaux pour les grid fins et le sol. La coordonnée verticale des UV de panache a été corrigée dans la source Blender. Le sampler des textures de normales de l'océan doit être Linear Color avec décodage explicite, car les textures du projet ne sont pas importées avec la compression Normalmap.

Les trajectoires ne sont pas certifiées : coefficients aérodynamiques et lois de guidage estimés, centre de masse fixe, représentation visuelle de l'étage supérieur après séparation et capture par contrainte rigide. Le contrôleur conserve un dégagement autour de la tour ; le landing burn simulé est plus long que certaines chronologies publiques.

## Vapeur, lumières et interface — 7 septembre 2026

La traînée d'ascension est maintenant un système Niagara GPU en espace monde, avec émission interpolée, dérive dans le vent et dispersion sur 28 à 42 secondes. Les volutes au sol et la traînée partagent un atlas original de seize volumes précalculés, en 2048 × 2048. Les jets combinent turbulence, cœur bleu, compression et expansion avec la baisse de pression. La condensation cesse hors atmosphère et à la coupure des moteurs. Le pool au sol est porté à 320 volutes pour éviter leur recyclage avant extinction. La recette, les sources et les limites sont dans RECOVERY_VFX.md.

L'audit Niagara indique zéro erreur, zéro avertissement et quatre informations de version ou d'utilisation. L'audit séparé des assets rechargés réussit. Les connexions des UV aux échantillons de texture et celles du Depth Fade ont été corrigées ; la vérification RHI finale ne rapporte pas d'échec de matériau. L'audit de texture distingue sa source 2048² d'une ressource de streaming temporaire.

Deux vols rendus avec les nouveaux effets, VFXFlight01 et VFXFlightFinal, réussissent la mission complète en 391,30 secondes simulées : altitude maximale 97,18 km, 134,20 secondes sans poussée, masse récupérée 294,80 t, erreur maximale des ferrures 31,84 cm et vitesse à la capture 0,057 m/s. Le vol interactif FrontendCrosswind, avec lumière à 150 %, réussit ensuite en 400,64 secondes : erreur des ferrures 30,32 cm, vitesse 0,068 m/s et aucune dérive de maintien.

L'accueil Slate montre la scène 3D de Starbase avec un cadrage dédié. Les menus en français proposent le lancement, les trois scénarios, les neuf vues initiales, la télémétrie, les commandes et les réglages graphiques. Échap ouvre une pause réelle du monde ; la reprise conserve le vol, tandis que le redémarrage et le retour à l'accueil réinitialisent la mission. Les entrées de pilotage sont désactivées dans les menus. Les choix de résolution, de mode d'écran, de qualité, d'échelle de rendu, de cadence et d'intensité lumineuse persistent. Le lanceur respecte ces choix.

Chacun des 33 moteurs possède sa propre lumière mobile, placée sous sa tuyère et synchronisée avec sa poussée. Les groupes 33/13/3 utilisent les mêmes identifiants que les jets ; la lumière disparaît à la coupure. Trois sources centrales projettent des ombres. La valeur lumineuse est artistique, avec atténuation en carré inverse et portée bornée.

L'audit rendu de l'interface réussit ses onze contrôles : accueil sans lancement automatique, 33 sources enregistrées et éteintes, allumage en ascension, pause du monde, résolution temporaire, retour automatique après 15 secondes même en pause, horloge et position physique inchangées, reprise puis retour à l'accueil avec extinction. Le premier audit avait révélé que le callback de redimensionnement d'Unreal écrasait la résolution confirmée ; la restauration conserve désormais sa propre copie de l'affichage précédent. Le second audit valide la correction et restaure le 2560 × 1440 sans bordure.

Les captures d'accueil, de graphismes, de confirmation en 720p et de pause en 1440p ont été inspectées. Les vues de décollage, de traînée, de tuyères éclairées, de panache en altitude et de ferrures maintenues ont également été inspectées dans les vols rendus. Les preuves sont agrégées dans RECOVERY_VALIDATION.json ; les rapports et logs sources restent dans Saved/Recovery. Ces vérifications ne constituent pas une mesure de performances stable ni une validation CFD ou industrielle.

Le dernier vol MenuLightingFlight termine à nouveau la capture en 391,30 secondes. Son journal vérifie les états lumineux 0, 33, 13 et 3 : toujours 33 composants enregistrés, un nombre de lumières actives correspondant au nombre de moteurs, et extinction finale. La collecte conclut à quatre vols rendus réussis, onze contrôles d'interface réussis et un audit lumineux réussi, en conservant les neuf validations physiques précédentes. La compilation finale et la vérification des espaces du diff passent.

## Terre géographique et navigation — 7 septembre 2026

La présentation actuelle remplace les anciens patches océaniques par une Terre sphérique de rayon 6 371 km. NASA Blue Marble couvre le globe et le golfe du Mexique ; les images USGS/USDA NAIP couvrent les environs, avec seize tuiles locales sur 12 × 12 km à environ 1,46 m par texel. Le relief provient de 3DEP, nettoyé des zones marines sans données. Le centre, les rayons, les crédits, les résolutions et les limites sont documentés dans RECOVERY_EARTH.md. Le vol conserve son modèle physique local estimé.

Le site reçoit des équipements de service, barrières et détails industriels. Le profil d'environnement contient 9 370 touffes de végétation et 191 rochers, répartis d'après les images et le relief, avec exclusion de l'aire de lancement. L'audit des assets rechargés valide les dix-neuf surfaces, les seize tuiles Nanite et leurs textures. Les collisions géographiques sont explicitement désactivées par profil persistant afin de préserver les contacts du véhicule avec son installation.

L'atmosphère suit le centre terrestre, avec diffusion solaire par pixel, nuages volumétriques et retrait progressif du brouillard local en altitude. Le soleil est maintenant orienté avec les arguments nommés d'Unreal Rotator. Le globe et les couches régionales conservent leur maillage complet ; seules les tuiles locales utilisent Nanite, avec une précision adaptée au terrain. Le cadrage initial est calculé avant activation de la caméra, ce qui élimine l'avertissement de débordement des pages d'ombres observé au démarrage. Les journaux rendus finaux ne signalent ni ce débordement ni un échec de compilation de matériau ; ils conservent des avertissements de widgets internes de l'éditeur et de r.MotionVectorSimulation.

Quatorze caméras sont proposées dans un sélecteur en deux colonnes accessible par V ou Tab pendant le vol. Les ajouts sont les spectateurs à 3 et 8 km, le panorama du site, l'horizon terrestre et la Terre entière. La sélection ne suspend pas la mission. Les transitions de position aboutissent en 1,2 seconde, y compris depuis la vue globale ; la vitesse de caméra libre s'adapte à l'altitude. Échap conserve sa fonction de pause.

L'audit rendu EarthAudit réussit ses huit contrôles : vol actif dans le sélecteur, choix à 3 et 8 km, position de la vue Terre entière, retour depuis cette vue, caméra libre, pause et retour à l'accueil. Les deux vols rendus EarthFlight et EarthFlightFinal réussissent en 391,30 secondes simulées : apogée 97,176 km, 134,20 secondes sans poussée principale, masse récupérée 294,80 t, vitesse de capture 0,057 m/s, erreur maximale des ferrures 31,84 cm et dérive nulle après trois secondes de maintien. Le script test_recovery_earth.ps1 vérifie les résultats et restaure les réglages d'affichage de l'utilisateur après les tests.

Les images finales d'accueil, du sélecteur, de la vue à 3 km, du panorama, de la Terre entière, de l'horizon à 93 km, de l'ascension et de la capture ont été inspectées. RECOVERY_VALIDATION.json agrège maintenant six vols rendus réussis, les audits d'interface et de caméras, ainsi que les audits d'assets, en conservant les validations physiques historiques. Il ne s'agit pas d'un benchmark de cadence ni d'une reproduction photogrammétrique exhaustive des installations.

Une dernière manipulation de la version interactive en 2560 × 1440 sans bordure valide le bouton Lancer, le raccourci V, le clic sur Spectateur 8 km, le cadrage large obtenu, puis Échap et Retour à Starbase. Le simulateur est laissé ouvert sur l'accueil avec les réglages d'affichage restaurés.

## Refonte visuelle et contacts porteurs — 7 septembre 2026

Cette version remplace la capture par contrainte décrite dans les validations historiques. Deux ergots soudés au corps physique portent désormais le booster sur les rails des bras. Le premier contact valide coupe la propulsion et les corrections d'attitude ; deux appuis stables puis huit secondes de maintien passif sont nécessaires au succès. Aucune pose ni vitesse n'est imposée à la capture. Le lancement et le retour partagent le même axe, et l'approche traverse l'ouverture par l'avant. Les bras possèdent aussi des collisions structurelles bloquantes. Leur déformation reste hors modèle.

La présentation suit les six photographies fournies : réglage solaire sur 24 heures depuis l'accueil et la pause, brume volumétrique, exposition solaire et lunaire, éclairage du site, nuages composés en pleine résolution, flou de mouvement, grain et profondeur de champ réglables. Le nouveau bandeau Slate est dessiné à la résolution de sortie. Le Starship est reconstruit dans Blender avec ses quatre volets et six tuyères ; six modules de réaction sur le booster produisent des jets correspondant aux forces appliquées. Les textures métal/rugosité du booster sont corrigées sur un nouveau matériau en conservant les originaux. La vapeur reçoit l'éclairage de la scène, son pool passe à 1 280 volutes et la traînée GPU atteint 330 particules par seconde. Les textures terrestres sont relevées et les raccords entre imagerie locale et régionale s'effacent progressivement en altitude.

La compilation finale et les audits d'assets passent. Les neuf vols physiques Nominal, Crosswind et Offset à 60, 30 et 15 Hz réussissent, avec une dérive de maintien comprise entre 0,65 et 20,30 mm. Trois fixtures sans propulsion vérifient séparément le soutien des ergots, la chute entre les rails lorsque le cap est incorrect et le blocage lors d'un impact latéral. Deux vols rendus Crosswind réussissent également : apogées de 97,171 et 97,200 km, deux appuis réels, propulsion coupée et dérives de maintien de 0,65 et 2,39 mm après huit secondes. Les six contrôles de présentation et huit contrôles de caméras passent. Les résultats et les limites sont rassemblés dans OVERHAUL_VALIDATION.json.

Les captures du ciel à plusieurs heures, du décollage, de l'espace, de la Terre, de la vapeur et de la capture finale ont été inspectées. Le contrôle interactif en 2560 × 1440 valide l'ouverture de Lumière & caméra et la sélection par clic de 12:01 puis 17:54, avec changement de ciel et d'ombres et enregistrement de la préférence. Le geste de glissement envoyé par l'outil de contrôle n'a pas produit de changement mesurable ; le clic sur la barre est vérifié, mais ce test ne valide pas le glissement continu à la souris. Le widget utilise le SSlider standard d'Unreal. Le simulateur est laissé sur l'accueil en lumière de fin de journée. RECOVERY_OVERHAUL.md décrit l'utilisation, les sources, la reconstruction et les simplifications physiques.
