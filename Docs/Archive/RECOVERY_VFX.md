# Effets de propulsion et vapeur

La scène `L_RecoveryLab` utilise une traînée Niagara GPU en espace monde. Les particules restent derrière le booster, dérivent dans le vent du profil de mission, se dispersent par turbulence et disparaissent progressivement après 28 à 42 secondes. L'émission est interpolée entre les images et diminue avec la pression atmosphérique. La traînée de condensation concerne l'ascension ; elle cesse dans l'air raréfié et à la coupure des moteurs. Les jets restent visibles pendant le boostback et le freinage final, sans produire une colonne de fumée blanche dans le vide.

Les 33 jets sont placés sur les `ThrustSocket` des moteurs. Leur longueur suit les gaz, leur expansion suit la pression et leur surface combine turbulence animée, cœur bleu et motifs de compression. Un panache commun représente le mélange en aval. Les groupes de 33, 13 et 3 moteurs suivent la simulation.

Chaque tuyère possède également une source ponctuelle mobile, positionnée 2,5 m sous son socket et mise à jour après la physique. L'intensité suit la poussée et une légère fluctuation propre au moteur ; les moteurs coupés n'éclairent plus. Le réglage standard utilise 3 millions de candelas par moteur à pleine poussée, valeur artistique ajustable de 0 à 200 % depuis les graphismes. La portée est bornée à 120 m. Les trois sources centrales projettent des ombres, les autres éclairent sans carte d'ombre pour limiter le coût. La décroissance suit le carré inverse de la distance et l'exposition de la scène ; voir les [unités de lumière Unreal](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-physical-lighting-units-in-unreal-engine).

La vapeur proche du sol emploie un pool borné de 1 280 particules visuelles, avec émission radiale sur trois hauteurs, montée, vent et disparition sur 22 secondes. L'émission s'étend jusqu'à 170 m de hauteur de base ; elle atteint huit volutes toutes les 0,14 seconde sur 33 moteurs, six sur 13 et quatre sur 3. Les volutes utilisent 16 variantes d'un volume de densité, avec extinction et ombre interne précalculées dans un atlas RGBA 2048². Elles reçoivent maintenant la lumière de la scène par l'éclairage translucide volumétrique directionnel, au lieu d'une couleur emissive permanente. Le débit Niagara de montée atteint 330 particules par seconde avant modulation par pression et poussée.

Ces effets sont des représentations artistiques de vapeur et de gaz chauds. L'ombrage interne est précalculé ; il ne s'agit pas d'une simulation de combustion ou de dynamique des fluides en trois dimensions en temps réel.

La refonte réchauffe le panache commun vers un cœur presque blanc et une bordure orangée. Six modules de buses de réaction et des jets commandés apparaissent selon les forces d'attitude calculées, surtout dans l'air raréfié. Leur géométrie et leur aspect sont estimés. Le mode volumétrique Unreal 3 calcule les nuages en pleine résolution et les compose avec les transparences pour éviter les bordures carrées de reconstruction autour du booster.

| Élément | Emplacement |
|---|---|
| Traînée GPU | `/Game/Recovery/FX/NS_RecoveryVaporTrail` |
| Atlas | `/Game/Recovery/Textures/T_RecoveryVaporAtlas` |
| Matériaux | `/Game/Recovery/Materials/M_VaporTrail`, `M_GroundVapor`, `M_RaptorPlume` |
| Liaison au vol | `Source/SuperHeavySim/Private/Recovery/RecoveryPresentationComponent.cpp` |
| Recette Niagara | `Scripts/recovery_niagara.json` |
| Génération de l'atlas | `Scripts/bake_vapor_atlas.py` — Python, NumPy et Pillow |
| Construction des matériaux | `Scripts/build_recovery_vfx.py` — Python Unreal |

Dans Niagara, `User.SpawnRate` contrôle l'émission, `User.ExhaustVelocity` la vitesse initiale et `User.Wind` le vent, en cm/s dans le monde. Le programme fournit ces valeurs pendant le vol. Les setters Niagara utilisent le nom court de la variable. Le matériau `M_RaptorPlume` expose `Throttle`, `Vacuum`, `FlightTime` et `MixingLayer`.

`build_flight_presentation.py` reconstruit aussi ces matériaux en fin d'exécution. Générer l'atlas avant de lancer ce script. Pour ne retoucher que les effets, lancer directement `build_recovery_vfx.py` après fermeture de l'éditeur et du jeu, ou depuis la console Python de l'éditeur actif. Ne pas écrire les mêmes assets depuis deux processus Unreal.

Après une reconstruction historique, `build_overhaul_presentation.py` réapplique la version éclairée de la fumée et les nouveaux matériaux. `polish_overhaul_materials.py` corrige les canaux métalliques du booster dans un matériau dérivé et des copies de textures linéaires.

L'option `-RecoveryReview` enregistre les phases et cinq vues supplémentaires de l'ascension dans `Saved/Recovery/Review`. `Scripts/audit_recovery_vfx.py` vérifie les assets sauvegardés. Le test de vol complet doit également produire un JSON `success: true` : une simple compilation ou une capture d'écran ne valide pas la récupération.
